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

// blindly loop through outputs headed to paddles connected to arbitrary set of boards

// export addhz=<addhz>
// export cpuhz=<cpuhz>
// ./loopedboardtest.`uname -m` [-nopads] <boardnames> <presetpairs> -loop <loopingpairs>
//   boardnames = space separated list of acl, alu, ma, pc, rpi, seq
//                if rpi, must be plugged into raspi gpio connector
//   presetpairs = name/value pairs to set once at startup
//   -loop = marks looping point (if omitted, all pairs are considered loopingpairs)
//   loopingpairs = name/value pairs that are looped

// example connected to seq board, set mq=1065 once,
// then loop toggling reset on then off, then clock on then off
//  . ./iow56sns.si
//  ./loadmod.sh
//  sudo chmod 666 /dev/usb/iowarrior*
//  ./loopedboardtest.armv7l seq mq 1065 -loop reset 1 reset 0 clok2 1 clok2 0

#include <stdio.h>
#include <string.h>

#include "abcd.h"
#include "gpiolib.h"
#include "pindefs.h"
#include "readprompt.h"

struct Step {
    Step *next;

    virtual void dostep (PhysLib *gpiolib) = 0;
};

// -hc halfcycle sleep delay
struct HCStep : Step {
    virtual void dostep (PhysLib *gpiolib);
};

// -print signal
struct PrintStep : Step {
    char const *signal;     // signal name given on command line
    int pad;                // which paddle or gpio
    uint32_t cycleno;       // current cycle number
    uint32_t mask;          // make of bit on paddle or gpio
    uint32_t valu;          // value of that bit on paddle or gpio

    PrintStep (int argc, char **argv, int *ip);

    virtual void dostep (PhysLib *gpiolib);
};

// -printne signal value
struct PrintNeStep : Step {
    char const *signal;     // signal name given on command line
    int pad;                // which paddle or gpio
    uint32_t cycleno;       // current cycle number
    uint32_t mask;          // make of bit on paddle or gpio
    uint32_t valu;          // value of that bit on paddle or gpio
    uint32_t value;         // value given on command line

    PrintNeStep (int argc, char **argv, int *ip);

    virtual void dostep (PhysLib *gpiolib);
};

// set signal value
struct SetStep : Step {
    int pad;
    uint32_t mask;
    uint32_t pins;

    virtual void dostep (PhysLib *gpiolib);
};

struct Module {
    char const name[4];
    uint32_t ins[5], outs[5];
};

#define NMODULES 6
static Module const modules[NMODULES] = {
    { "acl", acl_ains, acl_bins, acl_cins, acl_dins,   0,    acl_aouts, acl_bouts, acl_couts, acl_douts,   0   },
    { "alu", alu_ains, alu_bins, alu_cins, alu_dins,   0,    alu_aouts, alu_bouts, alu_couts, alu_douts,   0   },
    { "ma",  ma_ains,  ma_bins,  ma_cins,  ma_dins,    0,    ma_aouts,  ma_bouts,  ma_couts,  ma_douts,    0   },
    { "pc",  pc_ains,  pc_bins,  pc_cins,  pc_dins,    0,    pc_aouts,  pc_bouts,  pc_couts,  pc_douts,    0   },
    { "rpi", rpi_ains, rpi_bins, rpi_cins, rpi_dins, G_OUTS, rpi_aouts, rpi_bouts, rpi_couts, rpi_douts, G_INS },
    { "seq", seq_ains, seq_bins, seq_cins, seq_dins,   0,    seq_aouts, seq_bouts, seq_couts, seq_douts,   0   }
};

static uint32_t allins[NPADS+1];    // all inputs to board(s)
static uint32_t allouts[NPADS+1];   // all outputs from board(s)
static uint32_t outpins[NPADS+1];

static bool nopads;
static Step **fillpins (Step **laststep, char const *namestr, uint32_t newval);

