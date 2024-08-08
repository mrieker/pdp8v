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
 * Program runs on the raspberry pi to control the computer.
 * Can also be run on a zynq for testing the test program.
 *
 *  export addhz=<addhz>
 *  export cpuhz=<cpuhz>
 *  ./testrpiseqboard [-csrclib] [-paddles] [-resetonerror] [-verbose] [-zynqlib]
 *
 *       <addhz> : specify cpu frequency for add cycles (default cpuhz Hz)
 *       <cpuhz> : specify cpu frequency for all other cycles (default DEFCPUHZ Hz)
 *
 *      -paddles : paddles are connected up
 *      -resetonerror : reset on error (otherwise abort on error)
 *      -verbose : print information each cycle
 *      -zynqlib : running on Zynq with circuit loaded
 */

#include <errno.h>
#include <fcntl.h>
#include <map>
#include <math.h>
#include <pthread.h>
#include <sched.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "abcd.h"
#include "gpiolib.h"
#include "miscdefs.h"
#include "pindefs.h"
#include "rdcyc.h"

#define NSAVEDCYCLES 16

enum State {
    RESET1, FETCH1, FETCH2, DEFER1, DEFER2, DEFER3, INTAK1,
    AND1, AND2, TAD1, TAD2, TAD3, ISZ1, ISZ2, ISZ3, DCA1,
    DCA2, JMS1, JMS2, JMS3, JMP1, JMP2, IOT1, IOT2, GRP1
};

struct SavedCycle {
    uint64_t cycleno;
    uint32_t gpiocon;
    State state;
};

static pthread_t mintimestid;
static pthread_cond_t mintimescond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mintimeslock = PTHREAD_MUTEX_INITIALIZER;

bool aborting;
bool paddles;
bool resetonerror;
bool verbose;
GpioLib *gpio;
SavedCycle savedcycles[NSAVEDCYCLES];
State state;
uint32_t syncintreq;
uint64_t cyclecount;

static void endofcycle ();
static bool verifygpio (uint32_t sample, uint32_t expect);
static char const *statestr (State s);
static uint16_t randuint16 (int nbits);
static void *mintimesthread (void *dummy);
static void sighandler (int signum);
static void exithandler ();

