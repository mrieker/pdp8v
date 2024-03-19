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
 *
 *  export addhz=<addhz>
 *  export cpuhz=<cpuhz>
 *  ./raspictl [-binloader] [-corefile <filename>] [-cpuhz <freq>] [-csrclib] [-cyclelimit <cycles>] [-guihalt] [-haltcont]
 *          [-haltprint] [-haltstop] [-infloopstop] [-linc] [-mintimes] [-nohw] [-os8zap] [-paddles] [-pipelib] [-printinstr]
 *          [-printstate] [-quiet] [-randmem] [-resetonerr] [-rimloader] [-script] [-startpc <address>] [-stopat <address>]
 *          [-watchwrite <address>] [-zynqlib] <loadfile>
 *
 *       <addhz> : specify cpu frequency for add cycles (default cpuhz Hz)
 *       <cpuhz> : specify cpu frequency for all other cycles (default DEFCPUHZ Hz)
 *
 *      -binloader   : load file is in binloader format
 *      -corefile    : specify persistent core file image
 *      -csrclib     : use C-source for circuitry simulation (see csrclib.cc)
 *      -cyclelimit  : specify maximum number of cycles to execute
 *      -guihalt     : used by the GUI.java wrapper to start in halted mode
 *      -haltcont    : HALT instruction just continues
 *      -haltprint   : HALT instruction prints message
 *      -haltstop    : HALT instruction causes exit (else it is 'wait for interrupt')
 *      -infloopstop : infinite loop JMP causes exit
 *      -linc        : process linc instruction set
 *      -mintimes    : print cpu cycle info once a minute
 *      -nohw        : don't use hardware, simulate processor internally
 *      -os8zap      : zap out the OS/8 ISZ/JMP delay loop
 *      -paddles     : check ABCD paddles each cycle
 *      -pipelib     : simulate via pipe connected to NetGen
 *      -printinstr  : print message at beginning of each instruction
 *      -printstate  : print message at beginning of each state
 *      -quiet       : don't print undefined/unsupported opcode messages
 *      -randmem     : supply random opcodes and data for testing
 *      -resetonerr  : reset if error is detected
 *      -rimloader   : load file is in rimloader format
 *      -script      : load file is a TCL script
 *      -startpc     : starting program counter
 *      -stopat      : stop simulating when accessing the address
 *      -watchwrite  : print message if the given address is written to
 *      -zynqlib     : running on Zynq with circuit loaded
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

#include "binloader.h"
#include "extarith.h"
#include "gpiolib.h"
#include "linkloader.h"
#include "memext.h"
#include "memory.h"
#include "miscdefs.h"
#include "rdcyc.h"
#include "rimloader.h"
#include "scncall.h"
#include "script.h"
#include "shadow.h"

#define ever (;;)

bool lincenab;
bool quiet;
bool randmem;
bool scriptmode;
char **cmdargv;
ExtArith extarith;
GpioLib *gpio;
int cmdargc;
MemExt memext;
Shadow shadow;
uint16_t lastreadaddr;
uint32_t watchwrite = 0xFFFFFFFF;
uint16_t *memory;

pthread_cond_t haltcond = PTHREAD_COND_INITIALIZER;
pthread_cond_t haltcond2 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t haltmutex = PTHREAD_MUTEX_INITIALIZER;
uint32_t haltflags;
uint32_t haltsample;

static bool haltcont;
static bool haltprint;
static bool haltstop;
static bool infloopstop;
static uint16_t intreqmask;
static uint16_t lastreaddata;
static uint32_t syncintreq;

static void *mintimesthread (void *dummy);
static void haltcheck ();
static uint16_t group2io (uint32_t opcode, uint16_t input);
static uint16_t randuint16 ();
static void dumpregs ();
static void senddata (uint16_t data);
static uint16_t recvdata (void);
static void nullsighand (int signum);
static void sighandler (int signum);
static void exithandler ();

