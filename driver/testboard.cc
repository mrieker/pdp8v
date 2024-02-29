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

/**
 * Program runs on the raspberry pi to test arbitrary set of boards.
 * The paddles must be plugged in, but it does not use them, so it can run at fast speeds.
 *
 *  export addhz=<addhz>
 *  export cpuhz=<cpuhz>
 *  ./testboard [-csrclib] [-mintimes] [-pause] [-printgpio] [-printstate] <modname> ...
 *
 *       <addhz> : specify cpu frequency for add cycles (default cpuhz Hz)
 *       <cpuhz> : specify cpu frequency for all other cycles (default DEFCPUHZ Hz)
 *      -csrclib : use compiled-in c-source for simulator (fast)
 *                 otherwise use netgen in a thread for tcl-based sim (slow)
 *      -pause : pause in mid-cycle (with clock still high)
 *      -printgpio : print gpio reads and writes
 *      -printstate : print state info
 *
 *      <modname> gives names of boards assumed to be connected in addition to the rpi board
 *        any combo of acl, alu, ma, pc, seq
 */

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "disassemble.h"
#include "gpiolib.h"
#include "miscdefs.h"
#include "rdcyc.h"
#include "readprompt.h"

#define ever (;;)
#define EXAM(ptr,name) ((ptr != NULL) ? *ptr : (examine (name, 1) != 0))

struct Module {
    char const *name;
    bool selected;
};

#define NMODULES 6
static Module modules[NMODULES] = {
    { "acl" },
    { "alu" },
    { "ma"  },
    { "pc"  },
    { "rpi", true },
    { "seq" } };

static char const *const mnes[] = { "AND", "TAD", "ISZ", "DCA", "JMS", "JMP", "IOT", "GRP" };
static uint64_t seed = 0x123456789ABCDEF0ULL;

static bool pauseit;
static bool printgp;
static bool printst;
static GpioLib *simulatr;
static PhysLib *hardware;
static uint32_t cycleno;

static bool *_fetch2d;
static bool *fetch1q;
static bool *fetch2q;
static bool *defer1q;
static bool *defer2q;
static bool *defer3q;
static bool *exec1q;
static bool *exec2q;
static bool *exec3q;
static bool *_intak;

static void doareset ();
static uint32_t comparegpios (uint32_t ignore);
static bool justprintstate (char *pmt);
static bool printstate ();
static uint32_t examine (char const *varname, int width);
static uint16_t randuint16 ();
static void writegpios (bool wdata, uint32_t pins);
static void *mintimesthread (void *dummy);
static void sighandler (int signum);
static void exithandler ();

