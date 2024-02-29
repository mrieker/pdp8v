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
 * Program runs on the RasPI or a PC to test a single board via the ABCD paddles.
 * For the RPI board, it must be run on a RasPI plugged into the GPIO connector along with the paddles.
 *
 *  ./autoboardtest [-cpuhz <freq>] [-mult] [-pause] [-pipelib] [-verbose] <boards>...
 *
 *      <boards> : any combination of acl alu ma pc rpi seq
 *        -cpuhz : specify cpu frequency for hardware (default DEFCPUHZ Hz)
 *         -mult : use mainloop_mult() even if just one board
 *      -pipelib : test autoboardtest itself with ../modules/whole.mod via netgen TCL simulator
 *        -pause : pause at end of each cycle
 *      -verbose : print progress messages
 */

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "abcd.h"
#include "gpiolib.h"
#include "miscdefs.h"
#include "pindefs.h"
#include "rdcyc.h"
#include "readprompt.h"
#include "strprintf.h"

#define ever (;;)

#define ains inss[0]
#define bins inss[1]
#define cins inss[2]
#define dins inss[3]
#define aouts outss[0]
#define bouts outss[1]
#define couts outss[2]
#define douts outss[3]
#define hwacon hwcons[0]
#define hwbcon hwcons[1]
#define hwccon hwcons[2]
#define hwdcon hwcons[3]
#define smacon smcons[0]
#define smbcon smcons[1]
#define smccon smcons[2]
#define smdcon smcons[3]

enum struct OnError { Cont, Exit, Pause };

static bool looping;
static bool pagemode;
static bool pauseit;
static bool verbose;
static CSrcLib *simulatr;
static GpioLib *hardware;
static int stepcount;
static OnError onerror;
static uint32_t cycleno;
static uint32_t inss[NPADS];
static uint32_t outss[NPADS];
static uint32_t hwcons[NPADS];
static uint32_t smcons[NPADS];

static void sighandler (int signum);
static void exithandler ();
static void gotanerror ();
static void inccycleno ();
static void maybepause ();
static char const *onerrorstr ();
static void printvarvalue (GpioLib *lib, char const *label, char const *vname);

static void mainloop_pc ();

int main (int argc, char **argv)
{
    setlinebuf (stdout);

    onerror = OnError::Pause;
    verbose = true;

    // make sure we close gpio on exit
    signal (SIGABRT, sighandler);
    signal (SIGHUP,  sighandler);
    signal (SIGINT,  sighandler);
    signal (SIGTERM, sighandler);

    // close gpio on exit
    atexit (exithandler);

    // access tube circuitry and simulator
    hardware = new PhysLib ();
    simulatr = new CSrcLib (0, "pc");
    hardware->open ();
    simulatr->open ();

    // do testing
    mainloop_pc ();
    return 0;
}

// signal that terminates process, do exit() so exithandler() gets called
static void sighandler (int signum)
{
    if ((signum == SIGINT) && ! pauseit) {
        write (0, "\n", 1); // after the "^C"
        pauseit   = true;
        stepcount = 0;
        verbose   = true;
        return;
    }
    fprintf (stderr, "\nautoboardtest: terminated for signal %d\n", signum);
    exit (1);
}

// exiting
static void exithandler ()
{
    fprintf (stderr, "autoboardtest: exiting\n");

    // close gpio access
    if (hardware != NULL) hardware->close ();
    if (simulatr != NULL) simulatr->close ();
    hardware = NULL;
    simulatr = NULL;
}

static void gotanerror ()
{
    switch (onerror) {
        case OnError::Cont: break;
        case OnError::Exit: exit (1);
        case OnError::Pause: {
            verbose   = true;
            pauseit   = true;
            stepcount = 0;
            pagemode  = false;
            break;
        }
        default: ABORT ();
    }
}

// inc cycleno, print message once per min if quiet
static void inccycleno ()
{
    static uint32_t lasttime;

    ++ cycleno;
    if (! verbose) {
        uint32_t thistime = time (NULL) / 60;
        if (lasttime != thistime) {
            lasttime = thistime;
            printf ("%02u:%02u  %10u\n", thistime / 60 % 24, thistime % 60, cycleno);
        }
    }
}