int main (int argc, char **argv)
{
    bool csrclib = false;
    bool zynqlib = false;

    setlinebuf (stdout);

    for (int i = 0; ++ i < argc;) {
        if (strcasecmp (argv[i], "-csrclib") == 0) {
            csrclib = true;
            zynqlib = false;
            continue;
        }
        if (strcasecmp (argv[i], "-paddles") == 0) {
            paddles = true;
            continue;
        }
        if (strcasecmp (argv[i], "-resetonerror") == 0) {
            resetonerror = true;
            continue;
        }
        if (strcasecmp (argv[i], "-verbose") == 0) {
            verbose = true;
            continue;
        }
        if (strcasecmp (argv[i], "-zynqlib") == 0) {
            csrclib = false;
            zynqlib = true;
            continue;
        }
        fprintf (stderr, "unknown argument %s\n", argv[i]);
        return 1;
    }

    // access cpu circuitry
    // either physical circuit (tubes or de0) via gpio pins
    // ...or zynq fpga circuit
    gpio = csrclib ? (GpioLib *) new CSrcLib ("rpiseq") : zynqlib ? (GpioLib *) new ZynqLib ("rpiseq") : (GpioLib *) new PhysLib ();
    gpio->open ();

    // always call haspads() so it can float the paddles
    if (! gpio->haspads () && paddles) {
        fprintf (stderr, "-paddles given but no paddles present\n");
        return 1;
    }

    // if paddles present, clamp acqzero to zero so we will get TAD states
    if (paddles) {
        uint32_t masks[NPADS], pinss[NPADS];
        memset (masks, -1, sizeof masks);
        memset (pinss, -1, sizeof pinss);
        pinss[1] = ~ B_acqzero;
        gpio->writepads (masks, pinss);
    }

    // print cpu cyclecount once per minute
    setmintimes (true);

    // catch normal termination signals to clean up
    signal (SIGABRT, sighandler);
    signal (SIGHUP,  sighandler);
    signal (SIGINT,  sighandler);
    signal (SIGTERM, sighandler);

    atexit (exithandler);

    // reset cpu circuit
resetit:;
    gpio->writegpio (false, G_RESET);
    gpio->halfcycle (false);
    gpio->halfcycle (false);
    gpio->halfcycle (false);

    uint16_t opcode = 0;
    state = RESET1;

    while (! aborting) {

        // drop clock (or reset) then wait half cycle to end of cycle
        gpio->writegpio (false, syncintreq);
        gpio->halfcycle (false);

        // that cycle is now complete
        endofcycle ();

        // invariant:
        //  clock has been low for half cycle in state given by 'state' variable
        //  G_DENA has been asserted for half cycle so we can read MDL,MD coming from tubes

        // get input signals from tubes just before raising clock
        uint32_t sample = gpio->readgpio () & G_ALL;

        // see if those pins are consistent with the state we are at the end of
        // and determine the next state
        bool good;
        switch (state) {
            case RESET1:
            case FETCH1: {
                good = verifygpio (sample, G_DENA + G_READ);
                state = FETCH2;
                break;
            }
            case DEFER1: {
                good = verifygpio (sample, G_DENA + G_WRITE + G_READ);
                state = DEFER2;
                break;
            }
            case DEFER3: {
                good = verifygpio (sample, G_DENA);
                switch (opcode >> 9) {
                    case 0: state = AND1; break;
                    case 1: state = paddles ? TAD1 : AND1; break;
                    case 2: state = ISZ1; break;
                    case 3: state = DCA1; break;
                    case 4: state = JMS1; break;
                    case 5: state = JMP1; break;
                    default: ABORT ();
                }
                break;
            }
            case INTAK1: {
                good = verifygpio (sample, G_DENA + G_WRITE + G_IAK);
                state = JMS2;
                break;
            }
            case AND1: {
                good = verifygpio (sample, G_DENA + G_READ + ((opcode & 0400) ? G_DFRM : 0));
                state = AND2;
                break;
            }
            case TAD1: {
                good = verifygpio (sample, G_DENA + G_READ + ((opcode & 0400) ? G_DFRM : 0));
                state = TAD2;
                break;
            }
            case TAD3: {
                good = verifygpio (sample, G_DENA);
                state = syncintreq ? INTAK1 : FETCH1;
                break;
            }
            case ISZ1: {
                good = verifygpio (sample, G_DENA + G_WRITE + G_READ + ((opcode & 0400) ? G_DFRM : 0));
                state = ISZ2;
                break;
            }
            case ISZ3: {
                good = verifygpio (sample, G_DENA);
                state = syncintreq ? INTAK1 : FETCH1;
                break;
            }
            case DCA1: {
                good = verifygpio (sample, G_DENA + G_WRITE + ((opcode & 0400) ? G_DFRM : 0));
                state = DCA2;
                break;
            }
            case DCA2: {
                good = verifygpio (sample, G_DENA);
                state = syncintreq ? INTAK1 : FETCH1;
                break;
            }
            case JMS1: {
                good = verifygpio (sample, G_DENA + G_WRITE + G_JUMP);
                state = JMS2;
                break;
            }
            case JMS2: {
                good = verifygpio (sample, G_DENA);
                state = JMS3;
                break;
            }
            case JMS3: {
                if (syncintreq) {
                    good = verifygpio (sample, G_DENA);
                    state = INTAK1;
                } else {
                    good = verifygpio (sample, G_DENA + G_READ);
                    state = FETCH2;
                }
                break;
            }
            case JMP1: {
                if (syncintreq) {
                    good = verifygpio (sample, G_DENA + G_JUMP);
                    state = INTAK1;
                } else {
                    good = verifygpio (sample, G_DENA + G_READ + G_JUMP);
                    state = FETCH2;
                }
                break;
            }
            case IOT1: {
                good = verifygpio (sample, G_DENA + G_IOIN);
                state = IOT2;
                break;
            }
            case GRP1: {
                if (! (opcode & 00400)) {
                    good = verifygpio (sample, G_DENA);
                    state = syncintreq ? INTAK1 : FETCH1;
                } else if (syncintreq) {
                    good = verifygpio (sample, G_DENA + G_READ);
                    state = INTAK1;
                } else {
                    good = verifygpio (sample, G_DENA);
                    state = FETCH2;
                }
                break;
            }
            default: ABORT ();
        }
        if (! good) goto goterror;

        // raise clock then wait for half a cycle for the new state to soak into tubes
        // occaisionally do a reset instead, forcing us to FETCH1
        if (randuint16 (4) == 0) {
            gpio->writegpio (false, G_RESET);
            sample = 0;
            state = RESET1;
            syncintreq = 0;
        } else {
            gpio->writegpio (false, G_CLOCK | syncintreq);
        }
        gpio->halfcycle (false);

        // we are now halfway through the cycle with the clock still high

        // process the signal sample from just before raising clock

        // - interrupt acknowledge (turns off interrupt request)
        if (sample & G_IAK) {
            syncintreq = 0;
        }

        if (sample & (G_READ | G_IOIN)) {

            // read memory contents or generate IO instruction result
            uint16_t data = randuint16 (14);

            // send data to processor
            ASSERT (G_LINK  == 010000 * G_DATA0);
            ASSERT (G_IOS   == 020000 * G_DATA0);
            uint32_t sample = (data & 037777) * G_DATA0 | syncintreq;

            // drop the clock and start sending skip,link,data to cpu
            // then let data soak into tubes (giving it a setup time)
            gpio->writegpio (true, sample);
            gpio->halfcycle (false);

            // that cycle is now complete
            endofcycle ();

            // determine the next state
            switch (state) {
                case FETCH2: {
                    opcode = data & 07777;
                    if (((opcode & 06000) != 06000) && (opcode & 00400)) {
                        state = DEFER1;
                    } else {
                        switch (opcode >> 9) {
                            case 0: state = AND1; break;
                            case 1: state = paddles ? TAD1 : AND1; break;
                            case 2: state = ISZ1; break;
                            case 3: state = DCA1; break;
                            case 4: state = JMS1; break;
                            case 5: state = JMP1; break;
                            case 6: state = IOT1; break;
                            case 7: state = IOT1; break;    // never gets GRP1 cuz MA=07777
                            default: ABORT ();
                        }
                    }
                    break;
                }
                case DEFER2: {
                    state = DEFER3;
                    break;
                }
                case AND2: {
                    state = syncintreq ? INTAK1 : FETCH1;
                    break;
                }
                case TAD2: {
                    state = TAD3;
                    break;
                }
                case ISZ2: {
                    state = ISZ3;
                    break;
                }
                case IOT2: {
                    state = syncintreq ? INTAK1 : FETCH1;
                    break;
                }
                default: {
                    fprintf (stderr, "bad state after read: %s\n", statestr (state));
                    goto goterror;
                }
            }

            // tell cpu data is ready to be read by raising the clock
            // hold data steady and keep sending data to cpu so it will be clocked in correctly
            gpio->writegpio (true, G_CLOCK | sample);

            // wait for cpu to clock data in (giving it a hold time)
            gpio->halfcycle (false);
        }

        // update irq pin in middle of cycle so it has time to soak in by the next clock up time
        if ((syncintreq == 0) && (state != RESET1) && (randuint16 (4) == 0)) {
            syncintreq = G_IRQ;
        }
    }
    return 0;

goterror:;
    for (int i = NSAVEDCYCLES; -- i > 0;) {
        if ((uint64_t) i > cyclecount) continue;
        int j = (cyclecount + i) % NSAVEDCYCLES;
        fprintf (stderr, "%9llu: %-6s  %08X %s\n",
            (LLU) savedcycles[j].cycleno,
            statestr (savedcycles[i].state),
            savedcycles[j].gpiocon,
            GpioLib::decogpio (savedcycles[j].gpiocon).c_str ());
    }
    if (resetonerror) {
        sleep (1);
        fprintf (stderr, "\nresetting...\n");
        goto resetit;
    }
    ABORT ();
}

