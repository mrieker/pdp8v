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

// ./tclboardtest [-csrclib] [-nopads] [-zynqlib] <boardnames> [<scriptname-or-hyphen> [<scriptargs> ...]]
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
static bool traceflag;
static GpioLib *physlib;
static GpioLib *shadlib;
static Tcl_Interp *interp;

static void Tcl_SetResultF (Tcl_Interp *interp, char const *fmt, ...);
static void tracecommand (int objc, Tcl_Obj *const *objv);
static void traceobjarray (int objc, Tcl_Obj *const *objv);
static void updatebidirgpio ();

static Tcl_ObjCmdProc cmd_exam;
static Tcl_ObjCmdProc cmd_force;
static Tcl_ObjCmdProc cmd_halfcycle;
static Tcl_ObjCmdProc cmd_help;
static Tcl_ObjCmdProc cmd_listpins;
static Tcl_ObjCmdProc cmd_listmods;
static Tcl_ObjCmdProc cmd_pinof;
static Tcl_ObjCmdProc cmd_sigof;
static Tcl_ObjCmdProc cmd_syncsh;

struct FunDef {
    Tcl_ObjCmdProc *func;
    void *data;
    char const *name;
    char const *help;
};

static FunDef const fundefs[] = {
    { cmd_exam,       &physlib, "exam",       "examine tube board pins" },
    { cmd_exam,       &shadlib, "examsh",     "examine shadow pins" },
    { cmd_force,      NULL,     "force",      "force <pinname> <pinvalu> ..." },
    { cmd_halfcycle,  NULL,     "halfcycle",  "delay a half cycle" },
    { cmd_help,       NULL,     "help",       "print this help" },
    { cmd_listmods,   NULL,     "listmods",   "return list of selected modules" },
    { cmd_listpins,   NULL,     "listpins",   "return list all pin names of selected modules" },
    { cmd_pinof,      NULL,     "pinof",      "return pin {A..D}{1..32},G{0..31} of given signal" },
    { cmd_sigof,      NULL,     "sigof",      "return signal of given pin" },
    { cmd_syncsh,     NULL,     "syncsh",     "synchronize shadow to tubes" },
    { NULL, NULL, NULL, NULL }
};

