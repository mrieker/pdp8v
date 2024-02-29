
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include "iodevptape.h"

IODevPTape iodevptape;

static IODevOps const iodevops[] = {
    { 06010, "RPI (PT) interrupt enable" },
    { 06011, "RSF (PT) skip if there is a reader byte to be read" },
    { 06012, "RRB (PT) reader buffer ored with accumulator; clear reader flag" },
    { 06014, "RFC (PT) clear reader flag and start reading next char from tape" },
    { 06016, "RRB RFC (PT) reader buffer ored into accumulator; reader flag cleared; start reading next character" },
    { 06020, "PCE (PT) clear interrupt enable" },
    { 06021, "PSF (PT) skip if punch ready to accept next byte" },
    { 06022, "PCF (PT) clear punch flag, pretending punch is busy" },
    { 06024, "PPC (PT) start punching a byte" },
    { 06026, "PLS (PT) punch load sequence = PCF + PPC; copy accum to punch buffer, initiate output, reset punch flag" },
};

IODevPTape::IODevPTape ()
{
    opscount = sizeof iodevops / sizeof iodevops[0];
    opsarray = iodevops;

    pthread_cond_init (&this->puncond, NULL);
    pthread_cond_init (&this->rdrcond, NULL);
    pthread_mutex_init (&this->lock, NULL);
    this->punrun = false;
    this->rdrrun = false;
    this->punfd = -1;
    this->rdrfd = -1;
}

// reset the device
// - clear flags
// - kill threads
// http://homepage.divms.uiowa.edu/~jones/pdp8/man/papertape.html
void IODevPTape::ioreset ()
{
    pthread_mutex_lock (&this->lock);
    this->rdrflag = false;
    this->intenab = false;
    this->punflag = false;
    this->punfull = false;
    clrintreqmask (IRQ_PTAPE);
    pthread_cond_broadcast (&this->rdrcond);

    if (this->rdrfd >= 0) {

        // tell existing thread to stop looping
        this->rdrrun = false;

        // prevent any new read() calls from blocking
        // ...whilst preserving this->rdrfd
        int nullfd = open ("/dev/null", O_RDWR);
        if (nullfd < 0) ABORT ();
        dup2 (nullfd, this->rdrfd);
        close (nullfd);

        // break rdrthread() out of any read() call it may be in
        pthread_kill (this->rdrtid, SIGCHLD);

        pthread_mutex_unlock (&this->lock);

        // wait for thread to terminate
        pthread_join (this->rdrtid, NULL);
        this->rdrtid = 0;

        // all done with the now /dev/null fd
        close (this->rdrfd);
        this->rdrfd = -1;

        pthread_mutex_lock (&this->lock);
    }

    if (this->punfd >= 0) {

        // tell existing thread to stop looping
        this->punrun = false;

        // prevent any new read() calls from blocking
        // ...whilst preserving this->punfd
        int nullfd = open ("/dev/null", O_RDWR);
        if (nullfd < 0) ABORT ();
        dup2 (nullfd, this->punfd);
        close (nullfd);

        pthread_mutex_unlock (&this->lock);

        // wait for thread to terminate
        pthread_join (this->puntid, NULL);
        this->puntid = 0;

        // all done with the now /dev/null fd
        close (this->punfd);
        this->punfd = -1;

        pthread_mutex_lock (&this->lock);
    }

    pthread_mutex_unlock (&this->lock);
}