int main (int argc, char **argv)
{
    bool binldr = false;
    bool csrclib = false;
    bool guihalt = false;
    bool mintimes = false;
    bool nohw = false;
    bool os8zap = false;
    bool pipelib = false;
    bool resetonerr = false;
    bool rimldr = false;
    bool zynqlib = false;
    char const *corename = NULL;
    char const *loadname = NULL;
    char *p;
    int initcount = 0;
    uint16_t initreads[4];
    uint32_t initdfif = 0;
    uint32_t stopataddr = -1;
    uint32_t cyclelimit = 0;

    setlinebuf (stdout);

    for (int i = 0; ++ i < argc;) {
        if (strcasecmp (argv[i], "-binloader") == 0) {
            binldr = true;
            randmem = false;
            rimldr = false;
            scriptmode = false;
            continue;
        }
        if (strcasecmp (argv[i], "-corefile") == 0) {
            if ((++ i >= argc) || (argv[i][0] == '-')) {
                fprintf (stderr, "raspictl: missing file name after -corefile\n");
                return 1;
            }
            corename = argv[i];
            continue;
        }
        if (strcasecmp (argv[i], "-csrclib") == 0) {
            csrclib = true;
            pipelib = false;
            zynqlib = false;
            continue;
        }
        if (strcasecmp (argv[i], "-cyclelimit") == 0) {
            if ((++ i >= argc) || (argv[i][0] == '-')) {
                fprintf (stderr, "raspictl: missing count after -cyclelimit\n");
                return 1;
            }
            cyclelimit = atoi (argv[i]);
            continue;
        }
        if (strcasecmp (argv[i], "-guihalt") == 0) {
            guihalt = true;
            continue;
        }
        if (strcasecmp (argv[i], "-haltcont") == 0) {
            haltcont = true;
            continue;
        }
        if (strcasecmp (argv[i], "-haltprint") == 0) {
            haltprint = true;
            continue;
        }
        if (strcasecmp (argv[i], "-haltstop") == 0) {
            haltstop = true;
            continue;
        }
        if (strcasecmp (argv[i], "-infloopstop") == 0) {
            infloopstop = true;
            continue;
        }
        if (strcasecmp (argv[i], "-linc") == 0) {
            lincenab = true;
            continue;
        }
        if (strcasecmp (argv[i], "-mintimes") == 0) {
            mintimes = true;
            continue;
        }
        if (strcasecmp (argv[i], "-nohw") == 0) {
            nohw = true;
            continue;
        }
        if (strcasecmp (argv[i], "-os8zap") == 0) {
            os8zap = true;
            continue;
        }
        if (strcasecmp (argv[i], "-paddles") == 0) {
            shadow.paddles = true;
            continue;
        }
        if (strcasecmp (argv[i], "-pipelib") == 0) {
            csrclib = false;
            pipelib = true;
            zynqlib = false;
            continue;
        }
        if (strcasecmp (argv[i], "-printinstr") == 0) {
            shadow.printinstr = true;
            continue;
        }
        if (strcasecmp (argv[i], "-printstate") == 0) {
            shadow.printstate = true;
            continue;
        }
        if (strcasecmp (argv[i], "-quiet") == 0) {
            quiet = true;
            continue;
        }
        if ((strcasecmp (argv[i], "-randmem") == 0) && (loadname == NULL)) {
            binldr = false;
            randmem = true;
            rimldr = false;
            scriptmode = false;
            quiet = true;
            initcount = 1;
            initreads[0] = 07300;   // CLA CLL
            continue;
        }
        if (strcasecmp (argv[i], "-resetonerr") == 0) {
            resetonerr = true;
            continue;
        }
        if (strcasecmp (argv[i], "-rimloader") == 0) {
            binldr = false;
            randmem = false;
            rimldr = true;
            scriptmode = false;
            continue;
        }
        if (strcasecmp (argv[i], "-script") == 0) {
            binldr = false;
            randmem = false;
            rimldr = false;
            scriptmode = true;
            continue;
        }
        if (strcasecmp (argv[i], "-startpc") == 0) {
            if ((++ i >= argc) || (argv[i][0] == '-')) {
                fprintf (stderr, "raspictl: missing address after -startpc\n");
                return 1;
            }
            uint16_t startpc = strtoul (argv[i], NULL, 8);
            initdfif = (startpc >> 12) & 7;
            initcount = 3;
            initreads[2] = 07300;   // CLA CLL
            initreads[1] = 05400;   // JMP @0
            initreads[0] = startpc & 07777;
            continue;
        }
        if (strcasecmp (argv[i], "-stopat") == 0) {
            if ((++ i >= argc) || (argv[i][0] == '-')) {
                fprintf (stderr, "raspictl: missing address after -stopat\n");
                return 1;
            }
            stopataddr = strtoul (argv[i], &p, 0);
            if ((*p != 0) || (stopataddr > 0xFFFF)) {
                fprintf (stderr, "raspictl: bad -stopat address '%s'\n", argv[i]);
                return 1;
            }
            continue;
        }
        if (strcasecmp (argv[i], "-watchwrite") == 0) {
            if ((++ i >= argc) || (argv[i][0] == '-')) {
                fprintf (stderr, "raspictl: missing address after -watchwrite\n");
                return 1;
            }
            watchwrite = strtoul (argv[i], NULL, 8);
            continue;
        }
        if (strcasecmp (argv[i], "-zynqlib") == 0) {
            csrclib = false;
            pipelib = false;
            zynqlib = true;
            continue;
        }
        if ((argv[i][0] == '-') && (argv[i][1] != 0)) {
            fprintf (stderr, "raspictl: unknown option %s\n", argv[i]);
            return 1;
        }
        if ((loadname != NULL) || randmem) {
            fprintf (stderr, "raspictl: unknown argument %s\n", argv[i]);
            return 1;
        }

        // load filename, remainder of command line args available to program
        loadname = argv[i];
        cmdargc  = argc - i;
        cmdargv  = argv + i;
        break;
    }

    if ((corename == NULL) && (loadname == NULL) && ! randmem) {
        fprintf (stderr, "raspictl: missing loadfile parameter\n");
        return 1;
    }

    // maybe there's a core image file
    if (corename == NULL) {
        memarray = (uint16_t *) calloc (MEMSIZE, sizeof *memarray);
    } else {
        int memfd = open (corename, O_RDWR | O_CREAT, 0666);
        if (memfd < 0) {
            fprintf (stderr, "raspictl: error opening corefile %s: %m\n", corename);
            return 1;
        }
        if (flock (memfd, LOCK_EX | LOCK_NB) < 0) {
            fprintf (stderr, "raspictl: error locking corefile %s: %m\n", corename);
            return 1;
        }
        if (ftruncate (memfd, MEMSIZE * sizeof *memarray) < 0) {
            fprintf (stderr, "raspictl: error extending corefile %s: %m\n", corename);
            return 1;
        }
        void *memptr = mmap (NULL, MEMSIZE * sizeof *memarray, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0);
        if (memptr == MAP_FAILED) {
            fprintf (stderr, "raspictl: error mmapping corefile %s: %m\n", corename);
            return 1;
        }
        memarray = (uint16_t *) memptr;
    }

    // read loadfile contents into memory
    if (loadname != NULL) {
        if (scriptmode) {
            // -script, run script that can do loading or whatever
            runscript (argv[0], loadname);
        } else {
            FILE *loadfile = fopen (loadname, "r");
            if (loadfile == NULL) {
                fprintf (stderr, "raspictl: error opening loadfile %s: %m\n", loadname);
                return 1;
            }
            if (binldr) {

                // pdp-8 binloader format
                uint16_t start;
                int rc = binloader (loadname, loadfile, &start);
                if (rc != 0) return rc;

                // maybe a start address was given but don't override -startpc
                if ((initcount == 0) && ! (start & 0x8000)) {
                    initdfif = (start >> 12) & 7;
                    initcount = 3;
                    initreads[2] = 07300;           // CLA CLL
                    initreads[1] = 05400;           // JMP @0
                    initreads[0] = start & 07777;
                }
            } else if (rimldr) {

                // pdp-8 rimloader format
                int rc = rimloader (loadname, loadfile);
                if (rc != 0) return rc;
            } else {

                // linker format
                int rc = linkloader (loadname, loadfile);
                if (rc < 0) return -rc;
                if ((initcount == 0) && (rc > 0)) {
                    initdfif = (rc >> 12) & 7;
                    initcount = 3;
                    initreads[2] = 07300;           // CLA CLL
                    initreads[1] = 05400;           // JMP @0
                    initreads[0] = rc & 07777;
                }
            }

            fclose (loadfile);
        }
    }

    // print cpu cyclecount once per minute
    if (mintimes) {
        pthread_t pid;
        int rc = pthread_create (&pid, NULL, mintimesthread, NULL);
        if (rc != 0) ABORT ();
        pthread_detach (pid);
    }

    // set up null signal handler to break out of reads
    signal (SIGCHLD, nullsighand);

    // make sure we undo termios on exit
    signal (SIGABRT, sighandler);
    signal (SIGHUP,  sighandler);
    signal (SIGINT,  sighandler);
    signal (SIGTERM, sighandler);

    // access cpu circuitry
    // either physical circuit via gpio pins
    // ...or netgen simulator via pipes
    // ...or nothing but shadow
    GpioLib *gp = pipelib ? (GpioLib *) new PipeLib ("proc") :
                  csrclib ? (GpioLib *) new CSrcLib ("proc") :
                        zynqlib ? (GpioLib *) new ZynqLib () :
                    nohw ? (GpioLib *) new NohwLib (&shadow) :
                                  (GpioLib *) new PhysLib ();
    gp->open ();
    gp->floatpaddles ();
    asm volatile ("");
    gpio = gp;

    // close gpio on exit
    atexit (exithandler);

    shadow.open (gpio);

reseteverything:;

    // reset cpu circuit
    gpio->doareset ();

    // tell shadowing that cpu has been reset
    shadow.reset ();

    // reset things we keep state of
    ioreset ();

    // do any initialization cycles
    // ignore HF_HALTIT, GUI is waiting for us to initialize
    if (initcount > 0) {
        if ((initcount == 3) && (initreads[1] == 05400) && (initreads[2] == 07300)) {
            fprintf (stderr, "raspictl: startpc=%o%04o\n", initdfif, initreads[0]);
        }
        memext.setdfif (initdfif);
        int ic = initcount;
        while (true) {
            // clock has been low for half cycle, just about to drive lock high
            uint32_t sample = gpio->readgpio ();
            if ((sample & G_READ) && (ic == 0)) break;
            shadow.clock (sample);
            gpio->writegpio (false, G_CLOCK);
            gpio->halfcycle (shadow.aluadd ());
            if (sample & G_READ) {
                sample = initreads[--ic] * G_DATA0;
                gpio->writegpio (true, sample);
                gpio->halfcycle (shadow.aluadd ());
                shadow.clock (gpio->readgpio ());
                gpio->writegpio (true, sample | G_CLOCK);
                gpio->halfcycle (shadow.aluadd ());
            }
            gpio->writegpio (false, 0);
            gpio->halfcycle (shadow.aluadd ());
        }
        // if we stopped on JMP1 state, that will be the reading of the opcode at the target PC
        // so set the shadow PC to the target in case of guihalt, so it will see target PC
        if (shadow.r.state == Shadow::JMP1) {
            shadow.r.pc = initreads[0];
        }
    }

    // if running from GUI, halt processor in next haltcheck() call
    if (guihalt) haltflags = HF_HALTIT;

    try {
        for ever {

            // maybe GUI is requesting halt at end of cycle
            haltcheck ();

            // invariant:
            //  clock has been low for half cycle
            //  G_DENA has been asserted for half cycle so we can read MDL,MD coming from cpu

            // get input signals just before raising clock
            uint32_t sample = gpio->readgpio ();

            // raise clock then wait for half a cycle
            shadow.clock (sample);
            if (shadow.r.state == Shadow::FETCH2) {
                // keep IR LEDs steady with old opcode instead of turning all on
                // cosmetic only, otherwise can use the else's writegpio()
                gpio->writegpio (true, G_CLOCK | syncintreq | shadow.r.ir * G_DATA0);
            } else {
                gpio->writegpio (false, G_CLOCK | syncintreq);
            }
            gpio->halfcycle (shadow.aluadd ());

            // we are now halfway through the cycle with the clock still high

            // process the signal sample from just before raising clock

            // - jump instruction (last cycle was either JMP1 or JMS1)
            //   if there was a pending change in iframe, get the new iframe now
            //   so the address presented by the JMP1 or JMS1 is interpreted in new iframe
            //   if it just finished JMP1, we probably have G_READ and the target address in the new iframe,
            //     so we want the new iframe set up before we fetch the instruction below
            //   likewise if it just finished JMS1, we have G_WRITE and the target address in the new iframe,
            //     so we want the new iframe set up before we write the return address below
            if (sample & G_JUMP) {
                memext.doingjump ();
            }

            // - interrupt acknowledge (turns off interrupt enable and other such flab)
            //   sets iframe to 0 so subsequent saving of the PC gets saved in 0.0000
            //   it just finished INTAK1 so we have G_WRITE at address 0 for frame 0
            //     so we have to change to iframe 0 before doing the write below
            if (sample & G_IAK) {
                memext.intack ();
            }

            // - memory access
            if (sample & (G_READ | G_WRITE)) {
                uint16_t addr = ((sample & G_DATA) / G_DATA0) | ((sample & G_DFRM) ? memext.dframe : memext.iframe);
                ASSERT (addr < MEMSIZE);

                if (addr == stopataddr) {
                    fprintf (stderr, "raspictl: stopat %05o\n", addr);
                    dumpregs ();
                    return 2;
                }

                if (sample & G_READ) {

                    // read memory contents
                    if (randmem) {
                        lastreaddata = randuint16 () & 07777;
                    } else if (os8zap && (lastreadaddr == 007424) && (lastreaddata == 02351) && (addr == 007551) && (memarray[007425] == 05224)) {
                        lastreaddata = 07777;   // patch 07424: ISZ 07551 / JMP 07424 to read value 07777
                    } else {
                        lastreaddata = memarray[addr];
                    }

                    // save last address being read from
                    lastreadaddr = addr;

                    // maybe stop if a 'JMP .' instruction
                    if (infloopstop && (shadow.r.state == Shadow::FETCH2) && ((lastreaddata & 07400) == 05000)) {
                        uint16_t ea = memext.iframe | ((lastreaddata & 00200) ? (addr & 07600) : 0) | (lastreaddata & 00177);
                        if (ea == addr) {
                            fprintf (stderr, "raspictl: JMP . at PC=%05o\n", lastreadaddr);
                            dumpregs ();
                            return 2;
                        }
                    }

                    // interrupts might have been turned on by the previous instruction
                    // but the enable gets blocked until the fetch of the next instruction
                    // so this read might be such a fetch
                    memext.doingread (lastreaddata);

                    // if interrupts were just turned on by memext.doingread(),
                    // recompute syncintreq before dropping clock so processor
                    // will see the new interrupt request before deciding if it
                    // will take the interrupt by the end of the instruciton
                    syncintreq = (intreqmask && memext.intenabd ()) ? G_IRQ : 0;

                    // send data to processor
                    senddata (lastreaddata);
                }

                if (sample & G_WRITE) {
                    uint16_t data = recvdata () & 07777;
                    if (addr == watchwrite) {
                        fprintf (stderr, "raspictl: watch write addr %05o data %04o\n", addr, data);
                        dumpregs ();
                    }
                    memarray[addr] = data;
                    if (randmem) {
                        uint16_t r = randuint16 ();
                        intreqmask = (r & 1) * IRQ_RANDMEM;
                        if ((r & 14) == 0) {
                            if (shadow.printinstr | shadow.printstate) {
                                printf ("%9llu randmem enabling interrupts\n", (LLU) shadow.getcycles ());
                            }
                            memext.ioinstr (06001, 0);
                        }
                    }
                }
            }

            // - io instruction (incl group 3 eae)
            //   last cycle was IOT1 and now in middle of IOT2
            //   lastreaddata contains i/o opcode
            //   sample contains AC,link
            //   processor waiting for updated AC,link and skip signal
            if (sample & G_IOIN) {
                ASSERT (G_LINK == G_DATA + G_DATA0);
                uint16_t linkac = (sample / G_DATA0) & 017777;
                // - in middle of IOT2 clock high
                uint16_t skplac = UNSUPIO;
                if ((lastreaddata & 07000) == 06000) {
                    skplac = ioinstr (lastreaddata, linkac);          // a real i/o instruction
                } else if ((lastreaddata & 07401) == 07401) {
                    skplac = extarith.ioinstr (lastreaddata, linkac); // group 3 (eae)
                } else if ((lastreaddata & 07401) == 07400) {
                    skplac = group2io (lastreaddata, linkac);         // group 2 with OSR and/or HLT
                }
                if (skplac == UNSUPIO) {
                    if (! quiet) {
                        fprintf (stderr, "raspictl: unsupported i/o opcode %04o at %05o\n", lastreaddata, lastreadaddr);
                        dumpregs ();
                    }
                    skplac = linkac;
                }
                syncintreq = (intreqmask && memext.intenabd ()) ? G_IRQ : 0;
                                                                // currently in middle of IOT2 state
                                                                // update irq in case i/o instr updates it
                                                                // should be time for it to soak in for processor
                                                                // ...to decide FETCH1 or INTAK1 state next
                // - in middle of IOT2 clock high
                //   for LINC, we are in DEFER2 and so are sending updated PC
                senddata (skplac);                              // we send SKIP,LINK,AC in final cycle
                // - in middle of FETCH1/INTAK1 clock high
            }

            // update irq pin in middle of cycle so it has time to soak in by the next clock up time
            bool intenabd = memext.intenabd ();
            syncintreq = (intreqmask && intenabd) ? G_IRQ : 0;

            // drop the clock and enable link,data bus read
            // then wait half a clock cycle while clock is low
            gpio->writegpio (false, syncintreq);
            gpio->halfcycle (shadow.aluadd ());

            // maybe reached cycle limit
            uint64_t cc;
            if ((cyclelimit != 0) && ((cc = shadow.getcycles ()) > cyclelimit)) {
                fprintf (stderr, "raspictl: cycle count %llu reached limit %u\n", (LLU) cc, cyclelimit);
                ABORT ();
            }
        }
    }
    catch (ResetProcessorException& rpe) {
        fprintf (stderr, "\nraspictl: resetting everything...\n");
        goto reseteverything;
    }
    catch (Shadow::StateMismatchException& sme) {
        if (resetonerr) {
            fprintf (stderr, "\nraspictl: resetting everything...\n");
            goto reseteverything;
        }
        exit (1);
    }
}

