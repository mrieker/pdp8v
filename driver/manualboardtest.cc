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

// interactive manual test of paddles (does not do any verification)
// prints values and prompts for changes
// if 'rpi' module included in list, must run on raspi plugged into gpio connector as well as paddles
// otherwise can run on a PC or raspi with just the paddles plugged in

//  . ./iow56sns.si
//  ./loadmod.sh
//  sudo -E ./manualboardtest.armv7l modules-being-tested ...
//    modules-being-tested = all or any combination of acl alu ma pc rpi seq

//  ./manualboardtest.x86_64 [-csrclib | -pipelib] modules-being-tested ...  << for debugging manualboardtest itself

#include <stdio.h>
#include <string.h>

#include "abcd.h"
#include "gpiolib.h"
#include "pindefs.h"
#include "readprompt.h"

struct Module {
    char const *name;
    uint32_t ins[5], outs[5];
    bool selected;
};

static Module aclmod = { "acl", acl_ains, acl_bins, acl_cins, acl_dins,   0,    acl_aouts, acl_bouts, acl_couts, acl_douts,   0   };
static Module alumod = { "alu", alu_ains, alu_bins, alu_cins, alu_dins,   0,    alu_aouts, alu_bouts, alu_couts, alu_douts,   0   };
static Module mamod  = { "ma",  ma_ains,  ma_bins,  ma_cins,  ma_dins,    0,    ma_aouts,  ma_bouts,  ma_couts,  ma_douts,    0   };
static Module pcmod  = { "pc",  pc_ains,  pc_bins,  pc_cins,  pc_dins,    0,    pc_aouts,  pc_bouts,  pc_couts,  pc_douts,    0   };
static Module rpimod = { "rpi", rpi_ains, rpi_bins, rpi_cins, rpi_dins, G_OUTS, rpi_aouts, rpi_bouts, rpi_couts, rpi_douts, G_INS };
static Module seqmod = { "seq", seq_ains, seq_bins, seq_cins, seq_dins,   0,    seq_aouts, seq_bouts, seq_couts, seq_douts,   0   };

#define NMODULES 6
static Module *const modules[NMODULES] = { &aclmod, &alumod, &mamod, &pcmod, &rpimod, &seqmod };

static ABCD abcd;
static uint32_t allins[5];  // all inputs to selected module set, minus all outputs from selected module set

static bool fillpins (char const *namestr, int namelen, uint32_t newval, uint32_t *writemask);

