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

// test of two 32-pin paddles plugged into each other
// can run on pc or raspi
// one paddle old (unbuffered)
// one paddle new (buffered)
//  ./iow56testpair.x86_64 [-auto] [-delayus 5000] [single | oldsn] newsn
//   single = test single new board with pullup resistors

#include <fcntl.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <unistd.h>

#include "myiowkit.h"
#include "readprompt.h"

// tell io-warrior-56 to set all open-drain outputs to 1 (high impeadance)
// this will let us read any signal sent to the pins
static IOWKIT56_IO_REPORT const allonepins = { 0, { 255, 255, 255, 255, 255, 255, 255 } };

// ask io-warrior-56 for state of all pins
static IOWKIT56_SPECIAL_REPORT const reqallpins[] = { 255 };

static bool automode;
static bool stoppedonmismatch;
static char const *oldsn, *newsn;
static IOWKIT_HANDLE oldiowh, newiowh;
static sigset_t sigintmask;
static uint32_t delayus;

static void listeach (void *ctx, char const *dn, int pipe, char const *sn, int pid, int rev);
static void writeallpins (IOWKIT_HANDLE iowh, uint64_t allpins);
static uint64_t readallpins (IOWKIT_HANDLE iowh);
static void blocksigint ();
static void allowsigint ();
static uint32_t randuint32 ();
static void manualtest ();
static void *rdthread (void *dummy);

int main (int argc, char **argv)
{
    delayus = 5000;

    for (int i = 1; i < argc; i ++) {
        if (strcasecmp (argv[i], "-auto") == 0) {
            automode = true;
            continue;
        }
        if (strcasecmp (argv[i], "-delayus") == 0) {
            if ((++ i >= argc) || (argv[i][0] == '-')) {
                fprintf (stderr, "missing usec after -delayus\n");
                return 1;
            }
            char *p;
            delayus = strtoul (argv[i], &p, 0);
            if (*p != 0) {
                fprintf (stderr, "bad used %s\n", argv[i]);
                return 1;
            }
            continue;
        }
        if (argv[i][0] == '-') {
            fprintf (stderr, "bad option %s\n", argv[i]);
            return 1;
        }
        if (oldsn == NULL) {
            oldsn = argv[i];
            continue;
        }
        if (newsn == NULL) {
            newsn = argv[i];
            continue;
        }
        fprintf (stderr, "bad argument %s\n", argv[i]);
        return 1;
    }
    if (oldsn == NULL) {
        fprintf (stderr, "missing oldsn newsn arguments\n");
        return 1;
    }
    if (newsn == NULL) {
        fprintf (stderr, "missing newsn argument\n");
        return 1;
    }

    sigemptyset (&sigintmask);
    sigaddset (&sigintmask, SIGINT);
    sigaddset (&sigintmask, SIGTERM);

    IowKit::list (listeach, NULL);

    if ((strcasecmp (oldsn, "single") != 0) && (oldiowh == NULL)) {
        fprintf (stderr, "did not find %s\n", oldsn);
        return 1;
    }
    if (newiowh == NULL) {
        fprintf (stderr, "did not find %s\n", newsn);
        return 1;
    }

    manualtest ();

    return 0;
}

static void listeach (void *ctx, char const *dn, int pipe, char const *sn, int pid, int rev)
{
    if (pipe != 0) return;
    if (pid != IOWKIT_PRODUCT_ID_IOW56) return;
    printf (" sn=%s\n", sn);
    IOWKIT_HANDLE iowh = new IowKit;
    if (! iowh->openbysn (sn)) abort ();
    if (strcasecmp (sn, oldsn) == 0) oldiowh = iowh;
    if (strcasecmp (sn, newsn) == 0) newiowh = iowh;

    // write 1s to all open-drain pins so we can read the pins
    writeallpins (iowh, -1ULL);
}

