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
#include "gpiolib.h"
#include "iodevrk8je.h"
#include "memory.h"
#include "rdcyc.h"
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

static SCRetErr *writeformat (int fd);

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
            char *lockerr = lockfile (fd, loadro ? F_RDLCK : F_WRLCK);
            if (lockerr != NULL) {
                SCRetErr *err = new SCRetErr ("%s", lockerr);
                close (fd);
                free (lockerr);
                return err;
            }
            long oldsize = lseek (fd, 0, SEEK_END);
            if (loadrw && (ftruncate (fd, NBLKS * 512) < 0)) {
                SCRetErr *err = new SCRetErr ("%m");
                close (fd);
                return err;
            }
            if (loadrw && (oldsize == 0)) {
                SCRetErr *err = writeformat (fd);
                if (err != NULL) {
                    close (fd);
                    return err;
                }
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
        int rc = createthread (&this->threadid, threadwrap, this);
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

// contents of a freshly zeroed decpack in OS/8
// second half has a couple different numbers
static uint32_t format1ofs = 00001000;
static uint16_t format1buf[] = {
    007777, 000007, 000000, 000000, 007777, 000000, 001527, 000000,
    003032, 001777, 003024, 005776, 001375, 003267, 006201, 001744,
    003667, 007040, 001344, 003344, 007040, 001267, 002020, 005215,
    007200, 006211, 005607, 000201, 000440, 002331, 002324, 000515,
    004010, 000501, 000400, 001774, 001373, 003774, 001774, 000372,
    001371, 003770, 001263, 003767, 003032, 001777, 003024, 006202,
    004424, 000200, 003400, 000007, 005766, 004765, 005776, 000000,
    001032, 007650, 005764, 005667, 001725, 002420, 002524, 004005,
    002222, 001722, 000000, 003344, 001763, 003267, 003204, 007346,
    003207, 001667, 007112, 007012, 007012, 000362, 007440, 004761,
    002204, 005340, 002267, 002207, 005311, 001344, 007640, 005760,
    001667, 007650, 005760, 001357, 004756, 002344, 007240, 005310,
    007240, 003204, 001667, 005315, 000000, 003020, 001411, 003410,
    002020, 005346, 005744, 000000, 000000, 000000, 005172, 000256,
    004620, 004627, 000077, 005533, 005622, 006443, 003110, 007622,
    007621, 006360, 000017, 007760, 007617, 006777, 002600, 006023,
    000000, 001777, 003024, 001776, 003025, 003775, 003774, 003773,
    001776, 003772, 005600, 000000, 007346, 004771, 001770, 007041,
    001027, 007500, 005233, 003012, 001770, 007440, 004771, 003410,
    002012, 005227, 005613, 003012, 001027, 007440, 004771, 001011,
    001012, 003011, 005613, 000000, 006201, 001653, 006211, 001367,
    007640, 005766, 005643, 003605, 000000, 004765, 007700, 004764,
    000005, 001025, 001363, 007650, 002032, 005654, 000000, 003254,
    001762, 007650, 005761, 001254, 007650, 004760, 005666, 000000,
    003200, 001357, 003030, 006211, 004765, 007700, 005315, 006202,
    004600, 000210, 001400, 000001, 005756, 003007, 001755, 001354,
    007650, 005326, 001200, 001363, 007640, 005330, 001353, 003030,
    005677, 003205, 002217, 004023, 003123, 007700, 007700, 005342,
    004764, 000002, 004764, 000000, 001352, 003751, 005750, 000000,
    002030, 002014, 003205, 000070, 007710, 001401, 005626, 000007,
    003700, 003023, 007600, 000171, 003522, 002473, 006141, 000600,
    007204, 006344, 002215, 002223, 002222, 002352, 006065, 006023,
    003034, 007240, 001200, 003224, 001377, 003624, 001776, 000375,
    007650, 005600, 001374, 004773, 005600, 001723, 005770, 004020,
    001120, 004026, 006164, 000100, 003033, 003506, 001772, 007450,
    007001, 000371, 007041, 005625, 000000, 001770, 003246, 006202,
    004646, 000011, 000000, 007667, 006212, 005634, 000000, 007643,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    001367, 001354, 000275, 006443, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    003457, 000077, 007646, 004600, 006615, 000007, 007644, 000020,
    005402, 002036, 004777, 000013, 006201, 001433, 006211, 001376,
    007640, 005225, 001234, 003775, 001235, 003243, 001236, 003242,
    003637, 001244, 003640, 001311, 003641, 004774, 007240, 003773,
    004772, 001771, 003034, 005600, 000174, 007747, 000005, 005422,
    006033, 005636, 000017, 007756, 005242, 000000, 003252, 007344,
    004770, 006201, 000000, 006211, 007414, 004767, 005645, 000000,
    001304, 007440, 001020, 007640, 005305, 001020, 007041, 003304,
    001033, 003022, 001304, 004766, 007650, 005311, 002022, 006201,
    001422, 006211, 004765, 005657, 000000, 004764, 006677, 004764,
    006330, 007600, 004764, 006652, 007200, 004764, 006667, 004764,
    007200, 004311, 001414, 000507, 000114, 004052, 004017, 002240,
    007700, 004017, 002024, 001117, 001640, 002516, 001316, 001727,
    001600, 004302, 000104, 004023, 002711, 002403, 001040, 001720,
    002411, 001716, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 004200, 007324, 006305, 007400,
    002670, 002220, 006433, 002723, 006713, 002724, 002600, 000200,
    004302, 000104, 004005, 003024, 000516, 002311, 001716, 000000,
    005410, 007240, 003244, 003030, 003031, 004777, 001376, 007650,
    005231, 003247, 001375, 004774, 006211, 000023, 006201, 006773,
    005610, 007201, 001773, 001247, 007640, 005245, 001023, 003030,
    002244, 005245, 001024, 005214, 007777, 004772, 006750, 000000,
    004771, 005647, 000000, 003315, 001770, 007006, 007700, 005767,
    001766, 007640, 005767, 001765, 007650, 001770, 000364, 007650,
    005274, 001763, 000362, 003765, 001715, 007450, 004761, 003715,
    002315, 001715, 007640, 005652, 001770, 000364, 007740, 007240,
    001360, 004774, 006201, 006773, 006211, 000000, 005652, 006061,
    007106, 007006, 007006, 005717, 000000, 007450, 005724, 003332,
    004732, 005724, 000000, 001023, 000357, 007650, 005756, 004755,
    004772, 007342, 004017, 002024, 001117, 001640, 000115, 000211,
    000725, 001725, 002300, 000000, 000000, 004240, 007474, 000077,
    007775, 006525, 000017, 007617, 000200, 007600, 002723, 007111,
    002537, 003426, 004200, 006103, 002670, 007774, 007506, 006000,
    000000, 001777, 007012, 007630, 001376, 001375, 003234, 001634,
    007640, 005600, 004774, 000012, 000000, 000000, 000000, 005773,
    001215, 003634, 001234, 000376, 007650, 005600, 002234, 001372,
    004771, 006201, 006773, 006211, 007600, 005600, 000000, 003304,
    004770, 003306, 004307, 005767, 007240, 001347, 003305, 004307,
    007410, 005766, 006201, 001705, 002305, 000365, 007640, 005253,
    001705, 006211, 000364, 007440, 004763, 001306, 003032, 001032,
    001362, 007650, 004761, 005636, 001023, 007112, 007012, 007012,
    001256, 000365, 001256, 005262, 000000, 000000, 000000, 000000,
    001360, 003351, 007346, 003350, 006201, 001704, 006211, 002304,
    007450, 005707, 003347, 001751, 007450, 005345, 000365, 007640,
    001365, 001357, 006201, 000747, 006211, 007041, 001751, 007640,
    005310, 002347, 002351, 002350, 005323, 002307, 005707, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 007700,
    000023, 003416, 007506, 006400, 000377, 000077, 007333, 006352,
    006000, 002670, 007774, 006615, 000200, 007600, 000005, 002537,
    000005, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 004000, 000000, 000000, 002000, 000000, 007607,
    007621, 007607, 007621, 007222, 007223, 007224, 007225, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 006202,
    004207, 001000, 000000, 000007, 007746, 006203, 005677, 000400,
    003461, 003340, 006214, 001275, 003336, 006201, 001674, 007010,
    006211, 007630, 005321, 006202, 004207, 005010, 000000, 000027,
    007402, 006202, 004207, 000610, 000000, 000013, 007602, 005020,
    006202, 004207, 001010, 000000, 000027, 007402, 006213, 005700,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    004230, 004230, 004230, 004230, 004230, 004230, 004230, 004230,
    000000, 001040, 004160, 004160, 001020, 002010, 000000, 000000,
    000000, 000070, 000002, 001472, 007777, 000102, 002314, 000000,
    002326, 003017, 007773, 000303, 001400, 000000, 000000, 000000,
    001441, 003055, 001573, 003574, 001572, 003442, 000000, 007004,
    007720, 001572, 001043, 007650, 005547, 005444, 000564, 000772,
    000000, 007665, 001010, 001565, 000000, 000000, 000000, 007653,
    007776, 000000, 000017, 000000, 000000, 000005, 000000, 004000,
    006202, 004575, 000210, 001000, 000015, 007402, 005467, 001011,
    002224, 000000, 002326, 003457, 007747, 002205, 002317, 000400,
    007741, 000070, 007642, 007400, 007643, 007760, 007644, 006001,
    000323, 007773, 007770, 000252, 000701, 000767, 000610, 000760,
    001403, 007666, 000371, 006006, 000651, 000752, 000007, 000377,
    001273, 000613, 000622, 000251, 004210, 001402, 000343, 007774,
    001404, 001400, 000250, 001401, 007757, 007646, 000771, 000247,
    000015, 000017, 000617, 000177, 000246, 007600, 000100, 000321,
    000544, 000240, 000020, 000563, 007761, 007740, 000620, 000220,
    007730, 007746, 007736, 007700, 000200, 007607, 007764, 006203,
    003461, 003055, 003040, 006214, 001177, 003345, 001345, 003241,
    004220, 002200, 007100, 001176, 007430, 005247, 001244, 003220,
    000467, 001200, 004343, 005620, 000247, 000401, 000603, 001011,
    001177, 000263, 000306, 000253, 000324, 000341, 000400, 001343,
    000620, 006213, 003600, 006213, 005640, 002057, 002057, 002057,
    002057, 002057, 002057, 007200, 006202, 004575, 000210, 001400,
    000056, 007402, 005657, 007330, 004351, 004220, 003041, 006202,
    004575, 000601, 000000, 000051, 005246, 001241, 006203, 004574,
    003241, 001241, 003345, 004351, 007120, 005637, 004220, 003041,
    006202, 004575, 000101, 007400, 000057, 005246, 001041, 006203,
    005713, 007200, 002200, 001040, 007110, 001200, 003573, 001241,
    003572, 007221, 006201, 000571, 006211, 007010, 007730, 005572,
    005570, 007120, 005325, 000223, 003240, 006213, 001640, 006213,
    005743, 000000, 001374, 003364, 006201, 001571, 006211, 007012,
    007630, 005751, 006202, 004575, 000000, 000000, 000033, 005246,
    005751, 006202, 004575, 000111, 001000, 000026, 005246, 005774,
    007201, 003054, 001055, 007440, 005246, 004567, 002574, 007450,
    005566, 003052, 004567, 007450, 005221, 001052, 007004, 007130,
    003052, 001165, 003017, 001164, 003042, 001417, 007041, 001052,
    007650, 005241, 002042, 005225, 001017, 007700, 005566, 001163,
    005222, 001042, 001162, 004561, 004567, 002574, 004560, 007450,
    001442, 007450, 005557, 007044, 007620, 001156, 001156, 003301,
    001041, 003047, 001441, 001054, 007640, 005332, 004567, 007010,
    007520, 005557, 007004, 000155, 003302, 004334, 003303, 006202,
    004575, 000100, 007200, 000016, 005554, 001164, 003044, 001044,
    004560, 007040, 001302, 007130, 001301, 007700, 003441, 004334,
    007041, 001303, 007640, 005330, 001442, 000153, 001302, 003441,
    002044, 005307, 001447, 005552, 000520, 001442, 007106, 007006,
    007006, 000151, 001150, 005734, 000511, 000151, 007450, 005547,
    003052, 001052, 001146, 003042, 001052, 001145, 003041, 001052,
    001144, 003050, 001441, 005744, 004631, 005723, 006373, 006473,
    006374, 006474, 006375, 006475, 005524, 004020, 004604, 004605,
    004024, 004224, 000000, 007100, 004222, 005213, 004301, 005220,
    001045, 007041, 001543, 004561, 002574, 001046, 007041, 004561,
    002574, 005557, 000000, 007440, 005251, 003045, 003046, 001055,
    004560, 007450, 005542, 003051, 004567, 003044, 007420, 007030,
    007012, 000450, 007640, 005220, 001450, 007700, 005622, 002222,
    007201, 003367, 001051, 000153, 007106, 007004, 001367, 007041,
    001007, 007450, 005270, 007041, 001007, 003007, 007330, 004360,
    001541, 007064, 007530, 005370, 007070, 003053, 001140, 003017,
    005622, 000000, 001417, 007650, 005340, 007240, 001017, 003017,
    001137, 003046, 001044, 003047, 001047, 004536, 007041, 001417,
    007640, 005335, 002047, 002046, 005314, 004352, 001417, 007450,
    005341, 007041, 003046, 002301, 005701, 001046, 007001, 004352,
    001417, 001045, 003045, 002053, 005302, 003045, 001535, 007440,
    005251, 005701, 000000, 001540, 007041, 001017, 003017, 005752,
    000000, 001134, 003365, 006202, 004451, 004210, 001400, 000001,
    005533, 005760, 000000, 000000, 000000, 000000, 000222, 000223,
    000224, 000225, 004576, 000605, 001010, 001011, 001200, 001312,
    000000, 003056, 004532, 005531, 004530, 003042, 001140, 003017,
    001055, 007112, 007012, 000127, 007041, 003043, 001450, 000126,
    007640, 005566, 001417, 007650, 005343, 007346, 004525, 001417,
    001042, 003042, 002053, 005232, 001056, 007640, 005263, 001535,
    007440, 005524, 001044, 004536, 007640, 001046, 007650, 005566,
    001054, 002056, 005524, 001017, 003041, 001137, 004525, 001540,
    007041, 001017, 001123, 007700, 005522, 001441, 003417, 007240,
    001041, 003041, 007344, 001017, 003017, 001041, 007161, 001052,
    007640, 005275, 001137, 003041, 001052, 003017, 001044, 004536,
    003417, 002044, 002041, 005316, 001521, 003417, 007001, 004525,
    003417, 001017, 003520, 007240, 001541, 003541, 001450, 001054,
    003450, 004517, 005516, 001417, 007141, 003041, 001043, 001046,
    007200, 001043, 007450, 007020, 001041, 007220, 001041, 007041,
    001046, 007670, 005374, 001041, 003046, 007344, 001017, 003052,
    001515, 003054, 001042, 003045, 001041, 007041, 005240, 004532
};

static uint32_t const format2ofs = 06261000;
static uint16_t const format2buf[] = {
    007777, 000007, 000000, 000000, 007777, 000000, 001527, 000000,
    003032, 001777, 003024, 005776, 001375, 003267, 006201, 001744,
    003667, 007040, 001344, 003344, 007040, 001267, 002020, 005215,
    007200, 006211, 005607, 000201, 000440, 002331, 002324, 000515,
    004010, 000501, 000400, 001774, 001373, 003774, 001774, 000372,
    001371, 003770, 001263, 003767, 003032, 001777, 003024, 006202,
    004424, 000200, 003400, 000007, 005766, 004765, 005776, 000000,
    001032, 007650, 005764, 005667, 001725, 002420, 002524, 004005,
    002222, 001722, 000000, 003344, 001763, 003267, 003204, 007346,
    003207, 001667, 007112, 007012, 007012, 000362, 007440, 004761,
    002204, 005340, 002267, 002207, 005311, 001344, 007640, 005760,
    001667, 007650, 005760, 001357, 004756, 002344, 007240, 005310,
    007240, 003204, 001667, 005315, 000000, 003020, 001411, 003410,
    002020, 005346, 005744, 000000, 000000, 000000, 005172, 000256,
    004620, 004627, 000077, 005533, 005622, 006443, 003110, 007622,
    007621, 006360, 000017, 007760, 007617, 006777, 002600, 006023,
    000000, 001777, 003024, 001776, 003025, 003775, 003774, 003773,
    001776, 003772, 005600, 000000, 007346, 004771, 001770, 007041,
    001027, 007500, 005233, 003012, 001770, 007440, 004771, 003410,
    002012, 005227, 005613, 003012, 001027, 007440, 004771, 001011,
    001012, 003011, 005613, 000000, 006201, 001653, 006211, 001367,
    007640, 005766, 005643, 003605, 000000, 004765, 007700, 004764,
    000005, 001025, 001363, 007650, 002032, 005654, 000000, 003254,
    001762, 007650, 005761, 001254, 007650, 004760, 005666, 000000,
    003200, 001357, 003030, 006211, 004765, 007700, 005315, 006202,
    004600, 000210, 001400, 000001, 005756, 003007, 001755, 001354,
    007650, 005326, 001200, 001363, 007640, 005330, 001353, 003030,
    005677, 003205, 002217, 004023, 003123, 007700, 007700, 005342,
    004764, 000002, 004764, 000000, 001352, 003751, 005750, 000000,
    002030, 002014, 003205, 000070, 007710, 001401, 005626, 000007,
    003700, 003023, 007600, 000171, 003522, 002473, 006141, 000600,
    007204, 006344, 002215, 002223, 002222, 002352, 006065, 006023,
    003034, 007240, 001200, 003224, 001377, 003624, 001776, 000375,
    007650, 005600, 001374, 004773, 005600, 001723, 005770, 004020,
    001120, 004026, 006164, 000100, 003033, 003506, 001772, 007450,
    007001, 000371, 007041, 005625, 000000, 001770, 003246, 006202,
    004646, 000011, 000000, 007667, 006212, 005634, 000000, 007643,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    001367, 001354, 000275, 006443, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    003457, 000077, 007646, 004600, 006615, 000007, 007644, 000020,
    005402, 002036, 004777, 000013, 006201, 001433, 006211, 001376,
    007640, 005225, 001234, 003775, 001235, 003243, 001236, 003242,
    003637, 001244, 003640, 001311, 003641, 004774, 007240, 003773,
    004772, 001771, 003034, 005600, 000174, 007747, 000005, 005422,
    006033, 005636, 000017, 007756, 005242, 000000, 003252, 007344,
    004770, 006201, 000000, 006211, 007414, 004767, 005645, 000000,
    001304, 007440, 001020, 007640, 005305, 001020, 007041, 003304,
    001033, 003022, 001304, 004766, 007650, 005311, 002022, 006201,
    001422, 006211, 004765, 005657, 000000, 004764, 006677, 004764,
    006330, 007600, 004764, 006652, 007200, 004764, 006667, 004764,
    007200, 004311, 001414, 000507, 000114, 004052, 004017, 002240,
    007700, 004017, 002024, 001117, 001640, 002516, 001316, 001727,
    001600, 004302, 000104, 004023, 002711, 002403, 001040, 001720,
    002411, 001716, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 004200, 007324, 006305, 007400,
    002670, 002220, 006433, 002723, 006713, 002724, 002600, 000200,
    004302, 000104, 004005, 003024, 000516, 002311, 001716, 000000,
    005410, 007240, 003244, 003030, 003031, 004777, 001376, 007650,
    005231, 003247, 001375, 004774, 006211, 000023, 006201, 006773,
    005610, 007201, 001773, 001247, 007640, 005245, 001023, 003030,
    002244, 005245, 001024, 005214, 007777, 004772, 006750, 000000,
    004771, 005647, 000000, 003315, 001770, 007006, 007700, 005767,
    001766, 007640, 005767, 001765, 007650, 001770, 000364, 007650,
    005274, 001763, 000362, 003765, 001715, 007450, 004761, 003715,
    002315, 001715, 007640, 005652, 001770, 000364, 007740, 007240,
    001360, 004774, 006201, 006773, 006211, 000000, 005652, 006061,
    007106, 007006, 007006, 005717, 000000, 007450, 005724, 003332,
    004732, 005724, 000000, 001023, 000357, 007650, 005756, 004755,
    004772, 007342, 004017, 002024, 001117, 001640, 000115, 000211,
    000725, 001725, 002300, 000000, 000000, 004240, 007474, 000077,
    007775, 006525, 000017, 007617, 000200, 007600, 002723, 007111,
    002537, 003426, 004200, 006103, 002670, 007774, 007506, 006000,
    000000, 001777, 007012, 007630, 001376, 001375, 003234, 001634,
    007640, 005600, 004774, 000012, 000000, 000000, 000000, 005773,
    001215, 003634, 001234, 000376, 007650, 005600, 002234, 001372,
    004771, 006201, 006773, 006211, 007600, 005600, 000000, 003304,
    004770, 003306, 004307, 005767, 007240, 001347, 003305, 004307,
    007410, 005766, 006201, 001705, 002305, 000365, 007640, 005253,
    001705, 006211, 000364, 007440, 004763, 001306, 003032, 001032,
    001362, 007650, 004761, 005636, 001023, 007112, 007012, 007012,
    001256, 000365, 001256, 005262, 000000, 000000, 000000, 000000,
    001360, 003351, 007346, 003350, 006201, 001704, 006211, 002304,
    007450, 005707, 003347, 001751, 007450, 005345, 000365, 007640,
    001365, 001357, 006201, 000747, 006211, 007041, 001751, 007640,
    005310, 002347, 002351, 002350, 005323, 002307, 005707, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 007700,
    000023, 003416, 007506, 006400, 000377, 000077, 007333, 006352,
    006000, 002670, 007774, 006615, 000200, 007600, 000005, 002537,
    000006, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 004000, 000000, 000000, 002000, 000000, 007607,
    007621, 007607, 007621, 007222, 007223, 007224, 007225, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 006202,
    004207, 001000, 000000, 000007, 007746, 006203, 005677, 000400,
    003461, 003340, 006214, 001275, 003336, 006201, 001674, 007010,
    006211, 007630, 005321, 006202, 004207, 005010, 000000, 000027,
    007402, 006202, 004207, 000610, 000000, 000013, 007602, 005020,
    006202, 004207, 001010, 000000, 000027, 007402, 006213, 005700,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    004230, 004230, 004230, 004230, 004230, 004230, 004230, 004230,
    000000, 001040, 004160, 004160, 001020, 002010, 000000, 000000,
    000000, 000070, 000002, 001472, 007777, 000102, 002314, 000000,
    002326, 003017, 007773, 000303, 001400, 000000, 000000, 000000,
    001441, 003055, 001573, 003574, 001572, 003442, 000000, 007004,
    007720, 001572, 001043, 007650, 005547, 005444, 000564, 000772,
    000000, 007665, 001010, 001565, 000000, 000000, 000000, 007654,
    007776, 000000, 000017, 000000, 000000, 000006, 000000, 004000,
    006202, 004575, 000210, 001000, 000015, 007402, 005467, 001011,
    002224, 000000, 002326, 003457, 007747, 002205, 002317, 000400,
    007741, 000070, 007642, 007400, 007643, 007760, 007644, 006001,
    000323, 007773, 007770, 000252, 000701, 000767, 000610, 000760,
    001403, 007666, 000371, 006006, 000651, 000752, 000007, 000377,
    001273, 000613, 000622, 000251, 004210, 001402, 000343, 007774,
    001404, 001400, 000250, 001401, 007757, 007646, 000771, 000247,
    000015, 000017, 000617, 000177, 000246, 007600, 000100, 000321,
    000544, 000240, 000020, 000563, 007761, 007740, 000620, 000220,
    007730, 007746, 007736, 007700, 000200, 007607, 007764, 006203,
    003461, 003055, 003040, 006214, 001177, 003345, 001345, 003241,
    004220, 002200, 007100, 001176, 007430, 005247, 001244, 003220,
    000467, 001200, 004343, 005620, 000247, 000401, 000603, 001011,
    001177, 000263, 000306, 000253, 000324, 000341, 000400, 001343,
    000620, 006213, 003600, 006213, 005640, 002057, 002057, 002057,
    002057, 002057, 002057, 007200, 006202, 004575, 000210, 001400,
    000056, 007402, 005657, 007330, 004351, 004220, 003041, 006202,
    004575, 000601, 000000, 000051, 005246, 001241, 006203, 004574,
    003241, 001241, 003345, 004351, 007120, 005637, 004220, 003041,
    006202, 004575, 000101, 007400, 000057, 005246, 001041, 006203,
    005713, 007200, 002200, 001040, 007110, 001200, 003573, 001241,
    003572, 007221, 006201, 000571, 006211, 007010, 007730, 005572,
    005570, 007120, 005325, 000223, 003240, 006213, 001640, 006213,
    005743, 000000, 001374, 003364, 006201, 001571, 006211, 007012,
    007630, 005751, 006202, 004575, 000000, 000000, 000033, 005246,
    005751, 006202, 004575, 000111, 001000, 000026, 005246, 005774,
    007201, 003054, 001055, 007440, 005246, 004567, 002574, 007450,
    005566, 003052, 004567, 007450, 005221, 001052, 007004, 007130,
    003052, 001165, 003017, 001164, 003042, 001417, 007041, 001052,
    007650, 005241, 002042, 005225, 001017, 007700, 005566, 001163,
    005222, 001042, 001162, 004561, 004567, 002574, 004560, 007450,
    001442, 007450, 005557, 007044, 007620, 001156, 001156, 003301,
    001041, 003047, 001441, 001054, 007640, 005332, 004567, 007010,
    007520, 005557, 007004, 000155, 003302, 004334, 003303, 006202,
    004575, 000100, 007200, 000016, 005554, 001164, 003044, 001044,
    004560, 007040, 001302, 007130, 001301, 007700, 003441, 004334,
    007041, 001303, 007640, 005330, 001442, 000153, 001302, 003441,
    002044, 005307, 001447, 005552, 000520, 001442, 007106, 007006,
    007006, 000151, 001150, 005734, 000511, 000151, 007450, 005547,
    003052, 001052, 001146, 003042, 001052, 001145, 003041, 001052,
    001144, 003050, 001441, 005744, 004631, 005723, 006373, 006473,
    006374, 006474, 006375, 006475, 005524, 004020, 004604, 004605,
    004024, 004224, 000000, 007100, 004222, 005213, 004301, 005220,
    001045, 007041, 001543, 004561, 002574, 001046, 007041, 004561,
    002574, 005557, 000000, 007440, 005251, 003045, 003046, 001055,
    004560, 007450, 005542, 003051, 004567, 003044, 007420, 007030,
    007012, 000450, 007640, 005220, 001450, 007700, 005622, 002222,
    007201, 003367, 001051, 000153, 007106, 007004, 001367, 007041,
    001007, 007450, 005270, 007041, 001007, 003007, 007330, 004360,
    001541, 007064, 007530, 005370, 007070, 003053, 001140, 003017,
    005622, 000000, 001417, 007650, 005340, 007240, 001017, 003017,
    001137, 003046, 001044, 003047, 001047, 004536, 007041, 001417,
    007640, 005335, 002047, 002046, 005314, 004352, 001417, 007450,
    005341, 007041, 003046, 002301, 005701, 001046, 007001, 004352,
    001417, 001045, 003045, 002053, 005302, 003045, 001535, 007440,
    005251, 005701, 000000, 001540, 007041, 001017, 003017, 005752,
    000000, 001134, 003365, 006202, 004451, 004210, 001400, 000001,
    005533, 005760, 000000, 000000, 000000, 000000, 000222, 000223,
    000224, 000225, 004576, 000605, 001010, 001011, 001200, 001312,
    000000, 003056, 004532, 005531, 004530, 003042, 001140, 003017,
    001055, 007112, 007012, 000127, 007041, 003043, 001450, 000126,
    007640, 005566, 001417, 007650, 005343, 007346, 004525, 001417,
    001042, 003042, 002053, 005232, 001056, 007640, 005263, 001535,
    007440, 005524, 001044, 004536, 007640, 001046, 007650, 005566,
    001054, 002056, 005524, 001017, 003041, 001137, 004525, 001540,
    007041, 001017, 001123, 007700, 005522, 001441, 003417, 007240,
    001041, 003041, 007344, 001017, 003017, 001041, 007161, 001052,
    007640, 005275, 001137, 003041, 001052, 003017, 001044, 004536,
    003417, 002044, 002041, 005316, 001521, 003417, 007001, 004525,
    003417, 001017, 003520, 007240, 001541, 003541, 001450, 001054,
    003450, 004517, 005516, 001417, 007141, 003041, 001043, 001046,
    007200, 001043, 007450, 007020, 001041, 007220, 001041, 007041,
    001046, 007670, 005374, 001041, 003046, 007344, 001017, 003052,
    001515, 003054, 001042, 003045, 001041, 007041, 005240, 004532
};

static SCRetErr *writeformat (int fd)
{
    uint32_t        const ofss[] = { format1ofs, format2ofs };
    uint16_t const *const bufs[] = { format1buf, format2buf };

    for (int i = 0; i < 2; i ++) {
        int rc = pwrite (fd, bufs[i], sizeof format1buf, ofss[i]);
        if (rc != (int) sizeof format2buf) {
            if (rc < 0) return new SCRetErr ("error writing format: %m\n");
            return new SCRetErr ("only wrote %d of %d-byte format\n", rc, (int) sizeof format2buf);
        }
    }
    return NULL;
}