int main (int argc, char **argv)
{
    bool csrclib = false;
    bool mintimes = false;

    setlinebuf (stdout);

    for (int i = 0; ++ i < argc;) {
        if (strcasecmp (argv[i], "-csrclib") == 0) {
            csrclib = true;
            continue;
        }
        if (strcasecmp (argv[i], "-mintimes") == 0) {
            mintimes = true;
            continue;
        }
        if (strcasecmp (argv[i], "-pause") == 0) {
            pauseit = true;
            printst = true;
            continue;
        }
        if (strcasecmp (argv[i], "-printgpio") == 0) {
            printgp = true;
            continue;
        }
        if (strcasecmp (argv[i], "-printstate") == 0) {
            printst = true;
            continue;
        }
        if (argv[i][0] == '-') {
            fprintf (stderr, "testboard: unknown option %s\n", argv[i]);
            return 1;
        }
        for (int j = 0; j < NMODULES; j ++) {
            if (strcasecmp (argv[i], modules[j].name) == 0) {
                modules[j].selected = true;
                goto gotmod;
            }
        }
        fprintf (stderr, "testboard: unknown parameter %s\n", argv[i]);
        return 1;
    gotmod:;
    }

    // make sure we close gpio on exit
    signal (SIGABRT, sighandler);
    signal (SIGHUP,  sighandler);
    signal (SIGINT,  sighandler);
    signal (SIGTERM, sighandler);

    // close gpio on exit
    atexit (exithandler);

    // access tube circuitry and simulator
    std::string modnamestr;
    for (int i = 0; i < NMODULES; i ++) {
        Module *mod = &modules[i];
        if (mod->selected) modnamestr.append (mod->name);
    }
    char const *modnamecst = modnamestr.c_str ();
    hardware = new PhysLib ();
    simulatr = csrclib ? (GpioLib *) new CSrcLib (modnamecst) : (GpioLib *) new PipeLib (modnamecst);
    hardware->open ();
    simulatr->open ();

    // open-collector all paddle outputs so they don't interfere with backplane signals
    uint32_t padinits[NPADS];
    memset (padinits, 0, sizeof padinits);
    hardware->writepads (padinits, padinits);

    // tell sim all paddles are driving ones to the abcd connectors
    memset (padinits, -1, sizeof padinits);
    simulatr->writepads (padinits, padinits);

    _fetch2d = simulatr->getvarbool ("_fetch2d/seqcirc", 0);
    fetch1q  = simulatr->getvarbool ("fetch1q", 0);
    fetch2q  = simulatr->getvarbool ("fetch2q", 0);
    defer1q  = simulatr->getvarbool ("defer1q", 0);
    defer2q  = simulatr->getvarbool ("defer2q", 0);
    defer3q  = simulatr->getvarbool ("defer3q", 0);
    exec1q   = simulatr->getvarbool ("exec1q", 0);
    exec2q   = simulatr->getvarbool ("exec2q", 0);
    exec3q   = simulatr->getvarbool ("exec3q", 0);
    _intak   = simulatr->getvarbool ("_intak", 0);

    // print cpu cyclecount once per minute
    if (mintimes) {
        pthread_t pid;
        int rc = pthread_create (&pid, NULL, mintimesthread, NULL);
        if (rc != 0) ABORT ();
        pthread_detach (pid);
    }

    // reset circuitry then start cycling
    doareset ();

    int hdweinit = 0;
    int nullstatecount = 0;
    uint16_t lastopcode = 0;
    uint32_t syncintreq = 0;
    for ever {

        // invariant:
        //  clock has been low for half cycle
        //  G_DENA has been asserted for half cycle so we can read MDL,MD coming from cpu

        // get input signals just before raising clock
        // ignore verifying LINK,DATA if the 'CLA CLL' hasn't taken effect yet
        uint32_t hdwsample = comparegpios ((hdweinit < 2) ? G_LINK | G_DATA : 0);

        // see if we are about to kerchunk into FETCH2 state
        bool isfetch2 = ! EXAM (_fetch2d, "_fetch2d/seqcirc");

        // raise clock then wait for half a cycle - kerchunks hardware and simulator into middle of new state
        if (printst) printf ("------------\n");
        if (isfetch2) {
            // cosmetic only - keep old opcode going out IR latch until new opcode ready
            // otherwise, could just use the else clause for everything
            writegpios (true, G_CLOCK | syncintreq | lastopcode * G_DATA0);
        } else {
            writegpios (false, G_CLOCK | syncintreq);
        }

        // we are now halfway through the cycle with the clock still high
        if (!printstate ()) nullstatecount = 0;

        // if 3 null states in a row, we're stuck so reset and then keep going
        else if (++ nullstatecount > 3) {
            if (printst) {
                printf ("------------\n");
                printf ("            NULL STATE RESET\n");
            }
            doareset ();
            continue;
        }

        // process the signal sample from just before raising clock

        // if acknowledging interrupt, clear interrupt request
        if (hdwsample & G_IAK) syncintreq = 0;

        // if it wants to read memory or io register, send a random number
        if (hdwsample & (G_READ | G_IOIN)) {
            ASSERT (G_LINK == G_DATA + G_DATA0);
            ASSERT (G_IOS  == G_LINK * 2);
            uint16_t data = (hdweinit < 2) ? 07300 : (randuint16 () & 037777);
                                                            // 'CLA CLL' if first fetch, else random
            if (isfetch2) lastopcode = data & 07777;        // if fetch2, this is the opcode being fetched
            if (printst) {
                if (hdwsample & G_READ) {
                    printf ("            mid-cycle:               MQ=%04o", data & 07777);
                    if (isfetch2) {
                        uint16_t pc = examine ("pcq", 12);
                        printf ("  %s", disassemble (data & 07777, pc).c_str ());
                    }
                    putchar ('\n');
                }
                if (hdwsample & G_IOIN) {
                    printf ("            mid-cycle: IOS=%u  MQL=%u  MQ=%04o\n", (data >> 13) & 1, (data >> 12) & 1, data & 07777);
                }
            }

            // drop clock, send data out, including link and ioskip
            // wait half cycle so data has time to soak into tubes (setup time)
            writegpios (true, (data * G_DATA0) | syncintreq);

            // get input signals just before raising clock
            // ignore verifying LINK if the 'CLA CLL' hasn't taken effect yet
            // data should match because we are still sending it out to processor
            hdwsample = comparegpios ((hdweinit < 2) ? G_LINK : 0);

            // raise clock then wait for half a cycle
            // keep sending data out (hold time)
            if (printst) printf ("------------\n");
            writegpios (true, G_CLOCK | (data * G_DATA0) | syncintreq);
            printstate ();
        }

        // update irq pin in middle of cycle so it has time to soak in by the next clock up time
        if (syncintreq == 0) syncintreq = (randuint16 () & 15) ? 0 : G_IRQ;

        // drop the clock and enable link,data bus read
        // then wait half a clock cycle while clock is low
        writegpios (false, syncintreq);

        // ignore LINK and DATA coming from hardware for first couple cycles
        // we send a 'CLA CLL' instruction first but it has to take effect
        if (hdweinit < 2) hdweinit ++;
    }
}

