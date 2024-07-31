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

// RK8 Disk System (small computer handbook 1972, p 360 v 7-138)
// envars iodevrk8_0,1,2,3 = disk file name (or softlink)
//  ENOENT quietly says disk offline
//  read-only access quietly write-locks drive
// files are 203*2*8*512 bytes = 1662976 bytes
//  dd if=/dev/zero bs=4096 count=406 of=disk0.rk8

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/time.h>
#include <unistd.h>

#include "iodevrk8.h"
#include "memory.h"
#include "rdcyc.h"
#include "shadow.h"

#define NCYLS 203
#define SECPERCYL 16
#define NBLKS (NCYLS*SECPERCYL)

#define SEEKRATE (441*1000/NCYLS)   // usec per cylinder = max access time / num cylinder on disk
#define SETTLEUS 4300
#define XFERRATE 17                 // usec per word

IODevRK8 iodevrk8;

static IODevOps const iodevops[] = {
    { 06732, "DLDC (RK8) load command register" },
    { 06733, "DLDR (RK8) load disk address and read" },
    { 06734, "DRDA (RK8) read disk address" },
    { 06735, "DLDW (RK8) load disk address and write" },
    { 06736, "DRDC (RK8) read disk command register" },
    { 06737, "DCHP (RK8) load disk address and check parity" },
    { 06741, "DRDS (RK8) read disk status register" },
    { 06742, "DCLS (RK8) clear status register" },
    { 06745, "DSKD (RK8) skip on transfer done" },
    { 06747, "DSKE (RK8) skip on disk error" },
    { 06751, "DCLA (RK8) clear all" },
    { 06752, "DRWC (RK8) read word count register" },
    { 06753, "DLWC (RK8) load word count register" },
    { 06755, "DLCA (RK8) load current address register" },
    { 06757, "DRCA (RK8) read current address register" },
};

IODevRK8::IODevRK8 ()
{
    opscount = sizeof iodevops / sizeof iodevops[0];
    opsarray = iodevops;

    iodevname = "rk8";

    pthread_cond_init (&this->cond, NULL);
    pthread_cond_init (&this->cond2, NULL);
    pthread_mutex_init (&this->lock, NULL);
    this->threadid = 0;
    memset (this->fds, -1, sizeof this->fds);
    memset (this->lastdas, 0, sizeof this->lastdas);
}

// reset the device
// - kill thread
// - close files
void IODevRK8::ioreset ()
{
    pthread_mutex_lock (&this->lock);

    if (this->threadid != 0) {
        while (this->rk8io != RK8IO_IDLE) {
            pthread_cond_wait (&this->cond2, &this->lock);
        }
    }
    this->rk8io = RK8IO_EXIT;
    pthread_cond_broadcast (&this->cond);

    close (this->fds[0]);
    close (this->fds[1]);
    close (this->fds[2]);
    close (this->fds[3]);
    memset (this->fds, -1, sizeof this->fds);
    memset (this->lastdas, 0, sizeof this->lastdas);

    this->command   = 0;
    this->diskaddr  = 0;
    this->memaddr   = 0;
    this->wordcount = 0;

    clrintreqmask (IRQ_RK8);

    pthread_mutex_unlock (&this->lock);

    if (this->threadid != 0) {
        pthread_join (this->threadid, NULL);
        this->threadid = 0;
    }
}