// check to see if GUI.java is requesting halt
// call at end of cycle with clock still low
// call just before taking gpio sample at end of cycle
// ...in case of halt where GUI alters what the sample would get
static void haltcheck ()
{
    if (haltflags & HF_HALTIT) {
        bool resetit = false;
        pthread_mutex_lock (&haltmutex);

        // re-check haltflags now that we have mutex
        if (haltflags & HF_HALTIT) {

            // IR is a latch so should now have memory contents
            if (shadow.r.state == Shadow::FETCH2) {
                shadow.r.ir = lastreaddata;
            }

            // tell GUI.java that we have halted
            haltflags |= HF_HALTED;
            pthread_cond_broadcast (&haltcond2);

            // wait for GUI.java to tell us to resume
            while (haltflags & HF_HALTED) {
                pthread_cond_wait (&haltcond, &haltmutex);
            }

            // maybe GUI is resetting us
            if (haltflags & HF_RESETIT) {
                resetit = true;
                haltflags &= ~ HF_RESETIT;
            }
        }

        pthread_mutex_unlock (&haltmutex);
        if (resetit) {
            ResetProcessorException rpe;
            throw rpe;
        }
    }
}

// group 2 with OSR and/or HLT gets passed to raspi like an i/o instruction
static uint16_t group2io (uint32_t opcode, uint16_t input)
{
    // group 2 (p 134 / v 3-8)
    bool skip = false;
    if ((opcode & 0100) &&  (input & 04000))       skip = true; // SMA
    if ((opcode & 0040) && ((input & 07777) == 0)) skip = true; // SZA
    if ((opcode & 0020) &&  (input & IO_LINK))     skip = true; // SNL
    if  (opcode & 0010)                          skip = ! skip; // reverse
    if (skip) input |= IO_SKIP;
    if  (opcode & 0200) input &= IO_SKIP | IO_LINK;             // CLA

    // for OSR and/or HLT, require permission to do i/o
    if ((opcode & 0006) && memext.candoio ()) {
        if (opcode & 0004) {                                    // OSR
            uint16_t swreg = readswitches ("switchregister");
            input |= swreg & 07777;
        }
        if  (opcode & 0002) {                                   // HLT
            haltinstr ("raspictl: HALT PC=%05o L=%o AC=%04o\n", lastreadaddr, (input >> 12) & 1, input & 07777);
        }
    }
    return input;
}