static void doareset ()
{
    hardware->doareset ();
    simulatr->doareset ();
    if (printst) printf ("------------\n");
    printstate ();
}

// print what state the simulator is in, in the middle of the cycle
// returns true iff null state
static bool printstate ()
{
    bool nullstate;

    if (printst) {
        char pmt[320];
        nullstate = justprintstate (pmt);

        if (pauseit) {
            while (true) {
                strcat (pmt, " > ");
                char const *line = readprompt (pmt);
                if (line == NULL) {
                    putchar ('\n');
                    exit (0);
                }
                char str[240];
                char *q = str;
                for (char c; (c = *line) != 0; line ++) {
                    if (c > ' ') *(q ++) = c;
                    if (q >= str + sizeof str - 1) break;
                }
                if (q == str) break;
                *q = 0;
                uint32_t value;
                int width = simulatr->examine (str, &value);
                if (width <= 0) {
                    sprintf (pmt, "unknown %s", str);
                    continue;
                }
                q = pmt + sprintf (pmt, "%s =", str);
                while (-- width >= 0) {
                    *(q ++) = ' ';
                    *(q ++) = '0' + ((value >> width) & 1);
                    if ((width > 0) && (width % 3 == 0)) *(q ++) = ' ';
                }
                *q = 0;
            }
        } else {
            printf ("%s\n", pmt);
        }
    } else {
        nullstate =
            ! EXAM (fetch1q, "fetch1q") &&
            ! EXAM (fetch2q, "fetch2q") &&
            ! EXAM (defer1q, "defer1q") &&
            ! EXAM (defer2q, "defer2q") &&
            ! EXAM (defer3q, "defer3q") &&
            ! EXAM (exec1q, "exec1q") &&
            ! EXAM (exec2q, "exec2q") &&
            ! EXAM (exec3q, "exec3q") &&
              EXAM (_intak, "_intak");
    }

    cycleno ++;

    return nullstate;
}

// examine var assumed to exist, puque if not
static uint32_t examine (char const *varname, int width)
{
    uint32_t value;
    int rc = simulatr->examine (varname, &value);
    if (rc != width) {
        fprintf (stderr, "testboard: error %d reading var %s width %d from simulator\n", rc, varname, width);
        ABORT ();
    }
    return value;
}

