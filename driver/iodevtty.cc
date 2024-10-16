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

#include "controls.h"
#include "iodevtty.h"
#include "linc.h"
#include "memory.h"
#include "rdcyc.h"
#include "shadow.h"

struct TTYStopOn {
    TTYStopOn *next;
    char *buff;
    int hits;
    char reas[1];
};

IODevTTY iodevtty;
IODevTTY iodevtty40(040);
IODevTTY iodevtty42(042);
IODevTTY iodevtty44(044);
IODevTTY iodevtty46(046);

static IODevOps const iodevopsdef[] = {
    { 06030, "KCF   (TTY) clear kb flag so we will know when another char gets read in" },
    { 06031, "KSF   (TTY) skip if there is a kb character to be read" },
    { 06032, "KCC   (TTY) clear kb flag, clear accumulator" },
    { 06034, "KRS   (TTY) read kb char but don't clear flag" },
    { 06035, "KIE   (TTY) set/clear interrupt enable for both kb and pr" },
    { 06036, "KRB   (TTY) read kb character and clear flag" },
    { 06040, "TFL   (TTY) pretend the printer is ready to accept a character" },
    { 06041, "TSF   (TTY) skip if printer is ready to accept a character" },
    { 06042, "TCF   (TTY) pretend the printer is busy printing a character" },
    { 06044, "TPC   (TTY) start printing a character" },
    { 06045, "TSK   (TTY) skip if currently requesting either keyboard or printer interrupt" },
    { 06046, "TLS   (TTY) turn off interrupt request for previous char and start printing new char" },
};

static uint64_t intreqbits;

static uint64_t getnowus ()
{
    struct timeval nowtv;
    if (gettimeofday (&nowtv, NULL) < 0) ABORT ();
    return nowtv.tv_sec * 1000000ULL + nowtv.tv_usec;
}

