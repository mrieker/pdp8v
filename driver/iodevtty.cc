
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include "iodevtty.h"
#include "linc.h"
#include "memory.h"
#include "shadow.h"

IODevTTY iodevtty;

static IODevOps const iodevops[] = {
    { 06030, "KCF (TTY) clear kb flag so we will know when another char gets read in" },
    { 06031, "KSF (TTY) skip if there is a kb character to be read" },
    { 06032, "KCC (TTY) clear kb flag, clear accumulator" },
    { 06034, "KRS (TTY) read kb char but don't clear flag" },
    { 06035, "KIE (TTY) set/clear interrupt enable for both kb and pr" },
    { 06036, "KRB (TTY) read kb character and clear flag" },
    { 06040, "TFL (TTY) pretend the printer is ready to accept a character" },
    { 06041, "TSF (TTY) skip if printer is ready to accept a character" },
    { 06042, "TCF (TTY) pretend the printer is busy printing a character" },
    { 06044, "TPC (TTY) start printing a character" },
    { 06045, "TSK (TTY) skip if currently requesting either keyboard or printer interrupt" },
    { 06046, "TLS (TTY) turn off interrupt request for previous char and start printing new char" },
};

IODevTTY::IODevTTY ()
{
    opscount = sizeof iodevops / sizeof iodevops[0];
    opsarray = iodevops;

    pthread_cond_init (&this->prcond, NULL);
    pthread_mutex_init (&this->lock, NULL);
    this->running = false;
    this->ttykbfd = -1;
    this->ttyprfd = -1;
    this->kbtid = 0;
    this->prtid = 0;
}

// reset the device
// - clear flags
// - kill threads
void IODevTTY::ioreset ()
{
    pthread_mutex_lock (&this->lock);

    if (this->ttyprfd >= 0) {
        this->kbflag  = false;
        this->intenab = false;
        this->prflag  = false;
        this->prfull  = false;

        // tell existing threads to stop looping
        this->running = false;

        // hide kb fd from closing so we can restore tty attributes
        int hidekbfd = (isatty (this->ttykbfd) > 0) ? dup (this->ttykbfd) : -1;

        // prevent any new read()/write() calls from blocking
        // ...whilst preserving this->ttykbfd,ttyprfd
        int nullfd = open ("/dev/null", O_RDWR);
        if (nullfd < 0) ABORT ();
        if (this->ttykbfd >= 0) dup2 (nullfd, this->ttykbfd);
        if (this->ttyprfd != this->ttykbfd) dup2 (nullfd, this->ttyprfd);
        close (nullfd);

        // break kbthread() out of any read() call it may be in
        if (this->ttykbfd >= 0) pthread_kill (this->kbtid, SIGCHLD);

        // break prthread() out of any cond_wait() it may be in
        pthread_cond_broadcast (&this->prcond);
        pthread_mutex_unlock (&this->lock);

        // wait for threads to terminate
        if (this->ttykbfd >= 0) pthread_join (this->kbtid, NULL);
        pthread_join (this->prtid, NULL);
        this->kbtid = 0;
        this->prtid = 0;

        // restore terminal attrs
        if (hidekbfd >= 0) {
            tcsetattr (hidekbfd, TCSANOW, &this->oldttyattrs);
            close (hidekbfd);
        }

        // all done with the now /dev/null fds
        close (this->ttykbfd);
        if (this->ttyprfd != this->ttykbfd) close (this->ttyprfd);
        this->ttykbfd = -1;
        this->ttyprfd = -1;

        pthread_mutex_lock (&this->lock);
    }

    this->kbflag  = false;
    this->intenab = true;   // CAF (6007) enables TTY interrupt
    this->prflag  = false;
    this->prfull  = false;
    clrintreqmask (IRQ_TTYKBPR);
    pthread_mutex_unlock (&this->lock);

    char const *debenv = getenv ("iodevtty_debug");
    this->debug = (debenv == NULL) ? 0 : atoi (debenv);
}

// used by LINC
bool IODevTTY::keystruck ()
{
    if (! this->running) this->startitup ();
    return this->kbflag;
}