// read GPIO signals from hardware and from simulator at end of cycle
// compare and print error message if different
static uint32_t comparegpios (uint32_t ignore)
{
    uint32_t hdwsample = hardware->readgpio () & G_ALL;
    uint32_t simsample = simulatr->readgpio () & G_ALL;

    if (printgp) {
        printf (" readgpios:");
        if (hdwsample & G_JUMP)  printf (" jump");
        if (hdwsample & G_IOIN)  printf (" ioin");
        if (hdwsample & G_DFRM)  printf (" dfrm");
        if (hdwsample & G_READ)  printf (" read");
        if (hdwsample & G_WRITE) printf (" write");
        if (hdwsample & G_IAK)   printf (" intak");
        if (hdwsample & G_DENA)  printf (" link=%o", (hdwsample / G_LINK) & 1);
        if (hdwsample & G_DENA)  printf (" data=%04o", (hdwsample % G_DATA) / G_DATA0);
        printf ("\n");
    }

    if ((hdwsample & ~ ignore) != (simsample & ~ ignore)) {
        std::string hdwstring = GpioLib::decogpio (hdwsample);
        std::string simstring = GpioLib::decogpio (simsample);
        fprintf (stderr, "hdwsample=%08X %s\n", hdwsample, hdwstring.c_str ());
        fprintf (stderr, "simsample=%08X %s\n", simsample, simstring.c_str ());
        char pmt[320];
        justprintstate (pmt);
        fprintf (stderr, "simulator state:\n%s\n", pmt);
        ABORT ();
    }

    if (printst) {
        std::string hdwstring = GpioLib::decogpio (hdwsample);
        printf ("            end-cycle: %s\n", hdwstring.c_str ());
    }

    return hdwsample;
}

// compute state string
//  output:
//   fills in 320-char pmt buffer
//   returns true iff null state (no state ff set)
static bool justprintstate (char *pmt)
{
    char *p, str[240];

    p = str;

    if (EXAM (fetch1q, "fetch1q")) p += sprintf (p, " fetch1");
    if (EXAM (fetch2q, "fetch2q")) p += sprintf (p, " fetch2");
    if (EXAM (defer1q, "defer1q")) p += sprintf (p, " defer1");
    if (EXAM (defer2q, "defer2q")) p += sprintf (p, " defer2");
    if (EXAM (defer3q, "defer3q")) p += sprintf (p, " defer3");
    if (EXAM (exec1q, "exec1q"))   p += sprintf (p, " exec1");
    if (EXAM (exec2q, "exec2q"))   p += sprintf (p, " exec2");
    if (EXAM (exec3q, "exec3q"))   p += sprintf (p, " exec3");
    if (! EXAM (_intak, "_intak")) p += sprintf (p, " intak");

    bool nullstate = (p == str);

    if (examine ("arith1q/seqcirc", 1)) p += sprintf (p, " arith1q");
    if (examine ("arith2q/seqcirc", 1)) p += sprintf (p, " arith2q");
    if (examine ("and2q/seqcirc", 1))   p += sprintf (p, " and2q");
    if (examine ("tad2q/seqcirc", 1))   p += sprintf (p, " tad2q");
    if (examine ("tad3q/seqcirc", 1))   p += sprintf (p, " tad3q");
    if (examine ("isz1q/seqcirc", 1))   p += sprintf (p, " isz1q");
    if (examine ("isz2q/seqcirc", 1))   p += sprintf (p, " isz2q");
    if (examine ("isz3q/seqcirc", 1))   p += sprintf (p, " isz3q");
    if (examine ("dca1q/seqcirc", 1))   p += sprintf (p, " dca1q");
    if (examine ("dca2q/seqcirc", 1))   p += sprintf (p, " dca2q");
    if (examine ("jms1q/seqcirc", 1))   p += sprintf (p, " jms1q");
    if (examine ("jms2q/seqcirc", 1))   p += sprintf (p, " jms2q");
    if (examine ("jms3q/seqcirc", 1))   p += sprintf (p, " jms3q");
    if (examine ("jmp1q/seqcirc", 1))   p += sprintf (p, " jmp1q");
    if (examine ("iot1q/seqcirc", 1))   p += sprintf (p, " iot1q");
    if (examine ("iot2q/seqcirc", 1))   p += sprintf (p, " iot2q");
    if (examine ("opr1q/seqcirc", 1))   p += sprintf (p, " opr1q");
    if (examine ("grpa1q/seqcirc", 1))  p += sprintf (p, " grpa1q");
    if (examine ("grpb1q/seqcirc", 1))  p += sprintf (p, " grpb1q");

    *p = 0;

    uint32_t lreg  = examine ("lnq",  1);
    uint32_t acreg = examine ("acq", 12);
    uint32_t mareg = examine ("maq", 12);
    uint32_t pcreg = examine ("pcq", 12);
    uint32_t ireg  = examine ("irq",  3);

    sprintf (pmt, "%10u : L=%o  AC=%04o  MA=%04o  PC=%04o  IR=%o/%s %s",
        cycleno, lreg, acreg, mareg, pcreg, ireg, mnes[ireg], str);

    return nullstate;
}