int main (int argc, char **argv)
{
    bool csrclib = false;
    bool pipelib = false;

    for (int i = 0; ++ i < argc;) {
        if (strcasecmp (argv[i], "-csrclib") == 0) {
            csrclib = true;
            pipelib = false;
            continue;
        }
        if (strcasecmp (argv[i], "-pipelib") == 0) {
            csrclib = false;
            pipelib = true;
            continue;
        }
        if (argv[i][0] == '-') {
            fprintf (stderr, "unknown option %s\n", argv[i]);
            return 1;
        }
        if (strcasecmp (argv[i], "all") == 0) {
            for (int j = 0; j < NMODULES; j ++) {
                modules[j]->selected = true;
            }
            continue;
        }
        for (int j = 0; j < NMODULES; j ++) {
            if (strcasecmp (argv[i], modules[j]->name) == 0) {
                modules[j]->selected = true;
                goto goodmod;
            }
        }
        fprintf (stderr, "bad module %s\n", argv[i]);
        return 1;
    goodmod:;
    }

    uint32_t allouts[] = { 0, 0, 0, 0, 0 };
    std::string modnames;
    for (int j = 0; j < NMODULES; j ++) {
        Module *module = modules[j];
        if (module->selected) {
            modnames.append (module->name);
            for (int i = 0; i < 5; i ++) {
                allins[i]  |= module->ins[i];
                allouts[i] |= module->outs[i];
            }
        }
    }
    if (modnames.length () == 0) {
        fprintf (stderr, "missing module name(s)\n");
        return 1;
    }
    for (int i = 0; i < 5; i ++) {
        allins[i] &= ~ allouts[i];
    }

    // open connection to paddles and if rpi present, gpio as well
    GpioLib *gpiolib = csrclib ? (GpioLib *) new CSrcLib (modnames.c_str ()) :
                      (pipelib ? (GpioLib *) new PipeLib (modnames.c_str ()) :
                                                 (GpioLib *) new PhysLib ());
    gpiolib->open ();

    while (true) {

        // read paddles then print out values
        gpiolib->readpads (abcd.cons);
        if (rpimod.selected) abcd.gcon = gpiolib->readgpio ();
        abcd.decode ();
        printf ("\n");
        abcd.printfull<FILE> (fprintf, stdout, aclmod.selected, alumod.selected, mamod.selected, pcmod.selected, rpimod.selected, seqmod.selected);

        // read possible modification
    read:;
        printf ("\n");
        char const *line = readprompt (" > ");
        if (line == NULL) break;
        if (line[0] == 0) continue;

        uint32_t writemask = 0;
        while (true) {

            // skip leading spaces, done processing if end of line
            while ((*line > 0) && (*line <= ' ')) line ++;
            if (*line == 0) break;

            // find '=' separating name and value
            char const *q = strchr (line, '=');
            if (q == NULL) {
                if (writemask != 0) printf ("\n");
                fprintf (stderr, "missing = in %s\n", line);
                goto read;
            }

            // get name length and trim trailing spaces
            int namelen = q - line;
            while ((namelen > 1) && (line[namelen-1] <= ' ')) -- namelen;

            // singleletter=hexvalue writes whole connector
            if (namelen == 1) {
                char *p;
                uint32_t newval = strtoul (line + 2, &p, 16);
                if (*p > ' ') {
                    if (writemask != 0) printf ("\n");
                    fprintf (stderr, "invalid hex value %s\n", line + 2);
                    goto read;
                }
                int i = (line[0] | 0x20) - 'a';
                if ((i < 0) || (i >= NPADS)) {
                    if (writemask != 0) printf ("\n");
                    fprintf (stderr, "bad paddle letter %c\n", line[0]);
                    goto read;
                }
                printf (" %c=%08X", 'a' + i, newval);
                abcd.cons[i] = newval;
                writemask |= 1 << i;
                line = p;
            }

            // signalname=octalvalue writes signal to the paddles
            else {
                do q ++; while ((*q > 0) && (*q <= ' '));

                // alua = 0,mq 0 m1 ma mq pc pc,mq
                if ((namelen == 4) && (strncasecmp (line, "alua", 4) == 0)) {
                    uint32_t alua_m1 = 0;
                    uint32_t alua_ma = 0;
                    uint32_t alua_mq0600 = 0;
                    uint32_t alua_mq1107 = 0;
                    uint32_t alua_pc0600 = 0;
                    uint32_t alua_pc1107 = 0;

                    if (strncasecmp (q, "pc,mq", 5) == 0) {
                        alua_pc1107 = 1;
                        alua_mq0600 = 1;
                        line = q + 5;
                    }
                    else if (strncasecmp (q, "0,mq", 4) == 0) {
                        alua_mq0600 = 1;
                        line = q + 4;
                    }
                    else if (strncasecmp (q, "m1", 2) == 0) {
                        alua_m1 = 1;
                        line = q + 2;
                    }
                    else if (strncasecmp (q, "ma", 2) == 0) {
                        alua_ma = 1;
                        line = q + 2;
                    }
                    else if (strncasecmp (q, "mq", 2) == 0) {
                        alua_mq1107 = 1;
                        alua_mq0600 = 1;
                        line = q + 2;
                    }
                    else if (strncasecmp (q, "pc", 2) == 0) {
                        alua_pc1107 = 1;
                        alua_pc0600 = 1;
                        line = q + 2;
                    }
                    else if (strncasecmp (q, "0", 1) == 0) {
                        line = q + 1;
                    }
                    if ((line < q) || (*line > ' ')) {
                        if (writemask != 0) printf ("\n");
                        fprintf (stderr, "invalid alua source %s\n", q);
                        goto read;
                    }

                    if (! fillpins ("alua_m1", 7, alua_m1, &writemask)) goto read;
                    if (! fillpins ("alua_ma", 7, alua_ma, &writemask)) goto read;
                    if (! fillpins ("alua_mq0600", 11, alua_mq0600, &writemask)) goto read;
                    if (! fillpins ("alua_mq1107", 11, alua_mq1107, &writemask)) goto read;
                    if (! fillpins ("alua_pc0600", 11, alua_pc0600, &writemask)) goto read;
                    if (! fillpins ("alua_pc1107", 11, alua_pc1107, &writemask)) goto read;
                }

                // alub = ac m1 1 0
                else if ((namelen == 4) && (strncasecmp (line, "alub", 4) == 0)) {
                    uint32_t alub_1  = 0;
                    uint32_t alub_ac = 0;
                    uint32_t alub_m1 = 0;

                    if (strncasecmp (q, "0", 1) == 0) {
                        line = q + 1;
                    }
                    else if (strncasecmp (q, "1", 1) == 0) {
                        alub_1 = 1;
                        line = q + 1;
                    }
                    else if (strncasecmp (q, "ac", 2) == 0) {
                        alub_ac = 1;
                        line = q + 2;
                    }
                    else if (strncasecmp (q, "m1", 2) == 0) {
                        alub_m1 = 1;
                        line = q + 2;
                    }
                    if ((line < q) || (*line > ' ')) {
                        if (writemask != 0) printf ("\n");
                        fprintf (stderr, "invalid alub source %s\n", q);
                        goto read;
                    }

                    if (! fillpins ("alub_1",  6, alub_1,  &writemask)) goto read;
                    if (! fillpins ("alub_ac", 7, alub_ac, &writemask)) goto read;
                    if (! fillpins ("alub_m1", 7, alub_m1, &writemask)) goto read;
                }

                // pinname = octalvalue
                else {
                    char *p;
                    uint32_t newval = strtoul (q, &p, 8);
                    if (*p > ' ') {
                        if (writemask != 0) printf ("\n");
                        fprintf (stderr, "invalid octal value %s\n", q);
                        goto read;
                    }

                    // if name starts with _, complement value,
                    // eg, convert _maq=1234 to maq=6543
                    if (line[0] == '_') {
                        newval = ~ newval;
                        line ++;
                    }

                    // change clock to clok2
                    if ((namelen == 5) && (memcmp (line, "clock", 5) == 0)) {
                        line = "clok2";
                    }

                    // fill in pin(s) with the given value
                    if (! fillpins (line, namelen, newval, &writemask)) goto read;

                    line = p;
                }
            }
        }

        // end of line, write out any updates then give it time to soak in before reading it back
        if (writemask != 0) {
            printf ("\nwriting");
            if (writemask & 15) { printf (" a=%08X b=%08X c=%08X d=%08X", abcd.acon, abcd.bcon, abcd.ccon, abcd.dcon); gpiolib->writepads (allins, abcd.cons); }
            if (writemask & 16) { printf (" g=%08X", abcd.gcon); gpiolib->writegpio ((abcd.gcon & G_QENA) != 0, abcd.gcon); }
        }
        printf ("\nschtepping\n");
        gpiolib->halfcycle ();
    }

    printf ("\n");
    return 0;
}