// unbuffered paddle: mapping of 32 connector pins to io-warrior-56 port bits
static uint8_t const oldpinmapping[] = {
        3 * 8 + 5,  // bit 00 -> pin 01 -> P3.5
        4 * 8 + 1,  // bit 01 -> pin 02 -> P4.1
        4 * 8 + 5,  // bit 02 -> pin 03 -> P4.5
        2 * 8 + 1,  // bit 03 -> pin 04 -> P2.1
        2 * 8 + 5,  // bit 04 -> pin 05 -> P2.5
        0 * 8 + 1,  // bit 05 -> pin 06 -> P0.1
        0 * 8 + 5,  // bit 06 -> pin 07 -> P0.5
        0 * 8 + 6,  // bit 07 -> pin 08 -> P0.6
        0 * 8 + 3,  // bit 08 -> pin 09 -> P0.3
        0 * 8 + 7,  // bit 09 -> pin 10 -> P0.7
        2 * 8 + 7,  // bit 10 -> pin 11 -> P2.7
        2 * 8 + 3,  // bit 11 -> pin 12 -> P2.3
        4 * 8 + 7,  // bit 12 -> pin 13 -> P4.7
        4 * 8 + 3,  // bit 13 -> pin 14 -> P4.3
        3 * 8 + 7,  // bit 14 -> pin 15 -> P3.7
        3 * 8 + 3,  // bit 15 -> pin 16 -> P3.3

        5 * 8 + 2,  // bit 16 -> pin 17 -> P5.2
        5 * 8 + 6,  // bit 17 -> pin 18 -> P5.6
        3 * 8 + 2,  // bit 18 -> pin 19 -> P3.2
        3 * 8 + 6,  // bit 19 -> pin 20 -> P3.6
        4 * 8 + 2,  // bit 20 -> pin 21 -> P4.2
        4 * 8 + 6,  // bit 21 -> pin 22 -> P4.6
        2 * 8 + 6,  // bit 22 -> pin 23 -> P2.6
        2 * 8 + 2,  // bit 23 -> pin 24 -> P2.2
        0 * 8 + 0,  // bit 24 -> pin 25 -> P0.0
        2 * 8 + 4,  // bit 25 -> pin 26 -> P2.4
        2 * 8 + 0,  // bit 26 -> pin 27 -> P2.0
        4 * 8 + 4,  // bit 27 -> pin 28 -> P4.4
        4 * 8 + 0,  // bit 28 -> pin 29 -> P4.0
        3 * 8 + 4,  // bit 29 -> pin 30 -> P3.4
        3 * 8 + 0,  // bit 30 -> pin 31 -> P3.0
        5 * 8 + 4   // bit 31 -> pin 32 -> P5.4
};

// mapping of 32 abcd connector pins to io-warrior-56 input port pins
static uint8_t const inputpinmapping[] = {
        031,  // abcd pin 01 -> iow56 P3.1 -> retval bit 00
        055,  // abcd pin 02 -> iow56 P5.5 -> retval bit 01
        051,  // abcd pin 03 -> iow56 P5.1 -> retval bit 02
        013,  // abcd pin 04 -> iow56 P1.3 -> retval bit 03
        025,  // abcd pin 05 -> iow56 P2.5 -> retval bit 04
        021,  // abcd pin 06 -> iow56 P2.1 -> retval bit 05
        045,  // abcd pin 07 -> iow56 P4.5 -> retval bit 06
        041,  // abcd pin 08 -> iow56 P4.1 -> retval bit 07
        023,  // abcd pin 09 -> iow56 P2.3 -> retval bit 08
        006,  // abcd pin 10 -> iow56 P0.6 -> retval bit 09
        005,  // abcd pin 11 -> iow56 P0.5 -> retval bit 10
        001,  // abcd pin 12 -> iow56 P0.1 -> retval bit 11
        053,  // abcd pin 13 -> iow56 P5.3 -> retval bit 12
        057,  // abcd pin 14 -> iow56 P5.7 -> retval bit 13
        033,  // abcd pin 15 -> iow56 P3.3 -> retval bit 14
        037,  // abcd pin 16 -> iow56 P3.7 -> retval bit 15

        056,  // abcd pin 17 -> iow56 P5.6 -> retval bit 16
        052,  // abcd pin 18 -> iow56 P5.2 -> retval bit 17
        016,  // abcd pin 19 -> iow56 P1.6 -> retval bit 18
        012,  // abcd pin 20 -> iow56 P1.2 -> retval bit 19
        042,  // abcd pin 21 -> iow56 P4.2 -> retval bit 20
        036,  // abcd pin 22 -> iow56 P3.6 -> retval bit 21
        000,  // abcd pin 23 -> iow56 P0.0 -> retval bit 22
        024,  // abcd pin 24 -> iow56 P2.4 -> retval bit 23
        034,  // abcd pin 25 -> iow56 P3.4 -> retval bit 24
        040,  // abcd pin 26 -> iow56 P4.0 -> retval bit 25
        044,  // abcd pin 27 -> iow56 P4.4 -> retval bit 26
        020,  // abcd pin 28 -> iow56 P2.0 -> retval bit 27
        067,  // abcd pin 29 -> iow56 P6.7 -> retval bit 28
        014,  // abcd pin 30 -> iow56 P1.4 -> retval bit 29
        050,  // abcd pin 31 -> iow56 P5.0 -> retval bit 30
        030   // abcd pin 32 -> iow56 P3.0 -> retval bit 31
};

