//    Copyright (C) Mike Rieker, Beverly, MA USA
//    www.outerworldapps.com
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; version 2 of the License.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    EXPECT it to FAIL when someone's HeALTh or PROpeRTy is at RISk.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    http://www.gnu.org/licenses/gpl-2.0.html

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <net/if.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <unistd.h>

#include "iodevtty.h"
#include "linc.h"
#include "memory.h"
#include "shadow.h"

IODevTTY iodevtty;
IODevTTY iodevtty40(040);
IODevTTY iodevtty42(042);
IODevTTY iodevtty44(044);
IODevTTY iodevtty46(046);

static IODevOps const iodevopsdef[] = {
    { 06030, "03 KCF (TTY) clear kb flag so we will know when another char gets read in" },
    { 06031, "03 KSF (TTY) skip if there is a kb character to be read" },
    { 06032, "03 KCC (TTY) clear kb flag, clear accumulator" },
    { 06034, "03 KRS (TTY) read kb char but don't clear flag" },
    { 06035, "03 KIE (TTY) set/clear interrupt enable for both kb and pr" },
    { 06036, "03 KRB (TTY) read kb character and clear flag" },
    { 06040, "03 TFL (TTY) pretend the printer is ready to accept a character" },
    { 06041, "03 TSF (TTY) skip if printer is ready to accept a character" },
    { 06042, "03 TCF (TTY) pretend the printer is busy printing a character" },
    { 06044, "03 TPC (TTY) start printing a character" },
    { 06045, "03 TSK (TTY) skip if currently requesting either keyboard or printer interrupt" },
    { 06046, "03 TLS (TTY) turn off interrupt request for previous char and start printing new char" },
};

static uint64_t intreqbits;

static uint64_t getnowus ()
{
    struct timeval nowtv;
    if (gettimeofday (&nowtv, NULL) < 0) ABORT ();
    return nowtv.tv_sec * 1000000ULL + nowtv.tv_usec;
}

// iobase by default is 003
// typical alternates are 040, 042, 044, 046
IODevTTY::IODevTTY (uint16_t iobase)
{
    // save base io opcode
    if ((iobase < 003) || (iobase > 076)) {
        fprintf (stderr, "IODevTTY: bad iobase 0%02o, allowed 003..076\n", iobase);
        ABORT ();
    }
    iobasem3 = iobase - 3;

    if (iobasem3 == 0) iodevname = "tty";
    else {
        sprintf (ttydevname, "tty%02o", iobase);
        iodevname = ttydevname;
    }

    int lodig = iobase & 7;
    int hidig = iobase / 8;
    this->servport = hidig * 10 + lodig + 12300;

    // set up disassembly based on base io opcode
    opscount = sizeof iodevopsdef / sizeof iodevopsdef[0];
    opsarray = iodevopsdef;
    if (iobasem3 > 0) {
        IODevOps *iodevopsall = new IODevOps[opscount];
        opsarray = iodevopsall;

        for (uint16_t i = 0; i < opscount; i ++) {
            iodevopsall[i].opcd = iodevopsdef[i].opcd + iobasem3 * 010;
            char *desc = strdup (iodevopsdef[i].desc);
            desc[0] += iobasem3 / 010;
            desc[1] += iobasem3 & 007;
            if (desc[1] == '8') {
                desc[0] ++;
                desc[1] = '0';
            }
            iodevopsall[i].desc = desc;
        }
    }

    // initialize struct elements
    pthread_cond_init (&this->prcond, NULL);
    pthread_mutex_init (&this->lock, NULL);
    this->intenab  = false;
    this->ksfwait  = false;
    this->tsfwait  = false;
    this->lastpc   = 0xFFFFU;
    this->lastin   = 0xFFFFU;
    this->lastout  = 0xFFFFU;
    this->kbfd     = -1;
    this->kbsetup  = false;
    this->kbstdin  = false;
    this->prfd     = -1;
    this->tlfd     = -1;
    this->kbtid    =  0;
    this->prtid    =  0;
    this->eight    = false;
    this->lcucin   = false;
    this->stopping = false;
    this->telnetd  = false;
    this->usperchr = 1000000 / 120;
}

