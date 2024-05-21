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

// access pins of arbitrary set of boards via TCL

// ./tclboardtest [-csrclib] [-nopads] [-zynqlib] <boardnames> [<scriptname>]
//   boardnames = space separated list of acl, alu, ma, pc, rpi, seq
//                if rpi, must be plugged into raspi gpio connector
//                if all, all boards selected
//   scriptname = optional script filename

#include <stdio.h>
#include <string.h>
#include <tcl.h>

#include "abcd.h"
#include "gpiolib.h"
#include "pindefs.h"
#include "readprompt.h"

struct Module {
    char const name[4];
    uint32_t const ins[5];
    uint32_t const outs[5];
    bool selected;
};

#define NMODULES 6
static Module modules[NMODULES] = {
    { "acl", acl_ains, acl_bins, acl_cins, acl_dins,   0,    acl_aouts, acl_bouts, acl_couts, acl_douts,   0   },
    { "alu", alu_ains, alu_bins, alu_cins, alu_dins,   0,    alu_aouts, alu_bouts, alu_couts, alu_douts,   0   },
    { "ma",  ma_ains,  ma_bins,  ma_cins,  ma_dins,    0,    ma_aouts,  ma_bouts,  ma_couts,  ma_douts,    0   },
    { "pc",  pc_ains,  pc_bins,  pc_cins,  pc_dins,    0,    pc_aouts,  pc_bouts,  pc_couts,  pc_douts,    0   },
    { "rpi", rpi_ains, rpi_bins, rpi_cins, rpi_dins, G_OUTS, rpi_aouts, rpi_bouts, rpi_couts, rpi_douts, G_INS },
    { "seq", seq_ains, seq_bins, seq_cins, seq_dins,   0,    seq_aouts, seq_bouts, seq_couts, seq_douts,   0   }
};

static uint32_t allins[NPADS+1];    // all inputs to board(s)
static uint32_t allouts[NPADS+1];   // all outputs from board(s)
static uint32_t outpins[NPADS+1];   // latest pin values we have sent to boards

static bool nopads;
static GpioLib *gpiolib;
static Tcl_Interp *interp;

static void Tcl_SetResultF (Tcl_Interp *interp, char const *fmt, ...);

static Tcl_ObjCmdProc cmd_exam;
static Tcl_ObjCmdProc cmd_force;
static Tcl_ObjCmdProc cmd_halfcycle;
static Tcl_ObjCmdProc cmd_help;
static Tcl_ObjCmdProc cmd_listpins;
static Tcl_ObjCmdProc cmd_listmods;

struct FunDef {
    Tcl_ObjCmdProc *func;
    char const *name;
    char const *help;
};

static FunDef fundefs[] = {
    { cmd_exam,       "exam",       "exam <pinname> ..." },
    { cmd_force,      "force",      "force <pinname> <pinvalu> ..." },
    { cmd_halfcycle,  "halfcycle",  "delay a half cycle" },
    { cmd_help,       "help",       "print this help" },
    { cmd_listmods,   "listmods",   "return list of selected modules" },
    { cmd_listpins,   "listpins",   "return list all pin names of selected modules" },
    { NULL, NULL, NULL }
};