static void endofcycle ()
{
    cyclecount ++;

    uint32_t sample = gpio->readgpio ();
    savedcycles[cyclecount%NSAVEDCYCLES].cycleno = cyclecount;
    savedcycles[cyclecount%NSAVEDCYCLES].state   = state;
    savedcycles[cyclecount%NSAVEDCYCLES].gpiocon = sample;

    if (verbose) {
        pthread_mutex_lock (&mintimeslock);
        printf ("================================================================================\n");
        printf ("%9llu: end of state %s\n", (LLU) cyclecount, statestr (state));
        printf ("--------------------------------------------------------------------------------\n");
        if (paddles) {
            ABCD abcd;
            gpio->readpads (abcd.cons);
            abcd.gcon = sample;
            abcd.decode ();
            abcd.printfull<FILE> (fprintf, stdout, false, false, false, false, true, true);
        } else {
            printf ("           %08X  %s\n", sample, GpioLib::decogpio (sample).c_str ());
        }
        printf ("\n");
        pthread_mutex_unlock (&mintimeslock);
    }
}

// check GPIO pins at end of cycle
static bool verifygpio (uint32_t sample, uint32_t expect)
{
    expect |= syncintreq;
    if (sample == expect) return true;
    fprintf (stderr, "%9llu %s:\n", (LLU) cyclecount, statestr (state));
    fprintf (stderr, "    sample=%08X %s\n", sample, GpioLib::decogpio (sample).c_str ());
    fprintf (stderr, "    expect=%08X %s\n", expect, GpioLib::decogpio (expect).c_str ());
    return false;
}

