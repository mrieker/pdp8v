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
    this->confd    = -1;
    this->lastpc   = 0xFFFFU;
    this->lastin   = 0xFFFFU;
    this->lastout  = 0xFFFFU;
    this->lisfd    = -1;
    this->listid   = 0;
    this->mask8    = 0177;
    this->prtid    = 0;
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
    // debug [0/1]
    if (strcmp (argv[0], "debug") == 0) {
        if (argc == 1) {
            return new SCRetInt (this->debug ? 1 : 0);
        }

        if (argc == 2) {
            this->debug = atoi (argv[1]) != 0;
            return NULL;
        }

        return new SCRetErr ("iodev tty debug [0/1]");
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

        return new SCRetErr ("iodev tty speed [<charpersec>]");
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
                fprintf (stderr, "IODevTTY::scriptcmd: socket error: %m\n");
                ABORT ();
            }
            int one = 1;
            if (setsockopt (lfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one) < 0) {
                fprintf (stderr, "IODevTTY::scriptcmd: SO_REUSEADDR error: %m\n");
                ABORT ();
            }
            if (bind (lfd, (sockaddr *)&servaddr, sizeof servaddr) < 0) {
                SCRetErr *err = new SCRetErr ("bind: %s", strerror (errno));
                close (lfd);
                return err;
            }
            if (listen (lfd, 1) < 0) {
                fprintf (stderr, "IODevTTY::scriptcmd: listen error: %m\n");
                ABORT ();
            }

            pthread_mutex_lock (&this->lock);
            stopthreads ();
            this->servport = port;
            startlisthread (lfd);
            pthread_mutex_unlock (&this->lock);
            return NULL;
        }

        return new SCRetErr ("iodev tty telnet <tcpport>");
    }

    return new SCRetErr ("unknown tty command %s", argv[0]);
}

// stop the listening, keyboard and printer threads
void IODevTTY::stopthreads ()
{
    int lfd = this->lisfd;
    if (lfd >= 0) {

        // replace listening device with null device
        // ...so poll() will complete immediately if it hasn't begun
        int nullfd = open ("/dev/null", O_RDWR);
        if (nullfd < 0) ABORT ();
        if (dup2 (nullfd, lfd) < 0) ABORT ();

        // maybe that thread is reading keyboard,
        // make that poll() complete immediately if it hasn't begun
        int cfd = this->confd;
        if (cfd >= 0) {
            if (dup2 (nullfd, cfd) < 0) ABORT ();
            this->confd = -1;
        }
        close (nullfd);

        // tell listhread() that we want it to exit
        this->lisfd = -2;
        pthread_mutex_unlock (&this->lock);

        // if it is in poll(), break it out
        pthread_kill (this->listid, SIGCHLD);

        // wait for listhread() to exit
        pthread_join (this->listid, NULL);
        pthread_mutex_lock (&this->lock);

        // tell anyone who cares that listhread() has exited
        pthread_cond_broadcast (&this->prcond);
    } else {

        // wait for other thread in stopthreads() to finish killing listhread()
        while (this->lisfd < -1) {
            pthread_cond_wait (&this->prcond, &this->lock);
        }
    }

    ASSERT (this->lisfd == -1);
    ASSERT (this->confd == -1);
}

// start listen thread in non-script mode using default TCP port
void IODevTTY::startdeflisten ()
{
    struct sockaddr_in servaddr;
    memset (&servaddr, 0, sizeof servaddr);
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons (this->servport);

    int lfd = socket (AF_INET, SOCK_STREAM, 0);
    if (lfd < 0) {
        fprintf (stderr, "IODevTTY::startdeflisten: socket error: %m\n");
        ABORT ();
    }
    int one = 1;
    if (setsockopt (lfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one) < 0) {
        fprintf (stderr, "IODevTTY::startdeflisten: SO_REUSEADDR error: %m\n");
        ABORT ();
    }
    if (bind (lfd, (sockaddr *)&servaddr, sizeof servaddr) < 0) {
        fprintf (stderr, "IODevTTY::startdeflisten: bind %d error: %m\n", this->servport);
        ABORT ();
    }
    if (listen (lfd, 1) < 0) {
        fprintf (stderr, "IODevTTY::startdeflisten: listen error: %m\n");
        ABORT ();
    }

    this->startlisthread (lfd);
}