// 03: teletype keyboard (p257)
// 04: teletype printer (p257)
// http://homepage.divms.uiowa.edu/~jones/pdp8/man/tty.html
uint16_t IODevPTape::ioinstr (uint16_t opcode, uint16_t input)
{
    // process opcode
    switch (opcode) {

        // RPI: interrupt enable
        case 06010: {
            pthread_mutex_lock (&this->lock);
            this->intenab = true;
            updintreq ();
            pthread_mutex_unlock (&this->lock);
            break;
        }

        // RSF: skip if there is a reader byte to be read
        case 06011: {
            if (this->rdrflag) input |= IO_SKIP;
            break;
        }

        // RRB: reader buffer ored with accumulator; clear reader flag
        case 06012: {
            input &= IO_LINK;
            pthread_mutex_lock (&this->lock);
            input |= this->rdrbuff;
            this->rdrflag = false;
            updintreq ();
            pthread_cond_broadcast (&this->rdrcond);
            pthread_mutex_unlock (&this->lock);
            break;
        }

        // RFC: clear reader flag and start reading next char from tape
        case 06014: {
            if (! this->rdrrun) this->rdrstart ();
            pthread_mutex_lock (&this->lock);
            this->rdrflag = false;
            updintreq ();
            pthread_cond_broadcast (&this->rdrcond);
            pthread_mutex_unlock (&this->lock);
            break;
        }

        // RRB RFC: reader buffer ored into accumulator; reader flag cleared; start reading next character
        case 06016: {
            if (! this->rdrrun) this->rdrstart ();
            pthread_mutex_lock (&this->lock);
            input |= this->rdrbuff;
            this->rdrflag = false;
            updintreq ();
            pthread_cond_broadcast (&this->rdrcond);
            pthread_mutex_unlock (&this->lock);
            break;
        }

        // PCE: clear interrupt enable
        case 06020: {
            pthread_mutex_lock (&this->lock);
            this->intenab = false;
            updintreq ();
            pthread_mutex_unlock (&this->lock);
            break;
        }

        // PSF: skip if punch ready to accept next byte
        case 06021: {
            if (this->punflag) input |= IO_SKIP;
            break;
        }

        // PCF: clear punch flag, pretending punch is busy
        case 06022: {
            pthread_mutex_lock (&this->lock);
            this->punflag = false;
            updintreq ();
            pthread_mutex_unlock (&this->lock);
            break;
        }

        // PPC: start punching a byte
        case 06024: {
            if (! this->punrun) this->punstart ();
            pthread_mutex_lock (&this->lock);
            this->punbuff = input & 0177;
            this->punfull = true;
            pthread_cond_broadcast (&this->puncond);
            pthread_mutex_unlock (&this->lock);
            break;
        }

        // PLS: punch load sequence = PCF + PPC
        //      copy accum to punch buffer, initiate output, reset punch flag
        case 06026: {
            if (! this->punrun) this->punstart ();
            pthread_mutex_lock (&this->lock);
            this->punbuff = input & 0177;
            this->punfull = true;
            this->punflag = false;
            pthread_cond_broadcast (&this->puncond);
            pthread_mutex_unlock (&this->lock);
            break;
        }

        default: input = UNSUPIO;
    }
    return input;
}

// start thread running
void IODevPTape::rdrstart ()
{
    this->rdrrun = true;

    char const *rdrname = getenv ("iodevptaperdr");
    if (rdrname == NULL) {
        fprintf (stderr, "IODevPTape::rdrstart: envar iodevptaperdr not defined\n");
        ABORT ();
    }
    this->rdrfd = open (rdrname, O_RDONLY);
    if (this->rdrfd < 0) {
        fprintf (stderr, "IODevPTape::rdrstart: error opening %s: %m\n", rdrname);
        ABORT ();
    }

    char const *env = getenv ("iodevptaperdr_debug");
    this->rdrdebug = (env != NULL) && (env[0] & 1);
    if (this->rdrdebug) printf ("IODevPTape::rdrstart: starting file %s\n", rdrname);

    int rc = pthread_create (&this->rdrtid, NULL, rdrthreadwrap, this);
    if (rc != 0) ABORT ();
}

void *IODevPTape::rdrthreadwrap (void *zhis)
{
    ((IODevPTape *)zhis)->rdrthread ();
    return NULL;
}