// perform i/o instruction
uint16_t IODevRK8::ioinstr (uint16_t opcode, uint16_t input)
{
    pthread_mutex_lock (&this->lock);

    switch (opcode) {

        // DLDC - load command register
        // p 364 v 7-142
        case 06732: {
            if (this->status & 00001) this->status |= 01000;
            else {
                this->command = input & IO_DATA;
                this->updateirq ();
            }
            input &= ~ IO_DATA;
            break;
        }

        // DLDR - load disk address and read
        // p 365 v 7-143
        case 06733: {
            if (this->status & 00001) this->status |= 01000;
            else if ( !(this->status & 04000)) {
                this->diskaddr = input & IO_DATA;
                this->startio (RK8IO_READ);
            }
            input &= ~ IO_DATA;
            break;
        }

        // DRDA - read disk address
        // p 365 v 7-143
        case 06734: {
            input &= ~ IO_DATA;
            input |= this->diskaddr;
            break;
        }

        // DLDW - load disk address and write
        // p 366 v 7-144
        case 06735: {
            if (this->status & 00001) this->status |= 01000;
            else if ( !(this->status & 04000)) {
                this->diskaddr = input & IO_DATA;
                this->startio (RK8IO_WRITE);
            }
            input &= ~ IO_DATA;
            break;
        }

        // DRDC - read disk command register
        // p 366 v 7-144
        case 06736: {
            input &= ~ IO_DATA;
            input |= this->command;
            break;
        }

        // DCHP - load disk address and check parity
        // p 366 v 7-144
        case 06737: {
            if (this->status & 00001) this->status |= 01000;
            else if ( !(this->status & 04000)) {
                this->diskaddr = input & IO_DATA;
                this->startio (RK8IO_CHECK);
            }
            input &= ~ IO_DATA;
            break;
        }

        // DRDS - read disk status register
        // p 366 v 7-144
        case 06741: {
            input &= ~ IO_DATA;
            input |= this->status;
            break;
        }

        // DCLS - clear status register
        // p 367 v 7-145
        case 06742: {
            if (this->status & 00001) this->status |= 01000;
            else {
                this->status = 0;
                clrintreqmask (IRQ_RK8);
            }
            break;
        }

        // DSKD - skip on transfer done
        // p 367 v 7-145
        case 06745: {
            if (this->status & 02000) input |= IO_SKIP;
            break;
        }

        // DSKE - skip on disk error
        // p 367 v 7-145
        case 06747: {
            if (this->status & 04000) input |= IO_SKIP;
            break;
        }

        // DCLA - clear all
        // p 368 v 7-146
        case 06751: {
            if (this->status & 00001) this->status |= 01000;
            else if ( !(this->status & 04000)) {
                this->diskaddr  = 0;
                this->command  &= 6;
                this->wordcount = 0;
                this->memaddr   = 0;
                this->startio (RK8IO_CLEAR);
            }
            break;
        }

        // DRWC - read word count register
        // p 368 v 7-146
        case 06752: {
            input &= ~ IO_DATA;
            input |= this->wordcount;
            break;
        }

        // DLWC - load word count register
        // p 368 v 7-146
        case 06753: {
            if (this->status & 00001) this->status |= 01000;
            else this->wordcount = input & IO_DATA;
            input &= ~ IO_DATA;
            break;
        }

        // DLCA - load current address register
        // p 368 v 7-146
        case 06755: {
            if (this->status & 00001) this->status |= 01000;
            else this->memaddr = input & IO_DATA;
            input &= ~ IO_DATA;
            break;
        }

        // DRCA - read current address register
        // p 368 v 7-146
        case 06757: {
            input &= ~ IO_DATA;
            input |= this->memaddr;
            break;
        }

        default: {
            input = UNSUPIO;
            break;
        }
    }
    pthread_mutex_unlock (&this->lock);
    return input;
}

void IODevRK8::startio (RK8IO rk8io)
{
    // about to go busy so not requesting an interrupt
    clrintreqmask (IRQ_RK8);

    // don't allow any changing of registers while this I/O is in progress
    this->status = 00001;

    // tell thread to start doing an I/O request
    this->rk8io = rk8io;
    if (this->threadid == 0) {
        int rc = pthread_create (&this->threadid, NULL, threadwrap, this);
        if (rc != 0) ABORT ();
    }
    pthread_cond_broadcast (&this->cond);
}

#define SETST(n) this->status = (this->status & 01000) | (n)