IODevTTY::~IODevTTY ()
{
    pthread_mutex_lock (&this->lock);
    stopthreads ();
    pthread_mutex_unlock (&this->lock);
}

// process commands from TCL script
SCRet *IODevTTY::scriptcmd (int argc, char const *const *argv)
{
    // debug [0/1/2]
    if (strcmp (argv[0], "debug") == 0) {
        if (argc == 1) {
            return new SCRetInt (this->debug);
        }

        if (argc == 2) {
            this->debug = atoi (argv[1]);
            return NULL;
        }

        return new SCRetErr ("iodev %s debug [0/1/2]", iodevname);
    }

    // eight [0/1]
    if (strcmp (argv[0], "eight") == 0) {
        if (argc == 1) {
            return new SCRetInt (this->eight ? 1 : 0);
        }

        if (argc == 2) {
            this->eight = atoi (argv[1]) != 0;
            return NULL;
        }

        return new SCRetErr ("iodev %s eight [0/1]", iodevname);
    }

    if (strcmp (argv[0], "help") == 0) {
        puts ("");
        puts ("valid sub-commands:");
        puts ("");
        puts ("  debug               - see if debug is enabled");
        puts ("  debug 0/1/2         - set debug printing");
        puts ("  eight [0/1]         - allow all 8 bits to pass through");
        puts ("  lcucin [0/1]        - lowercase->uppercase on input");
        puts ("  pipes <kb> [<pr>]   - use named pipes or /dev/pty/... for i/o");
        puts ("                        <pr> defaults to same as <kb>");
        puts ("                        dash (-) or dash dash (- -) means stdin and stdout");
        puts ("  speed               - see what simulated chars-per-second rate is");
        puts ("  speed <charspersec> - set simulated chars-per-second rate");
        puts ("                        allowed range 1..1000000");
        puts ("  telnet <tcpport>    - start listening for inbound telnet connection");
        puts ("                        allowed range 1..65535");
        puts ("");
        return NULL;
    }

    // lcucin [0/1/2]
    if (strcmp (argv[0], "lcucin") == 0) {
        if (argc == 1) {
            return new SCRetInt (this->lcucin ? 1 : 0);
        }

        if (argc == 2) {
            this->lcucin = atoi (argv[1]) != 0;
            return NULL;
        }

        return new SCRetErr ("iodev %s lcucin [0/1]", iodevname);
    }

    // pipes <kb> <pr>
    if (strcmp (argv[0], "pipes") == 0) {
        if (argc == 2) {
            return openpipes (argv[1], argv[1]);
        }
        if (argc == 3) {
            return openpipes (argv[1], argv[2]);
        }

        return new SCRetErr ("iodev %s pipes <kb-pipe-or-pty-or-dash> [<pr-pipe-or-pty-or-dash>]", iodevname);
    }

    // speed <charpersec>
    if (strcmp (argv[0], "speed") == 0) {
        if (argc == 1) {
            return new SCRetInt (1000000 / this->usperchr);
        }

        if (argc == 2) {
            char *p;
            int cps = strtol (argv[1], &p, 0);
            if ((*p != 0) || (cps < 1) || (cps > 1000000)) return new SCRetErr ("speed %s not in range 1..1000000", argv[1]);
            this->usperchr = 1000000 / cps;
            return NULL;
        }

        return new SCRetErr ("iodev %s speed [<charpersec>]", iodevname);
    }

    // telnet <tcpport>
    if (strcmp (argv[0], "telnet") == 0) {
        if (argc == 2) {
            char *p;
            int port = strtol (argv[1], &p, 0);
            if ((*p != 0) || (port < 1) || (port > 65535)) return new SCRetErr ("tcpport %s not in range 1..65535", argv[1]);

            struct sockaddr_in servaddr;
            memset (&servaddr, 0, sizeof servaddr);
            servaddr.sin_family = AF_INET;
            servaddr.sin_port = htons (port);

            int lfd = socket (AF_INET, SOCK_STREAM, 0);
            if (lfd < 0) {
                fprintf (stderr, "IODevTTY::scriptcmd: %02o socket error: %m\n", iobasem3 + 3);
                ABORT ();
            }
            int one = 1;
            if (setsockopt (lfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one) < 0) {
                fprintf (stderr, "IODevTTY::scriptcmd: %02o SO_REUSEADDR error: %m\n", iobasem3 + 3);
                ABORT ();
            }
            if (bind (lfd, (sockaddr *)&servaddr, sizeof servaddr) < 0) {
                SCRetErr *err = new SCRetErr ("bind: %s", strerror (errno));
                close (lfd);
                return err;
            }
            if (listen (lfd, 1) < 0) {
                fprintf (stderr, "IODevTTY::scriptcmd: %02o listen error: %m\n", iobasem3 + 3);
                ABORT ();
            }

            pthread_mutex_lock (&this->lock);
            stopthreads ();
            this->servport = port;
            this->kbsetup  = false;
            this->kbstdin  = false;

            ASSERT (this->kbfd == -1);
            ASSERT (this->prfd == -1);
            ASSERT (this->tlfd == -1);
            ASSERT (this->kbtid == 0);
            ASSERT (this->prtid == 0);

            this->tlfd = lfd;
            this->telnetd = true;

            int rc = pthread_create (&this->kbtid, NULL, tcpthreadwrap, this);
            if (rc != 0) ABORT ();

            pthread_mutex_unlock (&this->lock);
            return NULL;
        }

        return new SCRetErr ("iodev %s telnet <tcpport>", iodevname);
    }

    return new SCRetErr ("unknown tty command %s - valid: debug eight help lcucin pipes speed telnet", argv[0]);
}