static bool stoponcheck (TTYStopOn *const stopon, char prchar);

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
            desc[3] = ((iobasem3 + 3) / 010) + '0';
            desc[4] = ((iobasem3 + 3) & 007) + '0';
            iodevopsall[i].desc = desc;
        }
    }

    // initialize struct elements
    pthread_cond_init (&this->kbcond, NULL);
    pthread_cond_init (&this->prcond, NULL);
    pthread_mutex_init (&this->lock, NULL);
    this->intenab  = false;
    this->ksfwait  = false;
    this->tsfwait  = false;
    this->lastpc   = 0xFFFFU;
    this->lastin   = 0xFFFFU;
    this->lastout  = 0xFFFFU;
    this->logfile  = NULL;
    this->logname[0] = 0;
    this->kbfd     = -1;
    this->kbsetup  = false;
    this->kbstdin  = false;
    this->prfd     = -1;
    this->tlfd     = -1;
    this->intid    =  0;
    this->kbtid    =  0;
    this->prtid    =  0;
    this->stopons  = NULL;
    this->injbuf   = NULL;
    this->injeko   = 0;
    this->injidx   = 0;
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
    if (this->logfile != NULL) {
        fclose (this->logfile);
        this->logfile = NULL;
    }
    this->stoponsfree ();
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
        puts ("  debug                   - see if debug is enabled");
        puts ("  debug 0..3              - set debug printing");
        puts ("  eight [0/1]             - allow all 8 bits to pass through");
        puts ("  inject                  - return remaining injection");
        puts ("  inject [-echo] <string> - set up keyboard injection");
        puts ("                            -echo: wait for each char echoed");
        puts ("  lcucin [0/1]            - lowercase->uppercase on input");
        puts ("  logfile                 - get name of current logfile or null");
        puts ("  logfile -               - turn logging off");
        puts ("  logfile <filename>      - log output to the file");
        puts ("  pipes <kb> [<pr>]       - use named pipes or /dev/pty/... for i/o");
        puts ("                            <pr> defaults to same as <kb>");
        puts ("                            dash (-) or dash dash (- -) means stdin and stdout");
        puts ("  speed                   - see what simulated chars-per-second rate is");
        puts ("  speed <charspersec>     - set simulated chars-per-second rate");
        puts ("                            allowed range 1..1000000");
        puts ("  stopon                  - return list of defined stopons");
        puts ("  stopon \"\"               - clear stopon strings");
        puts ("  stopon <string> ...     - set list of strings to stop on");
        puts ("  telnet <tcpport>        - start listening for inbound telnet connection");
        puts ("                            allowed range 1..65535");
        puts ("");
        return NULL;
    }

    // inject          - return remaining injection if any
    // inject <string> - set up injection string
    if (strcmp (argv[0], "inject") == 0) {
        if (argc == 1) {
            SCRet *scret = NULL;
            pthread_mutex_lock (&this->lock);
            if (this->injbuf != NULL) {
                int done = (this->injeko < this->injidx) ? this->injeko : this->injidx;
                scret = new SCRetStr ("%s", this->injbuf + done);
            }
            pthread_mutex_unlock (&this->lock);
            return scret;
        }

        if (argc >= 2) {
            bool echo = false;
            char const *str = NULL;
            for (int i = 0; ++ i < argc;) {
                if (strcmp (argv[i], "-echo") == 0) {
                    echo = true;
                    continue;
                }
                if (argv[i][0] == '-') return new SCRetErr ("bad option %s", argv[i]);
                if (str != NULL) return new SCRetErr ("extra arg %s", argv[i]);
                str = argv[i];
            }
            if (str == NULL) return new SCRetErr ("missing string");

            pthread_mutex_lock (&this->lock);

            // maybe injection thread is running and still outputting an old injection
            bool wasrunning = (this->injbuf != NULL);

            // if so, free off the old injection
            if (wasrunning) free (this->injbuf);

            // in any case, set up new injection
            this->injeko = echo ? 0 : strlen (str);
            this->injidx = 0;
            this->injbuf = strdup (str);
            if (this->injbuf == NULL) ABORT ();

            // if it wasn't running, make sure no other thread is waiting for old injection thread to finish exiting
            // ... then tell other threads we are waiting for old injection thread to finish exiting
            if (! wasrunning) {
                while (this->stopping) {
                    pthread_cond_wait (&this->prcond, &this->lock);
                }
                this->stopping = true;
            }
            pthread_mutex_unlock (&this->lock);

            // if it wasn't running,
            if (! wasrunning) {

                // wait for old thread to finish exiting
                if (this->intid != 0) {
                    pthread_join (this->intid, NULL);
                    this->intid = 0;
                }

                // wake other threads waiting for us to finish off old injection thread
                pthread_mutex_lock (&this->lock);
                this->stopping = false;
                pthread_cond_broadcast (&this->prcond);
                pthread_mutex_unlock (&this->lock);

                // start up a new injection thread
                int rc = createthread (&this->intid, inthreadwrap, this);
                if (rc != 0) ABORT ();
            }
            return NULL;
        }
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

    // logfile [-/filename]
    if (strcmp (argv[0], "logfile") == 0) {
        if (argc == 1) return new SCRetStr (this->logname);
        if (argc == 2) {
            if (strcmp (argv[1], "-") == 0) {
                pthread_mutex_lock (&this->lock);
                if (this->logfile != NULL) {
                    fclose (this->logfile);
                    this->logfile = NULL;
                }
                this->logname[0] = 0;
            } else {
                FILE *lf = fopen (argv[1], "w");
                if (lf == NULL) {
                    return new SCRetErr (strerror (errno));
                }
                setlinebuf (lf);
                pthread_mutex_lock (&this->lock);
                if (this->logfile != NULL) {
                    fclose (this->logfile);
                    this->logfile = NULL;
                }
                strncpy (this->logname, argv[1], sizeof this->logname);
                this->logname[(sizeof this->logname)-1] = 0;
                this->logfile = lf;
            }
            pthread_mutex_unlock (&this->lock);
            return NULL;
        }
        return new SCRetErr ("iodev %s logfile [-/<filename>]", iodevname);
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

    // stopon              - return list of current stopons
    // stopon ""           - clear list of stopons
    // stopon <string> ... - set list of stopons
    if (strcmp (argv[0], "stopon") == 0) {

        // no args, return list of existing stopon strings
        if (argc == 1) {
            pthread_mutex_lock (&this->lock);
            int i = 0;
            for (TTYStopOn *stopon = this->stopons; stopon != NULL; stopon = stopon->next) {
                i ++;
            }
            SCRetList *scretlist = new SCRetList (i);
            i = 0;
            for (TTYStopOn *stopon = this->stopons; stopon != NULL; stopon = stopon->next) {
                scretlist->elems[i++] = new SCRetStr ("%d:%s", stopon->hits, stopon->buff);
            }
            pthread_mutex_unlock (&this->lock);
            return scretlist;
        }

        // args, clear existing list, then make new list from non-null strings
        pthread_mutex_lock (&this->lock);
        this->stoponsfree ();
        TTYStopOn **laststopon = &this->stopons;
        for (int i = 1; i < argc; i ++) {
            int len = strlen (argv[i]);
            if (len > 0) {
                int dnlen = strlen (this->iodevname);
                TTYStopOn *stopon = (TTYStopOn *) malloc (sizeof *stopon + dnlen + 10 + len);
                if (stopon == NULL) ABORT ();
                *laststopon  = stopon;
                laststopon   = &stopon->next;
                stopon->hits = 0;
                // set up stopon->reas string: <iodevname> stopon <stoponstring>
                // then/and point stopon->buff to the <stoponstring> at the end
                memcpy (stopon->reas, this->iodevname, dnlen);
                memcpy (stopon->reas + dnlen, " stopon ", 8);
                stopon->buff = stopon->reas + dnlen + 8;
                memcpy (stopon->buff, argv[i], len + 1);
            }
        }
        *laststopon = NULL;
        pthread_mutex_unlock (&this->lock);

        return NULL;
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

            int rc = createthread (&this->kbtid, tcpthreadwrap, this);
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
    int rc = createthread (&this->kbtid, kbthreadwrap, this);
    if (rc != 0) ABORT ();

    rc = createthread (&this->prtid, prthreadwrap, this);
    if (rc != 0) ABORT ();

    pthread_mutex_unlock (&this->lock);
    return NULL;
}

// stop the injection, keyboard and printer threads and close fds
// call with mutex locked, returns with mutex locked
void IODevTTY::stopthreads ()
{
    // maybe another thread is stopping everything
    while (this->stopping) {
        pthread_cond_wait (&this->prcond, &this->lock);
    }

    // nothing else is trying to stop them, see if threads are stopped
    if ((this->intid != 0) || (this->kbtid != 0) || (this->prtid != 0)) {

        // if injection thread running, tell it to stop
        if (this->injbuf != NULL) {
            if (this->debug > 2) fprintf (stderr, "IODevTTY::stopthreads: stopping injection\n");
            free (this->injbuf);
            this->injbuf = NULL;
            this->injeko = 0;
            this->injidx = 0;
        }

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

        // null out printing device
        // make that poll() complete immediately if it hasn't begun
        int prfd = this->prfd;
        if ((prfd >= 0) && (dup2 (nullfd, prfd) < 0)) ABORT ();

        // maybe there is a tcp listener going
        // make that poll() complete immediately if it hasn't begun
        int tlfd = this->tlfd;
        if ((tlfd >= 0) && (dup2 (nullfd, tlfd) < 0)) ABORT ();

        close (nullfd);

        // in case inthread() or kbthread() is waiting for kbflag to clear
        pthread_cond_broadcast (&this->kbcond);

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
        if (this->intid != 0) pthread_join (this->intid, NULL);
        if (this->kbtid != 0) pthread_join (this->kbtid, NULL);
        if (this->prtid != 0) pthread_join (this->prtid, NULL);

        // the threads are no longer running
        pthread_mutex_lock (&this->lock);
        this->stopping = false;
        this->intid = 0;
        this->kbtid = 0;
        this->prtid = 0;

        // tell anyone who cares that threads have exited
        pthread_cond_broadcast (&this->prcond);
    }

    ASSERT (this->injbuf == NULL);
    ASSERT (this->kbfd   == -1);
    ASSERT (this->prfd   == -1);
    ASSERT (this->tlfd   == -1);
    ASSERT (this->intid  ==  0);
    ASSERT (this->kbtid  ==  0);
    ASSERT (this->prtid  ==  0);
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
        rc = createthread (&this->prtid, prthreadwrap, this);
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

            // otherwise it should be timed out or EINTR, repeat if so
            if ((rc < 0) && (en != EINTR)) ABORT ();
        }

        // if not reading from a tty, make sure processor is ready to accept char
        if (isatty (this->kbfd) <= 0) {
            while (this->kbflag) {
                if (this->kbfd < 0) return;
                pthread_cond_wait (&this->kbcond, &this->lock);
            }
        }

        // we know a char is available (or eof), read it
        {
            int rc = read (this->kbfd, &this->kbbuff, 1);
            if (rc <= 0) {
                if (rc == 0) fprintf (stderr, "IODevTTY::kbthread: %02o eof reading from pipe\n", iobasem3 + 3);
                  else fprintf (stderr, "IODevTTY::kbthread: %02o error reading from pipe: %m\n", iobasem3 + 3);
                if ((isatty (this->kbfd) > 0) && (tcsetattr (this->kbfd, TCSAFLUSH, &this->oldattr) < 0)) ABORT ();
                close (this->kbfd);
                this->kbfd = -1;
                break;
            }
        }

        // discard telnet IAC x x sequences
        if (this->telnetd) {
            if (this->kbbuff == 255) skipchars = 3;
            if ((skipchars > 0) && (-- skipchars >= 0)) continue;
        }

        // notify processor
        this->gotkbchar ();

        nextreadus = getnowus () + this->usperchr * 2;
    }
}