// if pausing enabled, output prompt
static void maybepause ()
{
    static char laststep[80] = "s";

    if (! pauseit) return;

    if ((stepcount > 0) && (-- stepcount > 0)) return;

    while (true) {

        // read keyboard line
        char const *line = readprompt (" > ");
        if (line == NULL) {
            putchar ('\n');
            exit (0);
        }
        char str[80];
        char *q = str;
        for (char c; (c = *line) != 0; line ++) {
            if (c > ' ') *(q ++) = c;
            if (q >= str + sizeof str - 1) break;
        }
        *q = 0;

        // if null string, repeat previous command
        if (q == str) strcpy (str, laststep);
                 else strcpy (laststep, str);

        // 'c' = turn pause off and continue
        if (strcasecmp (str, "c") == 0) {
            pauseit   = false;
            stepcount = 0;
            return;
        }

        // 'e' = toggle onerror
        if (strcasecmp (str, "e") == 0) {
            switch (onerror) {
                case OnError::Cont:  onerror = OnError::Exit;  break;
                case OnError::Exit:  onerror = OnError::Pause; break;
                case OnError::Pause: onerror = OnError::Cont;  break;
                default: ABORT ();
            }
            printf ("  onerror = %s\n", onerrorstr ());
            continue;
        }

        // 'l' = loop to repeat this cycle
        if (strcasecmp (str, "l") == 0) {
            looping   = true;
            stepcount = 0;
            return;
        }

        // 'q' = quietly continue
        if (strcasecmp (str, "q") == 0) {
            pauseit   = false;
            stepcount = 0;
            verbose   = false;
            return;
        }

        // 's' = single step through next cycle
        if (tolower (str[0]) == 's') {
            stepcount = atoi (str + 1);
            if (stepcount <= 0) stepcount = 1;
            looping = false;
            return;
        }

        // '?' - help
        if (str[0] == '?') {
            printf ("  c - continue loudly\n");
            printf ("  e - toggle onerror (now %s)\n", onerrorstr ());
            printf ("  l - loop mode on & step\n");
            printf ("  q - continue quietly\n");
            printf ("  s[count] - loop mode off & step\n");
            continue;
        }

        // otherwise, look for that variable then print it
        printvarvalue (simulatr, "sim", str);
        printvarvalue (hardware, "hdw", str);
    }
}

static char const *onerrorstr ()
{
    switch (onerror) {
        case OnError::Cont:  return "Cont";
        case OnError::Exit:  return "Exit";
        case OnError::Pause: return "Pause";
        default: ABORT ();
    }
}

// print value of given variable
//  input:
//   lib = simulatr for simulator
//         hardware for hardware
//   label = "sim" for simulator
//           "hdw" for hardware
//   vname = name of variable
static void printvarvalue (GpioLib *lib, char const *label, char const *vname)
{
    printf ("  %s: %s =", label, vname);
    uint32_t value;
    int width = lib->examine (vname, &value);
    if (width <= 0) {
        printf (" %s\n", (width < 0) ? "noaccess" : "undefined");
    } else {
        while (-- width >= 0) {
            printf (" %o", (value >> width) & 1);
            if ((width > 0) && (width % 3 == 0)) putchar (' ');
        }
        putchar ('\n');
    }
}

// write values to all 4 circuit board signal connectors
// write same values to simulator's connectors
static void writebits (uint32_t acon, uint32_t bcon, uint32_t ccon, uint32_t dcon)
{
    // write the bits to the board via IOW56s
    // if -pipelib, send bits via pipe to netgen simulator

    uint32_t pinss[NPADS] = { acon, bcon, ccon, dcon };
    hardware->writepads (inss, pinss);

    // send same bits to the csrclib simulator

    simulatr->writepads (inss, pinss);
}

// it is end of a cycle, the board signal connector pins should match what the simulator has
// if not, print differences and enable pause mode
static void checkendcycle ()
{
    // read values from hardware via IOW56s
    // if -pipelib, get bits via pipe from netgen simulator

    hardware->readpads (hwcons);

    // read values from csrclib simulator

    simulatr->readpads (smcons);

    // mask off the bits that aren't connected to anything

    hwacon &= ains | aouts;
    hwbcon &= bins | bouts;
    hwccon &= cins | couts;
    hwdcon &= dins | douts;

    smacon &= ains | aouts;
    smbcon &= bins | bouts;
    smccon &= cins | couts;
    smdcon &= dins | douts;

    // make sure it all matches

    uint32_t adiff = hwacon ^ smacon;
    uint32_t bdiff = hwbcon ^ smbcon;
    uint32_t cdiff = hwccon ^ smccon;
    uint32_t ddiff = hwdcon ^ smdcon;
    if (adiff | bdiff | cdiff | ddiff) {
        printf ("cycleno=%u\n", cycleno);
        printf ("  hwacon=%08X  smacon=%08X  diff=%08X  %s\n", hwacon, smacon, adiff, pinstring (adiff, apindefs).c_str ());
        printf ("  hwbcon=%08X  smbcon=%08X  diff=%08X  %s\n", hwbcon, smbcon, bdiff, pinstring (bdiff, bpindefs).c_str ());
        printf ("  hwccon=%08X  smccon=%08X  diff=%08X  %s\n", hwccon, smccon, cdiff, pinstring (cdiff, cpindefs).c_str ());
        printf ("  hwdcon=%08X  smdcon=%08X  diff=%08X  %s\n", hwdcon, smdcon, ddiff, pinstring (ddiff, dpindefs).c_str ());
        gotanerror ();
    }
}