// thread what does the disk file I/O
void *IODevRK8::threadwrap (void *zhis)
{
    rdcycuninit ();
    ((IODevRK8 *)zhis)->thread ();
    return NULL;
}
void IODevRK8::thread ()
{
    pthread_mutex_lock (&this->lock);
    while (true) {
        switch (this->rk8io) {

            // this->ioreset() is requesting us to exit
            case RK8IO_EXIT: {
                pthread_mutex_unlock (&this->lock);
                return;
            }

            // waiting for another I/O request
            case RK8IO_IDLE: {
                pthread_cond_broadcast (&this->cond2);
                int rc = pthread_cond_wait (&this->cond, &this->lock);
                if (rc != 0) ABORT ();
                break;
            }

            // I/O request queued
            case RK8IO_READ:
            case RK8IO_WRITE:
            case RK8IO_CHECK: {
                char fnvar[12];
                char const *fnenv;
                int cyldiff, fd, rc;
                uint16_t diskno, icnt, len, *ptr, wcnt, xma;
                uint16_t temp[4096];

                // this->startio() should have marked controller busy
                ASSERT (this->status & 00001);

                // validate parameters
                wcnt = 4096 - this->wordcount;
                if (this->diskaddr + (wcnt + 255) / 256 > NBLKS) {
                    fprintf (stderr, "IODevRK8::thread: bad disk addr %04o with neg wc %04o\n", this->diskaddr, this->wordcount);
                    SETST (06040);                                              // track address error
                    goto rwdone;
                }
                xma = ((this->command << 9) & 070000) | this->memaddr;
                if (this->memaddr + wcnt > 4096) {
                    fprintf (stderr, "IODevRK8::thread: bad mem addr %05o with neg wc %04o\n", xma, this->wordcount);
                    SETST (06100);                                              // data rate error
                    goto rwdone;
                }

                // ok to unlock, this->ioinstr() shouldn't allow any changes while marked busy
                pthread_mutex_unlock (&this->lock);

                // wait for a while to simulate the slow disk drive
                diskno  = (this->command >> 1) & 3;
                cyldiff = this->diskaddr / SECPERCYL - this->lastdas[diskno] / SECPERCYL;
                if (cyldiff < 0) cyldiff = - cyldiff;
                usleep (((cyldiff > 0) ? (cyldiff * SEEKRATE + SETTLEUS) : 0) + wcnt * XFERRATE);

                // lock during transfer so no i/o instructions can see status bits until we unlock
                pthread_mutex_lock (&this->lock);
                ASSERT (this->status & 00001);

                // get file fd and open if not already
                fd = this->fds[diskno];
                if (fd < 0) {
                    this->ros[diskno] = false;
                    sprintf (fnvar, "iodevrk8_%u", diskno);
                    fnenv = getenv (fnvar);
                    if (fnenv == NULL) {
                        fprintf (stderr, "IODevRK8::thread: undefined envar %s\n", fnvar);
                        SETST (06002);                                          // select error
                        goto rwdone;
                    }
                    int lkmode = LOCK_EX | LOCK_NB;
                    fd = open (fnenv, O_RDWR);
                    if ((fd < 0) && (errno == EACCES)) {
                        this->ros[diskno] = true;
                        lkmode = LOCK_SH | LOCK_NB;
                        fd = open (fnenv, O_RDONLY);
                    }
                    if (fd < 0) {
                        if (errno != ENOENT) {
                            fprintf (stderr, "IODevRK8::thread: error opening %s: %m\n", fnenv);
                        }
                        SETST (06002);                                          // select error
                        goto rwdone;
                    }
                    if (flock (fd, lkmode) < 0) {
                        fprintf (stderr, "IODevRK8::thread: error locking %s: %m\n", fnenv);
                        close (fd);
                        SETST (06002);                                          // select error
                        goto rwdone;
                    }
                    this->fds[diskno] = fd;
                }

                // perform the read or write
                this->lastdas[diskno] = this->diskaddr;                         // remember head position for next seek time calculation
                SETST (02000);                                                  // transfer complete
                if (! (this->command & 00200)) {                                // not 'seek only'
                    switch (this->rk8io) {
                        case RK8IO_READ:
                        case RK8IO_CHECK: {
                            ASSERT (wcnt * 2 <= sizeof temp);
                            rc = pread (fd, temp, wcnt * 2, this->diskaddr * 512);
                            if (rc < wcnt * 2) {
                                SETST (06200);                                  // parity error
                                if (rc < 0) {
                                    fprintf (stderr, "IODevRK8::thread: error reading %u words from disk %u at %04o: %m\n", wcnt, diskno, this->diskaddr);
                                    wcnt = 0;
                                } else {
                                    fprintf (stderr, "IODevRK8::thread: only read %d bytes of %u words from disk %u at %04o\n", rc, wcnt, diskno, this->diskaddr);
                                    wcnt = rc / 2;
                                }
                            }
                            if (this->rk8io == RK8IO_READ) {
                                for (icnt = 0; icnt < wcnt; icnt ++) {
                                    memarray[xma+icnt] = temp[icnt] & 07777;
                                }
                            }
                            break;
                        }
                        case RK8IO_WRITE: {
                            if (this->ros[diskno]) {
                                SETST (06010);                                  // write lock error
                                wcnt = 0;
                                break;
                            }
                            ASSERT (xma + wcnt <= MEMSIZE);
                            ptr = &memarray[xma];
                            len = wcnt * 2;
                            if (wcnt % 256 != 0) {
                                ASSERT (len < sizeof temp);
                                memcpy (temp, ptr, len);
                                memset (&temp[wcnt], 0x69, 512 - (len & 511));
                                ptr = temp;
                                len = (len + 511) & - 512;
                            }
                            rc = pwrite (fd, ptr, len, this->diskaddr * 512);
                            if (rc < wcnt * 2) {
                                SETST (06200);                                  // parity error
                                if (rc < 0) {
                                    fprintf (stderr, "IODevRK8::thread: error writing %u words from disk %u at %u: %m\n", wcnt, diskno, this->diskaddr);
                                    wcnt = 0;
                                } else {
                                    fprintf (stderr, "IODevRK8::thread: only wrote %d bytes of %u words from disk %u at %u\n", rc, wcnt, diskno, this->diskaddr);
                                    wcnt = rc / 2;
                                }
                            }
                            break;
                        }
                        default: ABORT ();
                    }

                    // increment addresses and neg wordcount by number of words actually transferred
                    this->diskaddr  += wcnt / 256;
                    this->memaddr   += wcnt;
                    this->wordcount += wcnt;
                }

                // maybe request an interrupt
            rwdone:;
                this->updateirq ();

                // we are now idle, wait in pthread_cond_wait() until we get another command
                this->rk8io = RK8IO_IDLE;
                break;
            }

            // clear I/O request queued
            case RK8IO_CLEAR: {

                // this->startio() should have marked controller busy
                ASSERT (this->status & 00001);

                // ok to unlock, this->ioinstr() shouldn't allow any changes while marked busy
                pthread_mutex_unlock (&this->lock);

                // wait for a while to simulate the slow disk drive
                uint16_t diskno = (this->command >> 1) & 3;
                useconds_t usec = this->lastdas[diskno] / SECPERCYL * SEEKRATE + SETTLEUS;
                usleep (usec);

                // lock during transfer so no i/o instructions can see status bits until we unlock
                pthread_mutex_lock (&this->lock);
                ASSERT (this->status & 00001);

                this->lastdas[diskno] = 0;

                // close file if open
                close (this->fds[diskno]);
                this->fds[diskno] = -1;

                // no longer busy (never interrupts)
                // transfer complete
                SETST (02000);

                // we are now idle, wait in pthread_cond_wait() until we get another command
                this->rk8io = RK8IO_IDLE;
                break;
            }

            // unknown state
            default: ABORT ();
        }
    }
}

void IODevRK8::updateirq ()
{
    if ((this->command & 02000 & this->status) || (this->command & 01000 & (this->status >> 2))) {
        setintreqmask (IRQ_RK8);
    } else {
        clrintreqmask (IRQ_RK8);
    }
}
