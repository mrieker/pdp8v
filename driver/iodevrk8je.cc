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

// RK8JE Disk System (minicomputer handbook 1976, p 9-115..120)
// envars iodevrk8je_0,1,2,3 = disk file name (or softlink)
//  ENOENT says disk offline
//  read-only access write-locks drive
// files are 203*2*16*512 bytes = 3325952 bytes
//  dd if=/dev/zero bs=8192 count=406 of=disk0.rk8

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/time.h>
#include <unistd.h>

#include "iodevrk8je.h"
#include "memory.h"
#include "shadow.h"

#define NCYLS 406
#define SECPERCYL 32
#define NBLKS (NCYLS*SECPERCYL)

#define SEEKRATE (441*1000/NCYLS)   // usec per cylinder = max access time / num cylinder on disk
#define SETTLEUS 4300
#define XFERRATE 17                 // usec per word

IODevRK8JE iodevrk8je;

static IODevOps const iodevops[] = {
    { 06741, "DSKP (RK8JE) skip if transfer done or error" },
    { 06742, "DCLR (RK8JE) function in AC<01:00>" },
    { 06743, "DLAG (RK8JE) load disk address, clear accumulator and start function in command register" },
    { 06744, "DLCA (RK8JE) load current address register from the AC" },
    { 06745, "DRST (RK8JE) clear the AC and read conents of status register into the AC" },
    { 06746, "DLDC (RK8JE) load command register from the AC, clear AC and clear status register" },
};

IODevRK8JE::IODevRK8JE ()
{
    opscount = sizeof iodevops / sizeof iodevops[0];
    opsarray = iodevops;

    iodevname = "rk8je";

    pthread_cond_init (&this->cond, NULL);
    pthread_cond_init (&this->cond2, NULL);
    pthread_mutex_init (&this->lock, NULL);
    this->resetting = false;
    this->threadid = 0;
    this->nsperus = 0xFFFFFFFFU;
    memset (this->fds, -1, sizeof this->fds);
    memset (this->lastdas, 0, sizeof this->lastdas);

    char const *env = getenv ("iodevrk8je_debug");
    this->debug = (env != NULL) && (env[0] & 1);
}

IODevRK8JE::~IODevRK8JE ()
{
    close (this->fds[0]);
    close (this->fds[1]);
    close (this->fds[2]);
    close (this->fds[3]);
    memset (this->fds, -1, sizeof this->fds);
}

// reset the device
// - kill thread
// - close files
void IODevRK8JE::ioreset ()
{
    if (this->nsperus == 0xFFFFFFFFU) {
        this->nsperus = 1000 * 600000 / DEFCPUHZ;
        char const *envar = getenv ("iodevrk8je_tfact");
        if (envar != NULL) {
            this->nsperus = (uint32_t)(1000.0 * atof (envar));
        }
        fprintf (stderr, "IODevRK8JE::ioreset: time factor %5.3f\n", this->nsperus / 1000.0);
    }

    this->resetting = true;

    pthread_mutex_lock (&this->lock);

    if (this->threadid != 0) {
        while (this->status & 04000) {
            pthread_cond_wait (&this->cond2, &this->lock);
        }
    }
    pthread_cond_broadcast (&this->cond);

    memset (this->lastdas, 0, sizeof this->lastdas);

    this->command  = 0;
    this->diskaddr = 0;
    this->memaddr  = 0;
    this->status   = 0;

    clrintreqmask (IRQ_RK8);

    pthread_mutex_unlock (&this->lock);

    if (this->threadid != 0) {
        pthread_join (this->threadid, NULL);
        this->threadid = 0;
    }

    this->resetting = false;
}