// 03: teletype keyboard (p257)
// 04: teletype printer (p257)
// http://homepage.divms.uiowa.edu/~jones/pdp8/man/tty.html
uint16_t IODevTTY::ioinstr (uint16_t opcode, uint16_t input)
{
    if (this->debug > 2) printf ("IODevTTY::ioinstr: opcode=%04o input=%05o\n", opcode, input);

    // process opcode
    switch (opcode) {

        // KCF: clear kb flag so we will know when another char gets read in
        case 06030: {
            pthread_mutex_lock (&this->lock);
            this->kbflag = false;
            updintreqlk ();
            pthread_mutex_unlock (&this->lock);
            break;
        }

        // KSF: skip if there is a kb character to be read
        case 06031: {
            if (! this->running) this->startitup ();
            if (this->kbflag) input |= IO_SKIP;
            break;
        }

        // KCC: clear kb flag, clear accumulator
        case 06032: {
            pthread_mutex_lock (&this->lock);
            this->kbflag = false;
            updintreqlk ();
            pthread_mutex_unlock (&this->lock);
            input &= IO_LINK;
            break;
        }

        // KRS: read kb char but don't clear flag
        case 06034: {
            input |= this->kbbuff;
            break;
        }

        // KIE: set/clear interrupt enable for both kb and pr
        case 06035: {
            if ((input & 1) && ! this->running) this->startitup ();
            pthread_mutex_lock (&this->lock);
            this->intenab = input & 1;
            updintreqlk ();
            pthread_mutex_unlock (&this->lock);
            break;
        }

        // KRB: read kb character and clear flag
        //     = KCC | KRS
        case 06036: {
            if (! this->running) this->startitup ();
            input &= ~ IO_DATA;
            input |= this->kbbuff;
            pthread_mutex_lock (&this->lock);
            this->kbflag = false;
            updintreqlk ();
            pthread_mutex_unlock (&this->lock);
            break;
        }

        // TFL: pretend the printer is ready to accept a character
        // - used during init to set initial printer ready status
        case 06040: {
            pthread_mutex_lock (&this->lock);
            if (this->debug > 1) printf ("IODevTTY::ioinstr: turn printer on\n");
            this->prflag = true;
            updintreqlk ();
            pthread_mutex_unlock (&this->lock);
            break;
        }

        // TSF: skip if printer is ready to accept a character
        case 06041: {
            if (this->prflag) input |= IO_SKIP;
            break;
        }

        // TCF: pretend the printer is busy printing a character
        // - used to turn off printer interrupt request while allowing keyboard to interrupt
        case 06042: {
            pthread_mutex_lock (&this->lock);
            if (this->debug > 1) printf ("IODevTTY::ioinstr: turn printer off\n");
            this->prflag = false;
            updintreqlk ();
            pthread_mutex_unlock (&this->lock);
            break;
        }

        // TPC: start printing a character
        case 06044: {
            if (! this->running) this->startitup ();
            pthread_mutex_lock (&this->lock);
            if (this->debug > 1) printf ("IODevTTY::ioinstr: start writing %03o at %05o\n", input & 0377, shadow.r.pc);
            this->prbuff = input;
            this->prfull = true;
            updintreqlk ();
            pthread_cond_broadcast (&this->prcond);
            pthread_mutex_unlock (&this->lock);
            break;
        }

        // TSK: skip if currently requesting either keyboard or printer interrupt
        case 06045: {
            if (getintreqmask () & IRQ_TTYKBPR) input |= IO_SKIP;
            break;
        }

        // TLS: turn off interrupt request for previous char and start printing new char
        case 06046: {
            if (! this->running) this->startitup ();
            pthread_mutex_lock (&this->lock);
            if (this->debug > 1) printf ("IODevTTY::ioinstr: start writing %03o at %05o\n", input & 0377, shadow.r.pc);
            this->prbuff = input;
            this->prfull = true;
            this->prflag = false;
            updintreqlk ();
            pthread_cond_broadcast (&this->prcond);
            pthread_mutex_unlock (&this->lock);
            break;
        }

        default: input = UNSUPIO;
    }

    if (this->debug > 2) printf ("IODevTTY::ioinstr:            output=%05o\n", input);
    return input;
}

