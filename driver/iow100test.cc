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

// test of iowarrior-100-based 32-pin paddles plugged into nothing
// can test from 1 to 4 paddles at a time (except -free only tests the first one given)
// can run on pc or raspi
//  sudo ./iow100test.`uname -m` [-delayus microsecs] [-free] serialnumbers ...
//   -delayus : usec delay after write before read (default 1000)
//   -free : test free-standing iow100 module not plugged into panel

#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <string.h>
#include <unistd.h>

#include "myiowkit.h"
#include "readprompt.h"

// tell io-warrior-100 to set all open-drain outputs to 1 (high impeadance)
// this will let us read any signal sent to the pins
static IOWKIT100_IO_REPORT const allonepins = { 0, { 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255, 255 } };

// ask io-warrior-100 for state of all pins
static IOWKIT100_SPECIAL_REPORT const reqallpins[] = { 255 };

#define MAXTESTIOWHS 4
static char const *testsns[MAXTESTIOWHS];
static int ntestiowhs;
static IOWKIT_HANDLE testiowhs[MAXTESTIOWHS];
static sigset_t sigintmask;
static uint32_t delayus;

static void each (void *ctx, char const *dn, int pipe, char const *sn, int pid, int rev);
static void writeallpins (IOWKIT_HANDLE iowh, uint64_t allpins);
static uint64_t readallpins (IOWKIT_HANDLE iowh);
static void blocksigint ();
static void allowsigint ();
static uint32_t randuint32 ();
static void freetest ();
static void fulltest ();

int main (int argc, char **argv)
{
    setlinebuf (stdout);

    delayus = 1000;

    bool freeflag = false;

    for (int i = 1; i < argc; i ++) {
        if (strcasecmp (argv[i], "-delayus") == 0) {
            if ((++ i >= argc) || (argv[i][0] == '-')) {
                fprintf (stderr, "missing usec after -delayus\n");
                return 1;
            }
            char *p;
            delayus = strtoul (argv[i], &p, 0);
            if (*p != 0) {
                fprintf (stderr, "bad usec %s\n", argv[i]);
                return 1;
            }
            continue;
        }
        if (strcasecmp (argv[i], "-free") == 0) {
            freeflag = true;
            continue;
        }
        if (argv[i][0] == '-') {
            fprintf (stderr, "bad option %s\n", argv[i]);
            return 1;
        }
        if (ntestiowhs < MAXTESTIOWHS) {
            testsns[ntestiowhs++] = argv[i];
            continue;
        }
        fprintf (stderr, "bad argument %s\n", argv[i]);
        return 1;
    }
    if (ntestiowhs == 0) {
        fprintf (stderr, "missing serialnumber argument\n");
        return 1;
    }

    sigemptyset (&sigintmask);
    sigaddset (&sigintmask, SIGINT);
    sigaddset (&sigintmask, SIGTERM);

    IowKit::list (each, NULL);

    for (int j = 0; j < ntestiowhs; j ++) {
        if (testiowhs[j] == NULL) {
            fprintf (stderr, "did not find %s\n", testsns[j]);
            return 1;
        }
    }

    if (freeflag) freetest ();
             else fulltest ();

    return 0;
}

static void each (void *ctx, char const *dn, int pipe, char const *sn, int pid, int rev)
{
    if (pipe != 0) return;
    printf ("  %s %d: pid=%X sn=%s\n", dn, pipe, pid, sn);
    if (pid != IOWKIT_PRODUCT_ID_IOW100) return;

    IOWKIT_HANDLE iowh = NULL;
    for (int j = 0; j < ntestiowhs; j ++) {
        if (strcasecmp (sn, testsns[j]) == 0) {
            iowh = new IowKit;
            if (! iowh->openbysn (sn)) abort ();
            testiowhs[j] = iowh;
            break;
        }
    }
    if (iowh == NULL) return;

    // write 1s to open-drain all pins
    IOWKIT100_IO_REPORT writeones;
    memset (&writeones, 0, sizeof writeones);
    memset (writeones.Bytes, 0xFFU, sizeof writeones.Bytes);

    blocksigint ();

    iowh->write (IOW_PIPE_IO_PINS, (char *) &writeones, sizeof writeones);

    allowsigint ();
}

// ask io-warrior-100 for state of all pins
static IOWKIT100_SPECIAL_REPORT const reqallpins100[] = { 255 };