// read the named switchregister environment variable
// if variable is an octal integer string, it is used as is
// otherwise, it is assumed to be a softlink giving the octal integer
uint16_t readswitches (char const *swvar)
{
    char *p, swbuf[10];

    char const *swenv = getenv (swvar);
    if ((swenv == NULL) && randmem) swenv = "random";
    if (swenv == NULL) {
        fprintf (stderr, "raspictl: envar %s not defined\n", swvar);
        ABORT ();
    }
    uint16_t swreg = strtoul (swenv, &p, 8);
    if (*p != 0) {
        if (strcasecmp (swenv, "random") == 0) {
            swreg = randuint16 ();
        } else {
            int swlen = readlink (swenv, swbuf, sizeof swbuf - 1);
            if (swlen < 0) {
                fprintf (stderr, "raspictl: error reading %s softlink %s: %m\n", swvar, swenv);
                ABORT ();
            }
            swbuf[swlen] = 0;
            swreg = strtoul (swbuf, &p, 8);
            if (*p != 0) {
                fprintf (stderr, "raspictl: bad %s softlink %s: %s\n", swvar, swenv, swbuf);
                ABORT ();
            }
        }
    }
    return swreg;
}

// processor has encountered an HALT instruction
// decide what to do
void haltinstr (char const *fmt, ...)
{
    pthread_mutex_lock (&haltmutex);
    if ((intreqmask == 0) && ! (haltflags & HF_HALTIT)) {

        // -haltprint means print a message
        if (haltprint | haltstop) {
            va_list ap;
            va_start (ap, fmt);
            vfprintf (stderr, fmt, ap);
            va_end (ap);
        }

        // -haltstop means print a message and exit
        if (haltstop) {
            pthread_mutex_unlock (&haltmutex);
            fprintf (stderr, "raspictl: haltstop\n");
            exit (0);
        }

        // wait for an interrupt request
        if (! randmem && ! haltcont) {
            do pthread_cond_wait (&haltcond, &haltmutex);
            while ((intreqmask == 0) && ! (haltflags & HF_HALTIT));
        }
    }
    pthread_mutex_unlock (&haltmutex);
}