int main (int argc, char **argv)
{
    bool csrclib = false;
    bool gotamod = false;
    bool zynqlib = false;
    char const *scriptfile = NULL;
    int scriptindex = -1;

    setlinebuf (stdout);

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
        if (strcasecmp (argv[i], "-trace") == 0) {
            traceflag = true;
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
        scriptindex = i;
        scriptfile = argv[i];
        break;
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

    // access the physical paddles and maybe gpio connector
    physlib =
        csrclib ? (GpioLib *) new CSrcLib (modnames.c_str ()) :
        zynqlib ? (GpioLib *) new ZynqLib (modnames.c_str ()) :
                  (GpioLib *) new PhysLib ();
    physlib->open ();

    // write all input pins with zeroes
    if (! nopads) physlib->writepads (allins, outpins);
    if (allins[NPADS] != 0) physlib->writegpio (false, 0);

    // set up shadow for comparing
    shadlib = new CSrcLib (modnames.c_str ());
    shadlib->open ();
    shadlib->writepads (allins, outpins);
    shadlib->writegpio (false, 0);

    // initialize tcl
    Tcl_FindExecutable (argv[0]);
    interp = Tcl_CreateInterp ();
    if (Tcl_Init (interp) != TCL_OK) ABORT ();
    for (int i = 0; fundefs[i].name != NULL; i ++) {
        if (Tcl_CreateObjCommand (interp, fundefs[i].name, fundefs[i].func, fundefs[i].data, NULL) == NULL) ABORT ();
    }

    // create argv variable
    if (scriptindex >= 0) {
        Tcl_Obj *argvobjs[argc-scriptindex];
        int nargs = 0;
        for (int i = scriptindex; i < argc; i ++) {
            argvobjs[nargs++] = Tcl_NewStringObj (argv[i], strlen (argv[i]));
        }
        Tcl_Obj *argvobjlist = Tcl_NewListObj (nargs, argvobjs);
        Tcl_ObjSetVar2 (interp, Tcl_NewStringObj ("argv", 4), NULL, argvobjlist, 0);
    }

    // maybe there is a script init file
    char const *tcltestini = getenv ("tcltestini");
    char *inihelp = NULL;
    if ((tcltestini != NULL) && (tcltestini[0] != 0)) {
        int rc = Tcl_EvalFile (interp, tcltestini);
        if (rc != TCL_OK) {
            char const *err = Tcl_GetStringResult (interp);
            if ((err == NULL) || (err[0] == 0)) fprintf (stderr, "error %d evaluating tcltestini %s\n", rc, tcltestini);
                                  else fprintf (stderr, "error %d evaluating tcltestini %s: %s\n", rc, tcltestini, err);
            Tcl_EvalEx (interp, "puts $::errorInfo", -1, TCL_EVAL_GLOBAL);
            return 1;
        }
        char const *res = Tcl_GetStringResult (interp);
        if ((res != NULL) && (res[0] != 0)) inihelp = strdup (res);
    }

    // if given a filename, process that file as a whole
    if ((scriptfile != NULL) && (strcmp (scriptfile, "-") != 0)) {

        // evaluate the file as a whole
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
    if (inihelp != NULL) {
        puts (inihelp);
        free (inihelp);
    }
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
    tracecommand (objc, objv);
    if (traceflag) fputs (" =>", stdout);

    // read the pins coming from the tubes + values we are outputting to the tubes
    // ...or likewise with shadow pins
    // - allins[0..NPADS-1] = mask of pins we are sending to the bus via paddles (they go to inputs on some boards)
    // - allouts[0..NPADS-1] = mask of pins we are reading from the bus via paddles (they come from outputs on some boards)
    uint32_t padlvalus[NPADS+1];
    GpioLib *gpiolib = *(GpioLib **) clientdata;
    if (! nopads) {
        gpiolib->readpads (padlvalus);
        for (int j = 0; j < NPADS; j ++) {
            padlvalus[j] = (padlvalus[j] & allouts[j]) | (outpins[j] & allins[j]);
        }
    }
    // - allins[NPADS] = mask of pins we are sending to rpi board via gpio connector (inputs to the rpi board)
    // - allouts[NPADS] = mask of pins we are reading from rpi board via gpio connector (outputs from the rpi board)
    padlvalus[NPADS] = outpins[NPADS] & allins[NPADS];
    if (allouts[NPADS] != 0) padlvalus[NPADS] |= gpiolib->readgpio () & allouts[NPADS];

    // check for 'exam <modulename>'
    if (objc == 2) {
        char const *namestr = Tcl_GetString (objv[1]);
        if (strcasecmp (namestr, "help") == 0) {
            puts ("");
            puts ("  exam <modulename>  - get all pins for the given module");
            puts ("  exam all           - get all pins for all selected modules");
            puts ("  exam <pinname> ... - get list values of the given pins, alternating name value ...");
            puts ("                       may be bus name such as pcq for all 12 bits as single value");
            puts ("");
            if (traceflag) fputc ('\n', stdout);
            return TCL_OK;
        }

        // set up struct that can hold all possible pins
        bool allmods = strcasecmp ("all", namestr) == 0;
        Element elements[32*(NPADS+1)*NMODULES];
        memset (elements, 0, sizeof elements);
        int nelements = 0;

        for (int i = 0; i < NMODULES; i ++) {
            Module const *mod = &modules[i];
            if ((allmods && mod->selected) || (strcasecmp (mod->name, namestr) == 0)) {

                // loop through all possible pins of the module
                for (int pad = 0; pad <= NPADS; pad ++) {
                    uint32_t insandouts = mod->outs[pad] | mod->ins[pad];
                    if ((pad == NPADS) && (insandouts != 0)) insandouts |= G_LINK | G_DATA;
                    for (PinDefs const *pin = pindefss[pad]; pin->pinmask != 0; pin ++) {

                        // see if this pin is being output by the module or is an input to the module
                        if (pin->pinmask & insandouts) {

                            // get pin name and bitnumber
                            int namelen = strlen (pin->varname);
                            int bitno = 0;
                            char const *p = strchr (pin->varname, '[');
                            if (p != NULL) {
                                namelen = p - pin->varname;
                                bitno = atoi (++ p);
                            }

                            // make slot in struct for the value
                            int j;
                            for (j = 0; j < nelements; j ++) {
                                if ((memcmp (pin->varname, elements[j].name, namelen) == 0) && (elements[j].name[namelen] == 0)) break;
                            }
                            if (j == nelements) {
                                ASSERT (namelen < (int) sizeof elements[j].name);
                                memcpy (elements[j].name, pin->varname, namelen);
                                nelements ++;
                            }

                            // fill in pin's value
                            if (padlvalus[pad] & pin->pinmask) elements[j].value |= 1U << bitno;
                        }
                    }
                }
            }
        }

        // make sure we got a board (otherwise the name might be a single pin name)
        if (nelements > 0) {

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
            traceobjarray (nelements * 2, tclist);
            if (traceflag) fputc ('\n', stdout);
            return TCL_OK;
        }
    }

    // specifically listed pins (can be a bus name such as pcq meaning all 12 bits of program counter)
    Tcl_Obj *results[objc-1];
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
            if (traceflag) fputc ('\n', stdout);
            return TCL_ERROR;
        }

        // add value to output list
        results[i-1] = Tcl_NewIntObj (value);
    }

    Tcl_SetObjResult (interp, Tcl_NewListObj (objc - 1, results));
    traceobjarray (objc - 1, results);
    if (traceflag) fputc ('\n', stdout);
    return TCL_OK;
}

