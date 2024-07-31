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

// paper tape device emulator
// small computer handbook 1972 p 7-41..43 (261..263)

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include "iodevptape.h"
#include "miscdefs.h"
#include "rdcyc.h"
#include "shadow.h"

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

    iodevname = "ptape";

    pthread_cond_init (&this->puncond, NULL);
    pthread_cond_init (&this->rdrcond, NULL);
    pthread_mutex_init (&this->lock, NULL);

    this->punfd   = -1;
    this->punrun  = false;
    this->puntid  = 0;
    this->punwarn = false;
    this->rdrfd   = -1;
    this->rdrrun  = false;
    this->rdrtid  = 0;
    this->rdrwarn = false;
}

IODevPTape::~IODevPTape ()
{
    pthread_mutex_lock (&this->lock);
    stoprdrthread ();
    stoppunthread ();
    pthread_mutex_unlock (&this->lock);
}

// reset the device
// - clear flags
// http://homepage.divms.uiowa.edu/~jones/pdp8/man/papertape.html
void IODevPTape::ioreset ()
{
    pthread_mutex_lock (&this->lock);
    this->rdrflag = false;
    this->intenab = true;
    this->punflag = false;
    this->punfull = false;
    clrintreqmask (IRQ_PTAPE);
    pthread_mutex_unlock (&this->lock);
}