// start thread to listen for incoming connection
void IODevTTY::startlisthread (int lfd)
{
    ASSERT (this->lisfd == -1);
    ASSERT (this->confd == -1);
    ASSERT (this->listid == 0);
    ASSERT (this->prtid  == 0);

    this->lisfd = lfd;

    int rc = pthread_create (&this->listid, NULL, listhreadwrap, this);
    if (rc != 0) ABORT ();
}

void *IODevTTY::listhreadwrap (void *zhis)
{
    ((IODevTTY *)zhis)->listhread ();
    return NULL;
}

// listen for incoming telnet connection
// process that one connection until it terminates or we are killed
void IODevTTY::listhread ()
{
    pthread_mutex_lock (&this->lock);
    while (this->lisfd >= 0) {

        fprintf (stderr, "IODevTTY::listhread: %02o listening on %d\n", iobasem3 + 3, this->servport);

        // wait for a connection to be available
        // use a poll() so we can be aborted by stopthreads()
        struct pollfd pfd = { this->lisfd, POLLIN, 0 };
        while (true) {
            pthread_mutex_unlock (&this->lock);
            int rc = poll (&pfd, 1, -1);
            int en = errno;
            pthread_mutex_lock (&this->lock);
            if (this->lisfd < 0) goto done;
            if (rc > 0) break;
            if ((rc == 0) || (en != EINTR)) ABORT ();
        }

        // accept a connection
        struct sockaddr_in clinaddr;
        memset (&clinaddr, 0, sizeof clinaddr);
        socklen_t addrlen = sizeof clinaddr;
        int cfd = accept (this->lisfd, (sockaddr *)&clinaddr, &addrlen);
        if (cfd < 0) {
            fprintf (stderr, "IODevTTY::listhread: accept error: %m\n");
            continue;
        }
        int one = 1;
        if (setsockopt (cfd, SOL_SOCKET, SO_KEEPALIVE, &one, sizeof one) < 0) {
            ABORT ();
        }
        fprintf (stderr, "IODevTTY::listhread: %02o accepted from %s:%d\n",
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
            fprintf (stderr, "IODevTTY::listhread: write error: %m\n");
            ABORT ();
        }
        if (rc != sizeof initmodes) {
            fprintf (stderr, "IODevTTY::listhread: only wrote %d of %d chars\n", rc, (int) sizeof initmodes);
            ABORT ();
        }

        this->confd = cfd;

        rc = pthread_create (&this->prtid, NULL, prthreadwrap, this);
        if (rc != 0) ABORT ();

        this->kbthreadlk ();
        ASSERT (this->confd < 0);

        pthread_cond_broadcast (&this->prcond);
        pthread_mutex_unlock (&this->lock);
        pthread_join (this->prtid, NULL);

        pthread_mutex_lock (&this->lock);
        this->prtid = 0;
    }
done:;
    ASSERT (this->confd < 0);
    this->lisfd = -1;
    pthread_mutex_unlock (&this->lock);
}

