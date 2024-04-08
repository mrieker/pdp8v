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
// files are 1474*129*2 bytes = 380292 bytes
// dd if=/dev/zero bs=258 count=1474 of=disk0.dtp

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

#include "dyndis.h"
#include "iodevdtape.h"
#include "memory.h"
#include "shadow.h"

#define MIN(a,b) (((a) < (b)) ? (a) : (b))

#define BLOCKSPERTAPE 1474  // 02702
#define WORDSPERBLOCK 129
#define BYTESPERBLOCK (WORDSPERBLOCK*2)

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

IODevDTape iodevdtape;

static uint16_t ocarray[4096];

static IODevOps const iodevops[] = {
    { 06761, "DTRA (DTape) read status register A" },
    { 06762, "DTCA (DTape) clear status register A" },
    { 06764, "DTXA (DTape) load status register A" },
    { 06766, "DTLA (DTape) clear and load status register A" },
    { 06771, "DTSF (DTape) skip on flag" },
    { 06772, "DTRB (DTape) read status register B" },
    { 06773, "DTSF DTRB (DTape) skip on flag, read status register B" },
    { 06774, "DTXB (DTape) load status register B" },
};

static char const *const cmdmnes[8] = { "MOVE", "SRCH", "RDAT", "RALL", "WDAT", "WALL", "WTIM", "ERR7" };

IODevDTape::IODevDTape ()
{
    opscount = sizeof iodevops / sizeof iodevops[0];
    opsarray = iodevops;

    iodevname = "dtape";

    // initialize obverse complement lookup table
    for (int i = 0; i < 4096; i ++) {
        uint16_t reverse = 0;
        for (int j = 0; j < 12; j ++) if (! (i & (1 << j))) reverse |= 2048 >> j;
        ocarray[i] = reverse;
    }

    // create shared memory for dtapestatus.cc program to access tape state
    this->shmfd = shm_open (shmname, O_RDWR | O_CREAT, 0600);
    if (this->shmfd < 0) {
        fprintf (stderr, "IODevDTape::IODevDTape: error creating shared memory %s: %m\n", shmname);
        ABORT ();
    }
    if (ftruncate (this->shmfd, sizeof *this->shm) < 0) {
        fprintf (stderr, "IODevDTape::IODevDTape: error setting shared memory %s size: %m\n", shmname);
        close (this->shmfd);
        this->shmfd = -1;
        shm_unlink (shmname);
        ABORT ();
    }

    // map it to va space and zero it out
    void *shmptr = mmap (NULL, sizeof *this->shm, PROT_READ | PROT_WRITE, MAP_SHARED, this->shmfd, 0);
    if (shmptr == MAP_FAILED) {
        fprintf (stderr, "IODevDTape::IODevDTape: error accessing shared memory %s: %m\n", shmname);
        close (this->shmfd);
        this->shmfd = -1;
        shm_unlink (shmname);
        ABORT ();
    }

    this->shm = (IODevDTapeShm *) shmptr;
    memset (this->shm, 0, sizeof *this->shm);

    // initialize everything
    pthread_condattr_t condattrs;
    if (pthread_condattr_init (&condattrs) != 0) ABORT ();
    if (pthread_condattr_setpshared (&condattrs, PTHREAD_PROCESS_SHARED) != 0) ABORT ();
    if (pthread_cond_init (&shm->cond, &condattrs) != 0) ABORT ();

    pthread_mutexattr_t lockattrs;
    if (pthread_mutexattr_init (&lockattrs) != 0) ABORT ();
    if (pthread_mutexattr_setpshared (&lockattrs, PTHREAD_PROCESS_SHARED) != 0) ABORT ();
    if (pthread_mutex_init (&shm->lock, &lockattrs) != 0) ABORT ();

    for (int i = 0; i < 8; i ++) {
        shm->drives[i].dtfd = -1;
    }

    // lock and unlock for a barrier
    pthread_mutex_lock (&shm->lock);
    shm->initted = true;
    pthread_mutex_unlock (&shm->lock);
}