// these iow56 pins clock the 8-bit output latches
#define LOAD1 035  // P3.5  =>  LAT 05,06,07,08,04,03,02,01
#define LOAD2 043  // P4.3  =>  LAT 13,14,15,16,12,11,10,09
#define LOAD3 004  // P0.4  =>  LAT 24,23,22,21,17,18,19,20
#define LOAD4 054  // P5.4  =>  LAT 32,31,30,29,25,26,27,28
static uint8_t loadpinmapping[] = { LOAD1, LOAD2, LOAD3, LOAD4 };

// these iow56 pins present data to the 8-bit output latches

#define OUT1 022    // P2.2
#define OUT2 046    // P4.6
#define OUT3 002    // P0.2
#define OUT4 026    // P2.6
#define OUT5 047    // P4.7
#define OUT6 007    // P0.7
#define OUT7 027    // P2.7
#define OUT8 003    // P0.3

static uint8_t outputpinmapping[] {

            // LOAD1
        OUT1,   // bit 00 -> LAT01
        OUT2,   // bit 01 -> LAT02
        OUT3,   // bit 02 -> LAT03
        OUT4,   // bit 03 -> LAT04
        OUT8,   // bit 04 -> LAT05
        OUT7,   // bin 05 -> LAT06
        OUT6,   // bin 06 -> LAT07
        OUT5,   // bin 07 -> LAT08

            // LOAD2
        OUT1,   // bit 08 -> LAT09
        OUT2,   // bit 09 -> LAT10
        OUT3,   // bit 10 -> LAT11
        OUT4,   // bit 11 -> LAT12
        OUT8,   // bit 12 -> LAT13
        OUT7,   // bit 13 -> LAT14
        OUT6,   // bit 14 -> LAT15
        OUT5,   // bit 15 -> LAT16

            // LOAD3
        OUT4,   // bin 16 -> LAT17
        OUT3,   // bin 17 -> LAT18
        OUT2,   // bin 18 -> LAT19
        OUT1,   // bin 19 -> LAT20
        OUT5,   // bin 20 -> LAT21
        OUT6,   // bin 21 -> LAT22
        OUT7,   // bin 22 -> LAT23
        OUT8,   // bin 23 -> LAT24

            // LOAD4
        OUT4,   // bin 24 -> LAT25
        OUT3,   // bin 25 -> LAT26
        OUT2,   // bin 26 -> LAT27
        OUT1,   // bin 27 -> LAT28
        OUT5,   // bin 28 -> LAT29
        OUT6,   // bin 29 -> LAT30
        OUT7,   // bin 30 -> LAT31
        OUT8    // bin 31 -> LAT32
};

// write 32-bit data word to old paddle
static void writeold (uint32_t val)
{
    uint64_t writepins = -1ULL;
    for (int i = 0; i < 32; i ++) {
        if (! (val & 1)) {
            uint32_t p = oldpinmapping[i];
            writepins &= ~ (1ULL << p);
        }
        val /= 2;
    }
    writeallpins (oldiowh, writepins);
}

// write 32-bit data word to new paddle
static void writenew (uint32_t val)
{
    for (int i = 0; i < 4; i ++) {
        uint64_t writepins = -1ULL;
        uint32_t ldp = loadpinmapping[i];
        writepins &= ~ (1ULL << ldp);
        for (int j = 0; j < 8; j ++) {
            if (val & 1) {
                uint32_t p = outputpinmapping[i*8+j];
                writepins &= ~ (1ULL << p);
            }
            val /= 2;
        }
        writeallpins (newiowh, writepins);
        writepins |= 1ULL << ldp;
        writeallpins (newiowh, writepins);
    }
}

// write all 56 pins of the iowarrior-56
static void writeallpins (IOWKIT_HANDLE iowh, uint64_t allpins)
{
    IOWKIT56_IO_REPORT writepins;
    memset (&writepins, 0, sizeof writepins);

    for (int i = 0; i < 7; i ++) {
        writepins.Bytes[i] = allpins;
        allpins >>= 8;
    }

    blocksigint ();

    usleep (delayus);
    iowh->write (IOW_PIPE_IO_PINS, &writepins, sizeof writepins);

    allowsigint ();
}