// force <name1> <value1> ...
static int cmd_force (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    tracecommand (objc, objv);
    if (traceflag) fputc ('\n', stdout);

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

                        updatebidirgpio ();

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
    if (writepads) {
        physlib->writepads (allins, outpins);
        shadlib->writepads (allins, outpins);
    }
    if (writegpio) {
        physlib->writegpio ((outpins[NPADS] & G_QENA) != 0, outpins[NPADS]);
        shadlib->writegpio ((outpins[NPADS] & G_QENA) != 0, outpins[NPADS]);
    }

    return TCL_OK;
}

// delay half a cycle
static int cmd_halfcycle (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    tracecommand (objc, objv);
    if (traceflag) fputc ('\n', stdout);

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
    physlib->halfcycle (addcyc);
    shadlib->halfcycle (addcyc);
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
        if ((pad == NPADS) && (padmask != 0)) padmask |= G_LINK | G_DATA;
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
        if ((pad == NPADS) && (padmask != 0)) padmask |= G_LINK | G_DATA;
        for (PinDefs const *pin = pindefss[pad]; pin->pinmask != 0; pin ++) {
            if (pin->pinmask & padmask) {
                pinnames[numpins++] = Tcl_NewStringObj (pin->varname, strlen (pin->varname));
            }
        }
    }

    Tcl_SetObjResult (interp, Tcl_NewListObj (numpins, pinnames));
    return TCL_OK;
}

// get pin for given signal
static int cmd_pinof (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    if (objc == 2) {
        char const *sig = Tcl_GetString (objv[1]);
        for (int pad = 0; pad <= NPADS; pad ++) {
            for (PinDefs const *pin = pindefss[pad]; pin->pinmask != 0; pin ++) {
                if (strcmp (pin->varname, sig) == 0) {
                    int pinnum = __builtin_ctz (pin->pinmask) + 1;
                    char pinlet = pad + 'A';
                    if (pad == NPADS) {
                        pinlet = 'G';
                        -- pinnum;
                    }
                    Tcl_SetResultF (interp, "%c%d", pinlet, pinnum);
                    return TCL_OK;
                }
            }
        }
        Tcl_SetResultF (interp, "unknown signal %s", sig);
        return TCL_ERROR;
    }
    Tcl_SetResultF (interp, "wrong num args - pinof <signalname>");
    return TCL_ERROR;
}

// get signal for given pin
static int cmd_sigof (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    if (objc == 2) {
        char const *pinxx = Tcl_GetString (objv[1]);
        char pinlet = pinxx[0];
        char *p;
        int pinnum = strtol (pinxx + 1, &p, 0);
        if ((pinlet >= 'a') && (pinlet <= 'z')) pinlet -= 'a' - 'A';
        int pad = pinlet - 'A';
        if (pinlet == 'G') {
            pad = NPADS;
            pinnum ++;
        }
        if ((pad >= 0) && (pad <= NPADS) && (pinnum >= 1) && (pinnum <= 32) && (*p == 0)) {
            for (PinDefs const *pin = pindefss[pad]; pin->pinmask != 0; pin ++) {
                if (pin->pinmask == 1U << (pinnum - 1)) {
                    Tcl_SetResult (interp, (char *) pin->varname, TCL_STATIC);
                    return TCL_OK;
                }
            }
        }
        Tcl_SetResultF (interp, "unknown pin %s", pinxx);
        return TCL_ERROR;
    }
    Tcl_SetResultF (interp, "wrong num args - sigof <pinname>");
    return TCL_ERROR;
}

