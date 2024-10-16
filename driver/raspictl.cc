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
 *  ./raspictl [-binloader] [-corefile <filename>] [-cpuhz <freq>] [-csrclib] [-cyclelimit <cycles>] [-guimode]
 *          [-jmpdotstop] [-linc] [-mintimes] [-nohwlib] [-os8zap] [-paddles] [-pidp] [-pipelib]
 *          [-printfile <filename>] [-printinstr] [-printstate] [-quiet] [-randmem] [-resetonerr] [-rimloader] [-script] [-skipopt]
 *          [-startpc <address>] [-stopat <address>] [-tubesaver] [-zynqlib] <loadfile>
 *
 *       <addhz> : specify cpu frequency for add cycles (default cpuhz Hz)
 *       <cpuhz> : specify cpu frequency for all other cycles (default DEFCPUHZ Hz)
 *
 *      -binloader   : load file is in binloader format
 *      -corefile    : specify persistent core file image
 *      -csrclib     : use C-source for circuitry simulation (see csrclib.cc)
 *      -cyclelimit  : specify maximum number of cycles to execute
 *      -guimode     : used by the GUI.java wrapper to start in stopped mode
 *      -jmpdotstop  : infinite loop JMP causes exit
 *      -linc        : process linc instruction set
 *      -mintimes    : print cpu cycle info once a minute
 *      -nohwlib     : don't use hardware, simulate processor internally
 *      -os8zap      : zap out the OS/8 ISZ/JMP delay loop
 *      -paddles     : check ABCD paddles each cycle
 *      -pidp        : operate pidp-8 panel via gpio
 *      -pipelib     : simulate via pipe connected to NetGen
 *      -printfile   : specify file for -printinstr -printstate output
 *      -printinstr  : print message at beginning of each instruction
 *      -printstate  : print message at beginning of each state
 *      -quiet       : don't print undefined/unsupported opcode/os8zap messages
 *      -randmem     : supply random opcodes and data for testing
 *      -resetonerr  : reset if error is detected
 *      -rimloader   : load file is in rimloader format
 *      -script      : load file is a TCL script
 *      -skipopt     : optimize ioskip/jmp.-1 loops
 *      -startpc     : starting program counter
 *      -stopat      : stop simulating when accessing the address
 *      -tubesaver   : do random instructions during skipopt/waitforint time
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

#include "abcd.h"
#include "binloader.h"
#include "controls.h"
#include "dyndis.h"
#include "extarith.h"
#include "gpiolib.h"
#include "linkloader.h"
#include "memext.h"
#include "memory.h"
#include "miscdefs.h"
#include "pidp.h"
#include "rdcyc.h"
#include "resetprocessorexception.h"
#include "rimloader.h"
#include "scncall.h"
#include "script.h"
#include "shadow.h"

#define ever (;;)

bool ctrlcflag;
bool jmpdotstop;
bool lincenab;
bool os8zap;
bool quiet;
bool randmem;
bool resetio;
bool scriptmode;
bool skipopt;
bool startln;
bool tubesaver;
bool waitingforinterrupt;
char **cmdargv;
ExtArith extarith;
GpioLib *gpio;
int cmdargc;
int numstopats;
MemExt memext;
Shadow shadow;
uint16_t lastreadaddr;
uint16_t startac;
uint16_t startpc;
uint16_t switchregister;
uint16_t *memory;
uint16_t stopats[MAXSTOPATS];

__thread bool thisismainthread;

static pthread_t mintimestid;
static pthread_cond_t mintimescond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mintimeslock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t stopcond = PTHREAD_COND_INITIALIZER;
static pthread_cond_t wficond = PTHREAD_COND_INITIALIZER;

pthread_cond_t stopcond2 = PTHREAD_COND_INITIALIZER;
pthread_mutex_t stopmutex = PTHREAD_MUTEX_INITIALIZER;
char const *stopreason = "";
uint32_t stopflags;

static bool guimode;
static bool pidpmode;
static bool setintreqcalled;
static bool volatile wakefromwfi;
static uint16_t intreqmask;
static uint16_t lastreaddata;
static uint32_t syncintreq;

static void writestartpcacln ();
static void initaccumlink ();
static void stopped (bool sigint);
static void *mintimesthread (void *dummy);
static uint16_t group2io (uint32_t opcode, uint16_t input);
static uint16_t randuint16 (int nbits);
static void dumpregs ();
static void wfiwait ();
static uint32_t ts_senddata (uint32_t intrq, uint16_t data);
static uint32_t ts_recvdata (uint32_t intrq);
static void senddata (uint16_t data);
static uint16_t recvdata ();
static void nullsighand (int signum);
static void scriptsigint (int signum);
static void sighandler (int signum);
static void *sigthread (void *dummy);
static void exithandler ();