IODevDTape::~IODevDTape ()
{
    // lock and unlock for a barrier
    pthread_mutex_lock (&shm->lock);
    shm->exiting = true;
    pthread_mutex_unlock (&shm->lock);

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
void IODevDTape::ioreset ()
{
    pthread_mutex_lock (&shm->lock);

    if (shm->resetting) {
        do pthread_cond_wait (&shm->cond, &shm->lock);
        while (shm->resetting);
    } else {
        shm->resetting = true;
        pthread_cond_broadcast (&shm->cond);

        if (shm->threadid != 0) {
            pthread_mutex_unlock (&shm->lock);
            pthread_join (shm->threadid, NULL);
            pthread_mutex_lock (&shm->lock);
            shm->threadid = 0;
        }

        shm->status_a = 0;
        shm->status_b = 0;
        clrintreqmask (IRQ_DTAPE);

        shm->resetting = false;
        pthread_cond_broadcast (&shm->cond);
        pthread_mutex_unlock (&shm->lock);
    }
}

// load/unload files
SCRet *IODevDTape::scriptcmd (int argc, char const *const *argv)
{
    // debug [0/1/2]
    if (strcmp (argv[0], "debug") == 0) {
        if (argc == 1) {
            return new SCRetInt (shm->debug);
        }

        if (argc == 2) {
            shm->debug = atoi (argv[1]);
            return NULL;
        }

        return new SCRetErr ("iodev %s debug [0/1/2]", iodevname);
    }

    // gofast [0/1]
    if (strcmp (argv[0], "gofast") == 0) {
        if (argc == 1) {
            return new SCRetInt (shm->gofast);
        }

        if (argc == 2) {
            shm->gofast = atoi (argv[1]);
            return NULL;
        }

        return new SCRetErr ("iodev %s gofast [0/1]", iodevname);
    }

    if (strcmp (argv[0], "help") == 0) {
        puts ("");
        puts ("valid sub-commands:");
        puts ("");
        puts ("  debug                           - get debug level");
        puts ("  debug 0/1/2                     - set debug level");
        puts ("  gofast                          - get gofast level");
        puts ("  gofast 0/1/2                    - enabled makes drive run quickly");
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
            IODevDTapeDrive *drive = &shm->drives[driveno];
            if (drive->dtfd < 0) return NULL;
            return new SCRetStr ("%c%s", drive->rdonly ? '-' : '+', drive->fname);
        }

        return new SCRetErr ("iodev dtape loaded <drivenumber>");
    }

    // loadro/loadrw <drivenumber> <filename>
    bool loadro = (strcmp (argv[0], "loadro") == 0);
    bool loadrw = (strcmp (argv[0], "loadrw") == 0);
    if (loadro | loadrw) {
        if (argc == 3) {
            char *p;
            int driveno = strtol (argv[1], &p, 0);
            if ((*p != 0) || (driveno < 0) || (driveno > 7)) return new SCRetErr ("drivenumber %s not in range 0..7", argv[1]);
            int fd = open (argv[2], loadrw ? O_RDWR : O_RDONLY);
            if (fd < 0) return new SCRetErr (strerror (errno));
            if (flock (fd, (loadrw ? LOCK_EX : LOCK_SH) | LOCK_NB) < 0) {
                SCRetErr *err = new SCRetErr (strerror (errno));
                close (fd);
                return err;
            }
            if (loadrw && (ftruncate (fd, BYTESPERBLOCK * BLOCKSPERTAPE) < 0)) {
                SCRetErr *err = new SCRetErr (strerror (errno));
                close (fd);
                return err;
            }
            fprintf (stderr, "IODevDTape::scriptcmd: drive %d loaded with read%s file %s\n", driveno, (loadro ? "-only" : "/write"), argv[2]);
            pthread_mutex_lock (&shm->lock);
            IODevDTapeDrive *drive = &shm->drives[driveno];
            close (drive->dtfd);
            drive->dtfd = fd;
            drive->rdonly = loadro;
            strncpy (drive->fname, argv[2], sizeof drive->fname);
            drive->fname[sizeof drive->fname-1] = 0;
            pthread_mutex_unlock (&shm->lock);
            return NULL;
        }

        return new SCRetErr ("iodev dtape loadro/loadrw <drivenumber> <filename>");
    }

    // unload <drivenumber>
    if (strcmp (argv[0], "unload") == 0) {
        if (argc == 2) {
            char *p;
            int driveno = strtol (argv[1], &p, 0);
            if ((*p != 0) || (driveno < 0) || (driveno > 7)) return new SCRetErr ("drivenumber %s not in range 0..7", argv[1]);
            fprintf (stderr, "IODevDTape::scriptcmd: drive %d unloaded\n", driveno);
            pthread_mutex_lock (&shm->lock);
            close (shm->drives[driveno].dtfd);
            shm->drives[driveno].dtfd = -1;
            pthread_mutex_unlock (&shm->lock);
            return NULL;
        }

        return new SCRetErr ("iodev dtape unload <drivenumber>");
    }

    return new SCRetErr ("unknown tape command %s - valid: help loaded loadro loadrw unload", argv[0]);
}