int main (int argc, char **argv)
{
    // get board name(s) at beginning
    bool gotamod = false;
    while (argc > 1) {
        if (strcasecmp (argv[1], "-nopads") == 0) {
            nopads = true;
            argc --;
            argv ++;
            continue;
        }
        for (int i = 0; i < NMODULES; i ++) {
            Module const *mod = &modules[i];
            if (strcasecmp (argv[1], mod->name) == 0) {
                for (int j = 0; j <= NPADS; j ++) {
                    allins[j]  |= mod->ins[j];
                    allouts[j] |= mod->outs[j];
                }
                goto gotmod;
            }
        }
        break;
    gotmod:;
        gotamod = true;
        argv ++;
        -- argc;
    }
    if (! gotamod) {
        fprintf (stderr, "no module(s) specified\n");
        return 1;
    }

    // we can't write to any output pins
    // so remove output pins of one board from input pins of another board
    for (int j = 0; j <= NPADS; j ++) {
        allins[j] &= ~ allouts[j];
    }

    // access the paddles and maybe gpio connector
    PhysLib *gpiolib = new PhysLib ();
    gpiolib->open ();

    // write all input pins with zeroes
    if (! nopads) gpiolib->writepads (allins, outpins);
    if (allins[NPADS] != 0) gpiolib->writegpio (false, 0);

    // build list of steps from remaining command line args
    Step *firststep;
    Step **laststep = &firststep;

    // parse arg pairs: pinname pinvalue
    // assume it all loops unless -loop given to mark looping point
    // also accept "-hc" for halfcycle delay
    Step **loopstep = &firststep;
    for (int i = 1; i < argc; i ++) {
        if (strcasecmp (argv[i], "-hc") == 0) {
            HCStep *hcstep = new HCStep ();
            *laststep = hcstep;
            laststep = &hcstep->next;
            continue;
        }
        if (strcasecmp (argv[i], "-loop") == 0) {
            loopstep = laststep;
            continue;
        }
        if (strcasecmp (argv[i], "-print") == 0) {
            Step *printstep = new PrintStep (argc, argv, &i);
            *laststep = printstep;
            laststep = &printstep->next;
            continue;
        }
        if (strcasecmp (argv[i], "-printne") == 0) {
            Step *printnestep = new PrintNeStep (argc, argv, &i);
            *laststep = printnestep;
            laststep = &printnestep->next;
            continue;
        }
        if (argv[i][0] == '-') {
            fprintf (stderr, "unknown option %s\n", argv[i]);
            return 1;
        }

        // parse name value pair
        char const *name = argv[i];
        if ((++ i >= argc) || (argv[i][0] == '-')) {
            fprintf (stderr, "missing value for %s\n", name);
            return 1;
        }
        char *p;
        uint32_t valu = strtoul (argv[i], &p, 8);
        if ((p == argv[i]) || (*p != 0)) {
            fprintf (stderr, "invalid value %s\n", argv[i]);
            return 1;
        }

        // fill in step(s) to set the pin(s)
        laststep = fillpins (laststep, name, valu);
    }

    // null terminate list in case loopstep same as laststep
    // then point laststep to be followed by loopstep
    *laststep = NULL;
    *laststep = *loopstep;

    // do the steps, looping at loopstep
    // this is infinite loop unless -loop was last thing on command line
    for (Step *s = firststep; s != NULL; s = s->next) {
        s->dostep (gpiolib);
    }

    return 0;
}

// parse <-print signal> from command line
PrintStep::PrintStep (int argc, char **argv, int *ip)
{
    this->cycleno = 0;

    int i = *ip;

    if (++ i >= argc) {
        fprintf (stderr, "missing signal after -print\n");
        exit (1);
    }
    this->signal = argv[i];

    *ip = i;

    // scan each paddle and gpio for the named pin
    for (int pad = 0; pad <= NPADS; pad ++) {
        for (PinDefs const *pin = pindefss[pad]; pin->pinmask != 0; pin ++) {

            // see if string matches
            if (strcmp (this->signal, pin->varname) == 0) {

                // save mask and value
                this->pad  = pad;
                this->mask = pin->pinmask;
                return;
            }
        }
    }

    fprintf (stderr, "unknown %s\n", this->signal);
    exit (1);
}

// parse -printne signal value from command line
PrintNeStep::PrintNeStep (int argc, char **argv, int *ip)
{
    this->cycleno = 0;

    int i = *ip;

    if (++ i >= argc) {
        fprintf (stderr, "missing signal after -printne\n");
        exit (1);
    }
    this->signal = argv[i];

    if (++ i >= argc) {
        fprintf (stderr, "missing value after -printne\n");
        exit (1);
    }
    char *p;
    this->value = strtoul (argv[i], &p, 8);
    if ((p == argv[i]) || (*p != 0) || (this->value > 1)) {
        fprintf (stderr, "invalid value %s\n", argv[i]);
        exit (1);
    }

    *ip = i;

    // scan each paddle and gpio for the named pin
    for (int pad = 0; pad <= NPADS; pad ++) {
        for (PinDefs const *pin = pindefss[pad]; pin->pinmask != 0; pin ++) {

            // see if string matches
            if (strcmp (this->signal, pin->varname) == 0) {

                // save mask and value
                this->pad  = pad;
                this->mask = pin->pinmask;
                this->valu = (this->value & 1) ? pin->pinmask : 0;
                return;
            }
        }
    }

    fprintf (stderr, "unknown %s\n", this->signal);
    exit (1);
}