// open the given pipes and start the threads
SCRetErr *IODevTTY::openpipes (char const *kbname, char const *prname)
{
    bool kbstdin = strcmp (kbname, "-") == 0;
    int kbfd = kbstdin ? dup (0) : open (kbname, O_RDONLY);
    if (kbfd < 0) {
        return new SCRetErr ("error opening %s: %m", kbname);
    }
    int prfd = (strcmp (prname, "-") == 0) ? dup (1) : open (prname, O_WRONLY);
    if (prfd < 0) {
        int en = errno;
        close (kbfd);
        errno = en;
        return new SCRetErr ("error opening %s: %m", prname);
    }

    pthread_mutex_lock (&this->lock);
    stopthreads ();

    ASSERT (this->kbfd == -1);
    ASSERT (this->prfd == -1);
    ASSERT (this->tlfd == -1);
    ASSERT (this->kbtid == 0);
    ASSERT (this->prtid == 0);

    this->kbfd = kbfd;
    this->prfd = prfd;
    this->telnetd = false;
    this->kbstdin = kbstdin;

    // if processor started doing i/o on this keyboard aleady
    // ...set it to raw mode if keyboard is a tty
    if (this->kbsetup) this->setkbrawmode ();

    // start the threads going
    int rc = pthread_create (&this->kbtid, NULL, kbthreadwrap, this);
    if (rc != 0) ABORT ();

    rc = pthread_create (&this->prtid, NULL, prthreadwrap, this);
    if (rc != 0) ABORT ();

    pthread_mutex_unlock (&this->lock);
    return NULL;
}