// load/unload files
SCRet *IODevPTape::scriptcmd (int argc, char const *const *argv)
{
    if (strcmp (argv[0], "help") == 0) {
        puts ("");
        puts ("valid sub-commands:");
        puts ("");
        puts ("  load punch [-append] [-ascii] [-quick] <filename>");
        puts ("       - load file to be written as paper tape");
        puts ("          -append : append to possibly exsting file");
        puts ("           -ascii : discard nulls; strip top bit off");
        puts ("           -quick : minimal delay between bytes; else 50 bytes/second");
        puts ("  load reader [-ascii] [-quick] <filename>");
        puts ("       - load file to be read as paper tape");
        puts ("           -ascii : add top bit back on");
        puts ("           -quick : minimal delay between bytes; else 200 bytes/second");
        puts ("  punch leader  - punch 64 null bytes (even if -ascii)");
        puts ("  reader status - return reader status");
        puts ("  unload punch  - unload file being written as paper tape");
        puts ("  unload reader - unload file being read as paper tape");
        puts ("");
        return NULL;
    }

    // load [-append] [-ascii] [-quick] reader/punch <filename>
    if (strcmp (argv[0], "load") == 0) {
        bool append = false;
        bool ascii  = false;
        bool quick  = false;
        char const *dname = NULL;
        char const *fname = NULL;
        for (int i = 1; i < argc; i ++) {
            if (strcmp (argv[i], "-append") == 0) {
                append = true;
                continue;
            }
            if (strcmp (argv[i], "-ascii") == 0) {
                ascii = true;
                continue;
            }
            if (strcmp (argv[i], "-quick") == 0) {
                quick = true;
                continue;
            }
            if (argv[i][0] == '-') goto loadusage;
            if (dname == NULL) {
                dname = argv[i];
                continue;
            }
            if (fname == NULL) {
                fname = argv[i];
                continue;
            }
            goto loadusage;
        }
        if (fname == NULL) goto loadusage;

        // load reader <filename>
        if (strcmp (dname, "reader") == 0) {
            int fd = open (fname, O_RDONLY);
            if (fd < 0) return new SCRetErr (strerror (errno));
            pthread_mutex_lock (&this->lock);
            stoprdrthread ();
            this->rdrascii  = ascii;
            this->rdrquick  = quick;
            this->rdrinsert = ascii ? 0 : 0377U;    // insert null at beginning of ascii files
                                                    // OS-8 seems to discard first byte and it should be seen as leader in any case
            this->rdrlastch = 0;
            startrdrthread (fd);
            pthread_mutex_unlock (&this->lock);
            return NULL;
        }

        // load punch <filename>
        if (strcmp (dname, "punch") == 0) {
            int flags = O_WRONLY | O_CREAT | O_APPEND;
            if (! append) flags |= O_TRUNC;
            int fd = open (fname, flags, 0666);
            if (fd < 0) return new SCRetErr (strerror (errno));
            pthread_mutex_lock (&this->lock);
            stoppunthread ();
            this->punascii = ascii;
            this->punquick = quick;
            startpunthread (fd);
            pthread_mutex_unlock (&this->lock);
            return NULL;
        }

    loadusage:;
        return new SCRetErr ("iodev ptape load reader/punch [-append] [-ascii] [-quick] <filename>");
    }

    // punch leader
    if (strcmp (argv[0], "punch") == 0) {
        if ((argc == 2) && (strcmp (argv[1], "leader") == 0)) {
            char leader[64];
            memset (leader, 0, sizeof leader);
            pthread_mutex_lock (&this->lock);
            for (int i = 0; i < (int) sizeof leader;) {
                int rc = write (this->punfd, leader, sizeof leader - i);
                if (rc <= 0) {
                    pthread_mutex_unlock (&this->lock);
                    return new SCRetErr (strerror ((rc == 0) ? EPIPE : errno));
                }
                i += rc;
            }
            pthread_mutex_unlock (&this->lock);
            return NULL;
        }
        return new SCRetErr ("iodev ptape punch leader");
    }

    // reader status
    if (strcmp (argv[0], "reader") == 0) {
        if ((argc == 2) && (strcmp (argv[1], "status") == 0)) {
            pthread_mutex_lock (&this->lock);
            int fd = this->rdrfd;
            LLU pos = (fd < 0) ? 0 : (LLU) lseek (this->rdrfd, 0, SEEK_CUR);
            char name[4000];
            name[0] = 0;
            if (fd >= 0) {
                char link[30];
                snprintf (link, sizeof link, "/proc/self/fd/%d", fd);
                int rc = readlink (link, name, sizeof name - 1);
                if (rc < 0) rc = 0;
                if (rc > (int) sizeof name - 1) rc = sizeof name - 1;
                name[rc] = 0;
            }
            pthread_mutex_unlock (&this->lock);
            return new SCRetStr ("fd=%d;pos=%llu;fn=%s", fd, pos, name);
        }
        return new SCRetErr ("iodev ptape reader status");
    }

    // unload reader/punch
    if (strcmp (argv[0], "unload") == 0) {
        if (argc == 2) {

            // unload reader
            if (strcmp (argv[1], "reader") == 0) {
                pthread_mutex_lock (&this->lock);
                stoprdrthread ();
                pthread_mutex_unlock (&this->lock);
                return NULL;
            }

            // unload punch
            if (strcmp (argv[1], "punch") == 0) {
                pthread_mutex_lock (&this->lock);
                stoppunthread ();
                pthread_mutex_unlock (&this->lock);
                return NULL;
            }
        }

        return new SCRetErr ("iodev ptape unload reader/punch");
    }

    return new SCRetErr ("unknown ptape command %s - valid: help load punch reader unload", argv[0]);
}

// process paper tape I/O instruction
uint16_t IODevPTape::ioinstr (uint16_t opcode, uint16_t input)
{
    pthread_mutex_lock (&this->lock);

    // process opcode
    switch (opcode) {

        // RPI: interrupt enable
        case 06010: {
            this->intenab = true;
            updintreq ();
            break;
        }

        // RSF: skip if there is a reader byte to be read
        case 06011: {
            this->rdrstart ();
            if (this->rdrflag) input |= IO_SKIP;
            else skipoptwait (opcode, &this->lock, &this->rsfwait);
            break;
        }

        // RRB: reader buffer ored with accumulator; clear reader flag
        case 06012: {
            input &= IO_LINK;
            input |= this->rdrbuff;
            this->rdrflag = false;
            updintreq ();
            pthread_cond_broadcast (&this->rdrcond);
            break;
        }

        // RFC: clear reader flag and start reading next char from tape
        case 06014: {
            this->rdrstart ();
            this->rdrflag = false;
            this->rdrnext = true;
            updintreq ();
            pthread_cond_broadcast (&this->rdrcond);
            break;
        }

        // RRB RFC: reader buffer ored into accumulator; reader flag cleared; start reading next character
        case 06016: {
            this->rdrstart ();
            input |= this->rdrbuff;
            this->rdrflag = false;
            this->rdrnext = true;
            updintreq ();
            pthread_cond_broadcast (&this->rdrcond);
            break;
        }

        // PCE: clear interrupt enable
        case 06020: {
            this->intenab = false;
            updintreq ();
            break;
        }

        // PSF: skip if punch ready to accept next byte
        case 06021: {
            this->punstart ();
            if (this->punflag) input |= IO_SKIP;
            else skipoptwait (opcode, &this->lock, &this->psfwait);
            break;
        }

        // PCF: clear punch flag, pretending punch is busy
        case 06022: {
            this->punflag = false;
            updintreq ();
            break;
        }

        // PPC: start punching a byte
        case 06024: {
            this->punstart ();
            this->punbuff = input & 0377;
            this->punfull = true;
            pthread_cond_broadcast (&this->puncond);
            break;
        }

        // PLS: punch load sequence = PCF + PPC
        //      copy accum to punch buffer, initiate output, reset punch flag
        case 06026: {
            this->punstart ();
            this->punbuff = input & 0377;
            this->punfull = true;
            this->punflag = false;
            pthread_cond_broadcast (&this->puncond);
            break;
        }

        default: input = UNSUPIO;
    }
    pthread_mutex_unlock (&this->lock);
    return input;
}