// load/unload files
SCRet *IODevRK8JE::scriptcmd (int argc, char const *const *argv)
{
    // loadro/loadrw <drivenumber> <filename>
    bool loadro = (strcmp (argv[0], "loadro") == 0);
    bool loadrw = (strcmp (argv[0], "loadrw") == 0);
    if (loadro | loadrw) {
        if (argc == 3) {
            char *p;
            int driveno = strtol (argv[1], &p, 0);
            if ((*p != 0) || (driveno < 0) || (driveno > 3)) return new SCRetErr ("drivenumber %s not in range 0..3", argv[1]);
            int fd = open (argv[2], loadrw ? O_RDWR : O_RDONLY);
            if (fd < 0) return new SCRetErr (strerror (errno));
            if (flock (fd, (loadrw ? LOCK_EX : LOCK_SH) | LOCK_NB) < 0) {
                SCRetErr *err = new SCRetErr (strerror (errno));
                close (fd);
                return err;
            }
            fprintf (stderr, "IODevRK8JE::scriptcmd: drive %d loaded with read%s file %s\n", driveno, (loadro ? "-only" : "/write"), argv[2]);
            pthread_mutex_lock (&this->lock);
            close (this->fds[driveno]);
            this->fds[driveno] = fd;
            this->ros[driveno] = loadro;
            pthread_mutex_unlock (&this->lock);
            return NULL;
        }

        return new SCRetErr ("iodev rk8je loadro/loadrw <drivenumber> <filename>");
    }

    // unload <drivenumber>
    if (strcmp (argv[0], "unload") == 0) {
        if (argc == 2) {
            char *p;
            int driveno = strtol (argv[1], &p, 0);
            if ((*p != 0) || (driveno < 0) || (driveno > 3)) return new SCRetErr ("drivenumber %s not in range 0..3", argv[1]);
            fprintf (stderr, "IODevRK8JE::scriptcmd: drive %d unloaded\n", driveno);
            pthread_mutex_lock (&this->lock);
            close (this->fds[driveno]);
            this->fds[driveno] = -1;
            pthread_mutex_unlock (&this->lock);
            return NULL;
        }

        return new SCRetErr ("iodev rk8je unload <drivenumber>");
    }

    return new SCRetErr ("unknown rk8je command %s", argv[0]);
}

// perform i/o instruction
uint16_t IODevRK8JE::ioinstr (uint16_t opcode, uint16_t input)
{
    pthread_mutex_lock (&this->lock);

    switch (opcode) {

        // DSKP - skip if transfer done or error
        // p 225 v 9-116
        case 06741: {
            if (! (this->status & 04000)) input |= IO_SKIP;
            break;
        }

        // DCLR - function in AC<01:00>
        case 06742: {
            switch (input & 3) {

                // clear status (unless busy doing something)
                case 0: {
                    if (this->status & 04000) this->status |= 00100;
                                              else this->status = 0;
                    break;
                }

                // clear controller
                // aborts any i/o in progress
                case 1: {
                    this->status  = 0;                      // clear all status bits including busy bit
                    this->command = 0;
                    this->memaddr = 0;
                    pthread_cond_broadcast (&this->cond);   // break out of the seek sleep
                    break;
                }

                // reset drive
                case 2: {
                    if (this->status & 04000) this->status |= 00100;
                    else {
                        // seek cylinder 0
                        this->command  = (this->command & 00400) | 03000;
                        this->diskaddr = 0;
                        this->startio ();
                    }
                    break;
                }

                // clear status including busy bit
                // aborts any i/o in progress
                case 3: {
                    this->status = 0;                       // clear all status bits including busy bit
                    pthread_cond_broadcast (&this->cond);   // break out of the seek sleep
                    break;
                }
            }
            break;
        }

        // DLAG - load disk address, clear accumulator and start function in command register
        case 06743: {
            if (this->status & 04000) this->status |= 00100;
            else {
                this->diskaddr = input & IO_DATA;
                this->startio ();
                input &= ~ IO_DATA;
            }
            break;
        }

        // DLCA - load current address register from the AC
        case 06744: {
            if (this->status & 04000) this->status |= 00100;
            else {
                this->memaddr = input & IO_DATA;
                input &= ~ IO_DATA;
            }
            break;
        }

        // DRST - clear the AC and read conents of status register into the AC
        case 06745: {
            input &= ~ IO_DATA;
            input |= this->status;
            break;
        }

        // DLDC - load command register from the AC, clear AC and clear status register
        case 06746: {
            if (this->status & 04000) this->status |= 00100;
            else {
                this->command = input & IO_DATA;
                this->status  = 0;
                input &= ~ IO_DATA;
            }
            break;
        }

        default: {
            input = UNSUPIO;
            break;
        }
    }
    this->updateirq ();
    pthread_mutex_unlock (&this->lock);
    return input;
}