// thread what passes injection string to keyboard
void *IODevTTY::inthreadwrap (void *zhis)
{
    ((IODevTTY *)zhis)->inthread ();
    return NULL;
}

void IODevTTY::inthread ()
{
    if (this->debug > 2) fprintf (stderr, "IODevTTY::inthread: starting\n");
    while (true) {

        // pass next char to processor, but stop if none
        pthread_mutex_lock (&this->lock);
        while (true) {
            if (this->injbuf == NULL) goto done;
            // kbbuff must be empty and echoing must be all caught up
            if (this->debug > 2) fprintf (stderr, "IODevTTY::inthread: kbflag=%d injeko=%d injidx=%d\n", this->kbflag, this->injeko, this->injidx);
            if (! this->kbflag && (this->injeko >= this->injidx)) break;
            pthread_cond_wait (&this->kbcond, &this->lock);
        }
        this->kbbuff = this->injbuf[this->injidx++];
        if (this->debug > 2) fprintf (stderr, "IODevTTY::inthread: kbbuff=%02X\n", this->kbbuff);
        if (this->kbbuff == 0) {
            free (this->injbuf);
            this->injbuf = NULL;
            this->injeko = 0;
            this->injidx = 0;
            break;
        }
        this->gotkbchar ();
        pthread_mutex_unlock (&this->lock);

        // don't inject another until CPS delay
        usleep (this->usperchr * 2);
    }
done:;
    pthread_mutex_unlock (&this->lock);
    if (this->debug > 2) fprintf (stderr, "IODevTTY::inthread: finished\n");
}