// processor attempting to read when no ptape file loaded
void IODevPTape::rdrstart ()
{
    if (! this->rdrrun && ! this->rdrwarn) {
        fprintf (stderr, "IODevPTape::rdrstart: waiting for ptape load - iodev ptape load reader [-ascii] [-quick] <filename>\n");
        this->rdrwarn = true;
    }
}

// kill reader thread and close file
// call and returns with mutex locked
void IODevPTape::stoprdrthread ()
{
    if (this->rdrrun) {

        // tell rdrthread to exit
        this->rdrrun = false;

        // break it out of 'waiting for rdrnext' loop
        pthread_cond_broadcast (&this->rdrcond);

        // swap fd for /dev/null so select() will complete immediately if it hasn't quite started yet
        int nullfd = open ("/dev/null", O_RDONLY);
        if (nullfd < 0) ABORT ();
        if (dup2 (nullfd, this->rdrfd) < 0) ABORT ();
        close (nullfd);

        // break it out of select() if it is already in it
        pthread_kill (this->rdrtid, SIGCHLD);

        // wait for thread to terminate
        pthread_mutex_unlock (&this->lock);
        pthread_join (this->rdrtid, NULL);

        // remember thread has exited
        pthread_mutex_lock (&this->lock);
        this->rdrtid = 0;

        // make sure file is closed
        close (this->rdrfd);
        this->rdrfd = -1;

        // in case some other thread is waiting for close
        pthread_cond_broadcast (&this->rdrcond);
    } else {

        // wait if some other thread is doing stoprdrthread()
        // it will set rdrtid = 0 and rdrfd = -1 when done
        while ((this->rdrtid != 0) || (this->rdrfd >= 0)) {
            pthread_cond_wait (&this->rdrcond, &this->lock);
        }
    }
}

// open file and start reader thread
// call and returns with mutex locked
void IODevPTape::startrdrthread (int fd)
{
    ASSERT (! this->rdrrun);
    ASSERT (this->rdrfd < 0);
    ASSERT (this->rdrtid == 0);

    this->rdrrun = true;
    this->rdrfd  = fd;

    int rc = pthread_create (&this->rdrtid, NULL, rdrthreadwrap, this);
    if (rc != 0) ABORT ();
    ASSERT (this->rdrtid != 0);

    this->rdrwarn = false;
}

void *IODevPTape::rdrthreadwrap (void *zhis)
{
    rdcycuninit ();
    ((IODevPTape *)zhis)->rdrthread ();
    return NULL;
}