void IODevPTape::rdrthread ()
{
    fd_set readfds;
    int index = 0;
    uint64_t lastreadat = 0;
    FD_ZERO (&readfds);
start:;
    pthread_mutex_lock (&this->lock);
    while (this->rdrrun) {

        // wait for buffer emptied before reading another byte
        if (this->rdrflag) {
            pthread_cond_wait (&this->rdrcond, &this->lock);
            continue;
        }
        pthread_mutex_unlock (&this->lock);

        // make sure it has been 5mS since last read to give processor time to handle previous char
        struct timeval nowtv;
        gettimeofday (&nowtv, NULL);
        uint64_t now = nowtv.tv_sec * 1000000ULL + nowtv.tv_usec;
        uint64_t nextreadat = lastreadat + 1000000/200;  // 200 chars/sec
        if (nextreadat > now) {
            usleep (nextreadat - now);
            goto start;
        }
        lastreadat = now;

        // read char, waiting for one if necessary
        // allow IODevPTape::ioreset() to abort the read by using a select()
        FD_SET (this->rdrfd, &readfds);
        int rc;
        do {
            rc = select (this->rdrfd + 1, &readfds, NULL, NULL, NULL);
            if (! this->rdrrun) return;
        } while ((rc < 0) && (errno == EINTR));
        if (rc > 0) rc = read (this->rdrfd, &this->rdrbuff, 1);
        if (rc <= 0) {
            if (rc == 0) errno = EPIPE;
            fprintf (stderr, "IODevPTape::rdrthread: read error: %m\n");
            ABORT ();
        } else if (this->rdrdebug) {
            printf ("IODevPTape::rdrthread: [%6d] read %03o\n", index ++, this->rdrbuff);
        }

        // set flag saying there is a character ready and maybe request interrupt
        pthread_mutex_lock (&this->lock);
        this->rdrflag = true;
        updintreq ();
    }
    pthread_mutex_unlock (&this->lock);
}

// start punch thread running
void IODevPTape::punstart ()
{
    this->punrun = true;

    char const *punname = getenv ("iodevptapepun");
    if (punname == NULL) {
        fprintf (stderr, "IODevPTape::startitup: envar iodevptapepun not defined\n");
        ABORT ();
    }
    this->punfd = open (punname, O_WRONLY);
    if (this->punfd < 0) {
        fprintf (stderr, "IODevPTape::startitup: error opening %s: %m\n", punname);
        ABORT ();
    }

    int rc = pthread_create (&this->rdrtid, NULL, punthreadwrap, this);
    if (rc != 0) ABORT ();
}

void *IODevPTape::punthreadwrap (void *zhis)
{
    ((IODevPTape *)zhis)->punthread ();
    return NULL;
}

void IODevPTape::punthread ()
{
    uint64_t lastwriteat = 0;
start:;
    pthread_mutex_lock (&this->lock);
    while (this->punrun) {

        // wait for processor to put a character in buffer
        if (! this->punfull) {
            pthread_cond_wait (&this->puncond, &this->lock);
        } else {
            pthread_mutex_unlock (&this->lock);

            // make sure it has been at least 0.1S since last character printed
            struct timeval nowtv;
            gettimeofday (&nowtv, NULL);
            uint64_t now = nowtv.tv_sec * 1000000ULL + nowtv.tv_usec;
            uint64_t nextwriteat = lastwriteat + 1000000/50;    // 50 chars/sec
            if (nextwriteat > now) {
                usleep (nextwriteat - now);
                goto start;
            }
            lastwriteat = now;

            // start punching character
            int rc = write (this->punfd, &this->punbuff, 1);
            if (rc == 0) errno = EPIPE;
            if (rc <= 0) {
                fprintf (stderr, "IODevPTape::punthread: write error: %m\n");
                ABORT ();
            }

            // set flag saying punch able to accept another char and maybe request interrupt
            pthread_mutex_lock (&this->lock);
            this->punfull = false;
            this->punflag = true;
            updintreq ();
        }
    }
    pthread_mutex_unlock (&this->lock);
}

// update interrupt request line after changing intenab, rdrflag, punflag
// assumes this->lock is locked
void IODevPTape::updintreq ()
{
    if (this->intenab & (this->rdrflag | this->punflag)) {
        setintreqmask (IRQ_TTYKBPR);
    } else {
        clrintreqmask (IRQ_TTYKBPR);
    }
}