// start threads running
void IODevTTY::startitup ()
{
    this->running = true;

    char const *cpsenv = getenv ("iodevtty_cps");
    this->chpersec = (cpsenv == NULL) ? 10 : atoi (cpsenv);

    char const *doublcrenv = getenv ("iodevtty_doublcr");
    this->doublcr = (doublcrenv != NULL) && (doublcrenv[0] & 1);

    char const *mskenv = getenv ("iodevtty_8bit");
    this->mask8 = ((mskenv != NULL) && (mskenv[0] & 1)) ? 0377 : 0177;

    // get device to use for TTY from envar
    char const *ttyname = getenv ("iodevtty");
    if (ttyname == NULL) {
        fprintf (stderr, "IODevTTY::startitup: envar iodevtty not defined\n");
        ABORT ();
    }

    // see if it contains a pipe symbol
    bool usingstdin = false;
    char const *p = strchr (ttyname, '|');
    if (p == NULL) {

        // no pipe, assume it's an actual tty device, use for both keyboard and printer

        this->ttykbfd = open (ttyname, O_RDWR | O_NOCTTY);
        if (this->ttykbfd < 0) {
            fprintf (stderr, "IODevTTY::startitup: error opening %s: %m\n", ttyname);
            ABORT ();
        }
        this->ttyprfd = this->ttykbfd;
    } else {

        // pipe, first part is keyboard pipe, second part is printer pipe
        // if keyboard empty, assume printer only
        // hyphens mean use stdin/stdout

        char kbname[p-ttyname+1];
        memcpy (kbname, ttyname, p - ttyname);
        kbname[p-ttyname] = 0;
        if (kbname[0] != 0) {
            usingstdin = (strcmp (kbname, "-") == 0);
            this->ttykbfd = usingstdin ? dup (STDIN_FILENO) : open (kbname, O_RDONLY);
            if (this->ttykbfd < 0) {
                fprintf (stderr, "IODevTTY::startitup: error opening %s: %m\n", kbname);
                ABORT ();
            }
        }

        this->ttyprfd = (strcmp (++ p, "-") == 0) ? dup (STDOUT_FILENO) : open (p, O_WRONLY);
        if (this->ttyprfd < 0) {
            fprintf (stderr, "IODevTTY::startitup: error opening %s: %m\n", p);
            ABORT ();
        }
    }

    if (this->debug > 1) printf ("IODevTTY::startitup: accessing %s\n", ttyname);

    // if keyboard is a tty, set it to passthrough mode
    if (isatty (this->ttykbfd) > 0) {
        if (tcgetattr (this->ttykbfd, &this->oldttyattrs) < 0) {
            fprintf (stderr, "IODevTTY::startitup: error getting %s tty attrs: %m\n", ttyname);
            close (this->ttykbfd);
            this->ttykbfd = -1;
            ABORT ();
        }
        struct termios newttyattrs = this->oldttyattrs;
        cfmakeraw (&newttyattrs);
        if (usingstdin) {
            newttyattrs.c_lflag |= ISIG;    // pass ctrl-C etc on to raspictl
            newttyattrs.c_oflag |= OPOST;   // insert CR before NL on output
        }
        if (tcsetattr (this->ttykbfd, TCSANOW, &newttyattrs) < 0) {
            fprintf (stderr, "IODevTTY::startitup: error setting %s tty attrs: %m\n", ttyname);
            close (this->ttykbfd);
            this->ttykbfd = -1;
            ABORT ();
        }
    }

    // start a thread for each direction to process I/O
    if (this->ttykbfd >= 0) {
        int rc = pthread_create (&this->kbtid, NULL, kbthreadwrap, this);
        if (rc != 0) ABORT ();
    }
    int rc = pthread_create (&this->prtid, NULL, prthreadwrap, this);
    if (rc != 0) ABORT ();
}

void *IODevTTY::kbthreadwrap (void *zhis)
{
    ((IODevTTY *)zhis)->kbthread ();
    return NULL;
}