void IODevPTape::rdrthread ()
{
    bool printedbusy = false;
    fd_set readfds;
    uint64_t lastreadat = 0;
    FD_ZERO (&readfds);
start:;
    pthread_mutex_lock (&this->lock);
    while (this->rdrrun) {
        struct timespec nowts;
        if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) ABORT ();
        uint64_t nowns = (uint64_t) nowts.tv_sec * 1000000000 + nowts.tv_nsec;

        // wait until told to read next char
        if (! this->rdrnext) {
            if (printedbusy) {
                uint64_t endns = lastreadat + 3000000000U;
                if (nowns >= endns) {
                    printedbusy = false;
                    LLU sofar = (LLU) lseek (this->rdrfd, 0, SEEK_CUR);
                    fprintf (stderr, "IODevPTape::rdrthread: paused (%llu byte%s so far)\n", sofar, ((sofar == 1) ? "" : "s"));
                } else {
                    struct timespec endts;
                    endts.tv_sec  = endns / 1000000000U;
                    endts.tv_nsec = endns % 1000000000U;
                    pthread_cond_timedwait (&this->rdrcond, &this->lock, &endts);
                }
            } else {
                pthread_cond_wait (&this->rdrcond, &this->lock);
            }
            continue;
        }

        // make sure it has been 5mS since last read to give processor time to handle previous char
        if (! this->rdrquick) {
            uint64_t nextreadat = lastreadat + 1000000/200;  // 200 chars/sec
            if (nextreadat > nowns) {
                pthread_mutex_unlock (&this->lock);
                usleep ((nextreadat - nowns + 999) / 1000);
                goto start;
            }
            lastreadat = nowns;
        }

        if (! printedbusy) {
            printedbusy = true;
            fprintf (stderr, "IODevPTape::rdrthread: reading...\n");
        }

        // read char, waiting for one if necessary
        // allow IODevPTape::stoprdrthread() to abort the read by using a select()
        if (this->rdrinsert != 0377U) {
            // insert the rdrinsert byte to the read stream
            this->rdrbuff   = this->rdrinsert;
            this->rdrinsert = 0377U;
        } else {
            // read next byte from file (or pipe or whatever)
            // use select so it can be aborted by ioreset
            FD_SET (this->rdrfd, &readfds);
            int en, rc;
            do {
                pthread_mutex_unlock (&this->lock);
                rc = select (this->rdrfd + 1, &readfds, NULL, NULL, NULL);
                en = errno;
                pthread_mutex_lock (&this->lock);
                if (! this->rdrrun) goto done;
            } while ((rc < 0) && (en == EINTR));
            errno = en;
            if (rc > 0) rc = read (this->rdrfd, &this->rdrbuff, 1);
            if (rc < 0) {
                fprintf (stderr, "IODevPTape::rdrthread: read error: %m\n");
                ABORT ();
            }
            if (rc == 0) {
                // insert control-Z at end-of-file for ascii files
                if (! this->rdrascii || ((this->rdrlastch & 0177) == 26)) {
                    // end of file, unload the tape by closing the file
                    // the processor will just see that the reader is taking a long time
                    fprintf (stderr, "IODevPTape::rdrthread: end of ptape file reached, unloading\n");
                    close (this->rdrfd);
                    pthread_detach (this->rdrtid);
                    this->rdrfd  = -1;
                    this->rdrtid = 0;
                    this->rdrrun = false;
                    break;
                }
                this->rdrbuff = 26;
            }
            if (this->rdrascii) {
                // reading ascii file, insert <CR> before an <LF> if not already preceded by a <CR>
                if (((this->rdrbuff & 0177) == '\n') && ((this->rdrlastch & 0177) != '\r')) {
                    this->rdrbuff   = '\r';
                    this->rdrinsert = 0200 | '\n';
                }
                // reading ascii file, make sure bit <7> is set as expected by PDP-8 software
                this->rdrbuff |= 0200;
            }
        }

        // save latest char passed to processor
        this->rdrlastch = this->rdrbuff;

        // set flag saying there is a character ready and maybe request interrupt
        this->rdrnext = false;
        this->rdrflag = true;
        updintreq ();
    }
done:;
    pthread_mutex_unlock (&this->lock);
}

// processor attempting to punch when no ptape file loaded
void IODevPTape::punstart ()
{
    if (! this->punrun && ! this->punwarn) {
        fprintf (stderr, "IODevPTape::punstart: waiting for ptape load - iodev ptape load punch [-append] [-ascii] <filename>\n");
        this->punwarn = true;
    }
}

