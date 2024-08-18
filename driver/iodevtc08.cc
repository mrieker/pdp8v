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

// TC08 controller maintenance 1970
// boot code: OS/8 Handbook April 1974, p41
//  07613: 06774, 01222, 06766, 06771, 05216, 01223, 05215, 00600, 00220

// files are 1474*129*2 bytes = 380292 bytes
// loading a file read/write extends/truncates it to correct size
// if read/write file was null length, it is formatted for OS/8

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

#include "dyndis.h"
#include "gpiolib.h"
#include "iodevtc08.h"
#include "memext.h"
#include "memory.h"
#include "rdcyc.h"
#include "shadow.h"

/*
  status_a:
    | unitsel   |REV|GO |CON|  opcode   |IEN|PAF|PSF|
    +---+---+---+---+---+---+---+---+---+---+---+---+
    | 4   2   1 | 4   2   1 | 4   2   1 | 4   2   1 |

  status_b:
    |ERR|   |EOT|SEL|       | dma field |       |SUC|
    +---+---+---+---+---+---+---+---+---+---+---+---+
    | 4   2   1 | 4   2   1 | 4   2   1 | 4   2   1 |
*/

#define DBGPR if (shm->debug > 0) dbgpr

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#define BLOCKSPERTAPE 1474  // 02702
#define WORDSPERBLOCK 129
#define BYTESPERBLOCK (WORDSPERBLOCK*2)
#define UNLOADNS 5000000000ULL

#define CONTIN (shm->status_a & 00100)
#define NORMAL (! CONTIN)

#define REVBIT 00400
#define GOBIT  00200

#define REVERS (shm->status_a & REVBIT)
#define FORWRD (! REVERS)

#define DTFLAG 00001    // successful completion
#define ERFLAG 04000    // error completion
#define ERRORS 07700
#define ENDTAP 05000
#define SELERR 04400

#define INTENA 00004    // interrupt enable
#define INTREQ 04001    // request interrupt if error or success

#define IDWC 07754      // memory word containing 2s comp dma word count
#define IDCA 07755      // memory word containing dma address minus one

IODevTC08 iodevtc08;

static uint16_t ocarray[4096];

static IODevOps const iodevops[] = {
    { 06761, "DTRA (TC08) read status register A" },
    { 06762, "DTCA (TC08) clear status register A" },
    { 06764, "DTXA (TC08) load status register A" },
    { 06766, "DTLA (TC08) clear and load status register A" },
    { 06771, "DTSF (TC08) skip on flag" },
    { 06772, "DTRB (TC08) read status register B" },
    { 06773, "DTSF DTRB (TC08) skip on flag, read status register B" },
    { 06774, "DTXB (TC08) load status register B" },
};

static char const *const cmdmnes[8] = { "MOVE", "SRCH", "RDAT", "RALL", "WDAT", "WALL", "WTIM", "ERR7" };

static uint64_t getnowns ();
static bool dmareadoverwritesinstructions (uint16_t field, uint16_t idca, uint16_t idwc);
static SCRetErr *writeformat (int fd);

IODevTC08::IODevTC08 ()
{
    opscount = sizeof iodevops / sizeof iodevops[0];
    opsarray = iodevops;

    iodevname = "tc08";

    // initialize obverse complement lookup table
    //   abcdefghijkl <=> ~jklghidefabc
    for (int i = 0; i < 4096; i ++) {
        uint16_t reverse = 07777;
        for (int j = 0; j < 12; j += 3) {
            uint32_t threebits = (i >> j) & 7;
            reverse -= threebits << (9 - j);
        }
        ocarray[i] = reverse;
    }

    // initialize everything
    this->shm = NULL;
    this->shmfd = -1;
    this->allowskipopt = true;
    this->dskpwait = false;
}

IODevTC08::~IODevTC08 ()
{
    // nothing to do if shared memory was never set up
    if (this->shm == NULL) return;

    // lock and unlock for a barrier
    pthread_mutex_lock (&shm->lock);
    shm->exiting = true;
    pthread_mutex_unlock (&shm->lock);

    // stop thread
    this->ioreset ();

    // delete shared memory
    shm_unlink (shmname);
    for (int i = 0; i < 8; i ++) {
        close (shm->drives[i].dtfd);
        shm->drives[i].dtfd = -1;
        shm->drives[i].fname[0] = 0;
    }
    munmap (this->shm, sizeof *this->shm);
    this->shm = NULL;
    close (this->shmfd);
    this->shmfd = -1;
}

// reset the device
// - kill thread
void IODevTC08::ioreset ()
{
    // nothing to do if shared memory isn't set up
    if (this->shm == NULL) return;

    pthread_mutex_lock (&shm->lock);

    // if another thread is doing the reset, wait for it to complete
    if (shm->resetting) {
        do pthread_cond_wait (&shm->cond, &shm->lock);
        while (shm->resetting);
    } else {

        // no other thread doing reset, tell others we are doing reset
        shm->resetting = true;
        pthread_cond_broadcast (&shm->cond);

        // if our thread is running, tell it to stop and wait for it to stop
        if (shm->threadid != 0) {
            pthread_mutex_unlock (&shm->lock);
            pthread_join (shm->threadid, NULL);
            pthread_mutex_lock (&shm->lock);
            shm->threadid = 0;
        }

        // clear status to reset state
        shm->status_a = 0;
        shm->status_b = 0;
        clrintreqmask (IRQ_DTAPE);

        shm->resetting = false;
        pthread_cond_broadcast (&shm->cond);
    }

    pthread_mutex_unlock (&shm->lock);
}