// synchronize shadow state to tubes
static int cmd_syncsh (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    struct FFBit { char const *pinname; char const *acthiff; char const *actloff; int rbit; };
    static FFBit const ffbits[] = {

        // padlpin    same as padlpin      comp of padlpin
        { "acq[0]",  "Q/aclo/aclcirc",    "_Q/aclo/aclcirc",   0 },
        { "acq[1]",  "Q/aclo/aclcirc",    "_Q/aclo/aclcirc",   1 },
        { "acq[2]",  "Q/aclo/aclcirc",    "_Q/aclo/aclcirc",   2 },
        { "acq[3]",  "Q/aclo/aclcirc",    "_Q/aclo/aclcirc",   3 },
        { "acq[4]",  "Q/acmid/aclcirc",   "_Q/acmid/aclcirc",  0 },
        { "acq[5]",  "Q/acmid/aclcirc",   "_Q/acmid/aclcirc",  1 },
        { "acq[6]",  "Q/acmid/aclcirc",   "_Q/acmid/aclcirc",  2 },
        { "acq[7]",  "Q/acmid/aclcirc",   "_Q/acmid/aclcirc",  3 },
        { "acq[8]",  "Q/achi/aclcirc",    "_Q/achi/aclcirc",   0 },
        { "acq[9]",  "Q/achi/aclcirc",    "_Q/achi/aclcirc",   1 },
        { "acq[10]", "Q/achi/aclcirc",    "_Q/achi/aclcirc",   2 },
        { "acq[11]", "Q/achi/aclcirc",    "_Q/achi/aclcirc",   3 },

        { "defer1q", "_Q/defer1/seqcirc", "Q/defer1/seqcirc",  0 },
        { "defer2q", "_Q/defer2/seqcirc", "Q/defer2/seqcirc",  0 },
        { "defer3q", "_Q/defer3/seqcirc", "Q/defer3/seqcirc",  0 },
        { "exec1q",  "_Q/exec1/seqcirc",  "Q/exec1/seqcirc",   0 },
        { "exec2q",  "_Q/exec2/seqcirc",  "Q/exec2/seqcirc",   0 },
        { "exec3q",  "Q/exec3/seqcirc",   "_Q/exec3/seqcirc",  0 },
        { "fetch1q", "Q/fetch1/seqcirc",  "_Q/fetch1/seqcirc", 0 },
        { "fetch2q", "_Q/fetch2/seqcirc", "Q/fetch2/seqcirc",  0 },
        { "intak1q", "_Q/intak1/seqcirc", "Q/intak1/seqcirc",  0 },

        { "irq[9]",  "Q/ireg09/seqcirc",  "_Q/ireg09/seqcirc", 0 },
        { "irq[10]", "Q/ireg10/seqcirc",  "_Q/ireg10/seqcirc", 0 },
        { "irq[11]", "Q/ireg11/seqcirc",  "_Q/ireg11/seqcirc", 0 },

        { "lnq",     "_Q/lnreg/aclcirc",  "Q/lnreg/aclcirc",   0 },

        { "maq[0]",  "_Q/malo/macirc",    "Q/malo/macirc",     0 },
        { "maq[1]",  "_Q/malo/macirc",    "Q/malo/macirc",     1 },
        { "maq[2]",  "_Q/malo/macirc",    "Q/malo/macirc",     2 },
        { "maq[3]",  "_Q/malo/macirc",    "Q/malo/macirc",     3 },
        { "maq[4]",  "_Q/mamid/macirc",   "Q/mamid/macirc",    0 },
        { "maq[5]",  "_Q/mamid/macirc",   "Q/mamid/macirc",    1 },
        { "maq[6]",  "_Q/mamid/macirc",   "Q/mamid/macirc",    2 },
        { "maq[7]",  "_Q/mamid/macirc",   "Q/mamid/macirc",    3 },
        { "maq[8]",  "_Q/mahi/macirc",    "Q/mahi/macirc",     0 },
        { "maq[9]",  "_Q/mahi/macirc",    "Q/mahi/macirc",     1 },
        { "maq[10]", "_Q/mahi/macirc",    "Q/mahi/macirc",     2 },
        { "maq[11]", "_Q/mahi/macirc",    "Q/mahi/macirc",     3 },

        { "pcq[0]",  "Q/pc00/pccirc",     "_Q/pc00/pccirc",    0 },
        { "pcq[1]",  "Q/pc01/pccirc",     "_Q/pc01/pccirc",    0 },
        { "pcq[2]",  "Q/pc02/pccirc",     "_Q/pc02/pccirc",    0 },
        { "pcq[3]",  "Q/pc03/pccirc",     "_Q/pc03/pccirc",    0 },
        { "pcq[4]",  "Q/pc04/pccirc",     "_Q/pc04/pccirc",    0 },
        { "pcq[5]",  "Q/pc05/pccirc",     "_Q/pc05/pccirc",    0 },
        { "pcq[6]",  "Q/pc06/pccirc",     "_Q/pc06/pccirc",    0 },
        { "pcq[7]",  "Q/pc07/pccirc",     "_Q/pc07/pccirc",    0 },
        { "pcq[8]",  "Q/pc08/pccirc",     "_Q/pc08/pccirc",    0 },
        { "pcq[9]",  "Q/pc09/pccirc",     "_Q/pc09/pccirc",    0 },
        { "pcq[10]", "Q/pc10/pccirc",     "_Q/pc10/pccirc",    0 },
        { "pcq[11]", "Q/pc11/pccirc",     "_Q/pc11/pccirc",    0 },

        { NULL, NULL, NULL, 0 } };

    if (traceflag) fputs ("+ syncsh\n", stdout);

    // read tube values
    uint32_t padlvalus[NPADS];
    if (! nopads) {
        physlib->readpads (padlvalus);
        for (int j = 0; j < NPADS; j ++) {
            padlvalus[j] = (padlvalus[j] & allouts[j]) | (outpins[j] & allins[j]);
        }
    }

    // for each tube flipflop pin, write corresponding shadow flipflop bit
    for (FFBit const *ff = ffbits; ff->pinname != NULL; ff ++) {
        for (int pad = 0; pad < NPADS; pad ++) {
            for (PinDefs const *pin = pindefss[pad]; pin->pinmask != 0; pin ++) {
                if (strcmp (pin->varname, ff->pinname) == 0) {

                    // read value from tubes via the paddles
                    bool tubepin = (padlvalus[pad] & pin->pinmask) != 0;

                    // update csrclib shadowlib bits to match the value read from tubes via the paddles
                    bool *acthi = shadlib->getvarbool (ff->acthiff, ff->rbit);
                    bool *actlo = shadlib->getvarbool (ff->actloff, ff->rbit);
                    if (acthi != NULL) {
                        *acthi =   tubepin;
                        if (traceflag) printf ("  shad  %17s[%d] <=   %d %s\n", ff->acthiff, ff->rbit, tubepin, ff->pinname);
                    }
                    if (actlo != NULL) {
                        *actlo = ! tubepin;
                        if (traceflag) printf ("  shad  %17s[%d] <= ~ %d %s\n", ff->actloff, ff->rbit, tubepin, ff->pinname);
                    }

                    // maybe using another copy of csrclib instead of tubes, make sure complement is consistent
                    bool *cmpl = physlib->getvarbool (ff->actloff, ff->rbit);
                    if (cmpl != NULL) {
                        *cmpl = ! tubepin;
                        if (traceflag) printf ("  phys  %17s[%d] <= ~ %d %s\n", ff->actloff, ff->rbit, tubepin, ff->pinname);
                    }
                    goto found;
                }
            }
        }
    found:;
    }

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

static void tracecommand (int objc, Tcl_Obj *const *objv)
{
    if (traceflag) {
        fputc ('+', stdout);
        traceobjarray (objc, objv);
    }
}

static void traceobjarray (int objc, Tcl_Obj *const *objv)
{
    if (traceflag) {
        for (int i = 0; i < objc; i ++) {
            fputc (' ', stdout);
            fputs (Tcl_GetString (objv[i]), stdout);
        }
    }
}

// in case DENA or QENA changed, update list of input and output pins
static void updatebidirgpio ()
{
    // if DENA is set, allow LINK and DATA to be read so they will be received from the RPI board via GPIO connector
    if (outpins[NPADS] & G_DENA) {
        allouts[NPADS] |=    G_LINK | G_DATA;
    } else {
        allouts[NPADS] &= ~ (G_LINK | G_DATA);
    }

    // if QENA is set, allow LINK and DATA to be written so they will be sent to the RPI board via GPIO connector
    if (outpins[NPADS] & G_QENA) {
        allins[NPADS] |=    G_LINK | G_DATA;
    } else {
        allins[NPADS] &= ~ (G_LINK | G_DATA);
    }
}