// perform i/o instruction
uint16_t IODevDTape::ioinstr (uint16_t opcode, uint16_t input)
{
    pthread_mutex_lock (&shm->lock);

    uint16_t  oldstata = shm->status_a;

    switch (opcode) {

        // DTRA (DTape) read status register A
        // p 396
        case 06761: {
            input |= shm->status_a;
            break;
        }

        // DTCA (DTape) clear status register A
        case 06762: {
            shm->status_a = 0;
            if (shm->debug > 0) printf ("IODevDTape::ioinstr: status A cleared\n");
            updateirq ();
            break;
        }

        // DTLA (DTape) clear and load status register A
        case 06766: {
            shm->status_a = 0;
            // fall through
        }

        // DTXA (DTape) load status register A
        case 06764: { 
            shm->status_a ^= input & 07774;
            if (! (input & 00001)) shm->status_b &= ~ DTFLAG;  // clear dectape flag
            if (! (input & 00002)) shm->status_b &= ~ ERRORS;  // clear all error flags
            updateirq ();

            if (shm->debug > 0) {
                int drno = (shm->status_a >> 9) & 7;
                printf ("IODevDTape::ioinstr: st_A=%04o  st_B=%04o  startio  %o %s %s %s %s  tpos=%05o  idwc=%04o  idca=%04o\n",
                    shm->status_a, shm->status_b, (shm->status_a >> 9) & 7, CONTIN ? "CON" : "NOR",
                    (shm->status_a & REVBIT) ? "REV" : "FWD", (shm->status_a & GOBIT) ? "GO" : "ST",
                    cmdmnes[(shm->status_a&070)>>3], shm->drives[drno].tapepos, memarray[IDWC], memarray[IDCA]);
            }

            if ((shm->status_a ^ oldstata) & (GOBIT | REVBIT)) {
                shm->startdelay = true;                     // additional delay if direction changed or just started or stopped
            }
            if (shm->status_a & GOBIT) {
                shm->iopend = true;                         // there is an i/o command to process
                shm->dmapc  = dyndispc;
                shm->cycles = shadow.getcycles ();
                pthread_cond_broadcast (&shm->cond);
                if (shm->threadid == 0) {
                    int rc = pthread_create (&shm->threadid, NULL, threadwrap, this);
                    if (rc != 0) ABORT ();
                }
            }

            input &= IO_LINK;                               // clear accumulator
            break;
        }

        // DTSF (DTape) skip on flag
        case 06771: {
            if (shm->status_b & INTREQ) input |= IO_SKIP;
            break;
        }

        // DTRB (DTape) read status register B
        case 06772: {
            input |= shm->status_b;
            break;
        }

        // DTSF (DTape) skip on flag
        // DTRB (DTape) read status register B
        case 06773: {
            if (shm->status_b & INTREQ) input |= IO_SKIP;
            input |= shm->status_b;
            break;
        }

        // DTXB (DTape) load status register B
        case 06774: {
            shm->status_b = (shm->status_b & 07707) | (input & 00070);
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

// 'toggled-in' TC08 boot code for OS/8
// OS8 Handbook April 1974, p41
static uint16_t const tc08bootcode[] = {
    /* 7613 */  06774, 01222, 06766, 06771, 05216, 01223, 05215, 00600, 00220
};

// contents of block 0 of TC08 tape
static uint16_t const tc08bootdata[] = {
    /* 7600 */  01236, 06766, 06771, 05202, 03231, 03232, 01237, 05224,
    /* 7610 */  00000, 00137, 01355, 01211, 07650, 05220, 07000, 05212,
    /* 7620 */  01235, 06774, 06771, 05222, 06764, 06774, 01234, 03355,
    /* 7630 */  03354, 06213, 05242, 05212, 07577, 00010, 00600, 00620,
    //                         ^JMP 17642
    /* 7640 */  00000, 00000, 03344, 06771, 05243, 06203, 05205, 07607,
    /* 7650 */  07607, 00000, 00000, 00000, 00000, 00000, 00000, 00000
};

// thread what does the tape file I/O
void *IODevDTape::threadwrap (void *zhis)
{
    ((IODevDTape *)zhis)->thread ();
    return NULL;
}

void IODevDTape::thread ()
{
    pthread_mutex_lock (&shm->lock);
    while (! shm->resetting) {

        // waiting for another I/O request
        if (! shm->iopend) {
            int rc = pthread_cond_wait (&shm->cond, &shm->lock);
            if (rc != 0) ABORT ();
        } else {
            shm->iopend = false;

            int field = (shm->status_b & 070) << 9;
            uint16_t *memfield = &memarray[field];

            // if tape stopped, we can't do anything
            if (! (shm->status_a & GOBIT)) continue;

            // select error if no tape loaded
            int driveno = (shm->status_a >> 9) & 007;
            IODevDTapeDrive *drive = &shm->drives[driveno];
            if (drive->dtfd < 0) {
                shm->status_b |= SELERR;
                goto finished;
            }

            // blocks are conceptually stored:
            //  fwdmark          revmark
            //    <n>   <data-n>   <n>       - where 00000<=n<=02701
            //  ^     ^          ^     ^     - the tape head is at any one of these positions
            //  a     b          c     d     - letter shown on dtapestatus.cc output
            //  0+4n  1+4n       2+4n  3+4n  - value stored in tapepos
            // tapepos = 0 : at very beginning of tape
            // tapepos = BLOCKSPERTAPE*4-1 : at very end of tape
            // the file just contains the data, as 129 16-bit words, rjzf * 1474 blocks
            // seek forward always stops at 'b' or end-of-tape
            // seek reverse always stops at 'c' or beg-of-tape
            // read/write forward always stop at 'c' or end-of-tape
            // read/write reverse always stop at 'b' or beg-of-tape
            ASSERT (drive->tapepos < BLOCKSPERTAPE*4);

            switch ((shm->status_a & 070) >> 3) {

                // MOVE (2.5.1.4 p 27)
                case 0: {
                    while (true) {
                        if (this->delayblk ()) goto finished;
                        if (this->stepskip (drive)) goto endtape;
                    }
                }

                // SEARCH (2.5.1.4 p 27)
                case 1: {
                    uint16_t idwc = memarray[IDWC];
                    uint16_t idca = memarray[IDCA];
                    ASSERT (idwc <= 07777);
                    ASSERT (idca <= 07777);
                    dyndisdma (IDWC, 1,  true, shm->dmapc);
                    dyndisdma (IDCA, 1, false, shm->dmapc);
                    dyndisdma (memfield + idca - memarray, 1, true, shm->dmapc);
                    do {
                        if (this->delayblk ()) goto finished;
                        if (this->stepskip (drive)) goto endtape;

                        memfield[idca] = drive->tapepos / 4;                    // write out mark we just hopped over
                        idwc = (idwc + 1) & 07777;
                        memarray[IDWC] = idwc;
                        if (shm->debug > 0) printf ("IODevDTape::thread: search memfield[%04o] <= %04o  idwc=%04o\n", idca, drive->tapepos / 4, idwc);
                    } while (CONTIN && (idwc != 0));
                    goto normal;
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
                            fprintf (stderr, "IODevDTape::thread: error reading tape %d file: %m\n", driveno);
                            ABORT ();
                        }
                        if (rc != BYTESPERBLOCK) {
                            fprintf (stderr, "IODevDTape::thread: only read %d of %d bytes from tape %d file\n", rc, BYTESPERBLOCK, driveno);
                            ABORT ();
                        }
                        if (shm->debug > 1) this->dumpbuf (drive, "read", buff);

                        idca = memarray[IDCA];
                        idwc = memarray[IDWC];
                        ASSERT (idca <= 07777);
                        ASSERT (idwc <= 07777);
                        dyndisdma (IDWC, 1, true, shm->dmapc);
                        dyndisdma (IDCA, 1, true, shm->dmapc);
                        dyndisdma (memfield + idca - memarray, MIN (WORDSPERBLOCK, 010000 - idwc), true, shm->dmapc);
                        if (REVERS) {
                            for (int i = WORDSPERBLOCK; -- i >= 0;) {
                                idca = (idca + 1) & 07777;
                                memfield[idca] = ocarray[buff[i]&07777];
                                idwc = (idwc + 1) & 07777;
                                if (idwc == 0) break;
                            }
                        } else {
                            // madness: TC08 OS/8 boot block changes from data field 0 to data field 1 mid-read
                            bool os8boot_a = (idca == 07577);
                            bool os8boot_b = (idwc == 07577);
                            bool os8boot_c = (drive->tapepos / 4 == 0);
                            bool os8boot_d = (memfield == memarray);
                            bool os8boot_e = (memcmp (&memarray[07613], tc08bootcode, sizeof tc08bootcode) == 0);
                            bool os8boot_f = (memcmp (buff, tc08bootdata, sizeof tc08bootdata) == 0);
                            bool os8boot = os8zap && os8boot_a && os8boot_b && os8boot_c && os8boot_d && os8boot_e && os8boot_f;

                            for (int i = 0; i < WORDSPERBLOCK; i ++) {
                                idca = (idca + 1) & 07777;
                                if (os8boot && (idca == 07640)) memfield += 010000;
                                memfield[idca] = buff[i] & 07777;
                                idwc = (idwc + 1) & 07777;
                                if (idwc == 0) break;
                            }
                        }
                        memarray[IDCA] = idca;
                        memarray[IDWC] = idwc;
                    } while (CONTIN && (idwc != 0));
                    goto normal;
                }

                // WRITE DATA (2.5.1.7 p 28)
                case 4: {
                    if (shm->drives[driveno].rdonly) {
                        shm->status_b |= SELERR;
                        goto finished;
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
                        dyndisdma (memfield + idca - memarray, MIN (WORDSPERBLOCK, 010000 - idwc), false, shm->dmapc);
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

                        if (shm->debug > 1) this->dumpbuf (drive, "write", buff);
                        int rc = pwrite (drive->dtfd, buff, BYTESPERBLOCK, drive->tapepos / 4 * BYTESPERBLOCK);
                        if (rc < 0) {
                            fprintf (stderr, "IODevDTape::thread: error writing tape %d file: %m\n", driveno);
                            ABORT ();
                        }
                        if (rc != BYTESPERBLOCK) {
                            fprintf (stderr, "IODevDTape::thread: only wrote %d of %d bytes to tape %d file\n", rc, BYTESPERBLOCK, driveno);
                            ABORT ();
                        }
                    } while (CONTIN && (idwc != 0));
                    goto normal;
                }

                default: {
                    fprintf (stderr, "IODevDTape::thread: unsupported command %u\n", ((shm->status_a & 00070) >> 3));
                    shm->status_b |= SELERR;
                    goto finished;
                }
            }
            ABORT ();
        endtape:;
            shm->status_a &= ~ GOBIT;   // end-of-tape shuts the GO bit off
            shm->status_b |=  ENDTAP;   // set up end-of-tape status
            goto finished;
        normal:;
            shm->status_b |=  DTFLAG;   // errors do not get DTFLAG
        finished:;
            if (shm->debug > 0) printf ("IODevDTape::thread: status B %04o\n", shm->status_b);

            // maybe request an interrupt
            this->updateirq ();
        }
    }

    // ioreset(), unlock and exit thread
    pthread_mutex_unlock (&shm->lock);
}

// step tape just before data in the current tape direction
//  returns false: tapepos updated as described
//           true: hit end-of-tape
//                 tapepos set to whichever end we're at
bool IODevDTape::stepskip (IODevDTapeDrive *drive)
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
bool IODevDTape::stepxfer (IODevDTapeDrive *drive)
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
void IODevDTape::dumpbuf (IODevDTapeDrive *drive, char const *label, uint16_t const *buff)
{
    printf ("IODevDTape::dumpbuf: %o  %5s %s  block %04o\n", (int) (drive - shm->drives), label, (REVERS ? "REV" : "FWD"), drive->tapepos / 4);
    for (int i = 0; i < WORDSPERBLOCK; i += 16) {
        printf ("  %04o:", i);
        for (int j = 0; (j < 16) && (i + j < WORDSPERBLOCK); j ++) {
            printf (" %04o", buff[i+j]);
        }
        printf ("\n");
    }
}

// delay processing for a block
//  returns:
//   false = it's ok to keep going as is
//    true = something changed, re-decode
bool IODevDTape::delayblk ()
{
    // no delay if going fast
    if (shm->gofast) return false;

    // 25ms standard per-block delay
    int usec = 25000;

    // 375ms additional for startup delay
    if (shm->startdelay) {
        shm->startdelay = false;
        usec += 375000;
    }

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
        if (waitingforinterrupt) return false;                  // HLT is ok (processor won't cycle)
        uint64_t ncycles = shadow.getcycles () - shm->cycles;   // see how many cycles since I/O started
        if (ncycles >= 100) return false;                       // if 100 cycles, assume IDCA,IDWC set up

        // not yet, wait another millisecond
        usec = 1000;
    }
}

// update interrupt request
// interrupt if enabled and command has completed
void IODevDTape::updateirq ()
{
    if ((shm->status_a & INTENA) && (shm->status_b & INTREQ)) {
        setintreqmask (IRQ_DTAPE);
    } else {
        clrintreqmask (IRQ_DTAPE);
    }
}