void IODevRK8JE::startio ()
{
    // don't allow any changing of registers while this I/O is in progress
    this->status = 04000;

    // tell thread to start doing an I/O request
    if (this->threadid == 0) {
        int rc = pthread_create (&this->threadid, NULL, threadwrap, this);
        if (rc != 0) ABORT ();
    }

    pthread_cond_broadcast (&this->cond);
}

#define SETST(n) this->status = (this->status & 00100) | (n)

// thread what does the disk file I/O
void *IODevRK8JE::threadwrap (void *zhis)
{
    ((IODevRK8JE *)zhis)->thread ();
    return NULL;
}
void IODevRK8JE::thread ()
{
    pthread_mutex_lock (&this->lock);
    while (! this->resetting) {

        // waiting for another I/O request
        if (! (this->status & 04000)) {
            pthread_cond_broadcast (&this->cond2);
            int rc = pthread_cond_wait (&this->cond, &this->lock);
            if (rc != 0) ABORT ();
        } else {
            int cyldiff, fd, rc;
            struct timespec endts, nowts;
            uint16_t blknum, diskno, icnt, len, *ptr, wcnt, xma;
            uint16_t temp[256];
            uint64_t delns, endns, nowns;

            blknum = ((this->command & 1) << 12) | this->diskaddr;
            wcnt   = (this->command & 00100) ? 128 : 256;
            xma    = ((this->command << 9) & 070000) | this->memaddr;

            // validate parameters
            if (blknum >= NBLKS) {
                fprintf (stderr, "IODevRK8JE::thread: bad disk addr %05o\n", blknum);
                SETST (00001);                                              // cylinder address error
                goto rwdone;
            }
            if (this->memaddr + wcnt > 4096) {
                fprintf (stderr, "IODevRK8JE::thread: bad mem addr %05o wordcount %u\n", xma, wcnt);
                SETST (00004);                                              // data request late
                goto rwdone;
            }

            diskno  = (this->command >> 1) & 3;
            cyldiff = blknum / SECPERCYL - this->lastdas[diskno] / SECPERCYL;
            if (cyldiff < 0) cyldiff = - cyldiff;
            if (cyldiff > 0) SETST (06000);                                 // busy; head in motion

            // wait for a while to simulate the slow disk drive
            // can be cancelled by DCLR clearing the busy bit
            // unlocked during wait, ioinstr() won't change anything while busy set (except DCLR that clears busy)
            delns = (((cyldiff > 0) ? (cyldiff * SEEKRATE + SETTLEUS) : 0) + wcnt * XFERRATE) * (uint64_t) this->nsperus;
            if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) ABORT ();
            nowns = (uint64_t) nowts.tv_sec * 1000000000 + nowts.tv_nsec;
            endns = nowns + delns;
            endts.tv_sec  = endns / 1000000000;
            endts.tv_nsec = endns % 1000000000;
            do {
                rc = pthread_cond_timedwait (&this->cond, &this->lock, &endts);
                if ((rc != 0) && (rc != ETIMEDOUT)) ABORT ();
                if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) ABORT ();
                nowns = (uint64_t) nowts.tv_sec * 1000000000 + nowts.tv_nsec;
            } while ((this->status & 04000) && (nowns < endns));

            // DCLR cleared busy so we're done
            if (! (this->status & 04000)) goto rwdone;

            // lock during transfer so no i/o instructions can see status bits until we unlock
            SETST (04000);                                                  // clear head in motion, leave busy set
            this->lastdas[diskno] = blknum;                                 // remember head position for next seek time calculation

            // error if no file loaded
            fd = this->fds[diskno];
            if (fd < 0) {
                SETST (00002);                                              // select error
                goto rwdone;
            }

            // perform the read or write

            switch (this->command >> 9) {

                // read data
                case 1: {
                    if (this->debug) fprintf (stderr, "IODevRK8JE::thread*: treating read all as normal read\n");
                }
                case 0: {
                    if (this->debug) fprintf (stderr, "IODevRK8JE::thread*: reading %u words at %u into %05o (%u ms)\n", wcnt, blknum, xma, (uint32_t) ((delns + 500000) / 1000000));
                    ASSERT (wcnt * 2 <= sizeof temp);
                    rc = pread (fd, temp, wcnt * 2, blknum * 512);
                    if (rc < wcnt * 2) {
                        SETST (00010);                                      // crc error
                        if (rc < 0) {
                            fprintf (stderr, "IODevRK8JE::thread: error reading %u words from disk %u at %05o: %m\n", wcnt, diskno, blknum);
                        } else {
                            fprintf (stderr, "IODevRK8JE::thread: only read %d bytes of %u words from disk %u at %05o\n", rc, wcnt, diskno, blknum);
                        }
                        break;
                    }
                    for (icnt = 0; icnt < wcnt; icnt ++) {
                        memarray[xma+icnt] = temp[icnt] & 07777;
                    }
                    this->memaddr = (this->memaddr + wcnt) & 07777;
                    SETST (00000);                                          // done
                    break;
                }

                // seek only
                case 3: {
                    SETST (00000);                                          // done
                    break;
                }

                // write data
                case 5: {
                    if (this->debug) fprintf (stderr, "IODevRK8JE::thread*: treating write all as normal write\n");
                }
                case 4: {
                    if (this->debug) fprintf (stderr, "IODevRK8JE::thread*: writing %u words at %u from %05o (%u ns)\n", wcnt, blknum, xma, (uint32_t) ((delns + 500000) / 1000000));
                    if (this->ros[diskno]) {
                        SETST (00020);                                      // write lock error
                        wcnt = 0;
                        break;
                    }
                    ASSERT (xma + wcnt <= MEMSIZE);
                    ptr = &memarray[xma];
                    len = wcnt * 2;
                    if (wcnt % 256 != 0) {
                        ASSERT (len < sizeof temp);
                        memcpy (temp, ptr, len);
                        memset (&temp[wcnt], 0, 512 - (len & 511));
                        ptr = temp;
                        len = (len + 511) & - 512;
                    }
                    rc = pwrite (fd, ptr, len, blknum * 512);
                    if (rc < wcnt * 2) {
                        SETST (00010);                                      // crc error
                        if (rc < 0) {
                            fprintf (stderr, "IODevRK8JE::thread: error writing %u words from disk %u at %05o: %m\n", wcnt, diskno, blknum);
                        } else {
                            fprintf (stderr, "IODevRK8JE::thread: only wrote %d bytes of %u words from disk %u at %05o\n", rc, wcnt, diskno, blknum);
                        }
                        break;
                    }
                    this->memaddr = (this->memaddr + wcnt) & 07777;
                    SETST (00000);                                          // done
                    break;
                }

                default: {
                    fprintf (stderr, "IODevRK8JE::thread: bad command code %04o\n", this->command);
                    SETST (00040);                                          // timing error
                    break;
                }
            }

            // maybe request an interrupt
        rwdone:;
            ASSERT (! (this->status & 04000));
            this->updateirq ();
        }
    }

    // ioreset(), unlock and exit thread
    pthread_mutex_unlock (&this->lock);
}

void IODevRK8JE::updateirq ()
{
    if ((this->command & 00400) && ! (this->status & 04000)) {
        setintreqmask (IRQ_RK8);
    } else {
        clrintreqmask (IRQ_RK8);
    }
}