void IODevTTY::kbthread ()
{
    fd_set readfds;
    uint64_t lastreadat = 0;

    FD_ZERO (&readfds);
    pthread_mutex_lock (&this->lock);
    while (running) {

        // waiting for keyboard char ready to be read
        // allow IODevTTY::ioreset() to abort the read by using a select()
        pthread_mutex_unlock (&this->lock);
        int rc;
        do {
            FD_SET (this->ttykbfd, &readfds);
            rc = select (this->ttykbfd + 1, &readfds, NULL, NULL, NULL);
            if (! running) return;
        } while ((rc < 0) && (errno == EINTR));
        if (rc < 0) {
            fprintf (stderr, "IODevTTY::kbthread: keyboard select error: %m\n");
            ABORT ();
        }
        pthread_mutex_lock (&this->lock);

        // make sure it has been 100mS since last read
        // also wait until processor has handled previous char
        uint64_t now;
        uint64_t nextreadat = lastreadat + 1000000 / chpersec;
        while (true) {
            uint32_t waitus = 16667;
            if (! this->kbflag) {
                struct timeval nowtv;
                gettimeofday (&nowtv, NULL);
                now = nowtv.tv_sec * 1000000ULL + nowtv.tv_usec;
                if (nextreadat <= now) break;
                waitus = nextreadat - now;
            }
            pthread_mutex_unlock (&this->lock);
            usleep (waitus);
            pthread_mutex_lock (&this->lock);
            if (! running) goto ret;
        }
        lastreadat = now;

        // now actually read character
        rc = read (this->ttykbfd, &this->kbbuff, 1);
        if (rc <= 0) {
            if (rc == 0) errno = EPIPE;
            fprintf (stderr, "IODevTTY::kbthread: keyboard read error: %m\n");
            ABORT ();
        }

        // if using pipes, transform LF -> CR
        if ((this->ttykbfd != this->ttyprfd) && (this->kbbuff == '\n')) this->kbbuff = '\r';

        // maybe print result for debug
        if (this->debug > 0) {
            uint8_t c = this->kbbuff;
            if ((c < ' ') || (c >= 127)) c = '.';
            printf ("IODevTTY::kbthread: read char %03o <%c>\n", this->kbbuff, c);
        }

        // force top bit if 7-bit mode (normal for pdp8s)
        this->kbbuff |= ~ this->mask8;

        // set flag saying there is a character ready and maybe request interrupt
        this->kbflag = true;
        updintreqlk ();
    }
ret:;
    pthread_mutex_unlock (&this->lock);
}

void *IODevTTY::prthreadwrap (void *zhis)
{
    ((IODevTTY *)zhis)->prthread ();
    return NULL;
}

void IODevTTY::prthread ()
{
    pthread_mutex_lock (&this->lock);
    while (running) {

        // wait for processor to put a character in buffer
        if (! this->prfull) {
            pthread_cond_wait (&this->prcond, &this->lock);
        } else {
            pthread_mutex_unlock (&this->lock);

            // a lot of PDP-8 programs set top bit so maybe mask it off
            this->prbuff &= this->mask8;

            // maybe print CR twice
            for (int i = ((this->prbuff == '\r') && this->doublcr) ? 2 : 1; -- i >= 0;) {

                // start timer now so 100mS includes any time taken by the write() itself
                // we just need the 100mS to be delay from setting prfull=true to setting prflag=true
                struct timeval nowtv;
                gettimeofday (&nowtv, NULL);
                uint64_t lastwriteat = nowtv.tv_sec * 1000000ULL + nowtv.tv_usec;

                // start printing character
                int rc = write (this->ttyprfd, &this->prbuff, 1);
                if (rc <= 0) {
                    if (rc == 0) errno = EPIPE;
                    fprintf (stderr, "IODevTTY::prthread: printer write error: %m\n");
                    ABORT ();
                }
                if (this->debug > 0) {
                    uint8_t c = this->prbuff;
                    if ((c < ' ') || (c >= 127)) c = '.';
                    printf ("IODevTTY::prthread: wrote char %03o <%c>\n", this->prbuff, c);
                }

                // wait 100mS before setting flag (or printing second CR)
                uint64_t nextwriteat = lastwriteat + 1000000 / chpersec;
                while (true) {
                    gettimeofday (&nowtv, NULL);
                    uint64_t now = nowtv.tv_sec * 1000000ULL + nowtv.tv_usec;
                    if (nextwriteat <= now) break;
                    usleep (nextwriteat - now);
                }
            }

            // set flag saying printer able to accept another char and maybe request interrupt
            pthread_mutex_lock (&this->lock);
            this->prfull = false;
            this->prflag = true;
            updintreqlk ();
        }
    }
    pthread_mutex_unlock (&this->lock);
}

// update interrupt request line after changing intenab, kbflag, prflag
void IODevTTY::updintreq ()
{
    pthread_mutex_lock (&this->lock);
    this->updintreqlk ();
    pthread_mutex_unlock (&this->lock);
}

// assumes this->lock is locked
void IODevTTY::updintreqlk ()
{
    if (this->debug > 0) printf ("IODevTTY::updintreqlk: intenab=%o kbflag=%o prflag=%o linc.specfuncs=%04o\n",
        this->intenab, this->kbflag, this->prflag, linc.specfuncs & 0040);

    if (this->intenab && (this->kbflag | this->prflag) && ! (linc.specfuncs & 0040)) {
        setintreqmask (IRQ_TTYKBPR);
    } else {
        clrintreqmask (IRQ_TTYKBPR);
    }
}