// stop the keyboard and printer threads and close fds
// call with mutex locked, returns with mutex locked
void IODevTTY::stopthreads ()
{
    // maybe another thread is stopping everything
    while (this->stopping) {
        pthread_cond_wait (&this->prcond, &this->lock);
    }

    // nothing else is trying to stop them, see if threads are stopped
    if ((this->kbtid != 0) || (this->prtid != 0)) {

        // if we had a keyboard set up, restore its attributes
        if (this->kbsetup && (isatty (this->kbfd) > 0) && (tcsetattr (this->kbfd, TCSAFLUSH, &this->oldattr) < 0)) ABORT ();

        // kb needs setup next time accessed
        // this flag gets set only when an i/o instruction is executed for this terminal
        // ...so we delay setting the attributes to raw until the processor starts using it
        this->kbsetup = false;

        // replace keyboard device with null device
        // ...so poll() will complete immediately if it hasn't begun
        int nullfd = open ("/dev/null", O_RDWR);
        if (nullfd < 0) ABORT ();

        int kbfd = this->kbfd;
        if ((kbfd >= 0) && (dup2 (nullfd, kbfd) < 0)) ABORT ();

        // same with printing device
        // make that poll() complete immediately if it hasn't begun
        int prfd = this->prfd;
        if ((prfd >= 0) && (dup2 (nullfd, prfd) < 0)) ABORT ();

        // maybe there is a tcp listener going
        // make that poll() complete immediately if it hasn't begun
        int tlfd = this->tlfd;
        if ((tlfd >= 0) && (dup2 (nullfd, tlfd) < 0)) ABORT ();

        close (nullfd);

        // in case prthread() is blocked waiting for processor to print something
        pthread_cond_broadcast (&this->prcond);

        // tell threads to exit
        this->kbfd = -1;
        this->prfd = -1;
        this->tlfd = -1;
        this->stopping = true;
        pthread_mutex_unlock (&this->lock);

        // break them out of poll()s
        if (this->kbtid != 0) pthread_kill (this->kbtid, SIGCHLD);
        if (this->prtid != 0) pthread_kill (this->prtid, SIGCHLD);

        // wait for threads to exit
        if (this->kbtid != 0) pthread_join (this->kbtid, NULL);
        if (this->prtid != 0) pthread_join (this->prtid, NULL);

        // the threads are no longer running
        pthread_mutex_lock (&this->lock);
        this->stopping = false;
        this->kbtid = 0;
        this->prtid = 0;

        // tell anyone who cares that threads have exited
        pthread_cond_broadcast (&this->prcond);
    }

    ASSERT (this->kbfd == -1);
    ASSERT (this->prfd == -1);
    ASSERT (this->tlfd == -1);
    ASSERT (this->kbtid == 0);
    ASSERT (this->prtid == 0);
}

void *IODevTTY::tcpthreadwrap (void *zhis)
{
    ((IODevTTY *)zhis)->tcpthread ();
    return NULL;
}