int main (int argc, char **argv)
{
    bool csrclib = false;
    bool gotamod = false;
    bool zynqlib = false;
    char const *scriptfile = NULL;

    for (int i = 0; ++ i < argc;) {
        if (strcasecmp (argv[i], "-csrclib") == 0) {
            csrclib = true;
            zynqlib = false;
            continue;
        }
        if (strcasecmp (argv[i], "-nopads") == 0) {
            nopads = true;
            continue;
        }
        if (strcasecmp (argv[i], "-zynqlib") == 0) {
            csrclib = false;
            zynqlib = true;
            continue;
        }
        if (strcasecmp (argv[i], "all") == 0) {
            for (int j = 0; j < NMODULES; j ++) {
                Module *mod = &modules[j];
                for (int k = 0; k <= NPADS; k ++) {
                    allins[k]  |= mod->ins[k];
                    allouts[k] |= mod->outs[k];
                }
                mod->selected = true;
            }
            gotamod = true;
            goto gotmod;
        }
        for (int j = 0; j < NMODULES; j ++) {
            Module *mod = &modules[j];
            if (strcasecmp (argv[i], mod->name) == 0) {
                for (int k = 0; k <= NPADS; k ++) {
                    allins[k]  |= mod->ins[k];
                    allouts[k] |= mod->outs[k];
                }
                mod->selected = true;
                gotamod = true;
                goto gotmod;
            }
        }
        if (scriptfile != NULL) {
            fprintf (stderr, "cannot have more than one scriptfile\n");
            return 1;
        }
        scriptfile = argv[i];
    gotmod:;
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

    // make sorted list of module names as required by csrclib
    std::string modnames;
    for (int j = 0; j < NMODULES; j ++) {
        Module *mod = &modules[j];
        if (mod->selected) modnames.append (mod->name);
    }

    // access the paddles and maybe gpio connector
    gpiolib =
        csrclib ? (GpioLib *) new CSrcLib (modnames.c_str ()) :
        zynqlib ? (GpioLib *) new ZynqLib (modnames.c_str ()) :
                  (GpioLib *) new PhysLib ();
    gpiolib->open ();

    // write all input pins with zeroes
    if (! nopads) gpiolib->writepads (allins, outpins);
    if (allins[NPADS] != 0) gpiolib->writegpio (false, 0);

    // initialize tcl
    Tcl_FindExecutable (argv[0]);
    interp = Tcl_CreateInterp ();
    if (Tcl_Init (interp) != TCL_OK) ABORT ();
    for (int i = 0; fundefs[i].name != NULL; i ++) {
        if (Tcl_CreateObjCommand (interp, fundefs[i].name, fundefs[i].func, NULL, NULL) == NULL) ABORT ();
    }

    // if given a filename, process that file as a whole
    if (scriptfile != NULL) {
        int rc = Tcl_EvalFile (interp, scriptfile);
        char const *res = Tcl_GetStringResult (interp);
        if (rc != TCL_OK) {
            if ((res == NULL) || (res[0] == 0)) fprintf (stderr, "error %d evaluating script %s\n", rc, scriptfile);
                                  else fprintf (stderr, "error %d evaluating script %s: %s\n", rc, scriptfile, res);
            Tcl_EvalEx (interp, "puts $::errorInfo", -1, TCL_EVAL_GLOBAL);
            return 1;
        }
        if ((res != NULL) && (res[0] != 0)) puts (res);
    }

    // either way, prompt and process commands from stdin
    // to have a script file with no stdin processing, end script file with 'exit'
    puts ("\nTCL scripting, do 'help' for tclboardtest-specific commands");
    for (char const *line;;) {
        line = readprompt ("tclboardtest> ");
        if (line == NULL) break;
        int rc = Tcl_EvalEx (interp, line, -1, TCL_EVAL_GLOBAL);
        char const *res = Tcl_GetStringResult (interp);
        if (rc != TCL_OK) {
            if ((res == NULL) || (res[0] == 0)) fprintf (stderr, "error %d evaluating command\n", rc);
                                  else fprintf (stderr, "error %d evaluating command: %s\n", rc, res);
            Tcl_EvalEx (interp, "puts $::errorInfo", -1, TCL_EVAL_GLOBAL);
        }
        else if ((res != NULL) && (res[0] != 0)) puts (res);
    }

    Tcl_Finalize ();
    return 0;
}

// exam <modulename>
// exam <name1> ...
struct Element { char name[14]; uint16_t value; };

static int cmpelements (void const *va, void const *vb)
{
    Element const *ea = (Element const *) va;
    Element const *eb = (Element const *) vb;
    char const *na = ea->name;
    char const *nb = eb->name;
    int sa = *na == '_';
    int sb = *nb == '_';
    int cmp = strcmp (na + sa, nb + sb);
    if (cmp == 0) cmp = sa - sb;
    return cmp;
}

static int cmd_exam (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Tcl_Obj *results[objc];
    uint32_t padlvalus[NPADS+1];

    // read the pins coming from the tubes + values we are outputting to the tubes
    if (! nopads) {
        gpiolib->readpads (padlvalus);
        for (int j = 0; j < NPADS; j ++) {
            padlvalus[j] = (padlvalus[j] & allouts[j]) | (outpins[j] & allins[j]);
        }
    }
    padlvalus[NPADS] = outpins[NPADS] & allins[NPADS];
    if (allouts[NPADS] != 0) padlvalus[NPADS] |= gpiolib->readgpio () & allouts[NPADS];

    // check for 'exam <modulename>'
    if (objc == 2) {
        char const *namestr = Tcl_GetString (objv[1]);
        if (strcasecmp (namestr, "help") == 0) {
            puts ("");
            puts ("  exam <modulename>  - get all pins for the given module");
            puts ("  exam <pinname> ... - get list values of the given pins, alternating name value ...");
            puts ("                       may be bus name such as pcq for all 12 bits as single value");
            puts ("");
            return TCL_OK;
        }

        for (int i = 0; i < NMODULES; i ++) {
            Module const *mod = &modules[i];
            if (strcasecmp (mod->name, namestr) == 0) {

                // module name found, set up struct that can hold all possible pins
                Element elements[32*(NPADS+1)];
                memset (elements, 0, sizeof elements);
                int nelements = 0;

                // loop through all possible pins
                for (int pad = 0; pad <= NPADS; pad ++) {
                    for (PinDefs const *pin = pindefss[pad]; pin->pinmask != 0; pin ++) {

                        // see if this pin is being output by the module or is an input to the module
                        if (pin->pinmask & (mod->outs[pad] | mod->ins[pad])) {

                            // get pin name and bitnumber
                            int namelen = strlen (pin->varname);
                            int bitno = 0;
                            char const *p = strchr (pin->varname, '[');
                            if (p != NULL) {
                                namelen = p - pin->varname;
                                bitno = atoi (++ p);
                            }

                            // make slot in struct for the value
                            for (i = 0; i < nelements; i ++) {
                                if ((memcmp (pin->varname, elements[i].name, namelen) == 0) && (elements[i].name[namelen] == 0)) break;
                            }
                            if (i == nelements) {
                                ASSERT (namelen < (int) sizeof elements[i].name);
                                memcpy (elements[i].name, pin->varname, namelen);
                                nelements ++;
                            }

                            // fill in pin's value
                            if (padlvalus[pad] & pin->pinmask) elements[i].value |= 1U << bitno;
                        }
                    }
                }

                // sort pin names
                qsort (elements, nelements, sizeof elements[0], cmpelements);

                // build a proper TCL list alternating names and values
                Tcl_Obj *tclist[nelements*2];
                for (int i = 0; i < nelements; i ++) {
                    tclist[i*2+0] = Tcl_NewStringObj (elements[i].name, strlen (elements[i].name));
                    tclist[i*2+1] = Tcl_NewIntObj (elements[i].value);
                }

                // return the list
                Tcl_SetObjResult (interp, Tcl_NewListObj (nelements * 2, tclist));
                return TCL_OK;
            }
        }
    }

    // specifically listed pins (can be a bus name such as pcq meaning all 12 bits of program counter)
    for (int i = 0; ++ i < objc;) {
        char const *namestr = Tcl_GetString (objv[i]);
        int namelen = strlen (namestr);

        // scan each paddle and gpio for the named pin
        bool foundit = false;
        uint16_t value = 0;
        for (int pad = 0; pad <= NPADS; pad ++) {
            for (PinDefs const *pin = pindefss[pad]; pin->pinmask != 0; pin ++) {

                // see if first part of string matches
                if (memcmp (namestr, pin->varname, namelen) == 0) {

                    // can be an exact match or be followed by [bitno]
                    int bitno = 0;
                    if (pin->varname[namelen] != 0) {
                        if (pin->varname[namelen] != '[') continue;
                        bitno = atoi (pin->varname + namelen + 1);
                    }

                    // save pin value
                    if (padlvalus[pad] & pin->pinmask) value |= 1U << bitno;
                    foundit = true;
                }
            }
        }
        if (! foundit) {
            Tcl_SetResultF (interp, "bad pin name %s", namestr);
            return TCL_ERROR;
        }

        // add value to output list
        results[i] = Tcl_NewIntObj (value);
    }

    Tcl_SetObjResult (interp, Tcl_NewListObj (objc - 1, results + 1));
    return TCL_OK;
}

// force <name1> <value1> ...
static int cmd_force (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    bool writegpio = false;
    bool writepads = false;

    for (int i = 0; ++ i < objc;) {

        // parse name value pair
        char const *namestr = Tcl_GetString (objv[i]);
        if (++ i >= objc) {
            Tcl_SetResultF (interp, "missing value for %s\n", namestr);
            return TCL_ERROR;
        }

        int newval;
        int rc = Tcl_GetIntFromObj (interp, objv[i], &newval);
        if (rc != TCL_OK) return rc;

        // make an active-high value if given active-low value
        if (*namestr == '_') {
            newval = ~ newval;
            namestr ++;
        }
        int namelen = strlen (namestr);

        // scan each paddle or gpio for the named pin(s)
        bool foundit = false;
        for (int pad = 0; pad <= NPADS; pad ++) {
            for (PinDefs const *pin = pindefss[pad]; pin->pinmask != 0; pin ++) {
                uint32_t pmask = pin->pinmask;
                if (pmask != 0) {
                    char const *pinname = pin->varname;

                    // if pin name begins with '_', get active-low value
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
                        if (! (allins[pad] & pmask)) {
                            Tcl_SetResultF (interp, "pin %s not an input to a board\n", pin->varname);
                            return false;
                        }
                        if ((pad < NPADS) && nopads) {
                            Tcl_SetResultF (interp, "pin %s requires paddles\n", pin->varname);
                            return false;
                        }

                        if (pad < NPADS) writepads = true;
                                    else writegpio = true;
                        if (val & 1) outpins[pad] |= pmask;
                              else outpins[pad] &= ~ pmask;

                        // if QENA is set, allow LINK and DATA to be written so they will be sent to the RPI board via GPIO connector
                        if (outpins[NPADS] & G_QENA) {
                            allins[NPADS] |=    G_LINK | G_DATA;
                        } else {
                            allins[NPADS] &= ~ (G_LINK | G_DATA);
                        }

                        foundit = true;
                    }
                }
            }
        }
        if (! foundit) {
            Tcl_SetResultF (interp, "bad pin name %s", namestr);
            return TCL_ERROR;
        }
    }

    // actually write paddles and/or gpio
    if (writepads) gpiolib->writepads (allins, outpins);
    if (writegpio) gpiolib->writegpio ((outpins[NPADS] & G_QENA) != 0, outpins[NPADS]);

    return TCL_OK;
}

