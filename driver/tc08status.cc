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

// display dectape (iodevtc08.cc) status
// uses shared memory to read status from raspictl

#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>

#define SAMPLESPERSEC 10
#define MAXDRIVES 8

#define BLOCKSPERTAPE 1474  // 02702
#define WORDSPERBLOCK 129

#define REVBIT 00400
#define GOBIT  00200

#define ESC_HOMEC "\033[H"          // home cursor
#define ESC_NORMV "\033[m"          // go back to normal video
#define ESC_REVER "\033[7m"         // turn reverse video on
#define ESC_UNDER "\033[4m"         // turn underlining on
#define ESC_BLINK "\033[5m"         // turn blink on
#define ESC_BOLDV "\033[1m"         // turn bold on
#define ESC_REDBG "\033[41m"        // red background
#define ESC_YELBG "\033[44m"        // yellow background
#define ESC_EREOL "\033[K"          // erase to end of line
#define ESC_EREOP "\033[J"          // erase to end of page

#include "iodevtc08.h"

static char const *const funcmnes[8] = { "MOVE", "SRCH", "RDAT", "RALL", "WDAT", "WALL", "WTIM", "ERR7" };

int main ()
{
    setlinebuf (stdout);

    bool warned = false;

    while (true) {

        // open shared memory created by raspictl IODevTC08::IODevTC08
        int shmfd = shm_open (IODevTC08::shmname, O_RDWR, 0);
        if (shmfd < 0) {
            if (errno != ENOENT) {
                fprintf (stderr, "error opening shared memory %s: %m\n", IODevTC08::shmname);
                return 1;
            }
            if (! warned) {
                warned = true;
                printf ("waiting for raspictl\n");
            }
            sleep (1);
            continue;
        }

        warned = false;

        // map it to va space
        void *shmptr = mmap (NULL, sizeof (IODevTC08Shm), PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
        if (shmptr == MAP_FAILED) {
            fprintf (stderr, "error accessing shared memory %s: %m\n", IODevTC08::shmname);
            return 1;
        }

        IODevTC08Shm *shm = (IODevTC08Shm *) shmptr;

        sigset_t sigintmask;
        sigemptyset (&sigintmask);
        sigaddset (&sigintmask, SIGINT);

        char bargraphs[MAXDRIVES*66];
        uint32_t filesizes[MAXDRIVES];
        uint16_t blocknosamples[MAXDRIVES][SAMPLESPERSEC];
        memset (bargraphs, 0, sizeof bargraphs);
        memset (filesizes, -1, sizeof filesizes);
        memset (blocknosamples, 0, sizeof blocknosamples);
        int sampleindex = 0;

        int j = 0;
        while (shm->initted && ! shm->exiting) {
            struct timeval nowtv;
            if (gettimeofday (&nowtv, NULL) < 0) ABORT ();
            int waitus = 1000000/SAMPLESPERSEC - nowtv.tv_usec % (1000000/SAMPLESPERSEC);
            usleep (waitus);

            // grab a copy while locked so we get consistent state
            // block control-C so we don't leave mutex locked
            if (pthread_sigmask (SIG_BLOCK, &sigintmask, NULL) != 0) ABORT ();
            if (pthread_mutex_lock (&shm->lock) != 0) ABORT ();
            IODevTC08Shm tmp = *shm;
            if (pthread_mutex_unlock (&shm->lock) != 0) ABORT ();
            if (pthread_sigmask (SIG_UNBLOCK, &sigintmask, NULL) != 0) ABORT ();

            // decode and print status line
            int driveno = (tmp.status_a >> 9) & 7;
            int func    = (tmp.status_a >> 3) & 7;
            bool go     = (tmp.status_a & 00200) != 0;
            bool rev    = (tmp.status_a & 00400) != 0;
            printf (ESC_HOMEC ESC_EREOL "\nstatus_A %04o <%o %s %s %s %s %s>  status_B %04o" ESC_EREOL "\n",
                tmp.status_a, driveno, (rev ? "REV" : "FWD"), (go ? " GO " : "STOP"),
                ((tmp.status_a & 000100) ? "CON" : "NOR"), funcmnes[func], ((tmp.status_a & 00004) ? "IENA" : "IDIS"),
                tmp.status_b);

            // display line for each drive with a tape file loaded
            char rw = "  rRwWW "[func];
            for (int i = 0; i < MAXDRIVES; i ++) {
                IODevTC08Drive *drive = &tmp.drives[i];
                if (drive->dtfd >= 0) {
                    char *bargraph = &bargraphs[i*66];
                    if (filesizes[i] == 0xFFFFFFFFU) {
                        char namebuf[60];
                        sprintf (namebuf, "/proc/%d/fd/%d", tmp.dtpid, drive->dtfd);
                        struct stat statbuf;
                        filesizes[i] = stat (namebuf, &statbuf) < 0 ? 0 : statbuf.st_size;
                        memset (&bargraphs[i*66], '-', 64);
                        int j = filesizes[i] * 64 / BLOCKSPERTAPE / WORDSPERBLOCK / 2;
                        bargraph[j]  = '|';
                        bargraph[64] = ']';
                    }
                    uint16_t blocknumber  = drive->tapepos / 4;
                    uint16_t oldblocknum  = blocknosamples[i][sampleindex];
                    uint16_t blockspersec = (blocknumber > oldblocknum) ? blocknumber - oldblocknum : oldblocknum - blocknumber;
                    blocknosamples[i][sampleindex] = blocknumber;

                    bool gofwd = go && ! rev && (i == driveno);
                    bool gorev = go &&   rev && (i == driveno);
                    printf (ESC_EREOL "\n  %o: %c%s" ESC_EREOL "\n", i, (drive->rdonly ? '-' : '+'), drive->fname);
                    printf (" [%s" ESC_EREOL "\n", bargraph);
                    printf ("%*s%c%c^%04o%c%c%c%6u wps" ESC_EREOL "\n",
                        drive->tapepos * 16 / BLOCKSPERTAPE, "",
                        (gorev ? rw : ' '), (gorev ? '<' : ' '),
                        blocknumber, 'a' + drive->tapepos % 4,
                        (gofwd ? '>' : ' '), (gofwd ? rw : ' '),
                        blockspersec * WORDSPERBLOCK);
                } else {
                    filesizes[i] = 0xFFFFFFFFU;
                }
            }

            if (++ sampleindex == SAMPLESPERSEC) sampleindex = 0;

            printf (ESC_EREOL "\n    0000a = beg-of-tape"
                    ESC_EREOL "\n    ____b = between leading block number and data"
                    ESC_EREOL "\n    ____c = between data and trailing block number"
                    ESC_EREOL "\n    2701d = end-of-tape" ESC_EREOL "\n");

            // little twirly to show we are cycling
            printf (ESC_EREOL "\n  %c" ESC_EREOP "\r", "-\\|/"[++j&3]);
        }

        printf ("raspictl exited\n");
        munmap (shmptr, sizeof (IODevTC08Shm));
        close (shmfd);
    }

    return 0;
}