// listen for incoming telnet connection
// process that one connection until it terminates or we are killed
void IODevTTY::tcpthread ()
{
    pthread_mutex_lock (&this->lock);
    while (this->tlfd >= 0) {

        fprintf (stderr, "IODevTTY::tcpthread: %02o listening on %d\n", iobasem3 + 3, this->servport);

        // wait for a connection to be available
        // use a poll() so we can be aborted by stopthreads()
        struct pollfd pfd = { this->tlfd, POLLIN, 0 };
        while (true) {
            pthread_mutex_unlock (&this->lock);
            int rc = poll (&pfd, 1, -1);
            int en = errno;
            pthread_mutex_lock (&this->lock);
            if (this->tlfd < 0) goto done;
            if (rc > 0) break;
            if ((rc == 0) || (en != EINTR)) ABORT ();
        }

        // accept a connection
        struct sockaddr_in clinaddr;
        memset (&clinaddr, 0, sizeof clinaddr);
        socklen_t addrlen = sizeof clinaddr;
        int cfd = accept (this->tlfd, (sockaddr *)&clinaddr, &addrlen);
        if (cfd < 0) {
            fprintf (stderr, "IODevTTY::tcpthread: %02o accept error: %m\n", iobasem3 + 3);
            continue;
        }
        int one = 1;
        if (setsockopt (cfd, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof one) < 0) {
            ABORT ();
        }
        fprintf (stderr, "IODevTTY::tcpthread: %02o accepted from %s:%d\n",
            iobasem3 + 3, inet_ntoa (clinaddr.sin_addr), ntohs (clinaddr.sin_port));

        // ref: http://support.microsoft.com/kb/231866
        static uint8_t const initmodes[] = {
            255, 251, 0,        // IAC WILL TRANSMIT-BINARY
            255, 251, 1,        // IAC WILL ECHO
            255, 251, 3,        // IAC WILL SUPPRESS-GO-AHEAD
            255, 253, 0,        // IAC DO   TRANSMIT-BINARY
            255, 253, 1,        // IAC DO   ECHO
            255, 253, 3,        // IAC DO   SUPPRESS-GO-AHEAD
        };

        int rc = write (cfd, initmodes, sizeof initmodes);
        if (rc < 0) {
            fprintf (stderr, "IODevTTY::tcpthread: %02o write error: %m\n", iobasem3 + 3);
            ABORT ();
        }
        if (rc != sizeof initmodes) {
            fprintf (stderr, "IODevTTY::tcpthread: %02o only wrote %d of %d chars\n", iobasem3 + 3, rc, (int) sizeof initmodes);
            ABORT ();
        }

        this->kbfd = cfd;
        this->prfd = dup (cfd); // separate fd so stopthreads() won't double-close the same fd
        if (this->prfd < 0) ABORT ();

        // do printer stuff in separate thread
        rc = pthread_create (&this->prtid, NULL, prthreadwrap, this);
        if (rc != 0) ABORT ();

        // process keyboard stuff in this thread
        this->kbthreadlk ();
        ASSERT (this->kbfd < 0);

        // stop printer thread and close channel
        if (! this->stopping && (this->prfd >= 0)) {

            // replace tcp connection with /dev/null so poll() will complete immediately if it hasn't started yet
            int nullfd = open ("/dev/null", O_RDWR);
            if (nullfd < 0) ABORT ();
            if (dup2 (nullfd, this->prfd) < 0) ABORT ();
            close (nullfd);

            // wake prthread() out of pthread_cond_wait() if it is in one or just about to
            pthread_cond_broadcast (&this->prcond);
            this->stopping = true;
            pthread_mutex_unlock (&this->lock);

            // break it out of poll() if it is in one for the actual tcp connection
            pthread_kill (this->prtid, SIGCHLD);

            // wait for prthread() to exit
            pthread_join (this->prtid, NULL);
            pthread_mutex_lock (&this->lock);
            this->prtid = 0;

            // it should have closed its fd
            ASSERT (this->prfd < 0);
            this->stopping = false;

            // tell anyone who cares that threads have exited
            pthread_cond_broadcast (&this->prcond);
        }
    }
done:;
    ASSERT (this->kbfd < 0);
    ASSERT (this->prfd < 0);
    pthread_mutex_unlock (&this->lock);
}

void *IODevTTY::kbthreadwrap (void *zhis)
{
    pthread_mutex_lock (&((IODevTTY *)zhis)->lock);
    ((IODevTTY *)zhis)->kbthreadlk ();
    pthread_mutex_unlock (&((IODevTTY *)zhis)->lock);
    return NULL;
}