// delay half a cycle
static int cmd_halfcycle (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    int addcyc = 1;
    switch (objc) {
        case 1: break;
        case 2: {
            char const *st = Tcl_GetString (objv[1]);
            if (strcmp (st, "help") == 0) {
                puts ("");
                puts ("  halfcycle [<aluadd>]");
                puts ("    <aluadd> = 0: fast cycle");
                puts ("    <aluadd> = 1: slow cycle (default)");
                puts ("");
                return TCL_OK;
            }
            int rc = Tcl_GetBooleanFromObj (interp, objv[1], &addcyc);
            if (rc != TCL_OK) return rc;
            break;
        }
        default: {
            Tcl_SetResult (interp, (char *) "bad number args", TCL_STATIC);
            return TCL_ERROR;
        }
    }
    gpiolib->halfcycle (addcyc);
    return TCL_OK;
}

// print help messages
static int cmd_help (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    puts ("");
    for (int i = 0; fundefs[i].help != NULL; i ++) {
        printf ("  %10s - %s\n", fundefs[i].name, fundefs[i].help);
    }
    puts ("");
    puts ("for help on specific command, do '<command> help'");
    puts ("");
    return TCL_OK;
}

// return list of selected modules
static int cmd_listmods (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    int nmodules = 0;
    for (int i = 0; i < NMODULES; i ++) {
        if (modules[i].selected) nmodules ++;
    }

    Tcl_Obj *modnames[nmodules];
    nmodules = 0;
    for (int i = 0; i < NMODULES; i ++) {
        Module const *mod = &modules[i];
        if (mod->selected) {
            modnames[nmodules++] = Tcl_NewStringObj (mod->name, strlen (mod->name));
        }
    }

    Tcl_SetObjResult (interp, Tcl_NewListObj (nmodules, modnames));
    return TCL_OK;
}

