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
    this->intenab = false;
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
        puts ("  load punch <filename>  - load file to be written as paper tape");
        puts ("  load reader <filename> - load file to be read as paper tape");
        puts ("  unload punch           - unload file being written as paper tape");
        puts ("  unload reader          - unload file being read as paper tape");
        puts ("");
        return NULL;
    }

    // load reader/punch <filename>
    if (strcmp (argv[0], "load") == 0) {
        if (argc == 3) {

            // load reader <filename>
            if (strcmp (argv[1], "reader") == 0) {
                int fd = open (argv[2], O_RDONLY);
                if (fd < 0) return new SCRetErr (strerror (errno));
                pthread_mutex_lock (&this->lock);
                stoprdrthread ();
                startrdrthread (fd);
                pthread_mutex_unlock (&this->lock);
                return NULL;
            }

            // load punch <filename>
            if (strcmp (argv[1], "punch") == 0) {
                int fd = open (argv[2], O_WRONLY | O_CREAT, 0666);
                if (fd < 0) return new SCRetErr (strerror (errno));
                pthread_mutex_lock (&this->lock);
                stoppunthread ();
                startpunthread (fd);
                pthread_mutex_unlock (&this->lock);
                return NULL;
            }
        }

        return new SCRetErr ("iodev ptape load reader/punch <filename>");
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

    return new SCRetErr ("unknown ptape command %s - valid: help load unload", argv[0]);
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
            this->punbuff = input & 0177;
            this->punfull = true;
            pthread_cond_broadcast (&this->puncond);
            break;
        }

        // PLS: punch load sequence = PCF + PPC
        //      copy accum to punch buffer, initiate output, reset punch flag
        case 06026: {
            this->punstart ();
            this->punbuff = input & 0177;
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
        fprintf (stderr, "IODevPTape::rdrstart: waiting for ptape load - iodev ptape load reader <filename>\n");
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
    ((IODevPTape *)zhis)->rdrthread ();
    return NULL;
}

void IODevPTape::rdrthread ()
{
    fd_set readfds;
    uint64_t lastreadat = 0;
    FD_ZERO (&readfds);
start:;
    pthread_mutex_lock (&this->lock);
    while (this->rdrrun) {

        // wait until told to read next char
        if (! this->rdrnext) {
            pthread_cond_wait (&this->rdrcond, &this->lock);
            continue;
        }

        // make sure it has been 5mS since last read to give processor time to handle previous char
        struct timeval nowtv;
        if (gettimeofday (&nowtv, NULL) < 0) ABORT ();
        uint64_t now = nowtv.tv_sec * 1000000ULL + nowtv.tv_usec;
        uint64_t nextreadat = lastreadat + 1000000/200;  // 200 chars/sec
        if (nextreadat > now) {
            pthread_mutex_unlock (&this->lock);
            usleep (nextreadat - now);
            goto start;
        }
        lastreadat = now;

        // read char, waiting for one if necessary
        // allow IODevPTape::stoprdrthread() to abort the read by using a select()
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
            fprintf (stderr, "IODevPTape::rdrthread: end of ptape file reached, unloading\n");
            close (this->rdrfd);
            pthread_detach (this->rdrtid);
            this->rdrfd  = -1;
            this->rdrtid = 0;
            this->rdrrun = false;
            break;
        }

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
        fprintf (stderr, "IODevPTape::punstart: waiting for ptape load - iodev ptape load punch <filename>\n");
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

            // nothing to punch, say we are ready to punch
            this->punflag = true;
            updintreq ();
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
        setintreqmask (IRQ_TTYKBPR);
    } else {
        clrintreqmask (IRQ_TTYKBPR,
            (this->rsfwait & this->rdrflag) |
            (this->psfwait & this->punflag));
    }
}