// read all 56 pins of the iowarrior-56
static uint64_t readallpins (IOWKIT_HANDLE iowh)
{
    blocksigint ();

    // tell iow56 to send back what it has on the pins
    usleep (delayus);
    iowh->write (IOW_PIPE_SPECIAL_MODE, &reqallpins, sizeof reqallpins);

    // get state of all the iow56 pins
    IOWKIT56_SPECIAL_REPORT vfyallpins;
    memset (&vfyallpins, 0, sizeof vfyallpins);
    usleep (delayus);
    iowh->read (IOW_PIPE_SPECIAL_MODE, &vfyallpins, sizeof vfyallpins);
    if (vfyallpins.ReportID != 255) {
        fprintf (stderr, "readallpins: got report id %02X reading vfyallpins\n", vfyallpins.ReportID);
        abort ();
    }

    allowsigint ();

    uint64_t allpins = 0;
    for (int i = 7; -- i >= 0;) {
        allpins = (allpins << 8) | (vfyallpins.Bytes[i] & 0xFFU);
    }
    return allpins;
}

// the iowarrior chips get really bent out of shape if the access functions are aborted
// so block signals while they are running
static void blocksigint ()
{
    if (sigprocmask (SIG_BLOCK, &sigintmask, NULL) < 0) abort ();
}

static void allowsigint ()
{
    if (sigprocmask (SIG_UNBLOCK, &sigintmask, NULL) < 0) abort ();
}

// generate a random number
static uint32_t randuint32 ()
{
    static uint64_t seed = 0x123456789ABCDEF0ULL;

    uint32_t randval = 0;

    for (int i = 0; i < 32; i ++) {

        // https://www.xilinx.com/support/documentation/application_notes/xapp052.pdf
        uint64_t xnor = ~ ((seed >> 63) ^ (seed >> 62) ^ (seed >> 60) ^ (seed >> 59));
        seed = (seed << 1) | (xnor & 1);

        randval += randval + (seed & 1);
    }

    return randval;
}

static pthread_cond_t hltcond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t iowmutex = PTHREAD_MUTEX_INITIALIZER;

static void *rdthread (void *dummy);

static void manualtest ()
{
    pthread_t rdtid;
    int rci = pthread_create (&rdtid, NULL, rdthread, NULL);
    if (rci != 0) abort ();

    uint32_t count = 0;
    while (true) {
        usleep (delayus);

        if (pthread_mutex_lock (&iowmutex) != 0) abort ();

        uint32_t sendval = 0;
        if (automode) {
            sendval = randuint32 ();
            if (oldiowh == NULL) {
                printf ("auto %08X new => self  ", sendval);
                writenew (sendval);
            }
            else if (count & 1) {
                printf ("auto %08X new => old  ", sendval);
                writeold (-1U);
                writenew (sendval);
            }
            else {
                printf ("auto %08X old => new  ", sendval);
                writenew (-1U);
                writeold (sendval);
            }
        }

        uint64_t vfyallold = (oldiowh != NULL) ? readallpins (oldiowh) : 0;
        uint64_t vfyallnew = readallpins (newiowh);

        uint32_t oldinputs = 0;
        uint32_t newinputs = 0;
        for (int i = 32; -- i >= 0;) {
            uint32_t oldp = oldpinmapping[i];
            oldinputs += oldinputs + ((vfyallold >> oldp) & 1);
            uint32_t newp = inputpinmapping[i];
            newinputs += newinputs + ((vfyallnew >> newp) & 1);
        }
        newinputs = ~ newinputs;

        if (oldiowh != NULL) {
            printf ("%10u:  %02X  %02X  %02X  %02X  %02X  %02X  %02X   %08X     %02X  %02X  %02X  %02X  %02X  %02X  %02X   %08X\n",
                ++ count,
                    (uint8_t)(vfyallold >>  0), (uint8_t)(vfyallold >>  8), (uint8_t)(vfyallold >> 16), (uint8_t)(vfyallold >> 24), 
                    (uint8_t)(vfyallold >> 32), (uint8_t)(vfyallold >> 40), (uint8_t)(vfyallold >> 48), oldinputs,
                    (uint8_t)(vfyallnew >>  0), (uint8_t)(vfyallnew >>  8), (uint8_t)(vfyallnew >> 16), (uint8_t)(vfyallnew >> 24), 
                    (uint8_t)(vfyallnew >> 32), (uint8_t)(vfyallnew >> 40), (uint8_t)(vfyallnew >> 48), newinputs);
        } else {
            printf ("%10u:  %02X  %02X  %02X  %02X  %02X  %02X  %02X   %08X\n",
                ++ count,
                    (uint8_t)(vfyallnew >>  0), (uint8_t)(vfyallnew >>  8), (uint8_t)(vfyallnew >> 16), (uint8_t)(vfyallnew >> 24), 
                    (uint8_t)(vfyallnew >> 32), (uint8_t)(vfyallnew >> 40), (uint8_t)(vfyallnew >> 48), newinputs);
            oldinputs = sendval;
        }

        if (automode && ((oldinputs != sendval) || (newinputs != sendval))) {
            printf ("**MISMATCH** -- press enter\n");
            stoppedonmismatch = true;
            do {
                if (pthread_cond_wait (&hltcond, &iowmutex) != 0) abort ();
            } while (stoppedonmismatch);
        }

        if (pthread_mutex_unlock (&iowmutex) != 0) abort ();
    }
}

