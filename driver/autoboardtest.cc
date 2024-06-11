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
 * Program runs on the RasPI or a PC to test one or more boards via the ABCD paddles.
 * For the RPI board, it must be run on a RasPI plugged into the GPIO connector along with the paddles.
 * It generates random inputs and checks the outputs.
 *
 *  export addhz=<addhz>
 *  export cpuhz=<cpuhz>
 *  ./autoboardtest [-csrclib] [-mult] [-page] [-pause] [-pipelib] [-verbose] [-zynqlib] <boards>...
 *
 *      <boards> : all or any combination of acl alu ma pc rpi seq
 *       <addhz> : specify cpu frequency for add cycles (default cpuhz Hz)
 *       <cpuhz> : specify cpu frequency for all other cycles (default DEFCPUHZ Hz)
 *      -csrclib : C-based simulator for debugging autoboardtest itself on PC
 *         -mult : use mainloop_mult() even if just one board
 *      -pipelib : test autoboardtest itself with ../modules/whole.mod via netgen TCL simulator
 *         -page : paginate -verbose output
 *        -pause : pause at end of each cycle
 *      -verbose : print progress messages
 *      -zynqlib : test zynq instead of tubes
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
#define arand rands[0]
#define brand rands[1]
#define crand rands[2]
#define drand rands[3]
#define hwacon hwcons[0]
#define hwbcon hwcons[1]
#define hwccon hwcons[2]
#define hwdcon hwcons[3]
#define smacon smcons[0]
#define smbcon smcons[1]
#define smccon smcons[2]
#define smdcon smcons[3]

enum struct OnError { Cont, Exit, Pause };

struct Module {
    void (*mainloop) ();
    char const *name;
    uint32_t inss[NPADS];
    uint32_t outss[NPADS];
    bool selected;
};

static void mainloop_acl ();
static void mainloop_alu ();
static void mainloop_ma  ();
static void mainloop_pc  ();
static void mainloop_rpi ();

static Module aclmod = { mainloop_acl, "acl", acl_ains, acl_bins, acl_cins, acl_dins, acl_aouts, acl_bouts, acl_couts, acl_douts };
static Module alumod = { mainloop_alu, "alu", alu_ains, alu_bins, alu_cins, alu_dins, alu_aouts, alu_bouts, alu_couts, alu_douts };
static Module mamod  = { mainloop_ma,  "ma",  ma_ains,  ma_bins,  ma_cins,  ma_dins,  ma_aouts,  ma_bouts,  ma_couts,  ma_douts  };
static Module pcmod  = { mainloop_pc,  "pc",  pc_ains,  pc_bins,  pc_cins,  pc_dins,  pc_aouts,  pc_bouts,  pc_couts,  pc_douts  };
static Module rpimod = { mainloop_rpi, "rpi", rpi_ains, rpi_bins, rpi_cins, rpi_dins, rpi_aouts, rpi_bouts, rpi_couts, rpi_douts };
static Module seqmod = {         NULL, "seq", seq_ains, seq_bins, seq_cins, seq_dins, seq_aouts, seq_bouts, seq_couts, seq_douts };

#define NMODULES 6
static Module *const modules[NMODULES] = { &aclmod, &alumod, &mamod, &pcmod, &rpimod, &seqmod };

static bool looping;
static bool pagemode;
static bool pauseit;
static bool verbose;
static CSrcLib *simulatr;
static GpioLib *hardware;
static int stepcount;
static OnError onerror;
static std::string multitle;
static uint32_t cycleno;
static uint32_t inss[NPADS], gins;
static uint32_t outss[NPADS], gouts;
static uint32_t hwcons[NPADS], hwgcon;
static uint32_t smcons[NPADS], smgcon;
static uint32_t rands[NPADS];
static uint64_t seed = 0x123456789ABCDEF0ULL;

static uint32_t randuint32 (int nbits);
static void sighandler (int signum);
static void exithandler ();
static void gotanerror ();
static void inccycleno ();
static void maybepause ();
static char const *onerrorstr ();
static void printvarvalue (GpioLib *lib, char const *label, char const *vname);
static void mainloop_mult ();

int main (int argc, char **argv)
{
    bool csrclib = false;
    bool multflag = false;
    bool pipelib = false;
    bool zynqlib = false;

    setlinebuf (stdout);

    onerror = OnError::Pause;

    for (int i = 0; ++ i < argc;) {
        if (strcasecmp (argv[i], "-csrclib") == 0) {
            csrclib = true;
            pipelib = false;
            zynqlib = false;
            continue;
        }
        if (strcasecmp (argv[i], "-mult") == 0) {
            multflag = true;
            continue;
        }
        if (strcasecmp (argv[i], "-page") == 0) {
            pagemode = true;
            continue;
        }
        if (strcasecmp (argv[i], "-pause") == 0) {
            pauseit = true;
            verbose = true;
            continue;
        }
        if (strcasecmp (argv[i], "-pipelib") == 0) {
            csrclib = false;
            pipelib = true;
            zynqlib = false;
            continue;
        }
        if (strcasecmp (argv[i], "-verbose") == 0) {
            verbose = true;
            continue;
        }
        if (strcasecmp (argv[i], "-zynqlib") == 0) {
            csrclib = false;
            pipelib = false;
            zynqlib = true;
            continue;
        }
        if (argv[i][0] == '-') {
            fprintf (stderr, "autoboardtest: unknown option %s\n", argv[i]);
            return 1;
        }
        if (strcasecmp (argv[i], "all") == 0) {
            for (int j = 0; j < NMODULES; j ++) {
                modules[j]->selected = true;
            }
            continue;
        }
        for (int j = 0; j < NMODULES; j ++) {
            Module *mod = modules[j];
            if (strcasecmp (argv[i], mod->name) == 0) {
                mod->selected = true;
                goto gotit;
            }
        }
        fprintf (stderr, "autoboardtest: unknown module %s\n", argv[i]);
        return 1;
    gotit:;
    }

    // get selected module test function entrypoint
    // concat all selected module names in alphabetical order to get pipelib module name
    int nseldmods = 0;
    std::string pipelibmodstr;
    void (*mainloop) () = NULL;
    for (int j = 0; j < NMODULES; j ++) {
        Module const *mod = modules[j];
        if (mod->selected) {
            mainloop = mod->mainloop;
            pipelibmodstr.append (mod->name);
            if (nseldmods > 0) multitle.push_back (' ');
            multitle.append (mod->name);
            ++ nseldmods;
        }
    }
    if (nseldmods == 0) {
        fprintf (stderr, "autoboardtest: no board(s) given\n");
        return 1;
    }
    char const *pipelibmodcst = pipelibmodstr.c_str ();
    char const *csrclibmodcst = pipelibmodcst;
    if (multflag || (mainloop == NULL) || (nseldmods > 1)) {
        mainloop = mainloop_mult;
        csrclibmodcst = "aclalumapcrpiseq";
    }

    // make sure we close gpio on exit
    signal (SIGABRT, sighandler);
    signal (SIGHUP,  sighandler);
    signal (SIGINT,  sighandler);
    signal (SIGTERM, sighandler);

    // close gpio on exit
    atexit (exithandler);

    // access tube circuitry and simulator
    hardware = zynqlib ? (GpioLib *) new ZynqLib (pipelibmodcst) :
               pipelib ? (GpioLib *) new PipeLib (pipelibmodcst) :
               csrclib ? (GpioLib *) new CSrcLib (pipelibmodcst) :
                         (GpioLib *) new PhysLib ();
    simulatr = new CSrcLib (csrclibmodcst);
    hardware->open ();
    simulatr->open ();

    // print initial register contents
    if (aclmod.selected | mamod.selected | pcmod.selected | seqmod.selected) {
        ABCD abcd;
        hardware->readpads (abcd.cons);
        abcd.decode ();
        printf ("autoboardtest:");
        if (aclmod.selected) printf (" L.AC=%o.%04o", abcd.lnq, abcd.acq);
        if (mamod.selected) printf (" MA=%04o", abcd.maq);
        if (pcmod.selected) printf (" PC=%04o", abcd.pcq);
        if (seqmod.selected) printf (" IR=%o--- ST=%s", abcd.irq >> 9, abcd.states ().c_str ());
        putchar ('\n');
    }

    // do testing
    mainloop ();
    return 0;
}