int main (int argc, char **argv)
{
    bool binldr = false;
    bool csrclib = false;
    bool mintimes = false;
    bool nohwlib = false;
    bool pipelib = false;
    bool resetonerr = false;
    bool rimldr = false;
    bool zynqlib = false;
    char const *corename = NULL;
    char const *loadname = NULL;
    char *p;
    uint32_t cyclelimit = 0;

    setlinebuf (stdout);
    thisismainthread = true;

    resetio = true;
    startpc = 0xFFFFU;

    shadow.printname = "-";
    shadow.printfile = stdout;

    for (int i = 0; ++ i < argc;) {
        if (strcasecmp (argv[i], "-binloader") == 0) {
            binldr     = true;
            randmem    = false;
            rimldr     = false;
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
            nohwlib = false;
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
        if (strcasecmp (argv[i], "-guimode") == 0) {
            guimode = true;
            continue;
        }
        if (strcasecmp (argv[i], "-jmpdotstop") == 0) {
            jmpdotstop = true;
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
        if (strcasecmp (argv[i], "-nohwlib") == 0) {
            csrclib = false;
            nohwlib = true;
            pipelib = false;
            zynqlib = false;
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
        if (strcasecmp (argv[i], "-pidp") == 0) {
            pidpmode = true;
            continue;
        }
        if (strcasecmp (argv[i], "-pipelib") == 0) {
            csrclib = false;
            nohwlib = false;
            pipelib = true;
            zynqlib = false;
            continue;
        }
        if (strcasecmp (argv[i], "-printfile") == 0) {
            if ((++ i > argc) || ((argv[i][0] == '-') && (argv[i][0] != 0))) {
                fprintf (stderr, "raspictl: missing filename after -printfile\n");
                return 1;
            }
            if (shadow.printfile != stdout) {
                fclose (shadow.printfile);
                shadow.printfile = NULL;
            }
            shadow.printname = argv[i];
            if (strcmp (shadow.printname, "-") == 0) {
                shadow.printfile = stdout;
            } else {
                shadow.printfile = fopen (shadow.printname, "w");
                if (shadow.printfile == NULL) {
                    fprintf (stderr, "raspictl: error creating %s: %m\n", shadow.printname);
                    return 1;
                }
                setlinebuf (shadow.printfile);
            }
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
            binldr     = false;
            randmem    = true;
            rimldr     = false;
            scriptmode = false;
            quiet      = true;
            continue;
        }
        if (strcasecmp (argv[i], "-resetonerr") == 0) {
            resetonerr = true;
            continue;
        }
        if (strcasecmp (argv[i], "-rimloader") == 0) {
            binldr     = false;
            randmem    = false;
            rimldr     = true;
            scriptmode = false;
            continue;
        }
        if (strcasecmp (argv[i], "-script") == 0) {
            binldr     = false;
            randmem    = false;
            rimldr     = false;
            scriptmode = true;
            continue;
        }
        if (strcasecmp (argv[i], "-skipopt") == 0) {
            skipopt = true;
            continue;
        }
        if (strcasecmp (argv[i], "-startpc") == 0) {
            if ((++ i >= argc) || (argv[i][0] == '-')) {
                fprintf (stderr, "raspictl: missing address after -startpc\n");
                return 1;
            }
            startpc = strtoul (argv[i], NULL, 8) & 077777;
            continue;
        }
        if (strcasecmp (argv[i], "-stopat") == 0) {
            if ((++ i >= argc) || (argv[i][0] == '-')) {
                fprintf (stderr, "raspictl: missing address after -stopat\n");
                return 1;
            }
            p = argv[i];
            bool stopatwrite = (*p == '~');
            if (stopatwrite) p ++;
            int32_t stopat = strtol (p, &p, 8);
            if (stopatwrite) stopat = ~ stopat;
            if ((*p != 0) || (stopat < ~ 077777) || (stopat > 077777)) {
                fprintf (stderr, "raspictl: bad -stopat address '%s'\n", argv[i]);
                return 1;
            }
            if (numstopats == MAXSTOPATS) {
                fprintf (stderr, "raspictl: too many -stopats\n");
                return 1;
            }
            if (stopat < 0) stopat = (~ stopat) | STOPATWRITE;
            stopats[numstopats++] = stopat;
            continue;
        }
        if (strcasecmp (argv[i], "-tubesaver") == 0) {
            tubesaver = true;
            continue;
        }
        if (strcasecmp (argv[i], "-zynqlib") == 0) {
            csrclib = false;
            nohwlib = false;
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
                int rc = binloader (loadfile, &start);
                if (rc != 0) return rc;

                // maybe a start address was given but don't override -startpc
                if (startpc == 0xFFFFU) startpc = start & 077777;
            } else if (rimldr) {

                // pdp-8 rimloader format
                int rc = rimloader (loadfile);
                if (rc != 0) return rc;
            } else {

                // linker format
                int rc = linkloader (loadfile);
                if (rc < 0) return -rc;
                if (startpc == 0xFFFFU) startpc = rc & 077777;
            }

            fclose (loadfile);
        }
    }

    if (! guimode && ! randmem && ! scriptmode && (startpc == 0xFFFFU)) {
        fprintf (stderr, "raspictl: no -startpc given\n");
        return 1;
    }

    // set up null signal handler to break out of reads
    signal (SIGCHLD, nullsighand);

    // catch normal termination signals to clean up
    signal (SIGHUP,  sighandler);
    signal (SIGINT,  scriptmode ? scriptsigint : sighandler);
    signal (SIGTERM, sighandler);

    // access cpu circuitry
    // either physical circuit (tubes or de0) via gpio pins
    // ...or C-source simulator
    // ...or zynq fpga circuit
    // ...or netgen simulator via pipes
    // ...or nothing but shadow
    GpioLib *gp = pipelib ? (GpioLib *) new PipeLib ("proc") :
                  csrclib ? (GpioLib *) new CSrcLib ("proc") :
                        zynqlib ? (GpioLib *) new ZynqLib ("aclalumapcrpiseq", memarray) :
                        nohwlib ? (GpioLib *) new NohwLib () :
                                  (GpioLib *) new PhysLib ();
    gp->open ();

    // always call haspads() so it can float the paddles
    if (! gp->haspads () && shadow.paddles) {
        fprintf (stderr, "raspictl: -paddles given but paddles not present\n");
        return 1;
    }

    // if paddles present, print initial register contents
    if (shadow.paddles) {
        ABCD abcd;
        gp->readpads (abcd.cons);
        abcd.decode ();
        fprintf (stderr, "raspictl: L.AC=%o.%04o MA=%04o PC=%04o IR=%o--- ST=%s\n",
                abcd.lnq, abcd.acq, abcd.maq, abcd.pcq, abcd.irq >> 9, abcd.states ().c_str ());
    }

    // allow other things to access paddles now that they are all set up
    asm volatile ("");
    gpio = gp;

    // print cpu cyclecount once per minute
    setmintimes (mintimes);

    // close gpio on exit
    atexit (exithandler);

    shadow.open (gpio);

    // if running from GUI or script, stop processor in next stopcheck() call
    bool sigint = ctl_lock ();
    if (guimode | scriptmode) {
        stopreason = "RESET";
        stopflags  = SF_STOPIT;
    } else {
        stopreason = "";
        stopflags  = 0;
    }
    ctl_unlock (sigint);

    // maybe start pidp panel scanning
    if (pidpmode) pidp_start ();

reseteverything:;

    // tell shadowing that cpu is being reset
    shadow.reset ();

    // reset cpu circuit
    gpio->doareset ();

    // reset things we keep state of
    if (resetio) ioreset ();

    // do any initialization cycles
    // ignore stopflags, GUI.java or script.cc is waiting for us to initialize
    try {
        if (startpc != 0xFFFFU) {
            writestartpcacln ();
        } else if (randmem) {
            initaccumlink ();
        }
    } catch (Shadow::StateMismatchException& sme) {
        stopreason = "STATEMISMATCH";
        stopordie (stopreason);
    }

    uint16_t lastos8zap = 0;

    try {
        for ever {

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
            gpio->halfcycle (shadow.aluadd (), true);

            // we are now halfway through the cycle with the clock still high

            // maybe GUI or script is requesting stop in middle of cycle
            stopcheck ();

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

                if (numstopats > 0) {
                    uint16_t waddr = (((sample / G_WRITE) & 1) * STOPATWRITE) | addr;
                    for (int i = 0; i < numstopats; i ++) {
                        uint16_t sa = stopats[i];
                        if ((addr == sa) || (waddr == sa)) {
                            stopordie ("STOPAT");
                            break;
                        }
                    }
                }

                if (sample & G_READ) {

                    // read memory contents
                    if (randmem) {
                        lastreaddata = randuint16 (12);
                    } else if (os8zap &&
                            ((lastreaddata & 07600) == 02200) &&                            // page+x+0: ISZ page+y
                            (addr == (lastreadaddr & 07600) + (lastreaddata & 00177)) &&    // reading from page+y
                            ((lastreadaddr & 00177) != 00177) &&                            // not at end of page
                            (memarray[lastreadaddr+1] == 05200 + (lastreadaddr & 00177))) { // page+x+1: JMP page+x+0
                        if (! quiet && (lastos8zap != lastreadaddr)) {
                            lastos8zap = lastreadaddr;
                            fprintf (stderr, "raspictl: os8zap at %05o\n", lastreadaddr);
                        }
                        lastreaddata = 07777;   // patch to read value 07777
                    } else {
                        lastreaddata = memarray[addr];
                    }
                    if (dyndisena) dyndisread (addr);

                    // save last address being read from
                    lastreadaddr = addr;

                    // maybe stop if a 'JMP .' instruction
                    if (jmpdotstop && (shadow.r.state == Shadow::FETCH2) && ((lastreaddata & 07400) == 05000)) {
                        uint16_t ea = memext.iframe | ((lastreaddata & 00200) ? (addr & 07600) : 0) | (lastreaddata & 00177);
                        if (ea == addr) {
                            stopordie ("JMPDOT");
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
                    // eg in middle of FETCH2 or ISZ2
                    senddata (lastreaddata);
                    // eg in middle of EXEC1 or ISZ3
                    stopcheck ();
                }

                if (sample & G_WRITE) {
                    // eg in middle of DCA2
                    uint16_t data = recvdata () & 07777;
                    // eg in middle of FETCH1
                    memarray[addr] = data;
                    if (dyndisena) dyndiswrite (addr);
                    stopcheck ();
                    if (randmem) {
                        uint16_t r = randuint16 (4);
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

            // - io instruction (incl OSR, HLT and group 3 eae)
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
                // - in middle of IOT2 with clock still high
                //   for LINC, we are in DEFER2 and so are sending updated PC
                senddata (skplac);
                // - in middle of FETCH1/INTAK1 with clock still high
                stopcheck ();
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
        resetio = rpe.resetio;
        startac = rpe.startac;
        startln = rpe.startln;
        startpc = rpe.startpc;
    }
    catch (Shadow::StateMismatchException& sme) {
        if (! resetonerr) {
            if (! guimode && ! pidpmode && ! scriptmode) exit (1);
            bool sigint = ctl_lock ();

            // tell GUI.java or script.cc we are stopping
            stopreason = "STATEMISMATCH";
            stopflags  = SF_STOPPED;
            pthread_cond_broadcast (&stopcond2);

            // wait for GUI.java or script.cc to resume us
            while (stopflags & SF_STOPPED) {
                pthread_cond_wait (&stopcond, &stopmutex);
            }

            // allz we can do from here is reset everything
            ctl_unlock (sigint);
        }
    }
    if (resetio) fprintf (stderr, "\nraspictl: resetting everything...\n");
    goto reseteverything;
}

// load DF and IF with the frame and JMP to the address
// also sets accum and link
// assumes processor is at end of fetch1 with clock still low, shadow.clock() not called yet
// returns with processor at end of fetch1 with clock still low, shadow.clock() not called yet
// might throw StateMismatchException
static void writestartpcacln ()
{
    // set DF and IF registers to given frame
    memext.setdfif ((startpc >> 12) & 7);

    // clock has been low for half cycle, at end of FETCH1 as a result of the reset()
    // G_DENA has been asserted for half cycle so we can read MDL,MD coming from cpu
    // processor should be asking us to read from memory

    // drive clock high to transition to FETCH2
    shadow.clock (gpio->readgpio ());
    gpio->writegpio (false, G_CLOCK);
    gpio->halfcycle (shadow.aluadd ());

    // half way through FETCH2 with clock still high
    // drop clock and start sending it a JMP @0 opcode
    gpio->writegpio (true, 05400 * G_DATA0);
    gpio->halfcycle (shadow.aluadd ());

    // drive clock high to transition to DEFER1
    // keep sending opcode so tubes will clock it in
    shadow.clock (gpio->readgpio ());
    gpio->writegpio (true, 05400 * G_DATA0 | G_CLOCK);
    gpio->halfcycle (shadow.aluadd ());

    // half way through DEFER1 with clock still high
    // drop clock and step to end of cycle
    gpio->writegpio (false, 0);
    gpio->halfcycle (shadow.aluadd ());

    // drive clock high to transition to DEFER2
    shadow.clock (gpio->readgpio ());
    gpio->writegpio (false, G_CLOCK);
    gpio->halfcycle (shadow.aluadd ());

    // half way through DEFER2 with clock still high
    // drop clock and start sending it the PC contents - 2 for the CLA CLL and TAD we're about to send
    gpio->writegpio (true, ((startpc - 2) & 07777) * G_DATA0);
    gpio->halfcycle (shadow.aluadd ());

    // drive clock high to transition to EXEC1/JMP
    // keep sending startpc - 2 so it gets clocked into MA
    shadow.clock (gpio->readgpio ());
    gpio->writegpio (true, ((startpc - 2) & 07777) * G_DATA0 | G_CLOCK);
    gpio->halfcycle (shadow.aluadd ());

    // drop clock and run to end of EXEC1/JUMP
    gpio->writegpio (false, 0);
    gpio->halfcycle (shadow.aluadd ());

    // end of EXEC1/JUMP - processor should be reading the opcode at startpc - 2
    // but startpc - 2 has not clocked into PC yet

    // initialize accumulator and link
    // - clocks startpc - 2 into PC then increments PC to startpc
    // - also gets us to a real FETCH1 state with PC updated
    initaccumlink ();

    // the tubes are waiting at the end of FETCH1 with the clock still low
    // when resumed, raspictl.cc main will sample the GPIO lines, call shadow.clock() and resume processing from there
}

// initialize accumulator and link
// also increments program counter twice
// assumes processor is at end of FETCH1 or equiv with clock still low, shadow.clock() not called yet
// returns with processor at end of FETCH1 with clock still low, shadow.clock() not called yet
// might throw StateMismatchException
static void initaccumlink ()
{
    // raise clock to enter FETCH2
    shadow.clock (gpio->readgpio ());
    gpio->writegpio (false, G_CLOCK);
    gpio->halfcycle (shadow.aluadd ());

    // drop clock, start sending the tubes a CLA CLL CMA [CML] instruction
    uint16_t clacll = startln ? 07360 : 07340;
    gpio->writegpio (true, clacll * G_DATA0);
    gpio->halfcycle (shadow.aluadd ());

    // raise clock to enter EXEC1, keep sending opcode
    shadow.clock (gpio->readgpio ());
    gpio->writegpio (true, clacll * G_DATA0 | G_CLOCK);
    gpio->halfcycle (shadow.aluadd ());

    // drop clock, stop sending opcode
    gpio->writegpio (false, 0);
    gpio->halfcycle (shadow.aluadd ());

    // raise clock to enter FETCH1
    shadow.clock (gpio->readgpio ());
    gpio->writegpio (false, G_CLOCK);
    gpio->halfcycle (shadow.aluadd ());

    // drop clock, to get to end of FETCH1
    gpio->writegpio (false, 0);
    gpio->halfcycle (shadow.aluadd ());

    // raise clock to enter FETCH2
    shadow.clock (gpio->readgpio ());
    gpio->writegpio (false, G_CLOCK);
    gpio->halfcycle (shadow.aluadd ());

    // drop clock, start sending the tubes a AND 0000 instruction
    gpio->writegpio (true, 00000 * G_DATA0);
    gpio->halfcycle (shadow.aluadd ());

    // raise clock to enter EXEC1, keep sending opcode
    shadow.clock (gpio->readgpio ());
    gpio->writegpio (true, 00000 * G_DATA0 | G_CLOCK);
    gpio->halfcycle (shadow.aluadd ());

    // drop clock, stop sending opcode
    gpio->writegpio (false, 0);
    gpio->halfcycle (shadow.aluadd ());

    // raise clock to enter EXEC2
    shadow.clock (gpio->readgpio ());
    gpio->writegpio (true, G_CLOCK);
    gpio->halfcycle (shadow.aluadd ());

    // drop clock, start sending the tubes initial accum contents
    gpio->writegpio (true, startac * G_DATA0);
    gpio->halfcycle (shadow.aluadd ());

    // raise clock to enter FETCH1, keep sending startac so tubes will clock it in
    shadow.clock (gpio->readgpio ());
    gpio->writegpio (true, startac * G_DATA0 | G_CLOCK);
    gpio->halfcycle (shadow.aluadd ());

    // drop clock, to get to end of FETCH1
    gpio->writegpio (false, 0);
    gpio->halfcycle (shadow.aluadd ());
}

// if using GUI or scripts, stop the processor
// otherwise, print out registers and exit
// called in middle of cycle with clock still high
void stopordie (char const *reason)
{
    if (guimode | pidpmode | scriptmode) {
        bool sigint = ctl_lock ();
        if (stopreason[0] == 0) {
            stopreason = reason;
        }
        stopflags = SF_STOPIT;
        stopped (sigint);
    } else {
        fprintf (stderr, "stopordie: %s\n", reason);
        dumpregs ();
        exit (2);
    }
}

// check to see if script.cc or GUI.java is requesting stop
// call in middle of cycle with clock still high
// returns in middle of cycle with clock still high
// throws ResetProcessorException if script/GUI is requesting a reset
void stopcheck ()
{
    if (stopflags & SF_STOPIT) {
        bool sigint = ctl_lock ();

        // re-check stopflags now that we have mutex
        if (stopflags & SF_STOPIT) {
            stopped (sigint);
        } else {
            ctl_unlock (sigint);
        }
    }
}

static void stopped (bool sigint)
{
    // make sure clock is high
    uint32_t sample = gpio->readgpio ();
    ASSERT (sample & G_CLOCK);

    // IR is a latch so should now have memory contents
    if (shadow.r.state == Shadow::FETCH2) {
        shadow.r.ir = lastreaddata;
    }

    // tell script.cc or GUI.java that we have stopped
    stopflags |= SF_STOPPED;
    pthread_cond_broadcast (&stopcond2);

    // wait for script.cc or GUI.java to tell us to resume
    while (stopflags & SF_STOPPED) {
        pthread_cond_wait (&stopcond, &stopmutex);
    }

    // maybe script or GUI is resetting us
    if (stopflags & SF_RESETIT) {
        ctl_unlock (true);  // make sure to unblock SIGINT before throwing exception
        ResetProcessorException rpe;
        rpe.resetio = resetio;
        rpe.startac = startac;
        rpe.startln = startln;
        rpe.startpc = startpc;
        throw rpe;
    }

    // not a reset, make sure clock is high as expected by caller
    sample = gpio->readgpio ();
    ASSERT (sample & G_CLOCK);

    ctl_unlock (sigint);
}

// group 2 with OSR and/or HLT gets passed to raspi like an i/o instruction
// called and returns in middle of IOT2 with clock still high
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
            switchregister = readswitches ("switchregister");
            input |= switchregister & 07777;
        }
        if ((opcode & 0002) && ! randmem) {                     // HLT
            stopordie ("HALTSTOP");
        }
    }
    return input;
}

// read the named switchregister environment variable
// if variable is an octal integer string, it is used as is
// otherwise, it is assumed to be a softlink giving the octal integer
uint16_t readswitches (char const *swvar)
{
    if (randmem) return randuint16 (15);

    uint16_t hwsr = gpio->readhwsr (swvar);
    if (hwsr != UNSUPIO) return hwsr;

    if (strcmp (swvar, "switchregister") == 0) {
        if (pidpmode | scriptmode | guimode) return switchregister;
    }

    char const *swenv = getenv (swvar);
    if (swenv == NULL) {
        fprintf (stderr, "raspictl: envar %s not defined\n", swvar);
        ABORT ();
    }
    char *p, swbuf[10];
    uint16_t swreg = strtoul (swenv, &p, 8);
    if (*p != 0) {
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
    return swreg;
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

static void dumpregs ()
{
    fprintf (stderr, "raspictl:  L=%d AC=%04o PC=%05o\n",
            shadow.r.link, shadow.r.ac, memext.iframe | shadow.r.pc);
}

// called in main thread by ioinstr() methods that are handling a 'skip when flag set' instruction
// should be called with the given mutex locked
// to wake when io completes, call setintreqmask(x) or clrintreqmask(x,flag)
// called and returns in middle of IOT2 with clock still high
void skipoptwait (uint16_t skipopcode, pthread_mutex_t *lock, bool *flag)
{
    if (! skipopt) return;                                  // can't do it if not enabled by -skipopt
    if ((lastreadaddr & 00177) == 00177) return;            // can't do it if ioskip is last word of page
    if (memarray[lastreadaddr] != skipopcode) return;       // can't do if it's not an ioskip opcode
    uint16_t jmpskip = 05200 | (lastreadaddr & 00177);      // verify that ioskip followed by jmp ioskip
    uint16_t jmpzero = (lastreadaddr & 07600) ? 0 : 00200;  // maybe the ioskip/jmp.-1 is on page 0
    if ((memarray[lastreadaddr+1] | jmpzero) != jmpskip) return;

    *flag = true;
    pthread_mutex_unlock (lock);

    waitforinterrupt ();

    pthread_mutex_lock (lock);
    *flag = false;
}

// wait for interrupt
// - blocks until interrupt request, whether interrupts are enabled or not
void waitforinterrupt ()
{
    bool sigint = ctl_lock ();
    if (! (stopflags & SF_STOPIT) && (intreqmask == 0)) {
        if (setintreqcalled) setintreqcalled = false;
        else wfiwait ();
    }
    ctl_unlock (sigint);
}

// called in main thread to wait to be woken with wfiwake()
// used as part of WFI instruction or IOSKIP/JMP.-1 combination
// called and returns with stopmutex locked
// called and returns in middle of IOT2 with clock still high
static void wfiwait ()
{
    // we can do tube saving only from the main thread and we are doing an WFI or an I/O instruction
    // depends on the caller to send the real updated IOS/LINK/AC from the blocking I/O instruction on return
    ASSERT (thisismainthread && (shadow.r.state == Shadow::IOT2));

    // if tubesaver not enabled, block until some device thread or script or gui wakes us
    waitingforinterrupt = true;
    if (! tubesaver) {
        while (! wakefromwfi) {
            int rc = pthread_cond_wait (&wficond, &stopmutex);
            if (rc != 0) ABORT ();
        }
    } else if (! wakefromwfi) {
        pthread_mutex_unlock (&stopmutex);

        // tubesaver enabled, feed random instructions and data to tubes
        // ...until some device thread or script or gui wakes us

        // save PC of the IOT instruction
        // any skip has not been applied yet
        uint16_t savedpc = (shadow.r.pc - 1) & 07777;

        // mark all recorded state as being for tubesaver mode
        shadow.r.tsaver = true;

        // in middle of IOT2 with clock still high, send random IOS/LINK/AC
        ASSERT ((G_IOS == 020000 * G_DATA0) && (G_LINK == 010000 * G_DATA0));
        uint16_t data = randuint16 (14);
        uint32_t sample = ts_senddata (false, data);
        // now in middle of FETCH1 with clock still high

        uint32_t intrq = 0;
        do {
            // in middle of some cycle with clock still high
            // sample was taken just before clock driven high

            // if reading memory, send out a random number
            if (sample & G_READ) {
                // send random 12-bit data (maybe opcode) word to tubes
                data = randuint16 (12);
                ts_senddata (intrq, data);
            }

            // if writing memory, clock in and discard the data
            if (sample & G_WRITE) {
                ts_recvdata (intrq);
            }

            // if doing I/O, send out random IOS/LINK/AC
            if (sample & G_IOIN) {
                ASSERT (shadow.r.state == Shadow::IOT2);
                ASSERT ((G_IOS == 020000 * G_DATA0) && (G_LINK == 010000 * G_DATA0));
                data = randuint16 (14);
                ts_senddata (intrq, data);      // mid IOT2 -> mid FETCH1
            }

            // maybe start requesting interrupt
            if (randuint16 (6) == 0) intrq = G_IRQ;

            // maybe exercise reset circuitry
            // don't if we are in middle of cycle that writes IR latch cuz we would lose sync
            if ((shadow.r.state != Shadow::FETCH2) && (shadow.r.state != Shadow::INTAK1) && (randuint16 (5) == 0)) {
                shadow.reset ();                        // we don't clock at end of current state
                gpio->writegpio (false, G_RESET);       // send RESET (and drop CLOCK)
                gpio->halfcycle (shadow.aluadd ());     // let the reset soak into the tubes
                                                        // now halfway through FETCH1 cycle
            }

            // clock to middle of next cycle with clock still high
            sample = ts_recvdata (intrq);

            // maybe stop requesting interrupt
            if (sample & G_IAK) intrq = 0;

            // keep feeding random stuff until woken
        } while (! wakefromwfi);

        // woken from wait, restore state

        if (shadow.r.state != Shadow::FETCH2) {
            shadow.reset ();                        // we don't clock at end of current state
            gpio->writegpio (false, G_RESET);       // send RESET (and drop CLOCK)
            gpio->halfcycle (shadow.aluadd ());     // let the reset soak into the tubes
                                                    // now halfway through FETCH1 cycle
            ts_recvdata (0);                        // now halfway through FETCH2 cycle with clock still high
            ASSERT (shadow.r.state == Shadow::FETCH2);
        }

        // restore program counter with JMP to point to beginning of original IOT instruction
        {
            uint16_t jmpop;
            if ((savedpc & 07600) == 0) {
                // JMP page 0
                jmpop = 05000 + savedpc;
            } else {
                // JMPI 0/savedpc
                ts_senddata (false, 05400);     // mid FETCH2 -> mid DEFER1
                ts_recvdata (false);            // mid DEFER1 -> mid DEFER2
                jmpop = savedpc;
            }
            ts_senddata (false, jmpop);         // mid FETCH2/DEFER2 -> mid EXEC1/JMP
            sample = ts_recvdata (false);       // mid EXEC1/JMP -> FETCH2
            ASSERT ((sample & G_DATA) / G_DATA0 == savedpc);
        }

        // get back to middle of IOT2 with clock still high, as expected by caller
        // doesn't have to be same IOT opcode cuz the original has already been processed
        // ...so just use generic 6000 opcode
        ts_senddata (false, 06000);             // mid FETCH2 -> mid EXEC1/IOT
        ts_recvdata (false);                    // mid EXEC1/IOT -> mid EXEC2/IOT

        ASSERT (shadow.r.state == Shadow::IOT2);

        // now we're back to normal cycles
        shadow.r.tsaver = false;

        // caller will write IOS/LINK/AC as the result of original IOT processing

        pthread_mutex_lock (&stopmutex);
    }
    waitingforinterrupt = false;
    wakefromwfi = false;
}

// in middle of cycle with clock still high looking for data
// send it out then return in middle of next cycle with clock still high
static uint32_t ts_senddata (uint32_t intrq, uint16_t data)
{
    gpio->writegpio (true, data * G_DATA0 | intrq);
    gpio->halfcycle (shadow.aluadd ());
    gpio->halfcycle (false);    // compensate for wfiwait tubesaver loop simpler than main loop
    uint32_t sample = gpio->readgpio ();
    shadow.clock (sample);
    gpio->writegpio (true, data * G_DATA0 | intrq | G_CLOCK);
    gpio->halfcycle (shadow.aluadd ());
    return sample;
}

// in middle of cycle with clock still high
// return in middle of next cycle with clock still high
static uint32_t ts_recvdata (uint32_t intrq)
{
    gpio->writegpio (false, intrq);
    gpio->halfcycle (shadow.aluadd ());
    gpio->halfcycle (false);    // compensate for wfiwait tubesaver loop simpler than main loop
    uint32_t sample = gpio->readgpio ();
    shadow.clock (sample);
    gpio->writegpio (false, intrq | G_CLOCK);
    gpio->halfcycle (shadow.aluadd ());
    return sample;
}

// set the interrupt request bits
// wake from WFI or optimized ioskip
void setintreqmask (uint16_t mask)
{
    bool sigint = ctl_lock ();
    intreqmask |= mask;
    setintreqcalled = true;
    wfiwake ();
    ctl_unlock (sigint);
}

// clear the interrupt request bits
// optionally wake from optimized IOSKIP/JMP.-1
void clrintreqmask (uint16_t mask, bool wake)
{
    bool sigint = ctl_lock ();
    intreqmask &= ~ mask;
    if (wake) {
        setintreqcalled = true;
        wfiwake ();
    }
    ctl_unlock (sigint);
}

uint16_t getintreqmask ()
{
    return intreqmask;
}

// wake from WFI instruction or IOSKIP/JMP.-1 combination
void wfiwake ()
{
    wakefromwfi = true;
    int rc = pthread_cond_broadcast (&wficond);
    if (rc != 0) ABORT ();
}

// wake from stop state - caller has already cleared SF_STOPPED
void stopwake ()
{
    int rc = pthread_cond_broadcast (&stopcond);
    if (rc != 0) ABORT ();
}

// start/stop mintimes thread
bool getmintimes ()
{
    return mintimestid != 0;
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
    uint64_t lastcycs = shadow.getcycles ();
    uint64_t lastinss = shadow.getinstrs ();
    uint32_t lastsecs = nowts.tv_sec;

    waits.tv_nsec = 0;

    while (mintimestid != 0) {
        waits.tv_sec = (lastsecs / 60 + 1) * 60;
        int rc = pthread_cond_timedwait (&mintimescond, &mintimeslock, &waits);
        if (rc == 0) continue;
        if (rc != ETIMEDOUT) ABORT ();
        if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) ABORT ();
        pthread_mutex_lock (&gpio->trismutex);
        uint64_t nto = gpio->numtrisoff;
        uint64_t ntt = gpio->ntotaltris;
        gpio->numtrisoff = 0;
        gpio->ntotaltris = 0;
        pthread_mutex_unlock (&gpio->trismutex);
        uint64_t cycs = shadow.getcycles ();
        uint64_t inss = shadow.getinstrs ();
        uint32_t secs = nowts.tv_sec;
        char line[180];
        rc = snprintf (line, sizeof line,
                "raspictl: %02d:%02d:%02d  [%11llu cycles  %7llu cps %7.3f uS]  [%11llu instrs  %7llu ips %7.3f uS]  %4.2f cpi  [PC=%05o AC=%04o]",
                    secs / 3600 % 24, secs / 60 % 60, secs % 60,
                    (LLU) cycs, (LLU) ((cycs - lastcycs) / (secs - lastsecs)),
                    (secs - lastsecs) * 1000000.0 / (cycs - lastcycs),
                    (LLU) inss, (LLU) ((inss - lastinss) / (secs - lastsecs)),
                    (secs - lastsecs) * 1000000.0 / (inss - lastinss),
                    (cycs - lastcycs) / (double) (inss - lastinss),
                    shadow.r.pc, shadow.r.ac);
        ASSERT ((rc > 0) && (rc < (int) sizeof line));
        if (ntt > 0) {
            rc += snprintf (line + rc, sizeof line - rc, "  trison%6.2f %%", (ntt - nto) * 100.0 / ntt);
            ASSERT ((rc > 0) && (rc < (int) sizeof line));
        }
        fprintf (stderr, "%s\n", line);
        lastcycs = cycs;
        lastinss = inss;
        lastsecs = secs;
    }

    pthread_mutex_unlock (&mintimeslock);
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
    ASSERT (IO_DATA == 007777);
    ASSERT (IO_LINK == 010000);
    ASSERT (IO_SKIP == 020000);
    ASSERT (G_LINK  == 010000 * G_DATA0);
    ASSERT (G_IOS   == 020000 * G_DATA0);
    uint32_t sample = (data & 037777) * G_DATA0 | syncintreq;

    // drop the clock and start sending skip,link,data to cpu
    gpio->writegpio (true, sample);

    // let data soak into cpu (giving it a setup time)
    gpio->halfcycle (shadow.aluadd ());

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
static uint16_t recvdata ()
{
    // drop clock and wait for cpu to finish sending the data
    gpio->writegpio (false, syncintreq);
    gpio->halfcycle (shadow.aluadd ());

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

// if control-C with -script, stop the processor getting clocked
// this will eventually wake script out of 'wait' command if it is in one
// ...when it sees SF_STOPPED set
// also set a flag for scripts to test and don't allow unacknowleged control-C
static void scriptsigint (int signum)
{
    if ((isatty (0) > 0) && (write (0, "\n", 1) > 0)) { } // after the "^C"
    bool sigint = ctl_lock ();
    if (! ctrlcflag) {
        ctrlcflag = true;
        if (stopreason[0] == 0) {
            stopreason = "CONTROL_C";
        }
        stopflags |= SF_STOPIT;
        stopwake ();
        ctl_unlock (sigint);
        return;
    }
    ctl_unlock (sigint);

    // something is hung, die
    sighandler (signum);
}

// signal that terminates process, do exit() so exithandler() gets called
static void sighandler (int signum)
{
    signal (SIGHUP,  SIG_DFL);
    signal (SIGINT,  SIG_DFL);
    signal (SIGTERM, SIG_DFL);
    fprintf (stderr, "\nraspictl: terminated for signal %d\n", signum);
    pthread_t sigtid;
    if (pthread_create (&sigtid, NULL, sigthread, NULL) != 0) ABORT ();
}

// being requested to terminate, exit
// do this in a separate thread from main() because it is likely that sighandler() was called in main thread with a mutex locked
// calling exit() in a separate thread will allow main thread's mutex to be unlocked
static void *sigthread (void *dummy)
{
    pthread_detach (pthread_self ());
    exit (1);
}

// exiting, close everything down
static void exithandler ()
{
    if (! thisismainthread) {
        fprintf (stderr, "raspictl: stopping processor\n");
        ctl_stop ();
    }

    uint64_t cycles = shadow.getcycles ();
    fprintf (stderr, "raspictl: %llu cycle%s\n", (LLU) cycles, ((cycles == 1) ? "" : "s"));

    pidp_stop ();
    ioreset ();
    scncall.exithandler ();
    fprintf (stderr, "raspictl: closing %s\n", gpio->libname);
    gpio->close ();
    fprintf (stderr, "raspictl: exiting\n");
}