void IODevTTY::kbthreadlk ()
{
    int skipchars = 0;
    uint64_t nextreadus = getnowus ();

    // make sure we aren't aborted
    while (this->kbfd >= 0) {

        // wait for a character to be available
        // use a poll() so we can be aborted by stopthreads()
        struct pollfd pfd = { this->kbfd, POLLIN, 0 };
        while (true) {
            pthread_mutex_unlock (&this->lock);

            // don't check poll until it has been at least 1/CPS sec since last kb read
            if (skipchars == 0) {
                int64_t delay = nextreadus - getnowus ();
                if (delay > 0) {
                    usleep (delay);
                    pthread_mutex_lock (&this->lock);
                    continue;
                }
            }

            // it's been a while, so block here until keyboard char is available
            // this can get sigint'd by stopthreads()
            int rc = poll (&pfd, 1, -1);
            int en = errno;

            // return if connection fd has been closed
            pthread_mutex_lock (&this->lock);
            if (this->kbfd < 0) return;

            // break out of loop if char ready to be read
            if (rc > 0) break;

            // otherwise it should be EINTR, repeat if so
            if ((rc == 0) || (en != EINTR)) ABORT ();
        }

        // we know a char is available (or eof), read it
        int rc = read (this->kbfd, &this->kbbuff, 1);
        if (rc <= 0) {
            if (rc == 0) fprintf (stderr, "IODevTTY::kbthread: %02o eof reading from pipe\n", iobasem3 + 3);
              else fprintf (stderr, "IODevTTY::kbthread: %02o error reading from pipe: %m\n", iobasem3 + 3);
            if ((isatty (this->kbfd) > 0) && (tcsetattr (this->kbfd, TCSAFLUSH, &this->oldattr) < 0)) ABORT ();
            close (this->kbfd);
            this->kbfd = -1;
            break;
        }

        // discard telnet IAC x x sequences
        if (this->telnetd) {
            if (this->kbbuff == 255) skipchars = 3;
            if ((skipchars > 0) && (-- skipchars >= 0)) continue;
        }

        // maybe convert lower case to upper case
        if (this->lcucin && ((this->kbbuff & 0177) >= 'a') && ((this->kbbuff & 0177) <= 'z')) this->kbbuff += 'A' - 'a';

        // pdp-8 software likes top bit set (eg, focal, os/8)
        if (! this->eight) this->kbbuff |= 0200;

        if (this->debug > 0) {
            uint8_t kbchar = ((this->kbbuff < 0240) | (this->kbbuff > 0376)) ? '.' : (this->kbbuff & 0177);
            printf ("IODevTTY::kbthread: kbbuff=%03o <%c>\n", this->kbbuff, kbchar);
        }

        nextreadus = getnowus () + this->usperchr;

        // we have a character now, maybe request interrupt
        this->kbflag = true;
        this->updintreqlk ();
    }
}

void *IODevTTY::prthreadwrap (void *zhis)
{
    ((IODevTTY *)zhis)->prthread ();
    return NULL;
}

void IODevTTY::prthread ()
{
    uint64_t nextwriteus = getnowus ();

    pthread_mutex_lock (&this->lock);

    // make sure we aren't aborted
    while (this->prfd >= 0) {

        // make sure there is another character to print
        if (! this->prfull) {
            pthread_cond_wait (&this->prcond, &this->lock);
            continue;
        }

        // wait for a character room to be available
        // use a poll() so we can be aborted by stopthreads()
        struct pollfd pfd = { this->prfd, POLLOUT, 0 };
        while (true) {
            pthread_mutex_unlock (&this->lock);

            // don't check poll until it has been at least 1/CPS sec since last pr write
            int64_t delay = nextwriteus - getnowus ();
            if (delay > 0) {
                usleep (delay);
                pthread_mutex_lock (&this->lock);
                continue;
            }

            // it's been a while, so block here until line has room for char
            // this can get sigint'd by stopthreads()
            int rc = poll (&pfd, 1, -1);
            int en = errno;

            // return if connection fd has been closed
            pthread_mutex_lock (&this->lock);
            if (this->prfd < 0) goto done;

            // break out of loop if room for char
            if (rc > 0) break;

            // otherwise it should be EINTR, repeat if so
            if ((rc == 0) || (en != EINTR)) ABORT ();
        }

        if (this->debug > 0) {
            uint8_t prchar = ((this->prbuff < 0240) | (this->prbuff > 0376)) ? '.' : (this->prbuff & 0177);
            printf ("IODevTTY::prthread: prbuff=%03o <%c>\n", this->prbuff, prchar);
        }

        // strip top bit off for printing
        if (! this->eight) this->prbuff &= 0177;

        // we know there is room, write it
        int rc = write (this->prfd, &this->prbuff, 1);
        if (rc <= 0) {
            if (rc == 0) fprintf (stderr, "IODevTTY::prthread: %02o eof writing to link\n", iobasem3 + 3);
              else fprintf (stderr, "IODevTTY::prthread: %02o error writing to link: %m\n", iobasem3 + 3);
            close (this->prfd);
            this->prfd = -1;
            goto done;
        }

        nextwriteus = getnowus () + this->usperchr;

        // we have a room for another, maybe request interrupt
        this->prfull = false;
        this->prflag = true;
        this->updintreqlk ();
    }
done:;
    pthread_mutex_unlock (&this->lock);
}