// fill in all matching pins - all bits of a bit array and both active high and active low
//  input:
//   namestr = active high version of pin name to write
//   namelen = length of namestr
//   newval  = active high value to set the named pin(s) to
//  output:
//   returns false: bad pin name given
//            true: pin name ok
//   abcd.cons modified
static bool fillpins (char const *namestr, int namelen, uint32_t newval, uint32_t *writemask)
{
    // if QENA is set, allow LINK and DATA to be written so they will be sent to the RPI board via GPIO connector
    if (abcd.gcon & G_QENA) {
        allins[4] |=    G_LINK | G_DATA;
    } else {
        allins[4] &= ~ (G_LINK | G_DATA);
    }

    // scan each connector for the named pin(s)
    bool found = false;
    for (int i = 0; i < 5; i ++) {
        for (PinDefs const *pin = pindefss[i]; pin->pinmask != 0; pin ++) {
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

                // fill the bit into the connector
                if (! (allins[i] & pin->pinmask)) {
                    if (*writemask != 0) printf ("\n");
                    fprintf (stderr, "pin %s not an input\n", pin->varname);
                    return false;
                }
                printf (" %s=%o", pin->varname, val & 1);
                if (val & 1) abcd.cons[i] |= pin->pinmask;
                      else abcd.cons[i] &= ~ pin->pinmask;
                found = true;
                *writemask |= 1 << i;
            }
        }
    }
    if (found) return true;

    if (*writemask != 0) printf ("\n");
    fprintf (stderr, "pin [_]%.*s not found\n", namelen, namestr);
    return false;
}