// runs in background to read pin updates from stdin
//  1) press enter (blank line) blocks main program with mutex
//  2) enter oldnewpinno=value eg n15=0 for set new P1.5 = 0
//  3) value gets written to pin
//  4) mutex is released to let main run again
static void *rdthread (void *dummy)
{
    char *p;
    IOWKIT_HANDLE iowh;
    uint64_t *updpins;

    // set up to write pins with all ones
    uint64_t updallold = -1ULL;
    uint64_t updallnew = -1ULL;

    while (true) {
        char tmp[10];
        char const *line = fgets (tmp, sizeof tmp, stdin);
        if (pthread_mutex_lock (&iowmutex) != 0) abort ();
        if (line == NULL) goto done;

    readcmd:;
        line = readprompt (" > ");
        if (line == NULL) goto done;
        while ((*line != 0) && (*line <= ' ')) line ++;
        if (*line == 0) goto unlock;
        iowh = NULL;
        updpins = NULL;

        // commands for the old paddle
        if ((oldiowh != NULL) && (line[0] == 'o')) {
            iowh = oldiowh;
            updpins = &updallold;
            if (line[1] == 'd') {
                if (line[2] != '=') goto bad;
                uint32_t val = strtoul (line + 3, &p, 16);
                if (*p >= ' ') goto bad;
                for (int i = 0; i < 32; i ++) {
                    uint32_t p = oldpinmapping[i];
                    updallold &= ~ (1ULL << p);
                    if (val & 1) updallold |= 1ULL << p;
                    val /= 2;
                }
                goto writeit;
            }
            goto single;
        }

        // commands for the new paddle
        if (line[0] == 'n') {
            iowh = newiowh;
            updpins = &updallnew;
            if (line[1] == 'd') {
                uint32_t bank = line[2] - '1';
                if (bank > 3) goto bad;
                if (line[3] != '=') goto bad;
                uint32_t val = strtoul (line + 4, &p, 16);
                if (*p >= ' ') goto bad;
                if (val > 255) goto bad;
                for (int i = 0; i < 8; i ++) {
                    uint32_t p = outputpinmapping[i+bank*8];
                    updallnew &= ~ (1ULL << p);
                    if (val & 1) updallnew |= 1ULL << p;
                    val /= 2;
                }
                goto writeit;
            }
            if (line[1] == 'l') {
                uint32_t bank = line[2] - '1';
                if (bank > 3) goto bad;
                if (line[3] != '=') goto bad;
                uint32_t val = strtoul (line + 4, &p, 16);
                if (*p >= ' ') goto bad;
                if (val > 1) goto bad;
                uint32_t p = loadpinmapping[bank];
                updallnew &= ~ (1ULL << p);
                if (val & 1) updallnew |= 1ULL << p;
                goto writeit;
            }
            goto single;
        }
        goto bad;

    single:;
        {
            uint32_t pino = strtoul (line + 1, &p, 8);
            if ((iowh != NULL) && (*p == '=') && (pino <= 067)) {
                uint32_t val = strtoul (++ p, &p, 0);
                if ((*p < ' ') && (val < 2)) {

                    // set the output pin as given
                    *updpins &= ~ (1ULL << pino);
                    if (val & 1) *updpins |= (1ULL << pino);

                    // write to iowarrior
                    goto writeit;
                }
            }
        }

    bad:;
        fprintf (stderr, "bad %s\n", line);
        if (oldiowh != NULL) {
            fprintf (stderr, "  od=<32bithexdata>\n");
            fprintf (stderr, "  o<pin>=0/1\n");
        }
        fprintf (stderr, "  nd<bank1..4>=<8bithexdata>\n");
        fprintf (stderr, "  nl<bank1..4>=0/1\n");
        fprintf (stderr, "  n<pin>=0/1\n");
        goto readcmd;

    writeit:;
        writeallpins (iowh, *updpins);
        printf ("  wrote %016llX\n", (unsigned long long)*updpins);
        goto readcmd;

    unlock:;
        stoppedonmismatch = false;
        if (pthread_cond_broadcast (&hltcond) != 0) abort ();
        if (pthread_mutex_unlock (&iowmutex) != 0) abort ();
    }
done:;
    exit (0);
    return NULL;
}