// give tubes time to process whatever was just written to the connectors
// likewise update simulator state
static void halfcycle ()
{
    hardware->halfcycle ();
    simulatr->halfcycle ();
}

// test PC board

static void mainloop_pc ()
{
    ains = A__aluq_11 | A__aluq_10 | A__aluq_09 | A__aluq_08 | A__aluq_07;
    bins = B__aluq_06 | B__aluq_05 | B__aluq_04 | B__aluq_03;
    cins = C__aluq_02 | C__aluq_01 | C__aluq_00;
    dins = D_clok2    | D__pc_aluq | D__pc_inc  | D_reset | D_defer2q;

    aouts = A_pcq_11  | A_pcq_10   | A_pcq_09   | A_pcq_08;
    bouts = B_pcq_07  | B_pcq_06   | B_pcq_05   | B_pcq_04   | B_pcq_03;
    couts = C_pcq_02  | C_pcq_01   | C_pcq_00;
    douts = 0;

    // reset the tubes and simulator
    writebits (0, 0, 0, D__pc_aluq | D__pc_inc | D_reset);
    halfcycle ();
    halfcycle ();
    checkendcycle ();
    inccycleno ();
    writebits (0, 0, 0, D__pc_aluq | D__pc_inc);
    halfcycle ();

    int stateno = 259;
    for ever {

        // in middle of cycle

        // send out command to load something into program counter at next clock up
        // also negate clock, leave reset negated

        ABCD cmdabcd;
        switch (stateno) {
            case 259: cmdabcd._pc_aluq = false; cmdabcd._pc_inc = true;  cmdabcd._aluq = 01631; stateno ++; printf ("----------\n"); break;
            case 260: cmdabcd._pc_aluq = true;  cmdabcd._pc_inc = true;  cmdabcd._aluq = 05254; stateno ++; break;
            case 261: cmdabcd._pc_aluq = false; cmdabcd._pc_inc = false; cmdabcd._aluq = 04125; stateno ++; break;
            case 262: cmdabcd._pc_aluq = false; cmdabcd._pc_inc = false; cmdabcd._aluq = 07206; stateno ++; break;
            case 263: cmdabcd._pc_aluq = false; cmdabcd._pc_inc = false; cmdabcd._aluq = 02076; stateno ++; break;
            case 264: cmdabcd._pc_aluq = false; cmdabcd._pc_inc = true;  cmdabcd._aluq = 05555; stateno ++; break;
            case 265: cmdabcd._pc_aluq = false; cmdabcd._pc_inc = true;  cmdabcd._aluq = 00101; stateno ++; break;
            case 266: cmdabcd._pc_aluq = true;  cmdabcd._pc_inc = false; cmdabcd._aluq = 01514; stateno ++; cmdabcd.defer2q = true; break;
            case 267: cmdabcd._pc_aluq = true;  cmdabcd._pc_inc = false; cmdabcd._aluq = 02535; stateno = 259; break;
        }
        cmdabcd.encode ();
        writebits (cmdabcd.acon, cmdabcd.bcon, cmdabcd.ccon, cmdabcd.dcon);

        // wait for end of cycle and compare with what simulator thinks

        halfcycle ();
        if (false) checkendcycle ();
        else hardware->readpads (hwcons);

        if (verbose) {
            ABCD verabcd;
            verabcd.acon = hwacon;
            verabcd.bcon = hwbcon;
            verabcd.ccon = hwccon;
            verabcd.dcon = hwdcon;
            verabcd.decode ();

            // print hardware value at end of cycle + what's going to happen at next clock up
            printf ("%10u: pcq=%04o + _pc_aluq=%o _pc_inc=%o _aluq=%04o defer2q=%o =>\n",
                    cycleno, verabcd.pcq, verabcd._pc_aluq, verabcd._pc_inc, verabcd._aluq, verabcd.defer2q);

            maybepause ();
        }

        // about to start a new cycle
        inccycleno ();

        // raise clock then wait half a cycle, leaving all the other inputs as is
        // this clocks the new state into the tubes and simulator

        writebits (cmdabcd.acon, cmdabcd.bcon, cmdabcd.ccon, cmdabcd.dcon | D_clok2);
        halfcycle ();
    }
}