static char const *statestr (State s)
{
    switch (s) {
        case RESET1: return "RESET1";
        case FETCH1: return "FETCH1";
        case FETCH2: return "FETCH2";
        case DEFER1: return "DEFER1";
        case DEFER2: return "DEFER2";
        case DEFER3: return "DEFER3";
        case INTAK1: return "INTAK1";
        case AND1: return "AND1";
        case AND2: return "AND2";
        case TAD1: return "TAD1";
        case TAD2: return "TAD2";
        case TAD3: return "TAD3";
        case ISZ1: return "ISZ1";
        case ISZ2: return "ISZ2";
        case ISZ3: return "ISZ3";
        case DCA1: return "DCA1";
        case DCA2: return "DCA2";
        case JMS1: return "JMS1";
        case JMS2: return "JMS2";
        case JMS3: return "JMS3";
        case JMP1: return "JMP1";
        case JMP2: return "JMP2";
        case IOT1: return "IOT1";
        case IOT2: return "IOT2";
        case GRP1: return "GRP1";
        default: ABORT ();
    }
}

// generate a random number
static uint16_t randuint16 (int nbits)
{
    static uint64_t seed = 0x123456789ABCDEF0ULL;

    uint16_t randval = 0;

    while (-- nbits >= 0) {

        // https://www.xilinx.com/support/documentation/application_notes/xapp052.pdf
        uint64_t xnor = ~ ((seed >> 63) ^ (seed >> 62) ^ (seed >> 60) ^ (seed >> 59));
        seed = (seed << 1) | (xnor & 1);

        randval += randval + (seed & 1);
    }

    return randval;
}

void setmintimes (bool mintimes)
{
    pthread_mutex_lock (&mintimeslock);
    if (mintimes && (mintimestid == 0)) {
        int rc = createthread (&mintimestid, mintimesthread, NULL);
        if (rc != 0) ABORT ();
        pthread_mutex_unlock (&mintimeslock);
        return;
    }
    if (! mintimes && (mintimestid != 0)) {
        pthread_t tid = mintimestid;
        mintimestid = 0;
        pthread_cond_broadcast (&mintimescond);
        pthread_mutex_unlock (&mintimeslock);
        pthread_join (tid, NULL);
        return;
    }
    pthread_mutex_unlock (&mintimeslock);
}

// runs in background to print cycle rate at beginning of every minute for testing
static void *mintimesthread (void *dummy)
{
    struct timespec nowts, waits;

    pthread_mutex_lock (&mintimeslock);

    if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) ABORT ();
    uint64_t lastcycs = 0;
    uint32_t lastsecs = nowts.tv_sec;

    waits.tv_nsec = 0;

    while (mintimestid != 0) {
        waits.tv_sec = (lastsecs / 60 + 1) * 60;
        int rc = pthread_cond_timedwait (&mintimescond, &mintimeslock, &waits);
        if (rc == 0) continue;
        if (rc != ETIMEDOUT) ABORT ();
        if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) ABORT ();
        uint64_t cycs = cyclecount;
        uint32_t secs = nowts.tv_sec;
        fprintf (stderr, "%02d:%02d:%02d  [%11llu cycles  %7llu cps %7.3f uS]\n",
                    secs / 3600 % 24, secs / 60 % 60, secs % 60,
                    (LLU) cycs, (LLU) ((cycs - lastcycs) / (secs - lastsecs)),
                    (secs - lastsecs) * 1000000.0 / (cycs - lastcycs));
        lastcycs = cycs;
        lastsecs = secs;
    }

    pthread_mutex_unlock (&mintimeslock);
    return NULL;
}

// signal that terminates process, do exit() so exithandler() gets called
static void sighandler (int signum)
{
    if (aborting) {
        fprintf (stderr, "terminated for signal %d\n", signum);
        exit (1);
    }
    aborting = true;
}

// exiting
static void exithandler ()
{
    uint64_t cycles = cyclecount;
    fprintf (stderr, "exiting, %llu cycle%s\n", (LLU) cycles, ((cycles == 1) ? "" : "s"));
    gpio->close ();
}