// these iow100 pins get bits from 2N7000 input transistors
static uint8_t const inputpinmap100[32] = {
        0026,    // IN  01 => P2.6
        0024,    // IN  02 => P2.4
        0054,    // IN  03 => P5.4
        0065,    // IN  04 => P6.5
        0041,    // IN  05 => P4.1
        0043,    // IN  06 => P4.3
        0104,    // IN  07 => P8.4
        0056,    // IN  08 => P5.6
        0023,    // IN  09 => P2.3
        0025,    // IN  10 => P2.5
        0055,    // IN  11 => P5.5
        0031,    // IN  12 => P3.1
        0047,    // IN  13 => P4.7
        0051,    // IN  14 => P5.1
        0062,    // IN  15 => P6.2
        0011,    // IN  16 => P1.1

        0035,    // IN  17 => P3.5
        0071,    // IN  18 => P7.1
        0075,    // IN  19 => P7.5
        0107,    // IN  20 => P8.7
        0003,    // IN  21 => P0.3
        0007,    // IN  22 => P0.7
        0021,    // IN  23 => P2.1
        0002,    // IN  24 => P0.2
        0032,    // IN  25 => P3.2
        0020,    // IN  26 => P2.0
        0006,    // IN  27 => P0.6
        0036,    // IN  28 => P3.6
        0050,    // IN  29 => P5.0
        0076,    // IN  30 => P7.6
        0072,    // IN  31 => P7.2
        0012     // IN  32 => P1.2
};

// these iow100 pins send bits to the 2N3904 output transistors
static uint8_t const outputpinmap100[32] = {
        0052,    // P5.2 => OUT 01
        0061,    // P6.1 => OUT 02
        0067,    // P6.7 => OUT 03
        0063,    // P6.3 => OUT 04
        0030,    // P3.0 => OUT 05
        0106,    // P8.6 => OUT 06
        0042,    // P4.2 => OUT 07
        0040,    // P4.0 => OUT 08
        0057,    // P5.7 => OUT 09
        0105,    // P8.5 => OUT 10
        0027,    // P2.7 => OUT 11
        0103,    // P8.3 => OUT 12
        0066,    // P6.6 => OUT 13
        0064,    // P6.4 => OUT 14
        0053,    // P5.3 => OUT 15
        0060,    // P6.0 => OUT 16

        0037,    // P3.7 => OUT 17
        0033,    // P3.3 => OUT 18
        0073,    // P7.3 => OUT 19
        0077,    // P7.7 => OUT 20
        0001,    // P0.1 => OUT 21
        0000,    // P0.0 => OUT 22
        0005,    // P0.5 => OUT 23
        0045,    // P4.5 => OUT 24
        0022,    // P2.2 => OUT 25
        0034,    // P3.4 => OUT 26
        0044,    // P4.4 => OUT 27
        0004,    // P0.4 => OUT 28
        0046,    // P4.6 => OUT 29
        0010,    // P1.0 => OUT 30
        0074,    // P7.4 => OUT 31
        0070     // P7.0 => OUT 32
};

// write all 100 pins of the iowarrior-100
static void writeallpins (IOWKIT_HANDLE iowh, uint64_t allpins)
{
    IOWKIT100_IO_REPORT writepins;
    memset (&writepins, 0, sizeof writepins);

    for (int i = 0; i < 8; i ++) {
        writepins.Bytes[i] = allpins;
        allpins >>= 8;
    }
    writepins.Bytes[8]  = (writepins.Bytes[1] & 0xF8) | 0x07;
    writepins.Bytes[1] |= 0xF8;
    writepins.Bytes[9]  = 0xFFU;
    writepins.Bytes[10] = 0xFFU;
    writepins.Bytes[11] = 0xFFU;

    blocksigint ();

    iowh->write (IOW_PIPE_IO_PINS, &writepins, sizeof writepins);

    allowsigint ();

    usleep (delayus);
}

