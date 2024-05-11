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
// files are 203*2*16*512 bytes = 3325952 bytes
//  dd if=/dev/zero bs=8192 count=406 of=disk0.rk05

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/time.h>
#include <unistd.h>

#include "dyndis.h"
#include "iodevrk8je.h"
#include "memory.h"
#include "shadow.h"

#define NCYLS 203
#define SECPERCYL 32
#define NBLKS (NCYLS*SECPERCYL)

#define ST_BUSY 010000  // busy doing IO
#define ST_DONE 004000  // transfer complete
#define ST_HDIM 002000  // head in motion
#define ST_XFRX 001000  // transfer capacity exceeded
#define ST_SKFL 000400  // seek fail
#define ST_FLNR 000200  // file not ready
#define ST_CBSY 000100  // controller busy
#define ST_TMER 000040  // timing error
#define ST_WLER 000020  // write lock error
#define ST_CRCR 000010  // crc error
#define ST_DRLT 000004  // data request late
#define ST_DSER 000002  // drive status error
#define ST_CYLR 000001  // cylinder error

#define ST_SKIP (ST_DONE|ST_XFRX|ST_SKFL|ST_FLNR|ST_TMER|ST_WLER|ST_CRCR|ST_DRLT|ST_DSER|ST_CYLR)

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
    this->nsperus = 1000;
    memset (this->fds, -1, sizeof this->fds);
    memset (this->lastdas, 0, sizeof this->lastdas);

    this->debug = 0;
    this->lastdebst = 0xFFFFU;
    this->dskpwait = false;
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
    this->resetting = true;

    pthread_mutex_lock (&this->lock);

    if (this->threadid != 0) {
        while (this->status & ST_BUSY) {
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
    if (strcmp (argv[0], "debug") == 0) {
        if (argc == 1) {
            return new SCRetInt (this->debug);
        }
        if (argc == 2) {
            char *p;
            int dbg = strtol (argv[1], &p, 0);
            if ((*p != 0) || (dbg < 0) || (dbg > 2)) return new SCRetErr ("debug flag %s not 0..2", argv[1]);
            this->debug = dbg;
            this->lastdebst = 0xFFFFU;
            return NULL;
        }
        return new SCRetErr ("iodev rk8je debug [0..2]");
    }

    if (strcmp (argv[0], "help") == 0) {
        puts ("");
        puts ("valid sub-commands:");
        puts ("");
        puts ("  debug [0..2]                   - get / set debug flag");
        puts ("  loadro <disknumber> <filename> - load file write-locked");
        puts ("  loadrw <disknumber> <filename> - load file write-enabled");
        puts ("  nsperus [<factor>]             - get / set nanoseconds per microsecond");
        puts ("  unload <disknumber>            - unload file - drive reports offline");
        puts ("");
        return NULL;
    }

    // loadro/loadrw <disknumber> <filename>
    bool loadro = (strcmp (argv[0], "loadro") == 0);
    bool loadrw = (strcmp (argv[0], "loadrw") == 0);
    if (loadro | loadrw) {
        if (argc == 3) {
            char *p;
            int diskno = strtol (argv[1], &p, 0);
            if ((*p != 0) || (diskno < 0) || (diskno > 3)) return new SCRetErr ("disknumber %s not in range 0..3", argv[1]);
            int fd = open (argv[2], loadrw ? O_RDWR | O_CREAT : O_RDONLY, 0666);
            if (fd < 0) return new SCRetErr (strerror (errno));
            if (flock (fd, (loadro ? LOCK_SH : LOCK_EX) | LOCK_NB) < 0) {
                SCRetErr *err = new SCRetErr (strerror (errno));
                close (fd);
                return err;
            }
            if (loadrw && (ftruncate (fd, NBLKS * 512) < 0)) {
                SCRetErr *err = new SCRetErr (strerror (errno));
                close (fd);
                return err;
            }
            fprintf (stderr, "IODevRK8JE::scriptcmd: drive %d loaded with read%s file %s\n", diskno, (loadro ? "-only" : "/write"), argv[2]);
            pthread_mutex_lock (&this->lock);
            close (this->fds[diskno]);
            this->fds[diskno] = fd;
            this->ros[diskno] = loadro;
            pthread_mutex_unlock (&this->lock);
            return NULL;
        }

        return new SCRetErr ("iodev rk8je loadro/loadrw <disknumber> <filename>");
    }

    if (strcmp (argv[0], "nsperus") == 0) {
        if (argc == 1) {
            return new SCRetInt (this->nsperus);
        }
        if (argc == 2) {
            char *p;
            int dbg = strtol (argv[1], &p, 0);
            if ((*p != 0) || (dbg < 0)) return new SCRetErr ("nsperus %s not ge 0", argv[1]);
            this->nsperus = dbg;
            return NULL;
        }
        return new SCRetErr ("iodev rk8je nsperus [<factor>]");
    }

    // unload <disknumber>
    if (strcmp (argv[0], "unload") == 0) {
        if (argc == 2) {
            char *p;
            int diskno = strtol (argv[1], &p, 0);
            if ((*p != 0) || (diskno < 0) || (diskno > 3)) return new SCRetErr ("disknumber %s not in range 0..3", argv[1]);
            fprintf (stderr, "IODevRK8JE::scriptcmd: drive %d unloaded\n", diskno);
            pthread_mutex_lock (&this->lock);
            close (this->fds[diskno]);
            this->fds[diskno] = -1;
            pthread_mutex_unlock (&this->lock);
            return NULL;
        }

        return new SCRetErr ("iodev rk8je unload <disknumber>");
    }

    return new SCRetErr ("unknown rk8je command %s - valid: help loadro loadrw unload", argv[0]);
}

// perform i/o instruction
uint16_t IODevRK8JE::ioinstr (uint16_t opcode, uint16_t input)
{
    pthread_mutex_lock (&this->lock);

    switch (opcode) {

        // DSKP - skip if transfer done or error
        // p 225 v 9-116
        case 06741: {
            if (this->status & ST_SKIP) input |= IO_SKIP;
            else skipoptwait (opcode, &this->lock, &this->dskpwait);
            break;
        }

        // DCLR - function in AC<01:00>
        case 06742: {
            if ((this->debug > 0) && (this->status & ST_BUSY)) fprintf (stderr, "IODevRK8JE::ioinstr*: DCLR-%u when busy\n", input & 3);
            switch (input & 3) {

                // clear status (unless busy doing something)
                case 0: {
                    if (this->status & ST_BUSY) this->status |= ST_CBSY;
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
                    if (this->status & ST_BUSY) this->status |= ST_CBSY;
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
            if (this->status & ST_BUSY) {
                if (this->debug > 0) fprintf (stderr, "IODevRK8JE::ioinstr*: DLAG when busy\n");
                this->status |= ST_CBSY;
            } else {
                this->diskaddr = input & IO_DATA;
                this->startio ();
                input &= ~ IO_DATA;
            }
            break;
        }

        // DLCA - load current address register from the AC
        case 06744: {
            if (this->status & ST_BUSY) {
                if (this->debug > 0) fprintf (stderr, "IODevRK8JE::ioinstr*: DLCA when busy\n");
                this->status |= ST_CBSY;
            } else {
                this->memaddr = input & IO_DATA;
                input &= ~ IO_DATA;
            }
            break;
        }

        // DRST - clear the AC and read conents of status register into the AC
        case 06745: {
            input &= ~ IO_DATA;
            input |= this->status & IO_DATA;
            if ((this->debug > 0) && (this->lastdebst != this->status)) {
                this->lastdebst = this->status;
                fprintf (stderr, "IODevRK8JE::ioinstr*: status %04o\n", this->lastdebst);
            }
            break;
        }

        // DLDC - load command register from the AC, clear AC and clear status register
        case 06746: {
            if (this->status & ST_BUSY) {
                if (this->debug > 0) fprintf (stderr, "IODevRK8JE::ioinstr*: DLDC when busy\n");
                this->status |= ST_CBSY;
            } else {
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
    this->lastdebst = 0xFFFFU;
    if (this->debug > 0) fprintf (stderr, "IODevRK8JE::startio*: command=%04o\n", this->command);

    // don't allow any changing of registers while this I/O is in progress
    this->status = ST_BUSY;

    // save iot instr address for tracing
    this->dmapc = dyndispc;

    // tell thread to start doing an I/O request
    if (this->threadid == 0) {
        int rc = pthread_create (&this->threadid, NULL, threadwrap, this);
        if (rc != 0) ABORT ();
    }

    pthread_cond_broadcast (&this->cond);
}

#define SETST(n) this->status = (this->status & ST_CBSY) | (n)

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
        if (! (this->status & ST_BUSY)) {
            pthread_cond_broadcast (&this->cond2);
            int rc = pthread_cond_wait (&this->cond, &this->lock);
            if (rc != 0) ABORT ();
        } else {
            int cyldiff, fd, rc;
            struct timespec endts, nowts;
            uint16_t blknum, diskno, icnt, wcnt, xma;
            uint16_t temp[256];
            uint64_t delns, endns, nowns;

            blknum = ((this->command & 1) << 12) | this->diskaddr;
            wcnt   = (this->command & 00100) ? 128 : 256;
            xma    = ((this->command << 9) & 070000) | this->memaddr;
            diskno = (this->command >> 1) & 3;

            // maybe just setting write-locked mode
            if ((this->command >> 9) == 2) {
                if (! this->ros[diskno]) {
                    this->ros[diskno] = true;
                    fd = this->fds[diskno];
                    if (flock (fd, LOCK_SH | LOCK_NB) < 0) {
                        fprintf (stderr, "IODevRK8JE::thread: error downgrading to shared lock on disk %u: %m\n", diskno);
                        SETST (ST_DONE | ST_FLNR);                              // file not ready
                        goto iodone;
                    }
                }
                SETST (ST_DONE);
                goto iodone;
            }

            // validate parameters
            if (blknum >= NBLKS) {
                fprintf (stderr, "IODevRK8JE::thread: bad disk addr %05o\n", blknum);
                SETST (ST_DONE | ST_CYLR);                                      // cylinder address error
                goto iodone;
            }

            cyldiff = blknum / SECPERCYL - this->lastdas[diskno] / SECPERCYL;
            if (cyldiff < 0) cyldiff = - cyldiff;
            if (cyldiff > 0) SETST (ST_BUSY | ST_HDIM);                         // busy; head in motion

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
                if (! (this->status & ST_BUSY)) goto ioabrt;                    // DCLR cleared busy so we're done
                if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) ABORT ();
                nowns = (uint64_t) nowts.tv_sec * 1000000000 + nowts.tv_nsec;
            } while (nowns < endns);
            this->lastdas[diskno] = blknum;                                     // remember head position for next seek time calculation

            // error if no file loaded
            fd = this->fds[diskno];
            if (fd < 0) {
                SETST (ST_DONE | ST_FLNR);                                      // file not ready
                goto iodone;
            }

            // perform the read or write

            switch (this->command >> 9) {

                // read data
                case 1: {
                    if (this->debug > 0) fprintf (stderr, "IODevRK8JE::thread*: %u treating read all as normal read\n", diskno);
                }
                case 0: {
                    if (this->debug > 0) fprintf (stderr, "IODevRK8JE::thread*: %u reading %u words at %u into %05o (%u ms)\n", diskno, wcnt, blknum, xma, (uint32_t) ((delns + 500000) / 1000000));
                    ASSERT (wcnt * 2 <= sizeof temp);
                    rc = pread (fd, temp, wcnt * 2, blknum * 512);
                    if (rc < wcnt * 2) {
                        SETST (ST_DONE | ST_CRCR);                              // crc error
                        if (rc < 0) {
                            fprintf (stderr, "IODevRK8JE::thread: error reading %u words from disk %u at %05o: %m\n", wcnt, diskno, blknum);
                        } else {
                            fprintf (stderr, "IODevRK8JE::thread: only read %d bytes of %u words from disk %u at %05o\n", rc, wcnt, diskno, blknum);
                        }
                        break;
                    }
                    dyndisdma (xma, wcnt, true, this->dmapc);
                    for (icnt = 0; icnt < wcnt; icnt ++) {
                        memarray[(xma&070000)|((xma+icnt)&007777)] = temp[icnt] & 07777;
                    }
                    this->memaddr = (this->memaddr + wcnt) & 07777;
                    SETST (ST_DONE);                                            // done
                    break;
                }

                // seek only
                case 3: {
                    SETST (ST_DONE);                                            // done
                    break;
                }

                // write data
                case 5: {
                    if (this->debug > 0) fprintf (stderr, "IODevRK8JE::thread*: %u treating write all as normal write\n", diskno);
                }
                case 4: {
                    if (this->debug > 0) fprintf (stderr, "IODevRK8JE::thread*: %u writing %u words at %u from %05o (%u ms)\n", diskno, wcnt, blknum, xma, (uint32_t) ((delns + 500000) / 1000000));
                    if (this->ros[diskno]) {
                        SETST (ST_DONE | ST_WLER);                              // write lock error
                        wcnt = 0;
                        break;
                    }
                    dyndisdma (xma, wcnt, false, this->dmapc);
                    ASSERT (wcnt * 2 <= sizeof temp);
                    for (icnt = 0; icnt < wcnt; icnt ++) {
                        temp[icnt] = memarray[(xma&070000)|((xma+icnt)&007777)] & 07777;
                    }
                    if (wcnt < 256) memset (&temp[wcnt], 0, 512 - 2 * wcnt);
                    rc = pwrite (fd, temp, 512, blknum * 512);
                    if (rc < 512) {
                        SETST (ST_DONE | ST_CRCR);                              // crc error
                        if (rc < 0) {
                            fprintf (stderr, "IODevRK8JE::thread: error writing %u words from disk %u at %05o: %m\n", wcnt, diskno, blknum);
                        } else {
                            fprintf (stderr, "IODevRK8JE::thread: only wrote %d bytes of %u words from disk %u at %05o\n", rc, wcnt, diskno, blknum);
                        }
                        break;
                    }
                    this->memaddr = (this->memaddr + wcnt) & 07777;
                    SETST (ST_DONE);                                            // done
                    break;
                }

                default: {
                    fprintf (stderr, "IODevRK8JE::thread: bad command code %04o\n", this->command);
                    SETST (ST_DONE | ST_TMER);                                  // timing error
                    break;
                }
            }

            // maybe request an interrupt
        iodone:;
            ASSERT ((this->status & (ST_BUSY | ST_DONE)) == ST_DONE);
        ioabrt:;
            if (this->debug > 0) fprintf (stderr, "IODevRK8JE::thread*: status=%04o\n", this->status);
            this->updateirq ();
        }
    }

    // ioreset(), unlock and exit thread
    pthread_mutex_unlock (&this->lock);
}

void IODevRK8JE::updateirq ()
{
    if ((this->command & 00400) && (this->status & ST_DONE)) {
        setintreqmask (IRQ_RK8);
    } else {
        clrintreqmask (IRQ_RK8, this->dskpwait && (this->status & ST_SKIP));
    }
}