// do halfcycle delay
void HCStep::dostep (PhysLib *gpiolib)
{
    gpiolib->halfcycle (true);
}

void PrintStep::dostep (PhysLib *gpiolib)
{
    ++ this->cycleno;
    uint32_t readpins = (pad == NPADS) ? gpiolib->readgpio () : gpiolib->read32 (pad);
    printf ("PrintStep: %10u  %s = %u\n", this->cycleno, this->signal, (readpins & this->mask) ? 1 : 0);
}

void PrintNeStep::dostep (PhysLib *gpiolib)
{
    ++ this->cycleno;
    uint32_t readpins = (pad == NPADS) ? gpiolib->readgpio () : gpiolib->read32 (pad);
    if ((readpins & this->mask) != this->valu) {
        printf ("PrintNeStep: %10u  %s = %u\n", this->cycleno, this->signal, (readpins & this->mask) ? 1 : 0);
    }
}

// write value to single paddle or gpio connector
void SetStep::dostep (PhysLib *gpiolib)
{
    if (pad == NPADS) gpiolib->writegpio ((pins & G_QENA) != 0, pins);
                              else gpiolib->write32 (pad, mask, pins);
}

// fill in all matching pins - all bits of a bit array and both active high and active low
//  input:
//   namestr = pin name to write
//   newval  = value to set the named pin(s) to
//  output:
//   returns pointer to last step filled in
static Step **fillpins (Step **laststep, char const *namestr, uint32_t newval)
{
    if (*namestr == '_') {
        newval = ~ newval;
        namestr ++;
    }
    int namelen = strlen (namestr);

    // if QENA is set, allow LINK and DATA to be written so they will be sent to the RPI board via GPIO connector
    if (outpins[NPADS] & G_QENA) {
        allins[NPADS] |=    G_LINK | G_DATA;
    } else {
        allins[NPADS] &= ~ (G_LINK | G_DATA);
    }

    // scan each paddle or gpio for the named pin(s)
    int nsteps = 0;
    for (int pad = 0; pad <= NPADS; pad ++) {
        SetStep *step = NULL;
        for (PinDefs const *pin = pindefss[pad]; pin->pinmask != 0; pin ++) {
            uint32_t pmask = pin->pinmask;
            if (pmask != 0) {
                char const *pinname = pin->varname;

                // if pin name begins with '_', convert to active high equivalent
                uint32_t val = newval;
                if (pinname[0] == '_') {
                    val = ~ val;
                    pinname ++;
                }

                // see if first part of string matches (ignoring leading _)
                if (memcmp (namestr, pinname, namelen) == 0) {

                    // can be an exact match or be followed by [bitno]
                    if (pinname[namelen] != 0) {
                        if (pinname[namelen] != '[') continue;
                        int bitno = atoi (pinname + namelen + 1);
                        val >>= bitno;
                    }

                    // fill the bit into the paddle/gpio
                    if (! (allins[pad] & pin->pinmask)) {
                        fprintf (stderr, "pin %s not an input\n", pin->varname);
                        exit (1);
                    }
                    if (step == NULL) {
                        step = new SetStep ();
                        step->mask = 0;
                        step->pins = 0;
                    }
                    step->mask |= pmask;
                    if (val & 1) step->pins |= pmask;
                }
            }
        }
        if (step != NULL) {
            if (nopads && (pad < NPADS)) {
                fprintf (stderr, "pin %s requires paddles\n", namestr);
                ABORT ();
            }
            step->pad = pad;                            // remember paddle number (or gpio if NPADS)
            outpins[pad] &= ~  step->mask;              // update current output pins
            outpins[pad] |=    step->pins;
            step->pins    =  outpins[pad];              // output complete value for the paddle/gpio
            step->mask    =   allins[pad];              // ...including previous values
            *laststep = step;
            laststep = &step->next;
            nsteps ++;
        }
    }
    if (nsteps == 0) {
        fprintf (stderr, "unknown %s\n", namestr);
        exit (1);
    }
    return laststep;
}