// generate a random number
static uint16_t randuint16 ()
{
    static uint64_t seed = 0x123456789ABCDEF0ULL;

    uint16_t randval = 0;

    for (int i = 0; i < 16; i ++) {

        // https://www.xilinx.com/support/documentation/application_notes/xapp052.pdf
        uint64_t xnor = ~ ((seed >> 63) ^ (seed >> 62) ^ (seed >> 60) ^ (seed >> 59));
        seed = (seed << 1) | (xnor & 1);

        randval += randval + (seed & 1);
    }

    return randval;
}

static void dumpregs ()
{
    fprintf (stderr, "raspictl:  L=%d AC=%04o PC=%05o\n",
            shadow.r.link, shadow.r.ac, memext.iframe | shadow.r.pc);
}

// set the interrupt request bits
void setintreqmask (uint16_t mask)
{
    pthread_mutex_lock (&haltmutex);
    intreqmask |= mask;
    pthread_cond_broadcast (&haltcond);
    pthread_mutex_unlock (&haltmutex);
}

void clrintreqmask (uint16_t mask)
{
    pthread_mutex_lock (&haltmutex);
    intreqmask &= ~ mask;
    pthread_mutex_unlock (&haltmutex);
}

uint16_t getintreqmask ()
{
    return intreqmask;
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
    uint64_t lastcycs = shadow.getcycles ();
    uint32_t lastsecs = nowts.tv_sec;

    waits.tv_nsec = 0;

    while (true) {
        int rc;
        waits.tv_sec = (lastsecs / 60 + 1) * 60;
        do rc = pthread_cond_timedwait (&cond, &lock, &waits);
        while (rc == 0);
        if (rc != ETIMEDOUT) ABORT ();
        if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) ABORT ();
        uint64_t cycs = shadow.getcycles ();
        uint32_t secs = nowts.tv_sec;
        fprintf (stderr, "raspictl: %02d:%02d:%02d  %12llu cycles  avg %8llu Hz  %6.3f uS  [PC=%05o L=%o AC=%04o]\n",
                    secs / 3600 % 24, secs / 60 % 60, secs % 60,
                    (LLU) cycs, (LLU) ((cycs - lastcycs) / (secs - lastsecs)),
                    (secs - lastsecs) * 1000000.0 / (cycs - lastcycs),
                    shadow.r.pc, shadow.r.link, shadow.r.ac);
        lastcycs = cycs;
        lastsecs = secs;
    }
    return NULL;
}