void IODevTTY::kbthreadlk ()
{
    int skipchars = 0;
    uint64_t nextreadus = getnowus ();

    // make sure we aren't aborted
    while (this->confd >= 0) {

        // wait for a character to be available
        // use a poll() so we can be aborted by stopthreads()
        struct pollfd pfd = { this->confd, POLLIN, 0 };
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
            if (this->confd < 0) return;

            // break out of loop if char ready to be read
            if (rc > 0) break;

            // otherwise it should be EINTR, repeat if so
            if ((rc == 0) || (en != EINTR)) ABORT ();
        }

        // we know a char is available (or eof), read it
        int rc = read (this->confd, &this->kbbuff, 1);
        if (rc <= 0) {
            if (rc == 0) fprintf (stderr, "IODevTTY::kbthread: %02o eof reading from link\n", iobasem3 + 3);
              else fprintf (stderr, "IODevTTY::kbthread: %02o error reading from link: %m\n", iobasem3 + 3);
            close (this->confd);
            this->confd = -1;
            break;
        }

        // discard telnet IAC x x sequences
        if (this->kbbuff == 255) skipchars = 3;
        if ((skipchars > 0) && (-- skipchars >= 0)) continue;

        // pdp-8 software likes top bit set (eg, focal, os/8)
        this->kbbuff |= ~ this->mask8;

        if (this->debug) {
            uint8_t kbchar = ((this->kbbuff < 0240) | (this->kbbuff > 0376)) ? '.' : (this->kbbuff & 0177);
            printf ("IODevTTY::kbthread*: kbbuff=%03o <%c>\n", this->kbbuff, kbchar);
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
    while (this->confd >= 0) {

        // make sure there is another character to print
        if (! this->prfull) {
            pthread_cond_wait (&this->prcond, &this->lock);
            continue;
        }

        // wait for a character room to be available
        // use a poll() so we can be aborted by stopthreads()
        struct pollfd pfd = { this->confd, POLLOUT, 0 };
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
            if (this->confd < 0) goto done;

            // break out of loop if room for char
            if (rc > 0) break;

            // otherwise it should be EINTR, repeat if so
            if ((rc == 0) || (en != EINTR)) ABORT ();
        }

        if (this->debug) {
            uint8_t prchar = ((this->prbuff < 0240) | (this->prbuff > 0376)) ? '.' : (this->prbuff & 0177);
            printf ("IODevTTY::prthread*: prbuff=%03o <%c>\n", this->prbuff, prchar);
        }

        // strip top bit off for printing
        this->prbuff &= this->mask8;

        // we know there is room, write it
        int rc = write (this->confd, &this->prbuff, 1);
        if (rc <= 0) {
            if (rc == 0) fprintf (stderr, "IODevTTY::prthread: %02o eof writing to link\n", iobasem3 + 3);
              else fprintf (stderr, "IODevTTY::prthread: %02o error writing to link: %m\n", iobasem3 + 3);
            close (this->confd);
            this->confd = -1;
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

// reset the device
// - clear flags
// - kill threads
void IODevTTY::ioreset ()
{
    pthread_mutex_lock (&this->lock);
    this->kbflag  = false;
    this->intenab = true;   // CAF (6007) enables TTY interrupt
    this->prflag  = false;
    this->prfull  = false;
    clrintreqmask (IRQ_TTYKBPR);
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

    uint16_t oldinput = input;

    // if no listener going in non-script mode, start a default listener
    if ((this->lisfd < 0) && ! scriptmode) {
        this->startdeflisten ();
    }

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

    if (this->debug) {
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

            if (desc != NULL) printf ("IODevTTY::ioinstr*: %05o  %05o -> %05o  %s\n", pc, oldinput, input, desc);
                     else printf ("IODevTTY::ioinstr*: %05o  %05o -> %05o  %04o\n", pc, oldinput, input, opcode);
        }
    }

    pthread_mutex_unlock (&this->lock);
    return input;
}

void IODevTTY::updintreq ()
{
    pthread_mutex_lock (&this->lock);
    this->updintreqlk ();
    pthread_mutex_unlock (&this->lock);
}

// update interrupt request line after changing intenab, kbflag, prflag
void IODevTTY::updintreqlk ()
{
    if (this->intenab && (this->kbflag | this->prflag) && ! (linc.specfuncs & 0040)) {
        setintreqmask (IRQ_TTYKBPR);
    } else {
        clrintreqmask (IRQ_TTYKBPR);
    }
}