// a tty i/o instruction has been detected, set up keyboard
void IODevTTY::dokbsetup ()
{
    // if kb not open in non-script mode for the default tty, open stdin/stdout
    if ((this->kbfd < 0) && ! scriptmode && (iobasem3 == 0)) {
        pthread_mutex_unlock (&this->lock);
        fprintf (stderr, "IODevTTY::ioinstr: %02o connecting to stdin/stdout\n", iobasem3 + 3);
        SCRetErr *reterr = this->openpipes ("-", "-");
        if (reterr != NULL) {
            fprintf (stderr, "IODevTTY::ioinstr: %02o error opening stdin/stdout: %s\n", iobasem3 + 3, reterr->msg);
            ABORT ();
        }
        pthread_mutex_lock (&this->lock);
        ASSERT (this->kbfd >= 0);
        ASSERT (this->prfd >= 0);
    }

    // if using a tty, set it to raw mode
    if (! this->kbsetup && (this->kbfd >= 0)) this->setkbrawmode ();

    // keyboard is now set up
    // if this->kbfd is < 0, we didn't set it up, but when it gets opened, it will set itself up
    this->kbsetup = true;
}

// if using a tty, set it to raw mode
void IODevTTY::setkbrawmode ()
{
    if (isatty (this->kbfd) > 0) {
        if (tcgetattr (this->kbfd, &this->oldattr) < 0) ABORT ();
        struct termios newattr = this->oldattr;
        cfmakeraw (&newattr);
        newattr.c_oflag |= OPOST;       // still insert CR before LF on output
        if (this->kbstdin) {
            newattr.c_lflag |= ISIG;    // for stdin, still handle ^C, ^Z
        }
        if (tcsetattr (this->kbfd, TCSAFLUSH, &newattr) < 0) ABORT ();
    }
}

// reset the device
// - clear flags
void IODevTTY::ioreset ()
{
    pthread_mutex_lock (&this->lock);
    this->kbflag  = false;
    this->intenab = true;   // CAF (6007) enables TTY interrupt
    this->prflag  = false;
    this->prfull  = false;
    this->updintreqlk ();
    pthread_mutex_unlock (&this->lock);
}

// used by LINC
bool IODevTTY::keystruck ()
{
    return this->kbflag;
}