// generate a random number
static uint16_t randuint16 ()
{
    uint16_t randval = 0;

    for (int i = 0; i < 16; i ++) {

        // https://www.xilinx.com/support/documentation/application_notes/xapp052.pdf
        uint64_t xnor = ~ ((seed >> 63) ^ (seed >> 62) ^ (seed >> 60) ^ (seed >> 59));
        seed = (seed << 1) | (xnor & 1);

        randval += randval + (seed & 1);
    }

    return randval;
}

// write same value to both hardware and simulator gpio pins
// then wait half cycle for pins to soak in
//  input:
//   wdata = false: just writing control pins, data pins are put in proc->raspi direction
//            true: writing control & data pins, data pins are put in raspi->proc direction
#define HL(bit) ((bit) ? 'H' : 'L')
static void writegpios (bool wdata, uint32_t pins)
{
    if (printgp) {
        printf ("writegpios: reset=%c clock=%c ioskp=%c intrq=%c", HL(pins & G_RESET), HL(pins & G_CLOCK), HL(pins & G_IOS), HL(pins & G_IRQ));
        if (wdata) printf (" l.data=%o.%04o", (pins / G_LINK) & 1, (pins & G_DATA) / G_DATA0);
        printf ("\n");
    }
    hardware->writegpio (wdata, pins);
    simulatr->writegpio (wdata, pins);
    hardware->halfcycle (true);
    simulatr->halfcycle ();
}

// runs in background to print cycle rate at beginning of every minute for testing
static void *mintimesthread (void *dummy)
{
    pthread_cond_t cond;
    pthread_mutex_t lock;
    struct timespec nowts, waits;

    pthread_cond_init (&cond, NULL);
    pthread_mutex_init (&lock, NULL);
    pthread_mutex_lock (&lock);

    if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) ABORT ();
    uint32_t lastcycs = cycleno;
    uint32_t lastsecs = nowts.tv_sec;

    waits.tv_nsec = 0;

    while (true) {
        int rc;
        waits.tv_sec = (lastsecs / 60 + 1) * 60;
        do rc = pthread_cond_timedwait (&cond, &lock, &waits);
        while (rc == 0);
        if (rc != ETIMEDOUT) ABORT ();
        if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) ABORT ();
        uint32_t cycs = cycleno;
        uint32_t secs = nowts.tv_sec;
        fprintf (stderr, "testboard: %02d:%02d:%02d  %10u cycles  avg %8u Hz  %6.3f uS\n",
                    secs / 3600 % 24, secs / 60 % 60, secs % 60,
                    cycs, (cycs - lastcycs) / (secs - lastsecs),
                    (secs - lastsecs) * 1000000.0 / (cycs - lastcycs));
        lastcycs = cycs;
        lastsecs = secs;
    }
    return NULL;
}

// signal that terminates process, do exit() so exithandler() gets called
static void sighandler (int signum)
{
    fprintf (stderr, "testboard: terminated for signal %d\n", signum);
    exit (1);
}

// exiting
static void exithandler ()
{
    fprintf (stderr, "testboard: exiting\n");

    // close gpio access
    if (hardware != NULL) hardware->close ();
    if (simulatr != NULL) simulatr->close ();
}