// load/unload files
SCRet *IODevTC08::scriptcmd (int argc, char const *const *argv)
{
    // create shared memory if not already
    if (this->shm == NULL) {

        // create shared memory for tc08status.cc program to access tape state
        this->shmfd = shm_open (shmname, O_RDWR | O_CREAT, 0600);
        if (this->shmfd < 0) {
            fprintf (stderr, "IODevTC08::IODevTC08: error creating shared memory %s: %m\n", shmname);
            ABORT ();
        }
        if (ftruncate (this->shmfd, sizeof *this->shm) < 0) {
            fprintf (stderr, "IODevTC08::IODevTC08: error setting shared memory %s size: %m\n", shmname);
            close (this->shmfd);
            this->shmfd = -1;
            shm_unlink (shmname);
            ABORT ();
        }

        // map it to va space and zero it out
        void *shmptr = mmap (NULL, sizeof *this->shm, PROT_READ | PROT_WRITE, MAP_SHARED, this->shmfd, 0);
        if (shmptr == MAP_FAILED) {
            fprintf (stderr, "IODevTC08::IODevTC08: error accessing shared memory %s: %m\n", shmname);
            close (this->shmfd);
            this->shmfd = -1;
            shm_unlink (shmname);
            ABORT ();
        }

        IODevTC08Shm *lclshm = (IODevTC08Shm *) shmptr;
        memset (lclshm, 0, sizeof *lclshm);

        // initialize everything
        pthread_condattr_t condattrs;
        if (pthread_condattr_init (&condattrs) != 0) ABORT ();
        if (pthread_condattr_setpshared (&condattrs, PTHREAD_PROCESS_SHARED) != 0) ABORT ();
        if (pthread_cond_init (&lclshm->cond, &condattrs) != 0) ABORT ();

        pthread_mutexattr_t lockattrs;
        if (pthread_mutexattr_init (&lockattrs) != 0) ABORT ();
        if (pthread_mutexattr_setpshared (&lockattrs, PTHREAD_PROCESS_SHARED) != 0) ABORT ();
        if (pthread_mutex_init (&lclshm->lock, &lockattrs) != 0) ABORT ();

        lclshm->dtpid = getpid ();
        for (int i = 0; i < 8; i ++) {
            lclshm->drives[i].dtfd = -1;
            lclshm->drives[i].unldat = 0xFFFFFFFFFFFFFFFFULL;
        }

        // lock and unlock for a barrier
        pthread_mutex_lock (&lclshm->lock);
        lclshm->initted = true;
        pthread_mutex_unlock (&lclshm->lock);
        this->shm = lclshm;
    }

    // debug <level>
    if (strcmp (argv[0], "debug") == 0) {
        if (argc == 1) {
            return new SCRetInt (shm->debug);
        }

        if (argc == 2) {
            shm->debug = atoi (argv[1]);
            return NULL;
        }

        return new SCRetErr ("iodev %s debug <level>", iodevname);
    }

    if (strcmp (argv[0], "help") == 0) {
        puts ("");
        puts ("valid sub-commands:");
        puts ("");
        puts ("  debug                           - get debug level");
        puts ("  debug <level>                   - set debug level");
        puts ("  loaded <drivenumber>            - see what file is loaded on drive");
        puts ("                                    first char -: read-only; +: read/write");
        puts ("  loadro <drivenumber> <filename> - load file write-locked");
        puts ("  loadrw <drivenumber> <filename> - load file write-enabled");
        puts ("  unload <drivenumber>            - unload file - drive reports offline");
        puts ("");
        return NULL;
    }

    // return name of file loaded on drive (null if none)
    if (strcmp (argv[0], "loaded") == 0) {
        if (argc == 2) {
            char *p;
            int driveno = strtol (argv[1], &p, 0);
            if ((*p != 0) || (driveno < 0) || (driveno > 7)) return new SCRetErr ("drivenumber %s not in range 0..7", argv[1]);
            IODevTC08Drive *drive = &shm->drives[driveno];
            if (drive->dtfd < 0) return NULL;
            return new SCRetStr ("%c%s", drive->rdonly ? '-' : '+', drive->fname);
        }

        return new SCRetErr ("iodev tc08 loaded <drivenumber>");
    }

    // loadro/loadrw <drivenumber> <filename>
    bool loadro = (strcmp (argv[0], "loadro") == 0);
    bool loadrw = (strcmp (argv[0], "loadrw") == 0);
    if (loadro | loadrw) {
        if (argc == 3) {
            char *p;
            int driveno = strtol (argv[1], &p, 0);
            if ((*p != 0) || (driveno < 0) || (driveno > 7)) return new SCRetErr ("drivenumber %s not in range 0..7", argv[1]);
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
            if (loadrw && (ftruncate (fd, BYTESPERBLOCK * BLOCKSPERTAPE) < 0)) {
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
            fprintf (stderr, "IODevTC08::scriptcmd: drive %d loaded with read%s file %s\n", driveno, (loadro ? "-only" : "/write"), argv[2]);
            pthread_mutex_lock (&shm->lock);
            IODevTC08Drive *drive = &shm->drives[driveno];
            close (drive->dtfd);
            drive->dtfd    = fd;
            drive->rdonly  = loadro;
            drive->tapepos = 0;
            drive->unldat  = 0xFFFFFFFFFFFFFFFFULL;
            strncpy (drive->fname, argv[2], sizeof drive->fname);
            drive->fname[sizeof drive->fname-1] = 0;
            pthread_mutex_unlock (&shm->lock);
            return NULL;
        }

        return new SCRetErr ("iodev tc08 loadro/loadrw <drivenumber> <filename>");
    }

    // unload <drivenumber>
    if (strcmp (argv[0], "unload") == 0) {
        if (argc == 2) {
            char *p;
            int driveno = strtol (argv[1], &p, 0);
            if ((*p != 0) || (driveno < 0) || (driveno > 7)) return new SCRetErr ("drivenumber %s not in range 0..7", argv[1]);
            pthread_mutex_lock (&shm->lock);
            this->unloadrive (driveno);
            pthread_mutex_unlock (&shm->lock);
            return NULL;
        }

        return new SCRetErr ("iodev tc08 unload <drivenumber>");
    }

    return new SCRetErr ("unknown tape command %s - valid: help loaded loadro loadrw unload", argv[0]);
}

// perform i/o instruction
uint16_t IODevTC08::ioinstr (uint16_t opcode, uint16_t input)
{
    return (this->shm == NULL) ? ioinstroffline (opcode, input) : ioinstronline (opcode, input);
}

// - no shared memory, report drive offline status
uint16_t IODevTC08::ioinstroffline (uint16_t opcode, uint16_t input)
{
    switch (opcode) {

        // DTRA (TC08) read status register A
        // p 396
        case 06761: {
            break;
        }

        // DTCA (TC08) clear status register A
        case 06762: {
            break;
        }

        // DTLA (TC08) clear and load status register A
        case 06766: {
            // fall through
        }

        // DTXA (TC08) load status register A
        case 06764: {
            input &= IO_LINK;
            break;
        }

        // DTSF (TC08) skip on flag
        case 06771: {
            input |= IO_SKIP;
            break;
        }

        // DTRB (TC08) read status register B
        case 06772: {
            input |= SELERR;
            break;
        }

        // DTSF (TC08) skip on flag
        // DTRB (TC08) read status register B
        case 06773: {
            input |= IO_SKIP | SELERR;
            break;
        }

        // DTXB (TC08) load status register B <05:03>
        case 06774: {
            input &= IO_LINK;
            break;
        }

        default: {
            input = UNSUPIO;
            break;
        }
    }
    return input;
}

// - have shared memory, process i/o instruction
uint16_t IODevTC08::ioinstronline (uint16_t opcode, uint16_t input)
{
    pthread_mutex_lock (&shm->lock);

    uint16_t oldstata = shm->status_a;

    DBGPR (2, "IODevTC08::ioinstr: %05o %s\n", lastreadaddr, iodisas (opcode));

    switch (opcode) {

        // DTRA (TC08) read status register A
        // p 396
        case 06761: {
            DBGPR (2, "IODevTC08::ioinstr: read status_a %04o\n", shm->status_a);
            input |= shm->status_a;
            break;
        }

        // DTCA (TC08) clear status register A
        case 06762: {
            DBGPR (2, "IODevTC08::ioinstr: clear status_a\n");
            shm->status_a = 0;
            updateirq ();
            break;
        }

        // DTLA (TC08) clear and load status register A
        case 06766: {
            DBGPR (2, "IODevTC08::ioinstr: clear status_a\n");
            shm->status_a = 0;
            // fall through
        }

        // DTXA (TC08) load status register A
        case 06764: {
            shm->status_a ^= input & 07774;
            DBGPR (2, "IODevTC08::ioinstr: xor status_a with %04o giving %04o\n", input & 07774, shm->status_a);
            if (! (input & 00001)) {
                DBGPR (2, "IODevTC08::ioinstr: clear dectape flag\n");
                shm->status_b &= ~ DTFLAG;  // clear dectape flag
            }
            if (! (input & 00002)) {
                DBGPR (2, "IODevTC08::ioinstr: clear error flags\n");
                shm->status_b &= ~ ERRORS;  // clear all error flags
            }
            updateirq ();

            int driveno = (shm->status_a >> 9) & 7;
            IODevTC08Drive *drive = &shm->drives[driveno];
            drive->unldat = 0xFFFFFFFFFFFFFFFFULL;
            DBGPR (1, "IODevTC08::ioinstr: st_A=%04o  st_B=%04o  startio  %o %s %s %s %s  tpos=%04o%c\n",
                    shm->status_a, shm->status_b, (shm->status_a >> 9) & 7, CONTIN ? "CON" : "NOR",
                    (shm->status_a & REVBIT) ? "REV" : "FWD", (shm->status_a & GOBIT) ? "GO" : "ST",
                    cmdmnes[(shm->status_a&070)>>3], drive->tapepos / 4, (drive->tapepos & 3) + 'a');

            if ((shm->status_a ^ oldstata) & (GOBIT | REVBIT)) {
                shm->startdelay = true;                     // additional delay if direction changed or just started or stopped
            }
            if (shm->status_a & GOBIT) {
                shm->iopend = true;                         // there is an i/o command to process
                shm->dmapc  = dyndispc;
                shm->cycles = shadow.getcycles ();
                pthread_cond_broadcast (&shm->cond);
                if (shm->threadid == 0) {
                    int rc = createthread (&shm->threadid, threadwrap, this);
                    if (rc != 0) ABORT ();
                }
            }

            input &= IO_LINK;                               // clear accumulator
            break;
        }

        // DTSF (TC08) skip on flag
        case 06771: {
            DBGPR (2, "IODevTC08::ioinstr: skip on flag %04o = %s\n", shm->status_b, ((shm->status_b & INTREQ) ? "TRUE" : "FALSE"));
            if (shm->status_b & INTREQ) input |= IO_SKIP;
            else if (this->allowskipopt) skipoptwait (opcode, &shm->lock, &this->dskpwait);
            break;
        }

        // DTRB (TC08) read status register B
        case 06772: {
            DBGPR (2, "IODevTC08::ioinstr: read status_b %04o\n", shm->status_b);
            input |= shm->status_b;
            break;
        }

        // DTSF (TC08) skip on flag
        // DTRB (TC08) read status register B
        case 06773: {
            DBGPR (2, "IODevTC08::ioinstr: read status_b %04o\n", shm->status_b);
            DBGPR (2, "IODevTC08::ioinstr: skip on flag %04o = %s\n", shm->status_b, ((shm->status_b & INTREQ) ? "TRUE" : "FALSE"));
            input |= shm->status_b;
            if (shm->status_b & INTREQ) input |= IO_SKIP;
            else if (this->allowskipopt) skipoptwait (opcode, &shm->lock, &this->dskpwait);
            break;
        }

        // DTXB (TC08) load status register B <05:03>
        case 06774: {
            shm->status_b = (shm->status_b & 07707) | (input & 00070);
            DBGPR (2, "IODevTC08::ioinstr: load status_b %04o\n", shm->status_b);
            input &= IO_LINK;
            break;
        }

        default: {
            input = UNSUPIO;
            break;
        }
    }
    pthread_mutex_unlock (&shm->lock);
    return input;
}

// thread what does the tape file I/O
void *IODevTC08::threadwrap (void *zhis)
{
    ((IODevTC08 *)zhis)->thread ();
    return NULL;
}

void IODevTC08::thread ()
{
    pthread_mutex_lock (&shm->lock);
    while (! shm->resetting) {

        // waiting for another I/O request
        if (! shm->iopend) {

            // unload drives that were left running for UNLOADNS
            uint64_t nowns = getnowns ();
            uint64_t earliestunload = 0xFFFFFFFFFFFFFFFFULL;
            for (int i = 0; i < 8; i ++) {
                IODevTC08Drive *drive = &shm->drives[i];
                if (nowns > drive->unldat) this->unloadrive (i);
                if (earliestunload > drive->unldat) earliestunload = drive->unldat;
            }

            // wait for another I/O or a drive to unload
            if (earliestunload < 0xFFFFFFFFFFFFFFFFULL) {
                struct timespec unldts;
                unldts.tv_sec  = earliestunload / 1000000000;
                unldts.tv_nsec = earliestunload % 1000000000;
                int rc = pthread_cond_timedwait (&shm->cond, &shm->lock, &unldts);
                if ((rc != 0) && (rc != ETIMEDOUT)) ABORT ();
            } else {
                int rc = pthread_cond_wait (&shm->cond, &shm->lock);
                if (rc != 0) ABORT ();
            }
        } else {
            shm->iopend = false;

            uint16_t oldidwc = memarray[IDWC];
            uint16_t field = (shm->status_b & 070) << 9;
            uint16_t *memfield = &memarray[field];

            int driveno = (shm->status_a >> 9) & 007;
            IODevTC08Drive *drive = &shm->drives[driveno];
            drive->unldat = 0xFFFFFFFFFFFFFFFFULL;

            // select error if no tape loaded
            if (drive->dtfd < 0) {
                shm->status_b |= SELERR;
                DBGPR (2, "IODevTC08::thread: no file for drive %d\n", driveno);
                goto finerror;
            }

            // if tape stopped, we can't do anything
            if (! (shm->status_a & GOBIT)) continue;

            // blocks are conceptually stored:
            //  fwdmark          revmark
            //    <n>   <data-n>   <n>       - where 00000<=n<=02701
            //  ^     ^          ^     ^     - the tape head is at any one of these positions
            //  a     b          c     d     - letter shown on tc08status.cc output
            //  0+4n  1+4n       2+4n  3+4n  - value stored in tapepos
            // tapepos = 0 : at very beginning of tape
            // tapepos = BLOCKSPERTAPE*4-1 : at very end of tape
            // the file just contains the data, as 129 16-bit words, rjzf * 1474 blocks
            // seek forward always stops at 'b' or end-of-tape
            // seek reverse always stops at 'c' or beg-of-tape
            // read/write forward always stops at 'c' or end-of-tape
            // read/write reverse always stops at 'b' or beg-of-tape
            ASSERT (drive->tapepos < BLOCKSPERTAPE*4);

            switch ((shm->status_a & 070) >> 3) {

                // MOVE (2.5.1.4 p 27)
                case 0: {

                    // step the first one with startup delay if needed
                    if (this->delayblk ()) goto finished;
                    if (this->stepskip (drive)) goto endtape;

                    // spend at most 5 seconds to rewind tape
                    uint16_t blknum   = drive->tapepos / 4;
                    uint16_t blkstogo = REVERS ? blknum : BLOCKSPERTAPE - blknum;
                    uint32_t usperblk = (blkstogo < 200) ? 25000 : 5000000 / blkstogo;

                    // skip the rest of the way
                    while (true) {
                        if (this->delayloop (usperblk)) goto finished;
                        if (this->stepskip (drive)) goto endtape;
                    }
                }

                // SEARCH (2.5.1.4 p 27)
                case 1: {
                    uint16_t idwc, idca;
                    do {
                        if (this->delayblk ()) goto finished;
                        if (this->stepskip (drive)) goto endtape;

                        idwc = memarray[IDWC];
                        idca = memarray[IDCA];
                        ASSERT (idwc <= 07777);
                        ASSERT (idca <= 07777);
                        dyndisdma (IDWC, 1,  true, shm->dmapc);
                        dyndisdma (IDCA, 1, false, shm->dmapc);
                        dyndisdma (field + idca, 1, true, shm->dmapc);
                        DBGPR (2, "IODevTC08::thread: skip idwc=%04o idca=%o.%04o\n", idwc, field >> 12, idca);

                        idwc = (idwc + 1) & 07777;                              // update word count before writing out block number
                        memarray[IDWC] = idwc;                                  // ...os8 driver has idca==IDWC
                        memfield[idca] = drive->tapepos / 4;                    // write out mark we just hopped over
                        DBGPR (2, "IODevTC08::thread: search memarray[IDWC] = %04o ; memfield[%04o] <= %04o\n", idwc, idca, drive->tapepos / 4);
                    } while (CONTIN && (idwc != 0));
                    goto success;
                }

                // READ DATA (2.5.1.5 p 27)
                case 2: {
                    uint16_t idca, idwc;
                    do {
                        if (this->delayblk ()) goto finished;
                        if (this->stepxfer (drive)) goto endtape;

                        uint16_t buff[WORDSPERBLOCK];
                        int rc = pread (drive->dtfd, buff, BYTESPERBLOCK, drive->tapepos / 4 * BYTESPERBLOCK);
                        if (rc < 0) {
                            fprintf (stderr, "IODevTC08::thread: error reading tape %d file: %m\n", driveno);
                            ABORT ();
                        }
                        if (rc != BYTESPERBLOCK) {
                            fprintf (stderr, "IODevTC08::thread: only read %d of %d bytes from tape %d file\n", rc, BYTESPERBLOCK, driveno);
                            ABORT ();
                        }
                        if (shm->debug >= 3) this->dumpbuf (drive, "read", buff, WORDSPERBLOCK);

                        idca = memarray[IDCA];
                        idwc = memarray[IDWC];
                        ASSERT (idca <= 07777);
                        ASSERT (idwc <= 07777);
                        dyndisdma (IDWC, 1, true, shm->dmapc);
                        dyndisdma (IDCA, 1, true, shm->dmapc);
                        dyndisdma (field + idca, MIN (WORDSPERBLOCK, 010000 - idwc), true, shm->dmapc);
                        DBGPR (2, "IODevTC08::thread: read idwc=%04o idca=%o.%04o block=%04o\n", idwc, field >> 12, idca, drive->tapepos / 4);
                        if (REVERS) {
                            for (int i = WORDSPERBLOCK; -- i >= 0;) {
                                idca = (idca + 1) & 07777;
                                memfield[idca] = ocarray[buff[i]&07777];
                                idwc = (idwc + 1) & 07777;
                                if (idwc == 0) break;
                            }
                            memarray[IDCA] = idca;
                            memarray[IDWC] = idwc;
                        } else {
                            // madness: TC08 OS/8 boot block changes from data field 0 to data field 1 mid-read
                            // so pass data s-l-o-w-l-y when booting
                            if (dmareadoverwritesinstructions (field, idca, idwc)) {
                                ASSERT (this->allowskipopt);                    // this is only thing using allowskipopt
                                this->allowskipopt = false;                     // don't allow to enter skipopt wait
                                while (this->dskpwait) {                        // break out of skipopt wait if already in one
                                    setintreqmask (0);                          // wake it up
                                    pthread_mutex_unlock (&shm->lock);          // give it time to wake up
                                    usleep (1000);
                                    pthread_mutex_lock (&shm->lock);
                                }
                                for (int i = 0; i < WORDSPERBLOCK; i ++) {
                                    shm->cycles = shadow.getcycles ();          // wait for processor to run 100 cycles
                                    this->delayloop (1000);                     // ... at least 20 instructions
                                    memarray[IDWC] = idwc = (memarray[IDWC] + 1) & 07777;
                                    memarray[IDCA] = idca = (memarray[IDCA] + 1) & 07777;
                                    field = (shm->status_b & 070) << 9;
                                    memarray[field|idca] = buff[i] & 07777;     // transfer a word
                                    if (idwc == 0) break;
                                }
                                this->allowskipopt = true;
                            } else {
                                for (int i = 0; i < WORDSPERBLOCK; i ++) {
                                    idwc = (idwc + 1) & 07777;
                                    idca = (idca + 1) & 07777;
                                    memfield[idca] = buff[i] & 07777;
                                    if (idwc == 0) break;
                                }
                                memarray[IDCA] = idca;
                                memarray[IDWC] = idwc;
                            }
                        }
                    } while (CONTIN && (idwc != 0));
                    goto success;
                }

                // READ ALL
                // - runs MAINDEC D3RA test (forward and reverse)
                //   let it run at least 2 passes over tape to get some read-all-reverses
                case 3: {
                    uint16_t idca, idwc;
                    do {
                        if (this->delayblk ()) goto finished;
                        if (this->stepxfer (drive)) goto endtape;

                        // read 129 data words from file
                        // leave room for header words
                        uint16_t buff[5+WORDSPERBLOCK+5];
                        uint16_t blknum = drive->tapepos / 4;
                        uint16_t *databuff = &buff[5];
                        int rc = pread (drive->dtfd, databuff, BYTESPERBLOCK, blknum * BYTESPERBLOCK);
                        if (rc < 0) {
                            fprintf (stderr, "IODevTC08::thread: error reading tape %d file: %m\n", driveno);
                            ABORT ();
                        }
                        if (rc != BYTESPERBLOCK) {
                            fprintf (stderr, "IODevTC08::thread: only read %d of %d bytes from tape %d file\n", rc, BYTESPERBLOCK, driveno);
                            ABORT ();
                        }

                        // make up header words tacked on beginning and end of data words
                        ASSERT (blknum <= 07777);
                        if (REVERS) {
                            buff[WORDSPERBLOCK+9] = 0;
                            buff[WORDSPERBLOCK+8] = ocarray[blknum];
                            buff[WORDSPERBLOCK+7] = 0;
                            buff[WORDSPERBLOCK+6] = 0;
                            buff[WORDSPERBLOCK+5] = 07700;
                            uint16_t parity = 0;
                            for (int i = 0; i < WORDSPERBLOCK; i ++) parity ^= databuff[i] ^ (databuff[i] >> 6);
                            buff[4] = parity & 00077;
                            buff[3] = 0;
                            buff[2] = 0;
                            buff[1] = blknum;
                            buff[0] = 0;
                        } else {
                            buff[0] = 0;
                            buff[1] = blknum;
                            buff[2] = 0;
                            buff[3] = 0;
                            buff[4] = 00077;
                            uint16_t parity = 0;
                            for (int i = 0; i < WORDSPERBLOCK; i ++) parity ^= databuff[i] ^ (databuff[i] << 6);
                            buff[WORDSPERBLOCK+5] = parity & 07700;
                            buff[WORDSPERBLOCK+6] = 0;
                            buff[WORDSPERBLOCK+7] = 0;
                            buff[WORDSPERBLOCK+8] = ocarray[blknum];
                            buff[WORDSPERBLOCK+9] = 0;
                        }

                        if (shm->debug >= 3) this->dumpbuf (drive, "rall", buff, 5 + WORDSPERBLOCK + 1);

                        // copy out to dma buffer
                        idca = memarray[IDCA];
                        idwc = memarray[IDWC];
                        ASSERT (idca <= 07777);
                        ASSERT (idwc <= 07777);
                        dyndisdma (IDWC, 1, true, shm->dmapc);
                        dyndisdma (IDCA, 1, true, shm->dmapc);
                        dyndisdma (field + idca, MIN (5 + WORDSPERBLOCK + 1, 010000 - idwc), true, shm->dmapc);
                        DBGPR (2, "IODevTC08::thread: rall idwc=%04o (%4u) idca=%o.%04o block=%04o\n", idwc, (010000 - idwc), field >> 12, idca, blknum);
                        if (REVERS) {
                            for (int i = 5 + WORDSPERBLOCK + 5; -- i >= 0;) {
                                idca = (idca + 1) & 07777;
                                memfield[idca] = ocarray[buff[i]&07777];
                                idwc = (idwc + 1) & 07777;
                                if (idwc == 0) break;
                            }
                        } else {
                            for (int i = 0; i < 5 + WORDSPERBLOCK + 5; i ++) {
                                idwc = (idwc + 1) & 07777;
                                idca = (idca + 1) & 07777;
                                memfield[idca] = buff[i] & 07777;
                                if (idwc == 0) break;
                            }
                        }
                        memarray[IDCA] = idca;
                        memarray[IDWC] = idwc;
                    } while (CONTIN && (idwc != 0));
                    goto success;
                }

                // WRITE DATA (2.5.1.7 p 28)
                case 4: {
                    if (shm->drives[driveno].rdonly) {
                        DBGPR (2, "IODevTC08::thread: write attempt on read-only drive %d\n", driveno);
                        shm->status_b |= SELERR;
                        goto finerror;
                    }

                    uint16_t idca, idwc;
                    do {
                        if (this->delayblk ()) goto finished;
                        if (this->stepxfer (drive)) goto endtape;

                        uint16_t buff[WORDSPERBLOCK];
                        idca = memarray[IDCA];
                        idwc = memarray[IDWC];
                        ASSERT (idca <= 07777);
                        ASSERT (idwc <= 07777);
                        dyndisdma (IDWC, 1, true, shm->dmapc);
                        dyndisdma (IDCA, 1, true, shm->dmapc);
                        dyndisdma (field + idca, MIN (WORDSPERBLOCK, 010000 - idwc), false, shm->dmapc);
                        DBGPR (2, "IODevTC08::thread: write idwc=%04o idca=%o.%04o block=%04o\n", idwc, field >> 12, idca, drive->tapepos / 4);
                        if (REVERS) {
                            for (int i = WORDSPERBLOCK; i > 0;) {
                                idca = (idca + 1) & 07777;
                                buff[--i] = ocarray[memfield[idca]&07777];
                                idwc = (idwc + 1) & 07777;
                                if (idwc == 0) {
                                    while (-- i >= 0) buff[i] = 07777;
                                    break;
                                }
                            }
                        } else {
                            for (int i = 0; i < WORDSPERBLOCK;) {
                                idca = (idca + 1) & 07777;
                                buff[i++] = memfield[idca] & 07777;
                                idwc = (idwc + 1) & 07777;
                                if (idwc == 0) {
                                    memset (&buff[i], 0, (WORDSPERBLOCK - i) * 2);
                                    break;
                                }
                            }
                        }
                        memarray[IDCA] = idca;
                        memarray[IDWC] = idwc;

                        if (shm->debug >= 3) this->dumpbuf (drive, "write", buff, WORDSPERBLOCK);
                        int rc = pwrite (drive->dtfd, buff, BYTESPERBLOCK, drive->tapepos / 4 * BYTESPERBLOCK);
                        if (rc < 0) {
                            fprintf (stderr, "IODevTC08::thread: error writing tape %d file: %m\n", driveno);
                            ABORT ();
                        }
                        if (rc != BYTESPERBLOCK) {
                            fprintf (stderr, "IODevTC08::thread: only wrote %d of %d bytes to tape %d file\n", rc, BYTESPERBLOCK, driveno);
                            ABORT ();
                        }
                    } while (CONTIN && (idwc != 0));
                    goto success;
                }

                default: {
                    fprintf (stderr, "IODevTC08::thread: unsupported command %u\n", ((shm->status_a & 00070) >> 3));
                    shm->status_b |= SELERR;
                    goto finerror;
                }
            }
            ABORT ();
        endtape:;
            DBGPR (2, "IODevTC08::thread: - end of tape\n");
            shm->status_b |=  ENDTAP;   // set up end-of-tape status
        finerror:;
            shm->status_a &= ~ GOBIT;   // any error shuts the GO bit off
            goto finished;
        success:;
            DBGPR (2, "IODevTC08::thread: - final idwc=%04o idca=%o.%04o\n", memarray[IDWC], field >> 12, memarray[IDCA]);
            shm->status_b |=  DTFLAG;   // errors do not get DTFLAG
            // if normal mode with non-zero remaining IDWC,
            //  real dectapes will start the next transfer when the next bits come under the tape head in a few milliseconds
            //    failing with timing error if DTFLAG has not been cleared by the processor by then
            //  in our case, we process the next block when the processor clears DTFLAG because the tubes may take a while to clear DTFLAG
        finished:;
            DBGPR (1, "IODevTC08::thread: st_B=%04o idwc=%04o->%04o\n", shm->status_b, oldidwc, memarray[IDWC]);

            // maybe request an interrupt
            this->updateirq ();

            // if drive is running and we don't get another command in UNLOADNS, unload it
            if (shm->status_a & GOBIT) {
                drive->unldat = getnowns () + UNLOADNS;
            } else {
                drive->unldat = 0xFFFFFFFFFFFFFFFFULL;
            }
        }
    }

    // ioreset(), unlock and exit thread
    pthread_mutex_unlock (&shm->lock);
}

// step tape just before data in the current tape direction
//  returns false: tapepos updated as described
//           true: hit end-of-tape
//                 tapepos set to whichever end we're at
bool IODevTC08::stepskip (IODevTC08Drive *drive)
{
    if (REVERS) {
        if (drive->tapepos <= 2) {
            drive->tapepos = 0;
            return true;
        }
        switch (drive->tapepos & 3) {
            case 0: drive->tapepos -= 2; break;
            case 1: drive->tapepos -= 3; break;
            case 2: drive->tapepos -= 4; break;
            case 3: drive->tapepos --;   break;
        }
    } else {
        switch (drive->tapepos & 3) {
            case 0: drive->tapepos ++;   break;
            case 1: drive->tapepos += 4; break;
            case 2: drive->tapepos += 3; break;
            case 3: drive->tapepos += 2; break;
        }
        if (drive->tapepos >= BLOCKSPERTAPE*4) {
            drive->tapepos = BLOCKSPERTAPE*4 - 1;
            return true;
        }
    }
    return false;
}

// step tape just after data in the current tape direction
//  returns false: tapepos updated as described
//           true: hit end-of-tape
//                 tapepos set to whichever end we're at
bool IODevTC08::stepxfer (IODevTC08Drive *drive)
{
    if (REVERS) {
        if (drive->tapepos <= 1) {
            drive->tapepos = 0;
            return true;
        }
        switch (drive->tapepos & 3) {
            case 0: drive->tapepos -= 3; break;
            case 1: drive->tapepos -= 4; break;
            case 2: drive->tapepos --;   break;
            case 3: drive->tapepos -= 2; break;
        }
    } else {
        switch (drive->tapepos & 3) {
            case 0: drive->tapepos += 2; break;
            case 1: drive->tapepos ++;   break;
            case 2: drive->tapepos += 4; break;
            case 3: drive->tapepos += 3; break;
        }
        if (drive->tapepos >= BLOCKSPERTAPE*4) {
            drive->tapepos = BLOCKSPERTAPE*4 - 1;
            return true;
        }
    }
    return false;
}

// dump block buffer
void IODevTC08::dumpbuf (IODevTC08Drive *drive, char const *label, uint16_t const *buff, int nwords)
{
    printf ("IODevTC08::dumpbuf: %o  %5s %s  block %04o\n", (int) (drive - shm->drives), label, (REVERS ? "REV" : "FWD"), drive->tapepos / 4);
    for (int i = 0; i < nwords; i += 16) {
        printf ("  %04o:", i);
        for (int j = 0; (j < 16) && (i + j < nwords); j ++) {
            printf (" %04o", buff[i+j]);
        }
        printf ("\n");
    }
}

// see if a dma read will overwrite instructions currently executing
//  input:
//   field = dma transfer field in <14:12>, rest is zeroes
//   idca = start of transfer - 1
//   idwc = 2's comp transfer word count
static bool dmareadoverwritesinstructions (uint16_t field, uint16_t idca, uint16_t idwc)
{
    // frame must match exactly
    uint16_t curif = memext.iframe;
    if (curif != field) return false;

    // current PC must be within transfer area
    uint16_t endaddr = (idca - idwc) & 07777;
    idca = (idca + 1) & 07777;
    uint16_t curpc = shadow.r.pc;
    if (endaddr >= idca) {
        if ((curpc < idca) || (curpc > endaddr)) return false;
    } else {
        if ((curpc < idca) && (curpc > endaddr)) return false;
    }
    return true;
}

// delay processing for a block
//  returns:
//   false = it's ok to keep going as is
//    true = something changed, re-decode
bool IODevTC08::delayblk ()
{
    // 25ms standard per-block delay
    int usec = 25000;

    // 375ms additional for startup delay
    if (shm->startdelay) {
        shm->startdelay = false;
        usec += 375000;
    }

    return this->delayloop (usec);
}

// delay the given number of microseconds, unlocking during the wait
// check for changed command during the wait
// also make sure processor has executed at least 100 cycles since i/o instruction
// ...so processor has time to set up dma descriptor words after starting the i/o
//  output:
//   returns true: command changed during wait
//          false: command remained the same
bool IODevTC08::delayloop (int usec)
{
    // detect if status_a register changed while unlocked
    uint16_t savesta = shm->status_a;

    while (true) {

        // unlock while waiting (so we don't block processor doing i/o instructions)
        pthread_mutex_unlock (&shm->lock);
        usleep (usec);
        pthread_mutex_lock (&shm->lock);

        // tell caller to abort if resetting or status_a written
        if (shm->iopend || (shm-> status_a != savesta) || shm->resetting) return true;

        // the TC08 diagnostics do the I/O instruction to start the I/O THEN write IDCA and IDWC,
        // so make sure CPU has exeucted at least 100 cycles (20..30 instructions) or is doing an HLT
        if (waitingforinterrupt) return false;                  // HLT is ok (processor won't cycle), assume IDCA,IDWC set up
        uint64_t ncycles = shadow.getcycles () - shm->cycles;   // see how many cycles since I/O started
        if (ncycles >= 100) return false;                       // if 100 cycles, assume IDCA,IDWC set up

        // not yet, wait another millisecond
        usec = 1000;
    }
}

// update interrupt request
// interrupt if enabled and command has completed
void IODevTC08::updateirq ()
{
    if ((shm->status_a & INTENA) && (shm->status_b & INTREQ)) {
        setintreqmask (IRQ_DTAPE);
    } else {
        clrintreqmask (IRQ_DTAPE, this->dskpwait && (shm->status_b & INTREQ));
    }
}

// unload the drive
void IODevTC08::unloadrive (int driveno)
{
    IODevTC08Drive *drive = &shm->drives[driveno];
    close (drive->dtfd);
    drive->dtfd = -1;
    drive->unldat = 0xFFFFFFFFFFFFFFFFULL;
    fprintf (stderr, "IODevTC08::unloadrive: drive %d unloaded\n", driveno);
}

// print message if debug is at or above the given level
void IODevTC08::dbgpr (int level, char const *fmt, ...)
{
    if (shm->debug >= level) {
        va_list ap;
        va_start (ap, fmt);
        vfprintf (stderr, fmt, ap);
        va_end (ap);
    }
}

// get time that pthread_cond_timedwait() is based on
static uint64_t getnowns ()
{
    struct timespec nowts;
    if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) ABORT ();
    return ((uint64_t) nowts.tv_sec) * 1000000000 + nowts.tv_nsec;
}