// send data word to CPU in response to a MEM_READ cycle
// we are half way through the cycle after the CPU asserted MEM_READ with clock still high
// we must send the data to the cpu with time to let it soak in before raising clock again
// so we:
//  1) drop the clock and output data to CPU
//  2) leave the data there with clock negated for half a clock cycle
//  3) raise the clock
//  4) leave the data there with clock asserted for another half a clock cycle
// now we are halfway through a cpu cycle with the clock still asserted
// for example:
//  previous cycle was FETCH1 where the cpu sent us MEM_READ with the program counter
//  we are now halfway through FETCH2 with clock still high
//  we drop the clock and send the data to the cpu by dropping G_DENA
//  we wait a half cycle whilst the data soaks into the instruction register latch and instruction decoding circuitry
//  we raise the clock leaving the data going to the cpu so it will get clocked in correctly
//  we wait a half cycle whilst cpu is closing the instruction register latch and transitioning to first state of the instruction
static void senddata (uint16_t data)
{
    uint32_t sample = (data & 07777) * G_DATA0 | syncintreq;
    if (data & IO_LINK) sample |= G_LINK;   // <12> contains link (only used in io instruction)
    if (data & IO_SKIP) sample |= G_IOS;    // <13> contains skip (only used in io instruction)

    // drop the clock and start sending skip,link,data to cpu
    gpio->writegpio (true, sample);

    // let data soak into cpu (giving it a setup time)
    gpio->halfcycle (shadow.aluadd ());

    // maybe GUI is requesting halt at end of cycle
    haltcheck ();

    // tell cpu data is ready to be read by raising the clock
    // hold data steady and keep sending data to cpu so it will be clocked in correctly
    shadow.clock (gpio->readgpio ());
    gpio->writegpio (true, G_CLOCK | sample);

    // wait for cpu to clock data in (giving it a hold time)
    gpio->halfcycle (shadow.aluadd ());
}