// read all 100 pins of the iowarrior-100
static uint64_t readallpins (IOWKIT_HANDLE iowh)
{
    blocksigint ();

    // tell iow100 to send back what it has on the pins
    //  IOW100 chip doc 5.9.10p17 says send FF 00 00 00 00 ... 00 (total of 63 00s, overall total 64) to channel #1
    //  IOW_PIPE_SPECIAL_MODE = 1, sizeof reqallpins = 64
    iowh->write (IOW_PIPE_SPECIAL_MODE, &reqallpins, sizeof reqallpins);

    // get state of all the iow100 pins
    //  IOW100 chip doc 5.9.10p17 says read 64 bytes from channel #1, get FF port0 port1 ... port11 ... 00 (overall total 64 bytes)
    //  IOW_PIPE_SPECIAL_MODE = 1, sizeof reqallpins = 64
    IOWKIT100_SPECIAL_REPORT vfyallpins;
    memset (&vfyallpins, 0, sizeof vfyallpins);
    iowh->read (IOW_PIPE_SPECIAL_MODE, &vfyallpins, sizeof vfyallpins);
    if (vfyallpins.ReportID != 255) {
        fprintf (stderr, "readallpins: got report id %02X reading pins\n", vfyallpins.ReportID);
        abort ();
    }

    allowsigint ();

    vfyallpins.Bytes[1] = (vfyallpins.Bytes[8] & 0xF8) | (vfyallpins.Bytes[1] & 0x07);

    uint64_t allpins = 0;
    for (int i = 8; -- i >= 0;) {
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

static uint64_t getnowns ()
{
    struct timespec nowts;
    if (clock_gettime (CLOCK_MONOTONIC, &nowts) < 0) abort ();
    return nowts.tv_sec * 1000000000ULL + nowts.tv_nsec;
}

// iow100 is not plugged into paddle
static void freetest ()
{
    // send out random 64-bit numbers and read them back
    // tests going to the iow100 pins and reading back
    uint32_t error = 0;
    uint32_t count = 0;
    while (true) {
        uint64_t sendval = (((uint64_t) randuint32 ()) << 32) | randuint32 ();
        writeallpins (testiowhs[0], sendval);

        uint64_t readval = readallpins (testiowhs[0]);

        printf ("%10u %10u:  %016llX  %016llX  %016llX", ++ count, error,
            (unsigned long long) sendval, (unsigned long long) readval, (unsigned long long) (sendval ^ readval));

        if (readval != sendval) {
            printf ("  **MISMATCH**");
            error ++;
        }
        printf ("\n");
    }
}

struct Paddle {
    IOWKIT_HANDLE iowh;
    uint32_t error;
    uint32_t sendval;
    uint32_t readval;
};

// iow100 is plugged into paddle but paddle isn't plugged into anything
// can test multiple paddles
// reads of multiple paddles are overlapped to save time
static void fulltest ()
{
    Paddle pads[ntestiowhs];
    memset (pads, 0, sizeof pads);
    for (int j = 0; j < ntestiowhs; j ++) {
        pads[j].iowh = testiowhs[j];
    }

    // send out random 32-bit numbers and read them back
    // tests going through the output 2N3904s and coming back through the input 2N7000s
    uint32_t error = 0;
    uint32_t count = 0;
    uint64_t longest = 0;
    while (true) {
        if (++ count % 10000 == 0) longest = 0;
        blocksigint ();

        // write random 32-bit numbers to each paddle
        uint64_t t0 = getnowns ();
        for (int j = 0; j < ntestiowhs; j ++) {
            Paddle *pad = &pads[j];
            uint32_t val = pad->sendval = randuint32 ();
            IOWKIT100_IO_REPORT writepins;
            memset (&writepins, 0, sizeof writepins);
            memset (writepins.Bytes, 0xFFU, sizeof writepins.Bytes);
            for (int i = 0; i < 32; i ++) {
                if (val & 1) {  // output 2N3904 inverts
                    uint32_t p = outputpinmap100[i];
                    writepins.Bytes[p/8] &= ~ (1ULL << (p % 8));
                }
                val /= 2;
            }
            pad->iowh->write (IOW_PIPE_IO_PINS, &writepins, sizeof writepins);
        }
        usleep (delayus);
        uint64_t t1 = getnowns ();

        // read values back, maybe do a couple retries to get same value in case of straggler
        int nreads = 0;
        while (true) {
            uint64_t t2 = getnowns ();

            // tell iow100s to send back what they have on their pins
            for (int j = 0; j < ntestiowhs; j ++) {
                Paddle *pad = &pads[j];
                pad->iowh->write (IOW_PIPE_SPECIAL_MODE, &reqallpins, sizeof reqallpins);
            }

            // read iow100 pins and extract 32 inputs from each
            bool diff = false;
            for (int j = 0; j < ntestiowhs; j ++) {
                Paddle *pad = &pads[j];
                IOWKIT100_SPECIAL_REPORT readpins;
                memset (&readpins, 0, sizeof readpins);
                pad->iowh->read (IOW_PIPE_SPECIAL_MODE, &readpins, sizeof readpins);
                if (readpins.ReportID != 255) {
                    fprintf (stderr, "fulltest: got report id %02X reading pins\n", readpins.ReportID);
                    abort ();
                }
                uint32_t val = 0;
                for (int i = 32; -- i >= 0;) {
                    uint8_t p = inputpinmap100[i];
                    val += val + ((readpins.Bytes[p/8] >> (p % 8)) & 1);
                }
                pad->readval = ~ val;
                diff |= (pad->sendval != pad->readval);
            }

            uint64_t t3 = getnowns ();
            uint64_t delta = t3 - t0;
            ++ nreads;
            if (! diff) {
                if ((longest < delta) || (nreads > 1)) {
                    if (longest < delta) longest = delta;
                    printf ("%10u %10u:  nreads=%u  delta=%7llu  sendtime=%7llu  readtime=%7llu\n",
                        count, error, nreads, (unsigned long long) longest, (unsigned long long) (t1 - t0), (unsigned long long) (t3 - t2));
                }
                break;
            }
            if (nreads > 3) {
                printf ("%10u %10u:  **MISMATCH**\n", count, ++ error);
                break;
            }
        }
        allowsigint ();
    }
}