// generate a random number
static uint32_t randuint32 (int nbits)
{
    uint32_t randval = 0;

    for (int i = 0; i < nbits; i ++) {

        // https://www.xilinx.com/support/documentation/application_notes/xapp052.pdf
        uint64_t xnor = ~ ((seed >> 63) ^ (seed >> 62) ^ (seed >> 60) ^ (seed >> 59));
        seed = (seed << 1) | (xnor & 1);

        randval += randval + (seed & 1);
    }

    return randval;
}

// signal that terminates process, do exit() so exithandler() gets called
static void sighandler (int signum)
{
    if ((signum == SIGINT) && ! pauseit) {
        int ignored __attribute__ ((unused)) = write (0, "\n", 1); // after the "^C"
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

// generate random number for A,B,C,D pins
// always returns with clock and reset negated
// leave the numbers alone if set to repeat same cycle
static void nextrands ()
{
    if (! looping) {
        arand = randuint32 (32);
        brand = randuint32 (32);
        crand = randuint32 (32);
        drand = randuint32 (32);
    }
    drand &= ~ (D_clok2 | D_reset);
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
static bool checkendcycle ()
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
        return true;
    }
    return false;
}

// give tubes time to process whatever was just written to the connectors
// likewise update simulator state
static void halfcycle ()
{
    hardware->halfcycle ();
    simulatr->halfcycle ();
}

// test multiple modules by simulating the full processor and using the simulated bits to fill in missing boards
// the sim is stepped through normal instruction sequences along with whatever boards are present

static void multhalfcycle (bool grpbskipvalid, std::string *str, char const *desc);

static void mainloop_mult ()
{
    // get list of all inputs and outputs for selected boards
    for (int i = 0; i < NMODULES; i ++) {
        Module const *mod = modules[i];
        if (mod->selected) {
            for (int p = 0; p < NPADS; p ++) {
                inss[p]  |= mod->inss[p];
                outss[p] |= mod->outss[p];
            }
        }
    }

    // we don't write to inputs that are output by other boards
    for (int p = 0; p < NPADS; p ++) {
        inss[p] &= ~ outss[p];
    }

    // {a,b,c,d}ins = pins used as inputs by selected boards that aren't written by any selected board
    //                these must be filled in with simulated values by writing the bits to the paddles
    // {a,b,c,d}outs = pins written by selected boards
    //                 these are verified to match simulator at end of each half cycle by reading bits from the paddles

    // apply reset for a full cycle
    if (rpimod.selected) {
        hardware->writegpio (false, G_RESET);

        // also set up gpio pin bits
        gouts  = G_INS;     // outputs from RPI board (eg, mread)
        gins   = G_OUTS;    // inputs to RPI board (eg, clock)

        // MQ,MQL pins on abcd connectors are not valid because wdata=false passed to writegpio()
        aouts &= ~ (A_mq_11 | A_mq_10 | A_mq_09 | A_mq_08);
        bouts &= ~ (B_mq_07 | B_mq_06 | B_mq_05 | B_mq_04 | B_mq_03);
        couts &= ~ (C_mq_02 | C_mq_01 | C_mq_00);
        douts &= ~ (D_mql);
    } else {
        static uint32_t const resets[] = { 0, 0, 0, D_reset };
        hardware->writepads (inss, resets);
    }
    simulatr->writegpio (false, G_RESET);
    hardware->halfcycle ();
    hardware->halfcycle ();
    simulatr->halfcycle ();
    simulatr->halfcycle ();

    // read hardware AC, LINK, IR and MA, jam into simulator flipflops and latches
    // pipelib will have x's for this stuff
    ABCD abcd;
    hardware->readpads (abcd.cons);
    abcd.decode ();

    printf ("  reset hw AC=%04o  L=%o  _L=%o  IR=%04o  MA=%04o  _MA=%04o\n", abcd.acq, abcd.lnq, abcd._lnq, abcd.irq, abcd.maq, abcd._maq);
    uint16_t oldir = abcd.irq;

    *simulatr->getvarbool ("_Q/lnreg/aclcirc", 0) =  abcd.lnq;
    *simulatr->getvarbool  ("Q/lnreg/aclcirc", 0) = abcd._lnq;

    for (int i = 0; i < 4; i ++) {
        *simulatr->getvarbool  ("Q/achi/aclcirc",  i) =   (abcd.acq >> (8 + i)) & 1;
        *simulatr->getvarbool  ("Q/acmid/aclcirc", i) =   (abcd.acq >> (4 + i)) & 1;
        *simulatr->getvarbool  ("Q/aclo/aclcirc",  i) =   (abcd.acq >> (0 + i)) & 1;
        *simulatr->getvarbool ("_Q/achi/aclcirc",  i) = ~ (abcd.acq >> (8 + i)) & 1;
        *simulatr->getvarbool ("_Q/acmid/aclcirc", i) = ~ (abcd.acq >> (4 + i)) & 1;
        *simulatr->getvarbool ("_Q/aclo/aclcirc",  i) = ~ (abcd.acq >> (0 + i)) & 1;

        *simulatr->getvarbool ("_Q/mahi/macirc",   i) =   (abcd.maq >> (8 + i)) & 1;
        *simulatr->getvarbool ("_Q/mamid/macirc",  i) =   (abcd.maq >> (4 + i)) & 1;
        *simulatr->getvarbool ("_Q/malo/macirc",   i) =   (abcd.maq >> (0 + i)) & 1;
        *simulatr->getvarbool  ("Q/mahi/macirc",   i) =  (abcd._maq >> (8 + i)) & 1;
        *simulatr->getvarbool  ("Q/mamid/macirc",  i) =  (abcd._maq >> (4 + i)) & 1;
        *simulatr->getvarbool  ("Q/malo/macirc",   i) =  (abcd._maq >> (0 + i)) & 1;
    }

    for (int i = 9; i < 12; i ++) {
        char _varname[20];
        sprintf (_varname, "_Q/ireg%02d/seqcirc", i);
        *simulatr->getvarbool (_varname, 0) =   ~ (abcd.irq >> i) & 1;
        *simulatr->getvarbool (_varname + 1, 0) = (abcd.irq >> i) & 1;
    }

    bool clearacandlink = true;
    bool grpbskipvalid = false;
    bool intrequest = false;
    bool sendrandata = false;

    bool const *sim_fetch2d = simulatr->getvarbool ("_fetch2d/seqcirc", 0);
    bool const *sim_fetch2q = simulatr->getvarbool ("_fetch2q/seqcirc", 0);
    std::string str;

    while (true) {

        // at beginning of cycle with clock just (or reset still) asserted

        multhalfcycle (grpbskipvalid, &str, "midway");

        // at midpoint of cycle with clock or reset still asserted

        // negate clock and update other gpio pins going to tubes and simulator
        //  - G_IOS always gets a new random bit
        //  - G_IRQ gets set randomly rarely, cleared on interrupt acknowledge
        //  - G_LINK,G_DATA get random bits but they get sent only if last cycle had MREAD or IOINST set
        //  - G_RESET,G_CLOCK are always cleared
        uint32_t grand = randuint32 (32) & G_ALL;
        if (clearacandlink & sendrandata) {
            grand = (grand & ~ G_DATA) | (07300 * G_DATA0); // CLA CLL so we know AC,LINK in pipelib
            clearacandlink = false;
        }
        if (randuint32 (4) == 0) intrequest = true; // 1/16th chance
        smgcon = (grand & (G_IOS | G_DATA | G_LINK)) | (intrequest ? G_IRQ : 0) | (sendrandata ? G_QENA : G_DENA);
        if (rpimod.selected) {
            hardware->writegpio (sendrandata, smgcon);
            if (sendrandata) {
                // sending data to tubes
                // - the gpio pins are inputting data from the raspi
                // - the MQL and MQ abcd pins are valid outputs
                gouts &= ~ (G_DATA  | G_LINK);
                gins  |=    G_DATA  | G_LINK;
                aouts |=    A_mq_11 | A_mq_10 | A_mq_09 | A_mq_08;
                bouts |=    B_mq_07 | B_mq_06 | B_mq_05 | B_mq_04 | B_mq_03;
                couts |=    C_mq_02 | C_mq_01 | C_mq_00;
                douts |=    D_mql;

                // if in middle of FETCH2, save new IR contents
                if (seqmod.selected && ! *sim_fetch2q) oldir = (smgcon & G_DATA) / G_DATA0;
            } else {
                // receiving data from tubes
                // - the gpio pins are outputting data to the raspi
                // - the MQL and MQ abcd pins are floating so don't verify
                gouts |=    G_DATA  | G_LINK;
                gins  &= ~ (G_DATA  | G_LINK);
                aouts &= ~ (A_mq_11 | A_mq_10 | A_mq_09 | A_mq_08);
                bouts &= ~ (B_mq_07 | B_mq_06 | B_mq_05 | B_mq_04 | B_mq_03);
                couts &= ~ (C_mq_02 | C_mq_01 | C_mq_00);
                douts &= ~ (D_mql);
            }
        } else {
            // jam gpio output updates to abcdcons immediately
            smacon &= ~ (A_mq_11 | A_mq_10 | A_mq_09 | A_mq_08);
            smbcon &= ~ (B_mq_07 | B_mq_06 | B_mq_05 | B_mq_04 | B_mq_03);
            smccon &= ~ (C_mq_02 | C_mq_01 | C_mq_00);
            smdcon &= ~ (D_clok2 | D_reset | D_mql | D_intrq | D_ioskp);
            if (smgcon & (G_DATA0 << 11)) smacon |= A_mq_11;
            if (smgcon & (G_DATA0 << 10)) smacon |= A_mq_10;
            if (smgcon & (G_DATA0 <<  9)) smacon |= A_mq_09;
            if (smgcon & (G_DATA0 <<  8)) smacon |= A_mq_08;
            if (smgcon & (G_DATA0 <<  7)) smbcon |= B_mq_07;
            if (smgcon & (G_DATA0 <<  6)) smbcon |= B_mq_06;
            if (smgcon & (G_DATA0 <<  5)) smbcon |= B_mq_05;
            if (smgcon & (G_DATA0 <<  4)) smbcon |= B_mq_04;
            if (smgcon & (G_DATA0 <<  3)) smbcon |= B_mq_03;
            if (smgcon & (G_DATA0 <<  2)) smccon |= C_mq_02;
            if (smgcon & (G_DATA0 <<  1)) smccon |= C_mq_01;
            if (smgcon & (G_DATA0 <<  0)) smccon |= C_mq_00;
            if (smgcon & G_LINK) smdcon |= D_mql;
            if (smgcon & G_IRQ)  smdcon |= D_intrq;
            if (smgcon & G_IOS)  smdcon |= D_ioskp;
            hardware->writepads (inss, smcons);
        }
        simulatr->writegpio ((smgcon & G_QENA) != 0, smgcon);

        // at midpoint of cycle with clock just negated

        multhalfcycle (grpbskipvalid, &str, "end of");

        // at end of cycle with clock still negated

        // clear interrupt request if just did an interrupt acknowledge
        if (! (smdcon & D__intak)) intrequest = false;

        // remember if it had MREAD or IOINST set
        // we will send random G_LINK,G_DATA next bus cycle if so
        sendrandata = ! (smdcon & D__mread) || (smdcon & D_ioinst);

        // grpb_skip becomes valid when link is written (always accompanied by writing accumulator)
        if (! (smdcon & D__ln_wrt)) grpbskipvalid = true;

        // about to start next cycle
        inccycleno ();

        // assert clock, do not change other gpio pins so the values given above hold through the transition and soak into tubes
        bool doreset = (randuint32 (4) & 15) == 0;
        if (doreset) {
            smdcon |= D_reset;
            smgcon |= G_RESET;
        } else {
            smdcon |= D_clok2;
            smgcon |= G_CLOCK;
            if (rpimod.selected && seqmod.selected && ! *sim_fetch2d) {
                // but if about to start FETCH2, output old IR contents to MQ bus cuz tubes open IR latch
                smgcon = (smgcon & ~ G_DENA & ~ G_DATA) | G_QENA | (oldir * G_DATA0);
            }
        }
        if (rpimod.selected) {
            hardware->writegpio ((smgcon & G_QENA) != 0, smgcon);
        } else {
            hardware->writepads (inss, smcons);
        }
        simulatr->writegpio ((smgcon & G_QENA) != 0, smgcon);
    }
}

// called right after clock transition written to hardware gpio and bus pins and to simulator gpio
// waits for tubes to update, updates sim state to end of halfcycle
// if end of cycle (ie, clock low), compares output pins to make sure they match
//  * wait 1/4 cycle for tube DFFs to update with data from before clock transition
//  * update sim state to end of halfcycle
//  * fill in missing hardware output pins with what sim has for them
//    eg, if missing ACL board, fills in missing AC and LINK values
//  * wait 1/4 cycle to give missing pins time to soak into tubes
//  * if clock low, compare tube output pins to simulator
static uint32_t oldcyclenos[20];
static void multhalfcycle (bool grpbskipvalid, std::string *str, char const *desc)
{
    // give tube flipflips time to digest their inputs
    hardware->halfcycle (); // qtrcycle

    // update simulator state to end of halfcycle
    simulatr->halfcycle ();

    // read simulator pins
    simulatr->readpads (smcons);
    smgcon = simulatr->readgpio ();

    // write simulator pins to hardware to fill in pins for missing modules
    hardware->writepads (inss, smcons);

    // give those missing bits time to soak into tubes
    hardware->halfcycle (); // qtrcycle

    // read hardware at end of half cycle
    hardware->readpads (hwcons);
    if (gins | gouts) {
        hwgcon = hardware->readgpio ();
    }

    // compare the bits we care about
    // for bits we don't care about, the sim will have what should be there, the hardware will have 1s
    uint32_t amask = inss[0] | outss[0];
    uint32_t bmask = inss[1] | outss[1];
    uint32_t cmask = inss[2] | outss[2];
    uint32_t dmask = inss[3] | outss[3];
    uint32_t gmask = gins | gouts;
    uint32_t adiff = (hwacon ^ smacon) & amask;
    uint32_t bdiff = (hwbcon ^ smbcon) & bmask;
    uint32_t cdiff = (hwccon ^ smccon) & cmask;
    uint32_t ddiff = (hwdcon ^ smdcon) & dmask;
    uint32_t gdiff = (hwgcon ^ smgcon) & gmask;

    if (! grpbskipvalid) ddiff &= ~ D_grpb_skip;

    // check for error only at end of cycle, not middle
    // eg, alu outputs may not be correct yet
    bool endofcycle = ! (smdcon & (D_clok2 | D_reset));
    if (endofcycle && (adiff | bdiff | cdiff | ddiff | gdiff)) {
        gotanerror ();
    }

    if (verbose) {
        uint32_t now = time (NULL);
        uint32_t dc = cycleno - oldcyclenos[(now+10)%20];
        oldcyclenos[now%20] = cycleno;
        char const *startpage = "";
        if (pagemode && ! pauseit) {
            startpage = "\033[H";   // home cursor
        }
        str->clear ();
        strprintf (str, "%s" ESC_EREOL "\n", startpage);
        strprintf (str, "%s ---------------- cycle %10u %s  %4.1f cps" ESC_EREOL "\n", multitle.c_str (), cycleno, desc, dc / 10.0);
        strprintf (str, "" ESC_EREOL "\n");
        if (endofcycle) {
            strprintf (str, "  hwacon=%08X  smacon=%08X  adiff=%08X  %s" ESC_EREOL "\n", hwacon & amask, smacon & amask, adiff, pinstring (adiff, apindefs).c_str ());
            strprintf (str, "  hwbcon=%08X  smbcon=%08X  bdiff=%08X  %s" ESC_EREOL "\n", hwbcon & bmask, smbcon & bmask, bdiff, pinstring (bdiff, bpindefs).c_str ());
            strprintf (str, "  hwccon=%08X  smccon=%08X  cdiff=%08X  %s" ESC_EREOL "\n", hwccon & cmask, smccon & cmask, cdiff, pinstring (cdiff, cpindefs).c_str ());
            strprintf (str, "  hwdcon=%08X  smdcon=%08X  ddiff=%08X  %s" ESC_EREOL "\n", hwdcon & dmask, smdcon & dmask, ddiff, pinstring (ddiff, dpindefs).c_str ());
        } else {
            strprintf (str, "  hwacon=%08X  smacon=%08X" ESC_EREOL "\n", hwacon & amask, smacon & amask);
            strprintf (str, "  hwbcon=%08X  smbcon=%08X" ESC_EREOL "\n", hwbcon & bmask, smbcon & bmask);
            strprintf (str, "  hwccon=%08X  smccon=%08X" ESC_EREOL "\n", hwccon & cmask, smccon & cmask);
            strprintf (str, "  hwdcon=%08X  smdcon=%08X" ESC_EREOL "\n", hwdcon & dmask, smdcon & dmask);
        }

        if (gins | gouts) {
            char gdiffdata[12];
            sprintf (gdiffdata, " data=%04o", (gdiff & G_DATA) / G_DATA0);

            std::string gdiffstr;
            if (gdiff & G_CLOCK) gdiffstr.append (" clock");
            if (gdiff & G_RESET) gdiffstr.append (" reset");
            if (gdiff & G_DATA)  gdiffstr.append (gdiffdata);
            if (gdiff & G_LINK)  gdiffstr.append (" link");
            if (gdiff & G_IOS)   gdiffstr.append (" ios");
            if (gdiff & G_QENA)  gdiffstr.append (" qena");
            if (gdiff & G_IRQ)   gdiffstr.append (" irq");
            if (gdiff & G_DENA)  gdiffstr.append (" dena");
            if (gdiff & G_JUMP)  gdiffstr.append (" jump");
            if (gdiff & G_IOIN)  gdiffstr.append (" ioin");
            if (gdiff & G_DFRM)  gdiffstr.append (" dfrm");
            if (gdiff & G_READ)  gdiffstr.append (" read");
            if (gdiff & G_WRITE) gdiffstr.append (" write");
            if (gdiff & G_IAK)   gdiffstr.append (" iak");

            if (endofcycle) {
                strprintf (str, "  hwgcon=%08X  smgcon=%08X  gdiff=%08X %s" ESC_EREOL "\n", hwgcon, smgcon, gdiff, gdiffstr.c_str ());
            } else {
                strprintf (str, "  hwgcon=%08X  smgcon=%08X" ESC_EREOL "\n", hwgcon, smgcon);
            }
        }

        if (endofcycle && (adiff | bdiff | cdiff | ddiff)) {
            uint32_t diffs[NPADS] = { adiff, bdiff, cdiff, ddiff };
            strprintf (str, ESC_EREOL "\n");
            for (int p = 0; p < NPADS; p ++) {
                uint32_t diff = diffs[p];
                uint32_t hwcon = hwcons[p];
                uint32_t smcon = smcons[p];
                for (int i = 0; i < 32; i ++) {
                    uint32_t m = 1U << i;
                    if (diff & m) {
                        char const *pn = "";
                        for (PinDefs const *pd = pindefss[p]; pd->pinmask != 0; pd ++) {
                            if (pd->pinmask & m) {
                                pn = pd->varname;
                                break;
                            }
                        }
                        strprintf (str, "   PIN %c%02d (%s) is %s should be %s" ESC_EREOL "\n",
                            'A' + p, i + 1, pn, hwcon & m ? "HIGH" : "LOW", smcon & m ? "HIGH" : "LOW");
                    }
                }
            }
        }

        strprintf (str, ESC_EREOL "\n");
        ABCD abcd;
        abcd.acon = smacon;
        abcd.bcon = smbcon;
        abcd.ccon = smccon;
        abcd.dcon = smdcon;
        abcd.gcon = smgcon;
        abcd.decode ();
        abcd.printfull<std::string> (strprintf, str, aclmod.selected, alumod.selected, mamod.selected, pcmod.selected, rpimod.selected, seqmod.selected);
        str->append (ESC_EREOP);

        char const *cst = str->c_str ();
        int len = str->length ();
        do {
            int rc = write (fileno (stdout), cst, len);
            if (rc <= 0) ABORT ();
            cst += rc;
            len -= rc;
        } while (len > 0);

        maybepause ();
    }
}

// test ACL board

static void mainloop_acl ()
{
    ains = A__aluq_11 | A__aluq_10 | A__aluq_09 | A__aluq_08 | A__aluq_07;
    bins = B__aluq_06 | B_maq_06 | B__aluq_05 | B_maq_05 | B__aluq_04 | B_maq_04 | B__aluq_03 | B__maq_03 | B_maq_03;
    cins = C__aluq_02 | C__maq_02 | C_maq_02 | C__ac_sc | C__aluq_01 | C__maq_01 | C_maq_01 | C__aluq_00 | C__ac_aluq;
    dins = D_clok2 | D__grpa1q | D_iot2q | D__ln_wrt | D_mql | D_reset | D__newlink | D_tad3q;

    // AC,L contents unknown to start so don't check them till loaded
    aouts = bouts = couts = douts = 0;

    // reset the tubes and simulator
    writebits (0, 0, C__ac_aluq, D_reset);
    halfcycle ();
    halfcycle ();
    checkendcycle ();
    inccycleno ();
    writebits (0, 0, C__ac_aluq, 0);
    halfcycle ();

    uint32_t c__ac_aluq = C__ac_aluq;

    for ever {

        // in middle of cycle

        // send out random values to its input pins
        // also negate clock, leave reset negated, leave _ac_aluq alone
        // _ac_aluq gets sampled on fall8ng edge of clock
        // - if low (asserted), AC and possibly LINK get written at next rising clock edge

        nextrands ();
        writebits (arand, brand, crand, (drand & ~ C__ac_aluq) | c__ac_aluq);

        // wait for end of cycle and compare with what simulator thinks

        halfcycle ();
        bool err = checkendcycle ();

        if (verbose) {
            for (int i = 0; i < 2; i ++) {
                ABCD abcd;
                abcd.acon = i ? hwacon : smacon;
                abcd.bcon = i ? hwbcon : smbcon;
                abcd.ccon = i ? hwccon : smccon;
                abcd.dcon = i ? hwdcon : smdcon;
                abcd.decode ();

                if (!i) {
                    uint32_t _lnq, rotq;
                    if (simulatr->examine ("_lnq/aclcirc", &_lnq) !=  1) ABORT ();
                    if (simulatr->examine ("rotq/aclcirc", &rotq) != 12) ABORT ();
                    printf ("%10u %s: _ac_aluq=%o _ln_wrt=%o _ac_sc=%o _aluq=%04o maq[6:1]=%02o _maq[3:1]=%o _newlink=%o _lnq=%o rotq=%04o => acq=%04o acqzero=%o _lnq=%o lnq=%o grpb_skip=%o\n",
                            cycleno, (err ? "sim" : "   "),
                            abcd._ac_aluq, abcd._ln_wrt, abcd._ac_sc, abcd._aluq, (abcd.maq >> 1) & 63, (abcd._maq >> 1) & 7, abcd._newlink, _lnq, rotq,
                            abcd.acq, abcd.acqzero, abcd._lnq, abcd.lnq, abcd.grpb_skip);
                    if (! err) break;
                } else {
                    printf ("           hdw: _ac_aluq=%o _ln_wrt=%o _ac_sc=%o _aluq=%04o maq[6:1]=%02o _maq[3:1]=%o _newlink=%o                  => acq=%04o acqzero=%o _lnq=%o lnq=%o grpb_skip=%o\n",
                            abcd._ac_aluq, abcd._ln_wrt, abcd._ac_sc, abcd._aluq, (abcd.maq >> 1) & 63, (abcd._maq >> 1) & 7, abcd._newlink,
                            abcd.acq, abcd.acqzero, abcd._lnq, abcd.lnq, abcd.grpb_skip);
                }
            }

            maybepause ();
        }

        // if next clock up will load AC, AC bits will be known so check them from then on

        if (! c__ac_aluq) {
            if (aouts == 0) printf ("ac will be checked starting next cycle\n");
            aouts = A_acq_11 | A_acq_10 | A_acq_09 | A_acq_08 | A_acq_07;
            bouts = B_acq_06 | B_acq_05 | B_acq_04 | B_acqzero | B_acq_03;
            couts = C_acq_02 | C_acq_01 | C_acq_00;

            // link gets written along with AC when clock raised if _ln_wrt is asserted

            if (! (drand & D__ln_wrt)) {
                if (douts == 0) printf ("link will be checked starting next cycle\n");
                douts = D_grpb_skip | D__lnq | D_lnq;
            }
        }

        // about to start a new cycle
        inccycleno ();

        // raise clock then wait half a cycle, possibly changing _ac_aluq for the new cycle (sampled on falling edge of clock)
        // keep _aluq, _newlink inputs steady so they get clocked in if _ac_aluq was asserted last cycle

        writebits (arand, brand, crand, drand | D_clok2);
        halfcycle ();

        // remember what we sent it for _ac_aluq

        c__ac_aluq = drand & C__ac_aluq;
    }
}

// test ALU board

static void mainloop_alu ()
{
    ains = A__maq_07 | A__maq_08 | A__maq_09 | A__maq_10 | A__maq_11 | A_acq_07 | A_acq_08 | A_acq_09 | A_acq_10 | A_acq_11 | A_maq_08 | A_maq_09 | A_maq_10 | A_maq_11 | A_mq_08 | A_mq_09 | A_mq_10 | A_mq_11 | A_pcq_08 | A_pcq_09 | A_pcq_10 | A_pcq_11;
    bins = B__alub_m1 | B__maq_03 | B__maq_04 | B__maq_05 | B__maq_06 | B_acq_03 | B_acq_04 | B_acq_05 | B_acq_06 | B_maq_03 | B_maq_04 | B_maq_05 | B_maq_06 | B_maq_07 | B_mq_03 | B_mq_04 | B_mq_05 | B_mq_06 | B_mq_07 | B_pcq_03 | B_pcq_04 | B_pcq_05 | B_pcq_06 | B_pcq_07;
    cins = C__alu_add | C__alu_and | C__alua_m1 | C__alua_ma | C__maq_00 | C__maq_01 | C__maq_02 | C_acq_00 | C_acq_01 | C_acq_02 | C_alua_mq0600 | C_alua_mq1107 | C_alua_pc0600 | C_alua_pc1107 | C_maq_00 | C_maq_01 | C_maq_02 | C_mq_00 | C_mq_01 | C_mq_02 | C_pcq_00 | C_pcq_01 | C_pcq_02;
    dins = D__alub_ac | D__grpa1q | D__lnq | D_alub_1 | D_inc_axb | D_lnq;

    aouts = A__aluq_11 | A__aluq_10 | A__aluq_09 | A__aluq_08 | A__aluq_07;
    bouts = B__aluq_06 | B__aluq_05 | B__aluq_04 | B__aluq_03;
    couts = C__aluq_02 | C__aluq_01 | C__aluq_00 | C__alucout;
    douts = D__newlink;

    ABCD abcd;

    for ever {

        // generate new random values unless we are in looping-on-same-values mode
        // if looping-on-same-values mode, just leave all the values as is
        if (! looping) {
            nextrands ();

            abcd.acon = arand;
            abcd.bcon = brand;
            abcd.ccon = crand;
            abcd.dcon = drand;
            abcd.decode ();

            // just one of _alu_add, _alu_and, _grpa1q asserted at a time
            // ...and inc_axb asserted only when _grpa1q is asserted
            // that give us 4 possibilities with 4 bits input
            uint32_t func = (abcd._alu_add ? 2 : 0) | (abcd._alu_and ? 1 : 0);
            abcd._alu_add = ! (func == 0);
            abcd._alu_and = ! (func == 1);
            abcd._grpa1q  = ! ((func == 2) | (func == 3));
            abcd.inc_axb  =   (func == 3);

            // just one selection for alu A operand
            //  so one of _alua_m1, _alua_ma, alua_mq0600+alua_mq1107, alua_pc0600+alua_pc1107,
            //            alua_mq0600+alua_pc1107, alua_mq1107+alua_pc0600, none (giving 0)
            uint32_t asel = ((abcd._alua_m1 ? 1 : 0) | (abcd._alua_ma ? 2 : 0) | (abcd.alua_mq0600 ? 4 : 0) |
                    (abcd.alua_mq1107 ? 8 : 0) | (abcd.alua_pc0600 ? 16 : 0) | (abcd.alua_pc1107 ? 32 : 0)) % 7;
            abcd._alua_m1 = ! (asel == 1);
            abcd._alua_ma = ! (asel == 2);
            abcd.alua_mq0600 = (asel == 3) || (asel == 5);
            abcd.alua_mq1107 = (asel == 3) || (asel == 4);
            abcd.alua_pc0600 = (asel == 6) || (asel == 4);
            abcd.alua_pc1107 = (asel == 6) || (asel == 5);

            // just one selection for alu B operand
            //  so one of alub_1, _alub_ac, _alub_m1, none (giving 0)
            uint32_t bsel = (abcd.alub_1 ? 1 : 0) | (abcd._alub_ac ? 2 : 0);
            abcd.alub_1   =   (bsel == 1);
            abcd._alub_ac = ! (bsel == 2);
            abcd._alub_m1 = ! (bsel == 3);

            // make sure complementary inputs consistent
            abcd._lnq = ! abcd.lnq;
            abcd._maq =   abcd.maq ^ 07777;

            // put that all back together
            abcd.encode ();
            arand = abcd.acon;
            brand = abcd.bcon;
            crand = abcd.ccon;
            drand = abcd.dcon;
        }

        // send out random values to its input pins
        writebits (arand, brand, crand, drand);

        // give it a whole cycle to soak through the ALU's all-combo circuits

        halfcycle ();
        halfcycle ();

        // check the outputs

        uint32_t savecycleno = cycleno;
        checkendcycle ();

        // print results

        if (verbose) {
            abcd.acon = hwacon;
            abcd.bcon = hwbcon;
            abcd.ccon = hwccon;
            abcd.dcon = hwdcon;
            abcd.decode ();

            printf ("cycle %10u:\n", savecycleno);

            uint32_t _aluashouldbe, _alubshouldbe;
            if (simulatr->examine ("_alua/alucirc", &_aluashouldbe) != 12) ABORT ();
            if (simulatr->examine ("_alub/alucirc", &_alubshouldbe) != 12) ABORT ();

            printf ("  _alua_m1=%o _alua_ma=%o alua_mq0600=%o alua_mq1107=%o alua_pc0600=%o alua_pc1107=%o\n",
                    abcd._alua_m1, abcd._alua_ma, abcd.alua_mq0600, abcd.alua_mq1107, abcd.alua_pc0600, abcd.alua_pc1107);
            printf ("  maq=%04o  _maq=%04o    mq=%04o  pcq=%04o  =>  _alua theoretically = %04o\n",
                    abcd.maq, abcd._maq, abcd.mq, abcd.pcq, _aluashouldbe);

            printf ("  alub_1=%o _alub_ac=%o _alub_m1=%o  acq=%04o  =>  _alub theoretically = %04o\n",
                    abcd.alub_1, abcd._alub_ac, abcd._alub_m1, abcd.acq, _alubshouldbe);

            printf ("  _alu_add=%o _alu_and=%o _grpa1q=%o inc_axb=%o lnq=%o\n",
                    abcd._alu_add, abcd._alu_and, abcd._grpa1q, abcd.inc_axb, abcd.lnq);

            printf ("    => _newlink=%o _alucout=%o _aluq=%04o", abcd._newlink, abcd._alucout, abcd._aluq);
            printf ("  :  %o.%04o  =  %04o  ", ! abcd._alucout, abcd._aluq ^ 07777, _aluashouldbe ^ 07777);
            if (! abcd._alu_add) putchar ('+');
            if (! abcd._alu_and) putchar ('&');
            if (! abcd._grpa1q)  putchar ('^');
            printf ("  %04o", _alubshouldbe ^ 07777);
            if (! abcd._grpa1q)  printf ("  +  %o", abcd.inc_axb);
            putchar ('\n');

            maybepause ();
        }

        // about to start a new cycle
        inccycleno ();
    }
}

// test MA board

static void mainloop_ma ()
{
    ains = A__aluq_11 | A__aluq_10 | A__aluq_09 | A__aluq_08 | A__aluq_07;
    bins = B__aluq_06 | B__aluq_05 | B__aluq_04 | B__aluq_03;
    cins = C__aluq_02 | C__aluq_01 | C__aluq_00;
    dins = D_clok2    | D__ma_aluq | D_reset;

    // MA contents unknown to start so don't check them till loaded
    aouts = bouts = couts = douts = 0;

    // reset the tubes and simulator
    writebits (0, 0, 0, D__ma_aluq | D_reset);
    halfcycle ();
    halfcycle ();
    checkendcycle ();
    inccycleno ();
    writebits (0, 0, 0, D__ma_aluq);
    halfcycle ();

    uint32_t d__ma_aluq = D__ma_aluq;

    for ever {

        // in middle of cycle

        // send out random values to its input pins
        // also negate clock, leave reset negated, leave _ma_aluq alone

        nextrands ();
        writebits (arand, brand, crand, (drand & ~ D__ma_aluq) | d__ma_aluq);

        // wait for end of cycle and compare with what simulator thinks

        halfcycle ();
        checkendcycle ();

        if (verbose) {
            ABCD abcd;
            abcd.acon = hwacon;
            abcd.bcon = hwbcon;
            abcd.ccon = hwccon;
            abcd.dcon = hwdcon;
            abcd.decode ();

            printf ("%10u: _ma_aluq=%o _aluq=%04o => maq=%04o _maq=%04o\n",
                    cycleno, abcd._ma_aluq, abcd._aluq, abcd.maq, abcd._maq);

            maybepause ();
        }

        // if next clock up will load MA, MA bits will be known so check them from then on

        if (! d__ma_aluq) {
            aouts = A__maq_11 | A_maq_11  | A__maq_10 | A_maq_10  | A__maq_09 | A_maq_09  | A__maq_08 | A_maq_08  | A__maq_07;
            bouts = B_maq_07  | B__maq_06 | B_maq_06  | B__maq_05 | B_maq_05  | B__maq_04 | B_maq_04  | B__maq_03 | B_maq_03;
            couts = C__maq_02 | C_maq_02  | C__maq_01 | C_maq_01  | C__maq_00 | C_maq_00;
        }

        // about to start a new cycle
        inccycleno ();

        // raise clock then wait half a cycle, possibly changing _ma_aluq for the new cycle

        writebits (arand, brand, crand, drand | D_clok2);
        halfcycle ();

        // remember what we sent it for _ma_aluq

        d__ma_aluq = drand & D__ma_aluq;
    }
}

// test PC board

static void mainloop_pc ()
{
    ains = A__aluq_11 | A__aluq_10 | A__aluq_09 | A__aluq_08 | A__aluq_07;
    bins = B__aluq_06 | B__aluq_05 | B__aluq_04 | B__aluq_03;
    cins = C__aluq_02 | C__aluq_01 | C__aluq_00;
    dins = D_clok2    | D__pc_aluq | D__pc_inc  | D_reset;

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

    for ever {

        // in middle of cycle

        // send out random values to its input pins, including random _pc_aluq and _pc_inc
        // also negate clock, leave reset negated

        nextrands ();
        writebits (arand, brand, crand, drand);

        // wait for end of cycle and compare with what simulator thinks

        halfcycle ();
        checkendcycle ();

        if (verbose) {
            ABCD abcd;
            abcd.acon = hwacon;
            abcd.bcon = hwbcon;
            abcd.ccon = hwccon;
            abcd.dcon = hwdcon;
            abcd.decode ();

            // print hardware value at end of cycle + what's going to happen at next clock up
            printf ("%10u: pcq=%04o + _pc_aluq=%o _pc_inc=%o _aluq=%04o =>\n",
                    cycleno, abcd.pcq, abcd._pc_aluq, abcd._pc_inc, abcd._aluq);

            maybepause ();
        }

        // about to start a new cycle
        inccycleno ();

        // raise clock then wait half a cycle, leaving all the other inputs as is
        // this clocks the new state into the tubes

        writebits (arand, brand, crand, drand | D_clok2);
        halfcycle ();
    }
}

// test RPI board - assumes running on RasPI plugged into GPIO connector as well as paddles

#define AH(field) (abcd.field ? '*' : '-')
#define AL(field) (abcd.field ? '-' : '*')
#define BC(field,bitno) (((abcd.field >> bitno) & 1) ? '*' : '-')

#define NLEDS 49
static uint32_t const ledpins[NLEDS*2] = {
        'D', D_lnq,
        'A', A_acq_11,
        'A', A_acq_10,
        'A', A_acq_09,
        'A', A_acq_08,
        'A', A_acq_07,
        'B', B_acq_06,
        'B', B_acq_05,
        'B', B_acq_04,
        'B', B_acq_03,
        'C', C_acq_02,
        'C', C_acq_01,
        'C', C_acq_00,
        'C', C_maq_00,
        'C', C_maq_01,
        'C', C_maq_02,
        'B', B_maq_03,
        'B', B_maq_04,
        'B', B_maq_05,
        'B', B_maq_06,
        'B', B_maq_07,
        'A', A_maq_08,
        'A', A_maq_09,
        'A', A_maq_10,
        'A', A_maq_11,
        'A', A_pcq_11,
        'A', A_pcq_10,
        'A', A_pcq_09,
        'A', A_pcq_08,
        'B', B_pcq_07,
        'B', B_pcq_06,
        'B', B_pcq_05,
        'B', B_pcq_04,
        'B', B_pcq_03,
        'C', C_pcq_02,
        'C', C_pcq_01,
        'C', C_pcq_00,
        'C', C_intak1q,
        'D', D_exec3q,
        'D', D_exec2q,
        'D', D_exec1q,
        'D', D_defer3q,
        'D', D_defer2q,
        'D', D_defer1q,
        'D', D_fetch2q,
        'C', C_fetch1q,
        'A', A_irq_09,
        'A', A_irq_10,
        'A', A_irq_11,
};

static bool rpiprintf (bool bad, char const *fmt, ...)
{
    if (bad | verbose) {
        putchar (bad ? 'X' : ' ');
        va_list ap;
        va_start (ap, fmt);
        vprintf (fmt, ap);
        va_end (ap);
    }
    return bad;
}

static void mainloop_rpi ()
{
    ABCD abcd;
    abcd.clok2 = true;
    abcd.intrq = true;
    abcd.ioskp = true;
    abcd.mql   = true;
    abcd.mq    = 07777;
    abcd.reset = true;
    abcd.encode ();
    ains = (~ abcd.acon) & ~1;  // pin 01 are grounded
    bins = (~ abcd.bcon) & ~1;  // ...so don't try to output to them
    cins = (~ abcd.ccon) & ~1;
    dins = (~ abcd.dcon) & ~1;

    // csrcmod_rpi is useless, so just use hardware and manually test it

    uint32_t grand = 0;

    bool ledecr = false;
    int ledoff  = 0;
    int ledon   = 0;
    int ledstep = 0;

    for ever {

        // generate random values for all pins, including gpio
        if (! looping) {
            arand = randuint32 (32);
            brand = randuint32 (32);
            crand = randuint32 (32);
            drand = randuint32 (32);
            grand = randuint32 (32) & G_ALL;

            if (++ ledstep % 32 == 0) {
                ledoff = ledon;
                if (ledecr) {
                    if (ledon == 0) ledecr = false;
                                     else -- ledon;
                } else {
                    if (ledon == NLEDS - 1) ledecr = true;
                                            else ++ ledon;
                }
            }
            uint32_t const *p = &ledpins[ledoff*2+0];
            switch (*(p ++)) {
                case 'A': arand &= ~ *p; break;
                case 'B': brand &= ~ *p; break;
                case 'C': crand &= ~ *p; break;
                case 'D': drand &= ~ *p; break;
            }
            uint32_t const *q = &ledpins[ledon*2+0];
            switch (*(q ++)) {
                case 'A': arand |=   *q; break;
                case 'B': brand |=   *q; break;
                case 'C': crand |=   *q; break;
                case 'D': drand |=   *q; break;
            }
        }

        bool qena = (grand & G_QENA) != 0;

        // write random values out
        hardware->writepads (inss, rands);
        hardware->writegpio (qena, grand);

        // let soak through transistors
        hardware->halfcycle ();

        // read all pins
        hardware->readpads (hwcons);
        hwgcon = hardware->readgpio ();

        // decode abcd pins
        abcd.acon = hwacon;
        abcd.bcon = hwbcon;
        abcd.ccon = hwccon;
        abcd.dcon = hwdcon;
        abcd.decode ();

        bool     abcdlink = qena ? abcd.mql : ! abcd._lnq;
        uint16_t abcddata = qena ? abcd.mq  : abcd._aluq ^ 07777;

        // maybe show what should be on LEDs
        if (verbose) {
            printf ("\n");
            printf ("   %c   %c %c %c   %c %c %c   %c %c %c   %c %c %c   AC\n", AH(lnq), BC(acq,11), BC(acq,10), BC(acq,9), BC(acq,8), BC(acq,7), BC(acq,6), BC(acq,5), BC(acq,4), BC(acq,3), BC(acq,2), BC(acq,1), BC(acq,0));
            printf (" LINK  %c %c %c   %c %c %c   %c %c %c   %c %c %c   MA\n", BC(maq,11), BC(maq,10), BC(maq,9), BC(maq,8), BC(maq,7), BC(maq,6), BC(maq,5), BC(maq,4), BC(maq,3), BC(maq,2), BC(maq,1), BC(maq,0));
            printf ("       %c %c %c   %c %c %c   %c %c %c   %c %c %c   PC\n", BC(pcq,11), BC(pcq,10), BC(pcq,9), BC(pcq,8), BC(pcq,7), BC(pcq,6), BC(pcq,5), BC(pcq,4), BC(pcq,3), BC(pcq,2), BC(pcq,1), BC(pcq,0));
            printf ("       %c %c %c   %c %c   %c %c %c   %c %c %c   %c\n", BC(irq,11), BC(irq,10), BC(irq,9), AH(fetch1q), AH(fetch2q), AH(defer1q), AH(defer2q), AH(defer3q), AH(exec1q), AH(exec2q), AH(exec3q), AH(intak1q));
            printf ("        IR    FETCH  DEFER   EXEC   IAK\n");
        }

        // decode gpio pins
        bool gpioclock = (hwgcon & G_CLOCK) != 0;
        bool gpioreset = (hwgcon & G_RESET) != 0;
        bool gpioios   = (hwgcon & G_IOS)   != 0;
        bool gpioirq   = (hwgcon & G_IRQ)   != 0;
        bool gpiojump  = (hwgcon & G_JUMP)  != 0;
        bool gpioioin  = (hwgcon & G_IOIN)  != 0;
        bool gpiodfrm  = (hwgcon & G_DFRM)  != 0;
        bool gpioread  = (hwgcon & G_READ)  != 0;
        bool gpiowrite = (hwgcon & G_WRITE) != 0;
        bool gpioiak   = (hwgcon & G_IAK)   != 0;

        bool     gpiolink = (hwgcon & G_LINK) != 0;
        uint16_t gpiodata = (hwgcon & G_DATA) / G_DATA0;

        // compare and print diffs
        char dir = qena ? 'q' : 'd';

        bool err = false;

        err |= rpiprintf (gpioclock != abcd.clok2,   " gpioclock %o   abcd.clok2   %o\n", gpioclock, abcd.clok2);
        err |= rpiprintf (gpiodfrm  == abcd._dfrm,   " gpiodfrm  %o   abcd._dfrm   %o\n", gpiodfrm,  abcd._dfrm);
        err |= rpiprintf (gpioiak   == abcd._intak,  " gpioiak   %o   abcd._intak  %o\n", gpioiak,   abcd._intak);
        err |= rpiprintf (gpioirq   != abcd.intrq,   " gpioirq   %o   abcd.intrq   %o\n", gpioirq,   abcd.intrq);
        err |= rpiprintf (gpioioin  != abcd.ioinst,  " gpioioin  %o   abcd.ioinst  %o\n", gpioioin,  abcd.ioinst);
        err |= rpiprintf (gpioios   != abcd.ioskp,   " gpioios   %o   abcd.ioskp   %o\n", gpioios,   abcd.ioskp);
        err |= rpiprintf (gpioread  == abcd._mread,  " gpioread  %o   abcd._mread  %o\n", gpioread,  abcd._mread);
        err |= rpiprintf (gpiowrite == abcd._mwrite, " gpiowrite %o   abcd._mwrite %o\n", gpiowrite, abcd._mwrite);
        err |= rpiprintf (gpioreset != abcd.reset,   " gpioreset %o   abcd.reset   %o\n", gpioreset, abcd.reset);
        err |= rpiprintf (gpiojump  == abcd._jump,   " gpiojump  %o   abcd._jump   %o\n", gpiojump,  abcd._jump);

        err |= rpiprintf (gpiolink != abcdlink, " gpiolink  %o   abcd.m%cl     %o\n",  gpiolink, dir, abcdlink);
        err |= rpiprintf (gpiodata != abcddata, " gpiodata %04o abcd.m%c   %04o\n", gpiodata, dir, abcddata);

        if (err) {
            gotanerror ();
        }

        maybepause ();
    }
}