// got kb char in kbbuff, tell processor to get it
void IODevTTY::gotkbchar ()
{
    // maybe convert lower case to upper case
    if (this->lcucin && ((this->kbbuff & 0177) >= 'a') && ((this->kbbuff & 0177) <= 'z')) this->kbbuff += 'A' - 'a';

    // pdp-8 software likes top bit set (eg, focal, os/8)
    if (! this->eight) this->kbbuff |= 0200;

    if (this->debug > 0) {
        uint8_t kbchar = ((this->kbbuff < 0240) | (this->kbbuff > 0376)) ? '.' : (this->kbbuff & 0177);
        fprintf (stderr, "IODevTTY::kbthread: kbbuff=%03o <%c>\n", this->kbbuff, kbchar);
    }

    // we have a character now, maybe request interrupt
    this->kbflag = true;
    this->updintreqlk ();
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

        uint8_t prbyte = this->prbuff;

        // strip top bit off for printing
        if (! this->eight) prbyte &= 0177;

        if (this->debug > 0) {
            uint8_t prchar = prbyte & 0177;
            if ((prchar < 0040) | (prchar > 0176)) prchar = '.';
            fprintf (stderr, "IODevTTY::prthread: prbyte=%03o <%c>\n", prbyte, prchar);
        }

        // we know there is room, write it
        if (this->logfile != NULL) fputc (prbyte, this->logfile);
        int rc = write (this->prfd, &prbyte, 1);
        if (rc <= 0) {
            if (rc == 0) fprintf (stderr, "IODevTTY::prthread: %02o eof writing to link\n", iobasem3 + 3);
              else fprintf (stderr, "IODevTTY::prthread: %02o error writing to link: %m\n", iobasem3 + 3);
            close (this->prfd);
            this->prfd = -1;
            goto done;
        }

        nextwriteus = getnowus () + this->usperchr;

        // check for echo of keyboard injection
        if (this->injeko < this->injidx) {
            if (this->debug > 2) fprintf (stderr, "IODevTTY::prthread: injbuf[%d]=%03o\n", this->injeko, this->injbuf[this->injeko]);
            if (this->injbuf[this->injeko] == prbyte) {
                this->injeko ++;
                pthread_cond_broadcast (&this->kbcond);
            }
        }

        // check any stopons, flag processor to stop if any triggered
        bool first = true;
        for (TTYStopOn *stopon = this->stopons; stopon != NULL; stopon = stopon->next) {
            if (stoponcheck (stopon, prbyte) && first) {
                ctl_stopfor (stopon->reas);
                first = false;
            }
        }

        // we have a room for another, maybe request interrupt
        this->prfull = false;
        this->prflag = true;
        this->updintreqlk ();
    }