// contents of a freshly zeroed dectape in OS/8
static uint16_t const formatofs = 0001000;
static uint16_t const formatbuf[] = {
    000000, 000000, 007777, 000007, 000000, 000000, 007777, 000000,
    006446, 000000, 003032, 001777, 003024, 005776, 001375, 003267,
    006201, 001744, 003667, 007040, 001344, 003344, 007040, 001267,
    002020, 005215, 007200, 006211, 005607, 000201, 000440, 002331,
    002324, 000515, 004010, 000501, 000400, 001774, 001373, 003774,
    001774, 000372, 001371, 003770, 001263, 003767, 003032, 001777,
    003024, 006202, 004424, 000200, 003400, 000007, 005766, 004765,
    005776, 000000, 001032, 007650, 005764, 005667, 001725, 002420,
    002524, 004005, 002222, 001722, 000000, 003344, 001763, 003267,
    003204, 007346, 003207, 001667, 007112, 007012, 007012, 000362,
    007440, 004761, 002204, 005340, 002267, 002207, 005311, 001344,
    007640, 005760, 001667, 007650, 005760, 001357, 004756, 002344,
    007240, 005310, 007240, 003204, 001667, 005315, 000000, 003020,
    001411, 003410, 002020, 005346, 005744, 000000, 000000, 000000,
    005172, 000256, 004620, 004627, 000077, 005533, 005622, 006443,
    003110, 007622, 007621, 006360, 000017, 007760, 007617, 006777,
    002600, 006023, 000000, 000000, 001777, 003024, 001776, 003025,
    003775, 003774, 003773, 001776, 003772, 005600, 000000, 007346,
    004771, 001770, 007041, 001027, 007500, 005233, 003012, 001770,
    007440, 004771, 003410, 002012, 005227, 005613, 003012, 001027,
    007440, 004771, 001011, 001012, 003011, 005613, 000000, 006201,
    001653, 006211, 001367, 007640, 005766, 005643, 003605, 000000,
    004765, 007700, 004764, 000005, 001025, 001363, 007650, 002032,
    005654, 000000, 003254, 001762, 007650, 005761, 001254, 007650,
    004760, 005666, 000000, 003200, 001357, 003030, 006211, 004765,
    007700, 005315, 006202, 004600, 000210, 001400, 000001, 005756,
    003007, 001755, 001354, 007650, 005326, 001200, 001363, 007640,
    005330, 001353, 003030, 005677, 003205, 002217, 004023, 003123,
    007700, 007700, 005342, 004764, 000002, 004764, 000000, 001352,
    003751, 005750, 000000, 002030, 002014, 003205, 000070, 007710,
    001401, 005626, 000007, 003700, 003023, 007600, 000171, 003522,
    002473, 006141, 000600, 007204, 006344, 002215, 002223, 002222,
    002352, 006065, 006023, 000000, 003034, 007240, 001200, 003224,
    001377, 003624, 001776, 000375, 007650, 005600, 001374, 004773,
    005600, 001723, 005770, 004020, 001120, 004026, 006164, 000100,
    003033, 003506, 001772, 007450, 007001, 000371, 007041, 005625,
    000000, 001770, 003246, 006202, 004646, 000011, 000000, 007667,
    006212, 005634, 000000, 007643, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 001367, 001354, 000275, 006443,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 003457, 000077, 007646, 004600,
    006615, 000007, 007644, 000020, 000000, 005402, 002036, 004777,
    000013, 006201, 001433, 006211, 001376, 007640, 005225, 001234,
    003775, 001235, 003243, 001236, 003242, 003637, 001244, 003640,
    001311, 003641, 004774, 007240, 003773, 004772, 001771, 003034,
    005600, 000174, 007747, 000005, 005422, 006033, 005636, 000017,
    007756, 005242, 000000, 003252, 007344, 004770, 006201, 000000,
    006211, 007414, 004767, 005645, 000000, 001304, 007440, 001020,
    007640, 005305, 001020, 007041, 003304, 001033, 003022, 001304,
    004766, 007650, 005311, 002022, 006201, 001422, 006211, 004765,
    005657, 000000, 004764, 006677, 004764, 006330, 007600, 004764,
    006652, 007200, 004764, 006667, 004764, 007200, 004311, 001414,
    000507, 000114, 004052, 004017, 002240, 007700, 004017, 002024,
    001117, 001640, 002516, 001316, 001727, 001600, 004302, 000104,
    004023, 002711, 002403, 001040, 001720, 002411, 001716, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 004200, 007324, 006305, 007400, 002670, 002220, 006433,
    002723, 006713, 002724, 002600, 000200, 000000, 004302, 000104,
    004005, 003024, 000516, 002311, 001716, 000000, 005410, 007240,
    003244, 003030, 003031, 004777, 001376, 007650, 005231, 003247,
    001375, 004774, 006211, 000023, 006201, 006773, 005610, 007201,
    001773, 001247, 007640, 005245, 001023, 003030, 002244, 005245,
    001024, 005214, 007777, 004772, 006750, 000000, 004771, 005647,
    000000, 003315, 001770, 007006, 007700, 005767, 001766, 007640,
    005767, 001765, 007650, 001770, 000364, 007650, 005274, 001763,
    000362, 003765, 001715, 007450, 004761, 003715, 002315, 001715,
    007640, 005652, 001770, 000364, 007740, 007240, 001360, 004774,
    006201, 006773, 006211, 000000, 005652, 006061, 007106, 007006,
    007006, 005717, 000000, 007450, 005724, 003332, 004732, 005724,
    000000, 001023, 000357, 007650, 005756, 004755, 004772, 007342,
    004017, 002024, 001117, 001640, 000115, 000211, 000725, 001725,
    002300, 000000, 000000, 004240, 007474, 000077, 007775, 006525,
    000017, 007617, 000200, 007600, 002723, 007111, 002537, 003426,
    004200, 006103, 002670, 007774, 007506, 006000, 000000, 000000,
    001777, 007012, 007630, 001376, 001375, 003234, 001634, 007640,
    005600, 004774, 000012, 000000, 000000, 000000, 005773, 001215,
    003634, 001234, 000376, 007650, 005600, 002234, 001372, 004771,
    006201, 006773, 006211, 007600, 005600, 000000, 003304, 004770,
    003306, 004307, 005767, 007240, 001347, 003305, 004307, 007410,
    005766, 006201, 001705, 002305, 000365, 007640, 005253, 001705,
    006211, 000364, 007440, 004763, 001306, 003032, 001032, 001362,
    007650, 004761, 005636, 001023, 007112, 007012, 007012, 001256,
    000365, 001256, 005262, 000000, 000000, 000000, 000000, 001360,
    003351, 007346, 003350, 006201, 001704, 006211, 002304, 007450,
    005707, 003347, 001751, 007450, 005345, 000365, 007640, 001365,
    001357, 006201, 000747, 006211, 007041, 001751, 007640, 005310,
    002347, 002351, 002350, 005323, 002307, 005707, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 007700, 000023,
    003416, 007506, 006400, 000377, 000077, 007333, 006352, 006000,
    002670, 007774, 006615, 000200, 007600, 000005, 002537, 000000,
    000013, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 004000, 000000, 000000, 002000, 000000, 007607,
    007621, 007607, 007621, 000000, 000000, 000000, 000000, 000000,
    000000, 007210, 007211, 000000, 000000, 000000, 000000, 006202,
    004207, 001000, 000000, 000007, 007746, 006203, 005677, 000400,
    003461, 003340, 006214, 001275, 003336, 006201, 001674, 007010,
    006211, 007630, 005321, 006202, 004207, 005010, 000000, 000027,
    007402, 006202, 004207, 000610, 000000, 000013, 007602, 005020,
    006202, 004207, 001010, 000000, 000027, 007402, 006213, 005700,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    000000, 000000, 000000, 000000, 000000, 000000, 000000, 000000,
    004230, 004230, 004230, 004230, 004230, 004230, 004230, 004230,
    000000, 001040, 004160, 004160, 001020, 002010, 000000, 000000,
    000000, 000000, 000070, 000002, 001472, 007777, 000102, 002314,
    000000, 002326, 003017, 007773, 000303, 001400, 000000, 000000,
    000000, 001441, 003055, 001573, 003574, 001572, 003442, 000000,
    007004, 007720, 001572, 001043, 007650, 005547, 005444, 000564,
    000772, 000000, 007665, 001010, 001565, 000000, 000000, 000000,
    007661, 007776, 000000, 000017, 000000, 000000, 000013, 000000,
    004000, 006202, 004575, 000210, 001000, 000015, 007402, 005467,
    001011, 002224, 000000, 002326, 003457, 007747, 002205, 002317,
    000400, 007741, 000070, 007642, 007400, 007643, 007760, 007644,
    006001, 000323, 007773, 007770, 000252, 000701, 000767, 000610,
    000760, 001403, 007666, 000371, 006006, 000651, 000752, 000007,
    000377, 001273, 000613, 000622, 000251, 004210, 001402, 000343,
    007774, 001404, 001400, 000250, 001401, 007757, 007646, 000771,
    000247, 000015, 000017, 000617, 000177, 000246, 007600, 000100,
    000321, 000544, 000240, 000020, 000563, 007761, 007740, 000620,
    000220, 007730, 007746, 007736, 007700, 000200, 007607, 007764,
    006203, 000000, 003461, 003055, 003040, 006214, 001177, 003345,
    001345, 003241, 004220, 002200, 007100, 001176, 007430, 005247,
    001244, 003220, 000467, 001200, 004343, 005620, 000247, 000401,
    000603, 001011, 001177, 000263, 000306, 000253, 000324, 000341,
    000400, 001343, 000620, 006213, 003600, 006213, 005640, 002057,
    002057, 002057, 002057, 002057, 002057, 007200, 006202, 004575,
    000210, 001400, 000056, 007402, 005657, 007330, 004351, 004220,
    003041, 006202, 004575, 000601, 000000, 000051, 005246, 001241,
    006203, 004574, 003241, 001241, 003345, 004351, 007120, 005637,
    004220, 003041, 006202, 004575, 000101, 007400, 000057, 005246,
    001041, 006203, 005713, 007200, 002200, 001040, 007110, 001200,
    003573, 001241, 003572, 007221, 006201, 000571, 006211, 007010,
    007730, 005572, 005570, 007120, 005325, 000223, 003240, 006213,
    001640, 006213, 005743, 000000, 001374, 003364, 006201, 001571,
    006211, 007012, 007630, 005751, 006202, 004575, 000000, 000000,
    000033, 005246, 005751, 006202, 004575, 000111, 001000, 000026,
    005246, 005774, 000000, 007201, 003054, 001055, 007440, 005246,
    004567, 002574, 007450, 005566, 003052, 004567, 007450, 005221,
    001052, 007004, 007130, 003052, 001165, 003017, 001164, 003042,
    001417, 007041, 001052, 007650, 005241, 002042, 005225, 001017,
    007700, 005566, 001163, 005222, 001042, 001162, 004561, 004567,
    002574, 004560, 007450, 001442, 007450, 005557, 007044, 007620,
    001156, 001156, 003301, 001041, 003047, 001441, 001054, 007640,
    005332, 004567, 007010, 007520, 005557, 007004, 000155, 003302,
    004334, 003303, 006202, 004575, 000100, 007200, 000021, 005554,
    001164, 003044, 001044, 004560, 007040, 001302, 007130, 001301,
    007700, 003441, 004334, 007041, 001303, 007640, 005330, 001442,
    000153, 001302, 003441, 002044, 005307, 001447, 005552, 000520,
    001442, 007106, 007006, 007006, 000151, 001150, 005734, 000511,
    000151, 007450, 005547, 003052, 001052, 001146, 003042, 001052,
    001145, 003041, 001052, 001144, 003050, 001441, 005744, 004631,
    005723, 006373, 006473, 006374, 006474, 006375, 006475, 005524,
    004020, 004604, 004605, 000000, 004024, 004224, 000000, 007100,
    004222, 005213, 004301, 005220, 001045, 007041, 001543, 004561,
    002574, 001046, 007041, 004561, 002574, 005557, 000000, 007440,
    005251, 003045, 003046, 001055, 004560, 007450, 005542, 003051,
    004567, 003044, 007420, 007030, 007012, 000450, 007640, 005220,
    001450, 007700, 005622, 002222, 007201, 003367, 001051, 000153,
    007106, 007004, 001367, 007041, 001007, 007450, 005270, 007041,
    001007, 003007, 007330, 004360, 001541, 007064, 007530, 005370,
    007070, 003053, 001140, 003017, 005622, 000000, 001417, 007650,
    005340, 007240, 001017, 003017, 001137, 003046, 001044, 003047,
    001047, 004536, 007041, 001417, 007640, 005335, 002047, 002046,
    005314, 004352, 001417, 007450, 005341, 007041, 003046, 002301,
    005701, 001046, 007001, 004352, 001417, 001045, 003045, 002053,
    005302, 003045, 001535, 007440, 005251, 005701, 000000, 001540,
    007041, 001017, 003017, 005752, 000000, 001134, 003365, 006202,
    004451, 004210, 001400, 000001, 005533, 005760, 000000, 000000,
    000000, 000000, 000222, 000223, 000000, 000224, 000225, 004576,
    000605, 001010, 001011, 001200, 001312, 000000, 003056, 004532,
    005531, 004530, 003042, 001140, 003017, 001055, 007112, 007012,
    000127, 007041, 003043, 001450, 000126, 007640, 005566, 001417,
    007650, 005343, 007346, 004525, 001417, 001042, 003042, 002053,
    005232, 001056, 007640, 005263, 001535, 007440, 005524, 001044,
    004536, 007640, 001046, 007650, 005566, 001054, 002056, 005524,
    001017, 003041, 001137, 004525, 001540, 007041, 001017, 001123,
    007700, 005522, 001441, 003417, 007240, 001041, 003041, 007344,
    001017, 003017, 001041, 007161, 001052, 007640, 005275, 001137,
    003041, 001052, 003017, 001044, 004536, 003417, 002044, 002041,
    005316, 001521, 003417, 007001, 004525, 003417, 001017, 003520,
    007240, 001541, 003541, 001450, 001054, 003450, 004517, 005516,
    001417, 007141, 003041, 001043, 001046, 007200, 001043, 007450,
    007020, 001041, 007220, 001041, 007041, 001046, 007670, 005374,
    001041, 003046, 007344, 001017, 003052, 001515, 003054, 001042,
    003045, 001041, 007041, 005240, 004532
};

static SCRetErr *writeformat (int fd)
{
    int rc = pwrite (fd, formatbuf, sizeof formatbuf, formatofs);
    if (rc != (int) sizeof formatbuf) {
        if (rc < 0) return new SCRetErr ("error writing format: %m\n");
        return new SCRetErr ("only wrote %d of %d-byte format\n", rc, (int) sizeof formatbuf);
    }
    return NULL;
}