// recv data word from CPU as part of a MEM_WRITE cycle
// we are half way through the cycle after the CPU asserted MEM_WRITE with clock still high
// give the CPU the rest of this cycle just up to the point where
// ...we are about to raise the clock for it to come up with the data
// so we:
//  1) drop the clock and make sure G_DENA is enabled so we can read the data
//  2) wait for the end of the cycle following MEM_WRITE cycle
//     this gives the CPU time to put the data on the bus
//  3) read the data from the CPU just before raising clock
//  4) raise clock
//  5) wait for half a clock cycle
// for example:
//  previous cycle was DCA1 where cpu sent us the address to write to with MEM_WRITE asserted
//  we are now halfway through DCA2 where cpu is sending us the data to write
//  we drop the clock
//  we wait a half cycle for the cpu to finish sending the data
//  it is now the very end of the DCA2 cycle
//  we read the data from the gpio pins
static uint16_t recvdata (void)
{
    // drop clock and wait for cpu to finish sending the data
    gpio->writegpio (false, syncintreq);
    gpio->halfcycle (shadow.aluadd ());

    // maybe GUI is requesting halt at end of cycle
    haltcheck ();

    // read data being sent by cpu just before raising clock
    // then raise clock and wait a half cycle to be at same timing as when called
    uint32_t sample = gpio->readgpio ();
    shadow.clock (sample);
    gpio->writegpio (false, G_CLOCK | syncintreq);
    gpio->halfcycle (shadow.aluadd ());

    // return value received from cpu to be written to memory or sent to I/O device
    //  <15:13> = zeroes
    //     <12> = link value (used only for i/o instruction)
    //  <11:00> = data value
    return ((sample / G_DATA0) & 07777) | ((sample & G_LINK) / (G_LINK >> 12));
}

// null signal handler used to break out of read()s
static void nullsighand (int signum)
{ }

// signal that terminates process, do exit() so exithandler() gets called
static void sighandler (int signum)
{
    // if control-C with -script, halt the processor getting clocked
    // this will eventually wake script out of 'wait' command if it is in one
    // ...when it sees HF_HALTED set
    if ((signum == SIGINT) && scriptmode) {
        pthread_mutex_lock (&haltmutex);
        haltflags |= HF_HALTIT;
        pthread_cond_broadcast (&haltcond);
        pthread_mutex_unlock (&haltmutex);
        return;
    }

    // something else, die
    fprintf (stderr, "raspictl: terminated for signal %d\n", signum);
    exit (1);
}

// exiting
static void exithandler ()
{
    uint64_t cycles = shadow.getcycles ();
    fprintf (stderr, "raspictl: exiting, %llu cycle%s\n", (LLU) cycles, ((cycles == 1) ? "" : "s"));

    ioreset ();
    scncall.exithandler ();

    // close gpio access
    gpio->close ();
}

char const *ResetProcessorException::what ()
{
    return "processor reset requested";
}