// 03: teletype keyboard (p257)
// 04: teletype printer (p257)
// http://homepage.divms.uiowa.edu/~jones/pdp8/man/tty.html
uint16_t IODevTTY::ioinstr (uint16_t opcode, uint16_t input)
{
    pthread_mutex_lock (&this->lock);

    // if no listener going in non-script mode, open stdin/stdout
    // also set it to raw if it is a tty device
    if (! this->kbsetup) this->dokbsetup ();

    uint16_t oldinput = input;

    // process opcode
    switch (opcode - iobasem3 * 010) {

        // KCF: clear kb flag so we will know when another char gets read in
        case 06030: {
            this->kbflag = false;
            this->updintreqlk ();
            break;
        }

        // KSF: skip if there is a kb character to be read
        case 06031: {
            if (this->kbflag) input |= IO_SKIP;

            // kbflag clear, check for KSF ; JMP.-1
            // block while waiting for keyboard input
            // unblocks if another interrupt is posted or GUI/script halts
            else skipoptwait (opcode, &this->lock, &this->ksfwait);

            break;
        }

        // KCC: clear kb flag, clear accumulator
        case 06032: {
            this->kbflag = false;
            this->updintreqlk ();
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
            this->intenab = input & 1;
            this->updintreqlk ();
            break;
        }

        // KRB: read kb character and clear flag
        //     = KCC | KRS
        case 06036: {
            input &= ~ IO_DATA;
            input |= this->kbbuff;
            this->kbflag = false;
            this->updintreqlk ();
            break;
        }

        // TFL: pretend the printer is ready to accept a character
        // - used during init to set initial printer ready status
        case 06040: {
            this->prflag = true;
            this->updintreqlk ();
            break;
        }

        // TSF: skip if printer is ready to accept a character
        case 06041: {
            if (this->prflag) input |= IO_SKIP;

            // prflag clear, check for TSF ; JMP.-1
            // block while waiting for printer ready
            // unblocks if another interrupt is posted or GUI/script halts
            else skipoptwait (opcode, &this->lock, &this->tsfwait);

            break;
        }

        // TCF: pretend the printer is busy printing a character
        // - used to turn off printer interrupt request while allowing keyboard to interrupt
        case 06042: {
            this->prflag = false;
            this->updintreqlk ();
            break;
        }

        // TPC: start printing a character
        case 06044: {
            this->prbuff = input;
            this->prfull = true;
            this->updintreqlk ();
            pthread_cond_broadcast (&this->prcond);
            break;
        }

        // TSK: skip if currently requesting either keyboard or printer interrupt
        case 06045: {
            if (getintreqmask () & IRQ_TTYKBPR) input |= IO_SKIP;
            break;
        }

        // TLS: turn off interrupt request for previous char and start printing new char
        case 06046: {
            this->prbuff = input;
            this->prfull = true;
            this->prflag = false;
            this->updintreqlk ();
            pthread_cond_broadcast (&this->prcond);
            break;
        }

        default: input = UNSUPIO;
    }

    if (this->debug > 1) {
        uint16_t pc = shadow.r.pc - 1;
        if ((this->lastpc != pc) || (this->lastin != oldinput) || (this->lastout != input)) {
            this->lastpc  = pc;
            this->lastin  = oldinput;
            this->lastout = input;

            char const *desc = NULL;
            for (uint16_t i = 0; i < this->opscount; i ++) {
                if (this->opsarray[i].opcd == opcode) {
                    desc = this->opsarray[i].desc;
                    break;
                }
            }

            if (desc != NULL) printf ("IODevTTY::ioinstr: %05o  %05o -> %05o  %s\n", pc, oldinput, input, desc);
                     else printf ("IODevTTY::ioinstr: %05o  %05o -> %05o  %04o\n", pc, oldinput, input, opcode);
        }
    }

    pthread_mutex_unlock (&this->lock);
    return input;
}

// update interrupt request line after changing intenab, kbflag, prflag
void IODevTTY::updintreq ()
{
    pthread_mutex_lock (&this->lock);
    this->updintreqlk ();
    pthread_mutex_unlock (&this->lock);
}

void IODevTTY::updintreqlk ()
{
    // see if this tty is requesting an interrupt
    bool intreq = this->intenab && (this->kbflag | this->prflag) && ! (linc.specfuncs & 0040);

    // see which of 64 possible ttys are requesting an interrupt
    ASSERT ((this->iobasem3 >= 0) && (this->iobasem3 <= 63));
    uint64_t newbits = intreq ? __sync_or_and_fetch (&intreqbits, 1ULL << this->iobasem3) :
                            __sync_and_and_fetch (&intreqbits, ~ (1ULL << this->iobasem3));

    // if any of the ttys are requesting an interrupt, send request to processor
    // ...will also unblock from skipoptwait()
    if (newbits) setintreqmask (IRQ_TTYKBPR);

    // none are requesting an interrupt, clear interrupt request going to processor for tty
    // ...but still unblock from skipoptwait()
    // ...if waiting for kb and kb ready or waiting for tt and tt ready
    else clrintreqmask (IRQ_TTYKBPR,
                (this->ksfwait & this->kbflag) |
                (this->tsfwait & this->prflag));
}