// return list of all pins on selected modules
static int cmd_listpins (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    int numpins = 0;
    for (int pad = 0; pad <= NPADS; pad ++) {
        uint32_t padmask = allins[pad] | allouts[pad];
        for (PinDefs const *pin = pindefss[pad]; pin->pinmask != 0; pin ++) {
            if (pin->pinmask & padmask) {
                numpins ++;
            }
        }
    }

    Tcl_Obj *pinnames[numpins];
    numpins = 0;
    for (int pad = 0; pad <= NPADS; pad ++) {
        uint32_t padmask = allins[pad] | allouts[pad];
        for (PinDefs const *pin = pindefss[pad]; pin->pinmask != 0; pin ++) {
            if (pin->pinmask & padmask) {
                pinnames[numpins++] = Tcl_NewStringObj (pin->varname, strlen (pin->varname));
            }
        }
    }

    Tcl_SetObjResult (interp, Tcl_NewListObj (numpins, pinnames));
    return TCL_OK;
}

static void Tcl_SetResultF (Tcl_Interp *interp, char const *fmt, ...)
{
    char *buf = NULL;
    va_list ap;
    va_start (ap, fmt);
    if (vasprintf (&buf, fmt, ap) < 0) ABORT ();
    va_end (ap);
    Tcl_SetResult (interp, buf, (void (*) (char *)) free);
}