// stop punch thread and close file
void IODevPTape::stoppunthread ()
{
    if (this->punrun) {

        // tell existing thread to stop looping
        this->punrun = false;

        // wake it up
        pthread_cond_broadcast (&this->puncond);

        // wait for thread to terminate
        pthread_mutex_unlock (&this->lock);
        pthread_join (this->puntid, NULL);

        pthread_mutex_lock (&this->lock);
        this->puntid = 0;

        // all done with the fd
        close (this->punfd);
        this->punfd = -1;

        pthread_cond_broadcast (&this->puncond);
    } else {

        // wait if some other thread is doing stoppunthread()
        // it will set puntid = 0 and punfd = -1 when done
        while ((this->puntid != 0) || (this->punfd >= 0)) {
            pthread_cond_wait (&this->puncond, &this->lock);
        }
    }
}

void IODevPTape::startpunthread (int fd)
{
    ASSERT (! this->punrun);
    ASSERT (this->punfd < 0);
    ASSERT (this->puntid == 0);

    this->punrun = true;
    this->punfd  = fd;

    int rc = pthread_create (&this->puntid, NULL, punthreadwrap, this);
    if (rc != 0) ABORT ();
    ASSERT (this->puntid != 0);

    this->punwarn = false;
}

void *IODevPTape::punthreadwrap (void *zhis)
{
    rdcycuninit ();
    ((IODevPTape *)zhis)->punthread ();
    return NULL;
}

void IODevPTape::punthread ()
{
    bool printedbusy = false;
    uint64_t lastwriteat = 0;

start:;
    pthread_mutex_lock (&this->lock);
    while (this->punrun) {
        struct timespec nowts;
        if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) ABORT ();
        uint64_t nowns = (uint64_t) nowts.tv_sec * 1000000000 + nowts.tv_nsec;

        // wait for processor to put a character in buffer
        if (! this->punfull) {

            // nothing to punch, say we are ready to punch
            this->punflag = true;
            updintreq ();

            // wait for something to punch
            if (printedbusy) {
                uint64_t endns = lastwriteat + 3000000000U;
                if (nowns >= endns) {
                    printedbusy = false;
                    LLU sofar = (LLU) lseek (this->punfd, 0, SEEK_CUR);
                    fprintf (stderr, "IODevPTape::punthread: paused (%llu byte%s so far)\n", sofar, ((sofar == 1) ? "" : "s"));
                } else {
                    struct timespec endts;
                    endts.tv_sec  = endns / 1000000000U;
                    endts.tv_nsec = endns % 1000000000U;
                    pthread_cond_timedwait (&this->puncond, &this->lock, &endts);
                }
            } else {
                pthread_cond_wait (&this->puncond, &this->lock);
            }
        } else {
            pthread_mutex_unlock (&this->lock);

            // make sure it has been at least 20mS since last character printed
            if (! this->punquick) {
                uint64_t nextwriteat = lastwriteat + 1000000000/50; // 50 chars/sec
                if (nextwriteat > nowns) {
                    usleep ((nextwriteat - nowns + 999) / 1000);
                    goto start;
                }
            }
            lastwriteat = nowns;

            if (! printedbusy) {
                printedbusy = true;
                fprintf (stderr, "IODevPTape::punthread: punching...\n");
            }

            // start punching character
            if (! this->punascii || (((this->punbuff &= 0177) != 0) && (this->punbuff != '\r') && (this->punbuff != 26))) {
                int rc = write (this->punfd, &this->punbuff, 1);
                if (rc == 0) errno = EPIPE;
                if (rc <= 0) {
                    fprintf (stderr, "IODevPTape::punthread: write error: %m\n");
                    ABORT ();
                }
            }

            // clear flag saying we are punching something
            pthread_mutex_lock (&this->lock);
            this->punfull = false;
        }
    }
    pthread_mutex_unlock (&this->lock);
}

// update interrupt request line after changing intenab, rdrflag, punflag
// assumes this->lock is locked
void IODevPTape::updintreq ()
{
    if (this->intenab & (this->rdrflag | this->punflag)) {
        setintreqmask (IRQ_PTAPE);
    } else {
        clrintreqmask (IRQ_PTAPE,
            (this->rsfwait & this->rdrflag) |
            (this->psfwait & this->punflag));
    }
}