done:;
    pthread_mutex_unlock (&this->lock);
}

// have outgoing char, step stopon state and see if complete match has occurred
static bool stoponcheck (TTYStopOn *const stopon, char prchar)
{
    // null doesn't match anything so reset
    if (prchar == 0) {
        stopon->hits = 0;
        return false;
    }

    // if char matches next coming up, increment number of matching chars
    // if reached the end, this one is completely matched
    // if previously completely matched, prchar will mismatch on the null
    int i = stopon->hits;
    if (stopon->buff[i] != prchar) {

        // mismatch, but maybe an earlier part is still matched
        // eg, looking for "abcabcabd" but got "abcabcabc", we reset to 6
        //     looking for "abcd" but got "abca", we reset to 1
        // also has to work for case where "aaa" was completely matched last time,
        //   if prchar is 'a', we say complete match this time, else reset to 0
        do {
            char *p = (char *) memrchr (stopon->buff, prchar, i);
            if (p == NULL) {
                i = -1;
                break;
            }
            i = p - stopon->buff;

            // prchar = 'c'
            // hits = 8 in "abcabcabd"
            //                      ^hits
            //    i = 5 in "abcabcabd"
            //                   ^i
        } while (memcmp (stopon->buff, stopon->buff + stopon->hits - i, i) != 0);
    }

    // i = index where prchar matches (or -1 if not at all)
    stopon->hits = ++ i;
    return stopon->buff[i] == 0;
}

// free all the stopons
void IODevTTY::stoponsfree ()
{
    bool sigint = ctl_lock ();
    for (TTYStopOn *stopon; (stopon = this->stopons) != NULL;) {
        this->stopons = stopon->next;
        if (stopreason == stopon->reas) stopreason = "";
        free (stopon);
    }
    ctl_unlock (sigint);
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
    pthread_cond_broadcast (&this->kbcond);
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
            pthread_cond_broadcast (&this->kbcond);
            break;
        }

        // KSF: skip if there is a kb character to be read
        case 06031: {
            if (this->kbflag) input |= IO_SKIP;

            // kbflag clear, check for KSF ; JMP.-1
            // block while waiting for keyboard input
            // unblocks if another interrupt is posted or stopped by GUI/script
            else skipoptwait (opcode, &this->lock, &this->ksfwait);

            break;
        }

        // KCC: clear kb flag, clear accumulator
        case 06032: {
            this->kbflag = false;
            this->updintreqlk ();
            pthread_cond_broadcast (&this->kbcond);
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
            pthread_cond_broadcast (&this->kbcond);
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
            // unblocks if another interrupt is posted or stopped by GUI/script
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

            if (desc != NULL) fprintf (stderr, "IODevTTY::ioinstr: %05o  %05o -> %05o  %s\n", pc, oldinput, input, desc);
                     else fprintf (stderr, "IODevTTY::ioinstr: %05o  %05o -> %05o  %04o\n", pc, oldinput, input, opcode);
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
