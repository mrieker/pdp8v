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

#include <pthread.h>
#include <string.h>
#include <tcl.h>

#include "abcd.h"
#include "binloader.h"
#include "controls.h"
#include "disassemble.h"
#include "dyndis.h"
#include "iodevs.h"
#include "linkloader.h"
#include "memext.h"
#include "memory.h"
#include "miscdefs.h"
#include "rdcyc.h"
#include "readprompt.h"
#include "rimloader.h"
#include "script.h"
#include "shadow.h"

static Tcl_Interp *interp;

static void *tclthread (void *fnv);
static int cmpdevnames (void const *v, void const *w);
static int procscret (Tcl_Interp *interp, SCRet *scret);
static void Tcl_SetResultF (Tcl_Interp *interp, char const *fmt, ...);

void runscript (char const *argv0, char const *filename)
{
    Tcl_FindExecutable (argv0);
    pthread_t tclpid;
    int rc = createthread (&tclpid, tclthread, (void *) filename);
    if (rc != 0) ABORT ();
    pthread_detach (tclpid);
    pthread_setname_np (tclpid, "script");
    haltflags = HF_HALTIT;
}

static Tcl_ObjCmdProc cmd_alliodevs;
static Tcl_ObjCmdProc cmd_cpu;
static Tcl_ObjCmdProc cmd_disasop;
static Tcl_ObjCmdProc cmd_dyndis;
static Tcl_ObjCmdProc cmd_end;
static Tcl_ObjCmdProc cmd_gpio;
static Tcl_ObjCmdProc cmd_halfcycle;
static Tcl_ObjCmdProc cmd_halt;
static Tcl_ObjCmdProc cmd_haltreason;
static Tcl_ObjCmdProc cmd_help;
static Tcl_ObjCmdProc cmd_iodev;
static Tcl_ObjCmdProc cmd_loadbin;
static Tcl_ObjCmdProc cmd_loadlink;
static Tcl_ObjCmdProc cmd_loadrim;
static Tcl_ObjCmdProc cmd_option;
static Tcl_ObjCmdProc cmd_readmem;
static Tcl_ObjCmdProc cmd_reset;
static Tcl_ObjCmdProc cmd_run;
static Tcl_ObjCmdProc cmd_stepcyc;
static Tcl_ObjCmdProc cmd_stepins;
static Tcl_ObjCmdProc cmd_swreg;
static Tcl_ObjCmdProc cmd_wait;
static Tcl_ObjCmdProc cmd_writemem;

struct FunDef {
    Tcl_ObjCmdProc *func;
    char const *name;
    char const *help;
};

static FunDef fundefs[] = {
    { cmd_alliodevs,  "alliodevs",  "list all i/o devices for iodev command" },
    { cmd_cpu,        "cpu",        "access shadow state" },
    { cmd_disasop,    "disasop",    "disassemble opcode" },
    { cmd_dyndis,     "dyndis",     "dynamic disassembly" },
    { cmd_end,        "end",        "exit script thread" },
    { cmd_gpio,       "gpio",       "access gpio state" },
    { cmd_halfcycle,  "halfcycle",  "delay a half cycle" },
    { cmd_halt,       "halt",       "halt processor" },
    { cmd_haltreason, "haltreason", "reason halted" },
    { cmd_help,       "help",       "print this help" },
    { cmd_iodev,      "iodev",      "access i/o device state" },
    { cmd_loadbin,    "loadbin",    "load bin format tape file" },
    { cmd_loadlink,   "loadlink",   "load link format file" },
    { cmd_loadrim,    "loadrim",    "load rim format file" },
    { cmd_option,     "option",     "turn various options on/off" },
    { cmd_readmem,    "readmem",    "read memory location" },
    { cmd_reset,      "reset",      "reset processor, optionally load initial if, pc" },
    { cmd_run,        "run",        "run processor" },
    { cmd_stepcyc,    "stepcyc",    "step a single cycle" },
    { cmd_stepins,    "stepins",    "step a single instruction" },
    { cmd_swreg,      "swreg",      "access switch register" },
    { cmd_wait,       "wait",       "wait for processor to halt" },
    { cmd_writemem,   "writemem",   "write memory location" },
    { NULL, NULL, NULL }
};

static void *tclthread (void *fnv)
{
    char const *fn = (char const *) fnv;

    interp = Tcl_CreateInterp ();
    if (interp == NULL) ABORT ();
    int rc = Tcl_Init (interp);
    if (rc != TCL_OK) {
        char const *err = Tcl_GetStringResult (interp);
        if ((err != NULL) && (err[0] != 0)) {
            fprintf (stderr, "script: error %d initialing tcl: %s\n", rc, err);
        } else {
            fprintf (stderr, "script: error %d initialing tcl\n", rc);
        }
        ABORT ();
    }

    // https://www.tcl-lang.org/man/tcl/TclLib/CrtObjCmd.htm

    for (int i = 0; fundefs[i].name != NULL; i ++) {
        if (Tcl_CreateObjCommand (interp, fundefs[i].name, fundefs[i].func, NULL, NULL) == NULL) ABORT ();
    }

    // wait for raspictl main initialization to complete
    ctl_wait ();

    // maybe there is a script init file
    char const *scriptini = getenv ("scriptini");
    char *inihelp = NULL;
    if (scriptini != NULL) {
        rc = Tcl_EvalFile (interp, scriptini);
        if (rc != TCL_OK) {
            char const *err = Tcl_GetStringResult (interp);
            if ((err == NULL) || (err[0] == 0)) fprintf (stderr, "script: error %d evaluating scriptini %s\n", rc, scriptini);
                                  else fprintf (stderr, "script: error %d evaluating scriptini %s: %s\n", rc, scriptini, err);
            Tcl_EvalEx (interp, "puts $::errorInfo", -1, TCL_EVAL_GLOBAL);
            ABORT ();
        }
        char const *res = Tcl_GetStringResult (interp);
        if ((res != NULL) && (res[0] != 0)) inihelp = strdup (res);
    }

    // if given a filename, process that file as a whole
    if (strcmp (fn, "-") != 0) {
        rc = Tcl_EvalFile (interp, fn);
        if (rc != TCL_OK) {
            char const *res = Tcl_GetStringResult (interp);
            if ((res == NULL) || (res[0] == 0)) fprintf (stderr, "script: error %d evaluating script %s\n", rc, fn);
                                  else fprintf (stderr, "script: error %d evaluating script %s: %s\n", rc, fn, res);
            Tcl_EvalEx (interp, "puts $::errorInfo", -1, TCL_EVAL_GLOBAL);
            ABORT ();
        }
    }

    // either way, prompt and process commands from stdin
    // to have a script file with no stdin processing, end script file with 'run ; wait ; exit'
    puts ("\nTCL scripting, do 'help' for raspictl-specific commands");
    puts ("  do 'exit' to exit raspictl");
    if (inihelp != NULL) {
        puts (inihelp);
        free (inihelp);
    }
    for (char const *line;;) {
        ctrlcflag = false;
        line = readprompt ("raspictl> ");
        if (line == NULL) break;
        ctrlcflag = false;
        rc = Tcl_EvalEx (interp, line, -1, TCL_EVAL_GLOBAL);
        char const *res = Tcl_GetStringResult (interp);
        if (rc != TCL_OK) {
            if ((res == NULL) || (res[0] == 0)) fprintf (stderr, "script: error %d evaluating command\n", rc);
                                  else fprintf (stderr, "script: error %d evaluating command: %s\n", rc, res);
            Tcl_EvalEx (interp, "puts $::errorInfo", -1, TCL_EVAL_GLOBAL);
        }
        else if ((res != NULL) && (res[0] != 0)) printf ("%s\n", res);
    }

    Tcl_Finalize ();
    exit (0);
}

// return list of all i/o device names
static int cmd_alliodevs (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    int numnameddevs = 0;
    for (IODev *iodev = alliodevs; iodev != NULL; iodev = iodev->nextiodev) {
        if (iodev->iodevname != NULL) numnameddevs ++;
    }

    char const *devnames[numnameddevs];
    int i = 0;
    for (IODev *iodev = alliodevs; iodev != NULL; iodev = iodev->nextiodev) {
        if (iodev->iodevname != NULL) devnames[i++] = iodev->iodevname;
    }
    qsort (devnames, numnameddevs, sizeof *devnames, cmpdevnames);

    Tcl_Obj *devnameobjs[numnameddevs];
    for (i = 0; i < numnameddevs; i ++) {
        devnameobjs[i] = Tcl_NewStringObj (devnames[i], -1);
    }

    Tcl_Obj *devnamelist = Tcl_NewListObj (numnameddevs, devnameobjs);
    Tcl_SetObjResult (interp, devnamelist);

    return TCL_OK;
}

// get cpu state (really the shadow state)
static int cmd_cpu (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    // must have at least one argument (<command>)
    if (objc < 2) {
        Tcl_SetResult (interp, (char *) "missing cpu sub-command", TCL_STATIC);
        return TCL_ERROR;
    }

    // convert all arguments to strings
    int argc = objc - 1;
    char const *argv[argc+1];
    for (int i = 0; i < argc; i ++) {
        argv[i] = Tcl_GetString (objv[i+1]);
    }
    argv[argc] = NULL;

    // run the command
    SCRet *scret = shadow.scriptcmd (argc, argv);

    // process return value
    return procscret (interp, scret);
}

// disassemble opcode
static int cmd_disasop (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    if ((objc < 2) || (objc > 3)) {
        Tcl_SetResult (interp, (char *) "wrong number args", TCL_STATIC);
        return TCL_ERROR;
    }

    if (strcmp (Tcl_GetString (objv[1]), "help") == 0) {
        puts ("");
        puts ("  disasop <opcode> [<address>]");
        puts ("");
        puts ("  returns string giving disassembly of opcode at that address");
        puts ("");
        return TCL_OK;
    }

    int ir = 00000;
    int pc = 07777;
    int rc = Tcl_GetIntFromObj (interp, objv[1], &ir);
    if (rc != TCL_OK) return rc;
    if (objc > 2) {
        rc = Tcl_GetIntFromObj (interp, objv[2], &pc);
        if (rc != TCL_OK) return rc;
    }

    char const *ioin = iodisas (ir);
    if (ioin != NULL) {
        Tcl_SetResult (interp, (char *) ioin, TCL_STATIC);
    } else {
        std::string opcd = disassemble (ir, pc);
        Tcl_SetResult (interp, strdup (opcd.c_str ()), (void (*) (char *)) free);
    }
    return TCL_OK;
}

// dynamic disassembly
static int cmd_dyndis (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    if (objc < 2) {
        Tcl_SetResult (interp, (char *) "missing sub-command", TCL_STATIC);
        return TCL_ERROR;
    }

    char const *subcommand = Tcl_GetString (objv[1]);

    if (strcmp (subcommand, "clear") == 0) {
        dyndisclear ();
        return TCL_OK;
    }

    if (strcmp (subcommand, "dump") == 0) {
        char const *filename = "-";
        if (objc > 2) {
            filename = Tcl_GetString (objv[2]);
        }
        FILE *dumpfile = stdout;
        if (strcmp (filename, "-") != 0) {
            dumpfile = fopen (filename, "w");
            if (dumpfile == NULL) {
                Tcl_SetResult (interp, (char *) strerror (errno), TCL_STATIC);
                return TCL_ERROR;
            }
        }
        dyndisdump (dumpfile);
        if ((dumpfile != stdout) && (fclose (dumpfile) < 0)) {
            Tcl_SetResult (interp, (char *) strerror (errno), TCL_STATIC);
            return TCL_ERROR;
        }
        return TCL_OK;
    }

    if (strcmp (subcommand, "enable") == 0) {
        switch (objc) {
            case 2: {
                Tcl_SetObjResult (interp, Tcl_NewIntObj (dyndisena));
                return TCL_OK;
            }
            case 3: {
                int enab;
                int rc = Tcl_GetIntFromObj (interp, objv[2], &enab);
                if (rc == TCL_OK) dyndisena = enab;
                return rc;
            }
        }
        Tcl_SetResult (interp, (char *) "wrong number args", TCL_STATIC);
        return TCL_ERROR;
    }

    if (strcmp (subcommand, "help") == 0) {
        puts ("");
        puts ("  clear");
        puts ("  dump [filename]");
        puts ("  enable");
        puts ("  enable 0/1");
        puts ("  help");
        puts ("");
        puts ("  dumps dynamic assembly captured during execution");
        puts ("");
        return TCL_OK;
    }

    Tcl_SetResult (interp, (char *) "unknown sub-command, try 'dyndis help'", TCL_STATIC);
    return TCL_ERROR;
}

// exit script thread
static int cmd_end (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    scriptmode = false;
    pthread_exit (NULL);
    return TCL_OK;
}

// get gpio state (actual tube state)
static int cmd_gpio (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    ABCD pinss;
    SCRet *scret;

    // must have at least one argument (<command>)
    if (objc < 2) {
        Tcl_SetResult (interp, (char *) "missing gpio sub-command", TCL_STATIC);
        return TCL_ERROR;
    }

    // convert all arguments to strings
    int argc = objc - 1;
    char const *argv[argc+1];
    for (int i = 0; i < argc; i ++) {
        argv[i] = Tcl_GetString (objv[i+1]);
    }
    argv[argc] = NULL;

    // decode <gpiogetstring> [<signalname>]
    if (strcmp (argv[0], "decode") == 0) {
        if ((argc == 2) || (argc == 3)) {

            // fill pinss with values from [gpio get] string
            //  a=value;b=value;c=value;d=value;g=value
            // if no a,b,c,d value, assume paddles not present
            memset (pinss.cons, 0, sizeof pinss.cons);
            bool haspads = false;
            for (char const *p = argv[1]; *p != 0; p ++) {
                if (memcmp (p, "a=", 2) == 0) {
                    pinss.acon = strtoul (p + 2, (char **) &p, 0);
                    haspads = true;
                } else if (memcmp (p, "b=", 2) == 0) {
                    pinss.bcon = strtoul (p + 2, (char **) &p, 0);
                    haspads = true;
                } else if (memcmp (p, "c=", 2) == 0) {
                    pinss.ccon = strtoul (p + 2, (char **) &p, 0);
                    haspads = true;
                } else if (memcmp (p, "d=", 2) == 0) {
                    pinss.dcon = strtoul (p + 2, (char **) &p, 0);
                    haspads = true;
                } else if (memcmp (p, "g=", 2) == 0) {
                    pinss.gcon = strtoul (p + 2, (char **) &p, 0);
                }
                if (*p == 0) break;
                if (*p != ';') {
                    scret = new SCRetErr ("bad gpio value string %s at %s", argv[1], p);
                    goto ret;
                }
            }

            // decode the values
            pinss.decode ();

            // make one big long string
            char *buf = NULL;
            if (haspads) {
                if (asprintf (&buf,
                    "acqzero=%o;"
                    "_jump=%o;"
                    "_ac_sc=%o;"
                    "intak1q=%o;"
                    "fetch1q=%o;"
                    "_ac_aluq=%o;"
                    "_alu_add=%o;"
                    "_alu_and=%o;"
                    "_alua_m1=%o;"
                    "_alucout=%o;"
                    "_alua_ma=%o;"
                    "alua_mq0600=%o;"
                    "alua_mq1107=%o;"
                    "alua_pc0600=%o;"
                    "alua_pc1107=%o;"
                    "_alub_m1=%o;"
                    "alub_1=%o;"
                    "_alub_ac=%o;"
                    "clok2=%o;"
                    "fetch2q=%o;"
                    "_grpa1q=%o;"
                    "defer1q=%o;"
                    "defer2q=%o;"
                    "defer3q=%o;"
                    "exec1q=%o;"
                    "grpb_skip=%o;"
                    "exec2q=%o;"
                    "_dfrm=%o;"
                    "inc_axb=%o;"
                    "_intak=%o;"
                    "intrq=%o;"
                    "exec3q=%o;"
                    "ioinst=%o;"
                    "ioskp=%o;"
                    "iot2q=%o;"
                    "_ln_wrt=%o;"
                    "_lnq=%o;"
                    "lnq=%o;"
                    "_ma_aluq=%o;"
                    "mql=%o;"
                    "_mread=%o;"
                    "_mwrite=%o;"
                    "_pc_aluq=%o;"
                    "_pc_inc=%o;"
                    "reset=%o;"
                    "_newlink=%o;"
                    "tad3q=%o;"
                    "CLOCK=%o;"
                    "RESET=%o;"
                    "LINK=%o;"
                    "IOS=%o;"
                    "QENA=%o;"
                    "IRQ=%o;"
                    "DENA=%o;"
                    "JUMP=%o;"
                    "IOIN=%o;"
                    "DFRM=%o;"
                    "READ=%o;"
                    "WRITE=%o;"
                    "IAK=%o;"
                    "acq=0%04o;"
                    "_aluq=0%04o;"
                    "_maq=0%04o;"
                    "maq=0%04o;"
                    "mq=0%04o;"
                    "pcq=0%04o;"
                    "irq=0%04o;"
                    "DATA=0%04o;"
                    "state=%*s",

                    pinss.acqzero,
                    pinss._jump,
                    pinss._ac_sc,
                    pinss.intak1q,
                    pinss.fetch1q,
                    pinss._ac_aluq,
                    pinss._alu_add,
                    pinss._alu_and,
                    pinss._alua_m1,
                    pinss._alucout,
                    pinss._alua_ma,
                    pinss.alua_mq0600,
                    pinss.alua_mq1107,
                    pinss.alua_pc0600,
                    pinss.alua_pc1107,
                    pinss._alub_m1,
                    pinss.alub_1,
                    pinss._alub_ac,
                    pinss.clok2,
                    pinss.fetch2q,
                    pinss._grpa1q,
                    pinss.defer1q,
                    pinss.defer2q,
                    pinss.defer3q,
                    pinss.exec1q,
                    pinss.grpb_skip,
                    pinss.exec2q,
                    pinss._dfrm,
                    pinss.inc_axb,
                    pinss._intak,
                    pinss.intrq,
                    pinss.exec3q,
                    pinss.ioinst,
                    pinss.ioskp,
                    pinss.iot2q,
                    pinss._ln_wrt,
                    pinss._lnq,
                    pinss.lnq,
                    pinss._ma_aluq,
                    pinss.mql,
                    pinss._mread,
                    pinss._mwrite,
                    pinss._pc_aluq,
                    pinss._pc_inc,
                    pinss.reset,
                    pinss._newlink,
                    pinss.tad3q,
                    pinss.CLOCK,
                    pinss.RESET,
                    pinss.LINK,
                    pinss.IOS,
                    pinss.QENA,
                    pinss.IRQ,
                    pinss.DENA,
                    pinss.JUMP,
                    pinss.IOIN,
                    pinss.DFRM,
                    pinss.READ,
                    pinss.WRITE,
                    pinss.IAK,
                    pinss.acq,
                    pinss._aluq,
                    pinss._maq,
                    pinss.maq,
                    pinss.mq,
                    pinss.pcq,
                    pinss.irq,
                    pinss.DATA,
                    54, "") < 0) ABORT ();
                char *s = strstr (buf, ";state=") + 7;
                if (pinss.fetch1q) { strcpy (s, "FETCH1"); s += 6; }
                if (pinss.fetch2q) { strcpy (s, "FETCH2"); s += 6; }
                if (pinss.defer1q) { strcpy (s, "DEFER1"); s += 6; }
                if (pinss.defer2q) { strcpy (s, "DEFER2"); s += 6; }
                if (pinss.defer3q) { strcpy (s, "DEFER3"); s += 6; }
                if (pinss.exec1q)  { strcpy (s, "EXEC1");  s += 5; }
                if (pinss.exec2q)  { strcpy (s, "EXEC2");  s += 5; }
                if (pinss.exec3q)  { strcpy (s, "EXEC3");  s += 5; }
                if (pinss.intak1q) { strcpy (s, "INTAK1"); s += 6; }
            } else {
                if (asprintf (&buf,
                    "CLOCK=%o;"
                    "RESET=%o;"
                    "LINK=%o;"
                    "IOS=%o;"
                    "QENA=%o;"
                    "IRQ=%o;"
                    "DENA=%o;"
                    "JUMP=%o;"
                    "IOIN=%o;"
                    "DFRM=%o;"
                    "READ=%o;"
                    "WRITE=%o;"
                    "IAK=%o;"
                    "DATA=0%04o",

                    pinss.CLOCK,
                    pinss.RESET,
                    pinss.LINK,
                    pinss.IOS,
                    pinss.QENA,
                    pinss.IRQ,
                    pinss.DENA,
                    pinss.JUMP,
                    pinss.IOIN,
                    pinss.DFRM,
                    pinss.READ,
                    pinss.WRITE,
                    pinss.IAK,
                    pinss.DATA) < 0) ABORT ();
            }
            if (argc == 2) {
                scret = new SCRetStr (buf);
            } else {

                // find the one requested signal
                const char *signam = argv[2];
                int signamlen = strlen (signam);
                for (char *p = buf;; p ++) {
                    if ((memcmp (p, signam, signamlen) == 0) && (p[signamlen] == '=')) {
                        p += signamlen + 1;
                        if (strcmp (signam, "state") == 0) {
                            scret = new SCRetStr (p);
                        } else {
                            int val = strtol (p, &p, 0);
                            scret = new SCRetInt (val);
                        }
                        goto gotval;
                    }
                    p = strchr (p, ';');
                    if (p == NULL) break;
                }
                char *q = buf;
                bool skip = false;
                for (char *p = buf; *p != 0; p ++) {
                    char c = *p;
                    if (c == '=') { skip = true; continue; }
                    if (c == ';') { skip = false; c = ' '; }
                    if (! skip) *(q ++) = c;
                }
                *q = 0;
                scret = new SCRetErr ("bad signal name %s - valid are %s", signam, buf);
            }
        gotval:;
            free (buf);
            goto ret;
        }

        scret = new SCRetErr ("decode <gpiogetstring> [<signalname>]");
        goto ret;
    }

    // get [a/b/c/d/g]
    if (strcmp (argv[0], "get") == 0) {
        if (! ctl_ishalted ()) {
            scret = new SCRetErr ("processor not halted");
            goto ret;
        }

        if (argc == 1) {
            pinss.gcon = gpio->readgpio ();
            if (shadow.paddles) {
                gpio->readpads (pinss.cons);
                scret = new SCRetStr ("a=0x%08X;b=0x%08X;c=0x%08X;d=0x%08X;g=0x%08X",
                    pinss.acon, pinss.bcon, pinss.ccon, pinss.dcon, pinss.gcon);
            } else {
                scret = new SCRetStr ("g=0x%08X", pinss.gcon);
            }
            goto ret;
        }
        if (argc == 2) {
            if (shadow.paddles) {
                if (strcmp (argv[1], "a")  == 0) {
                    gpio->readpads (pinss.cons);
                    scret = new SCRetLong (pinss.acon);
                    goto ret;
                }
                if (strcmp (argv[1], "b")  == 0) {
                    gpio->readpads (pinss.cons);
                    scret = new SCRetLong (pinss.bcon);
                    goto ret;
                }
                if (strcmp (argv[1], "c")  == 0) {
                    gpio->readpads (pinss.cons);
                    scret = new SCRetLong (pinss.ccon);
                    goto ret;
                }
                if (strcmp (argv[1], "d")  == 0) {
                    gpio->readpads (pinss.cons);
                    scret = new SCRetLong (pinss.dcon);
                    goto ret;
                }
            }
            if (strcmp (argv[1], "g")  == 0) {
                pinss.gcon = gpio->readgpio ();
                scret = new SCRetInt (pinss.gcon & 0x0FFFFFFF);
                goto ret;
            }
        }

        scret = new SCRetErr (shadow.paddles ? "gpio get [a/b/c/d/g]" :
            "gpio get [g] - do 'option set paddles 1' to access a/b/c/d");
        goto ret;
    }

    if (strcmp (argv[0], "help") == 0) {
        puts ("");
        puts ("valid sub-commands:");
        puts ("");
        puts ("  decode <gpiogetstring> [<signalname>] - decode get command output");
        puts ("                                          eg, whole thing: gpio decode [gpio get]");
        puts ("                                         just pc contents: gpio decode [gpio get] pcq");
        puts ("  get           - return all connector readings as a string");
        puts ("  get g         - return GPIO connector reading as a value");
        if (shadow.paddles) {
            puts ("  get [a/b/c/d] - return specific paddle connector reading as a value");
        } else {
            puts ("                  do 'option set paddles 1' to access a/b/c/d");
        }
        puts ("");
        puts ("  libname       - return library name:");
        puts ("                  csrclib - gate-level PC simulation");
        puts ("                  nohwlib - cycle-level PC simulation");
        puts ("                  physlib - tubes via RasPI GPIO connector");
        puts ("                  zynqlib - gate-level FPGA simulation");
        puts ("");
        puts ("  set g <value> - write value to gpio connector");
        puts ("                  most likely invalidates shadow state");
        puts ("                  requiring 'reset' command to restore");
        puts ("");
        printf ("connected via %s\n", gpio->libname);
        puts ("");
        return TCL_OK;
    }

    // libname
    if (strcmp (argv[0], "libname") == 0) {
        Tcl_SetResult (interp, (char *) gpio->libname, TCL_STATIC);
        return TCL_OK;
    }

    if (strcmp (argv[0], "set") == 0) {
        if (! ctl_ishalted ()) {
            scret = new SCRetErr ("processor not halted");
            goto ret;
        }
        if (argc == 3) {
            if (strcmp (argv[1], "g")  == 0) {
                char *p;
                int val = strtol (argv[2], &p, 0);
                if ((*p != 0) || (val < 0) || (val > 0x0FFFFFFF)) {
                    scret = new SCRetErr ("gcon value %s not a 28-bit number", argv[2]);
                    goto ret;
                }
                gpio->writegpio ((val & G_QENA) != 0, val);
                return TCL_OK;
            }
        }
    }

    scret = new SCRetErr ("unknown gpio command %s - valid: decode get help libname set", argv[0]);

    // process return value
ret:;
    return procscret (interp, scret);
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
    gpio->halfcycle (addcyc);
    return TCL_OK;
}

// halt processor if not already
// - tells raspictl main to suspend clocking the processor
// - halts at the end of a cycle
// returns: 0=was running, now halted; 1=was already halted
static int cmd_halt (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    bool washalted = ctl_halt ();
    Tcl_SetObjResult (interp, Tcl_NewBooleanObj ((int) washalted));
    return TCL_OK;
}

// returns reason for halt
static int cmd_haltreason (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    Tcl_SetResult (interp, (char *) haltreason, TCL_STATIC);
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

// send command to i/o device
//  iodev <devicename> <sub-command> ...
static int cmd_iodev (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    char const *devname = (objc > 1) ? Tcl_GetString (objv[1]) : NULL;
    if ((devname != NULL) && (strcmp (devname, "help") == 0)) {
        puts ("");
        puts ("access i/o device state");
        puts ("");
        puts ("  iodev <device> <sub-command> ...");
        puts ("");
        puts ("do 'alliodevs' for list of devices");
        puts ("do 'iodev <device> help' for help on that device");
        puts ("");
        return TCL_OK;
    }

    // must have at least one argument (<sub-command>) to pass to device
    if (objc < 3) {
        Tcl_SetResult (interp, (char *) "iodev <device> <sub-command> ...", TCL_STATIC);
        return TCL_ERROR;
    }

    // device name is 1st parameter
    IODev *iodev;
    for (iodev = alliodevs; iodev != NULL; iodev = iodev->nextiodev) {
        if ((iodev->iodevname != NULL) && (strcmp (iodev->iodevname, devname) == 0)) goto gotit;
    }
    Tcl_SetResultF (interp, "unknown i/o device %s - alliodevs for list", devname);
    return TCL_ERROR;

    // build array of strings for remaining parameters
gotit:;
    int argc = objc - 2;
    char const *argv[argc+1];
    for (int i = 0; i < argc; i ++) {
        argv[i] = Tcl_GetString (objv[i+2]);
    }
    argv[argc] = NULL;

    // run the command
    SCRet *scret = iodev->scriptcmd (argc, argv);

    // process return value
    return procscret (interp, scret);
}

// load a .bin format file into memory
// returns the start address (or -1 if none)
static int cmd_loadbin (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    switch (objc) {
        case 2: {
            char const *filename = Tcl_GetString (objv[1]);
            FILE *loadfile = fopen (filename, "r");
            if (loadfile == NULL) {
                Tcl_SetResult (interp, (char *) strerror (errno), TCL_STATIC);
                return TCL_ERROR;
            }
            uint16_t start;
            int rc = binloader (loadfile, &start);
            fclose (loadfile);
            if (rc != 0) return TCL_ERROR;
            Tcl_SetObjResult (interp, Tcl_NewIntObj ((start & 0x8000) ? -1 : (int) start));
            return TCL_OK;
        }
        default: {
            Tcl_SetResult (interp, (char *) "bad number of arguments", TCL_STATIC);
            return TCL_ERROR;
        }
    }
}

// load a link format file into memory
// returns the start address (or -1 if none)
static int cmd_loadlink (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    switch (objc) {
        case 2: {
            char const *filename = Tcl_GetString (objv[1]);
            FILE *loadfile = fopen (filename, "r");
            if (loadfile == NULL) {
                Tcl_SetResult (interp, (char *) strerror (errno), TCL_STATIC);
                return TCL_ERROR;
            }
            int rc = linkloader (loadfile);
            fclose (loadfile);
            if (rc < 0) return TCL_ERROR;
            Tcl_SetObjResult (interp, Tcl_NewIntObj ((rc == 0) ? -1 : rc));
            return TCL_OK;
        }
        default: {
            Tcl_SetResult (interp, (char *) "bad number of arguments", TCL_STATIC);
            return TCL_ERROR;
        }
    }
}

// load a .rim format file into memory
static int cmd_loadrim (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    switch (objc) {
        case 2: {
            char const *filename = Tcl_GetString (objv[1]);
            FILE *loadfile = fopen (filename, "r");
            if (loadfile == NULL) {
                Tcl_SetResult (interp, (char *) strerror (errno), TCL_STATIC);
                return TCL_ERROR;
            }
            int rc = rimloader (loadfile);
            fclose (loadfile);
            if (rc != 0) return TCL_ERROR;
            return TCL_OK;
        }
        default: {
            Tcl_SetResult (interp, (char *) "bad number of arguments", TCL_STATIC);
            return TCL_ERROR;
        }
    }
}

static bool cmdargsfreeable;

// get/set option state
static int cmd_option (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    char const *subcmd = (objc > 1) ? Tcl_GetString (objv[1]) : "";
    char const *opname = (objc > 2) ? Tcl_GetString (objv[2]) : "";

    // get <optionname>
    if (strcmp (subcmd, "get") == 0) {
        int retint;
        if (objc == 3) {
            if (strcmp (opname, "ctrlc")      == 0) { retint = ctrlcflag;         goto ret; }
            if (strcmp (opname, "haltstop")   == 0) { retint = haltstop;          goto ret; }
            if (strcmp (opname, "jmpdotstop") == 0) { retint = jmpdotstop;        goto ret; }
            if (strcmp (opname, "mintimes")   == 0) { retint = getmintimes ();    goto ret; }
            if (strcmp (opname, "os8zap")     == 0) { retint = os8zap;            goto ret; }
            if (strcmp (opname, "paddles")    == 0) { retint = shadow.paddles;    goto ret; }
            if (strcmp (opname, "printinstr") == 0) { retint = shadow.printinstr; goto ret; }
            if (strcmp (opname, "printstate") == 0) { retint = shadow.printstate; goto ret; }
            if (strcmp (opname, "quiet")      == 0) { retint = quiet;             goto ret; }
            if (strcmp (opname, "randmem")    == 0) { retint = randmem;           goto ret; }
            if (strcmp (opname, "skipopt")    == 0) { retint = skipopt;           goto ret; }
            if (strcmp (opname, "tubesaver")  == 0) { retint = tubesaver;         goto ret; }

            if (strcmp (opname, "cmdargs") == 0) {
                Tcl_Obj *cmdobjs[cmdargc];
                for (int i = 0; i < cmdargc; i ++) {
                    cmdobjs[i] = Tcl_NewStringObj (cmdargv[i], -1);
                }
                Tcl_Obj *cmdlist = Tcl_NewListObj (cmdargc, cmdobjs);
                Tcl_SetObjResult (interp, cmdlist);
                return TCL_OK;
            }

            if (strcmp (opname, "printfile") == 0) {
                Tcl_SetObjResult (interp, Tcl_NewStringObj (shadow.printname, -1));
                return TCL_OK;
            }

            if (strcmp (opname, "stopats") == 0) {
                Tcl_Obj *stopatobjs[numstopats];
                for (int i = 0; i < numstopats; i ++) {
                    stopatobjs[i] = Tcl_NewIntObj (stopats[i]);
                }
                Tcl_Obj *stopatlist = Tcl_NewListObj (numstopats, stopatobjs);
                Tcl_SetObjResult (interp, stopatlist);
                return TCL_OK;
            }

            if (strcmp (opname, "watchwrite") == 0) {
                Tcl_SetObjResult (interp, Tcl_NewIntObj (watchwrite));
                return TCL_OK;
            }
        }

        Tcl_SetResult (interp, (char *) "option get <optionname>", TCL_STATIC);
        return TCL_ERROR;

    ret:;
        Tcl_SetObjResult (interp, Tcl_NewBooleanObj (retint));
        return TCL_OK;
    }

    if ((subcmd[0] == 0) || (strcmp (subcmd, "help") == 0)) {
        puts ("");
        puts ("  get <optionname>");
        puts ("  set <optionname> <value>");
        puts ("");
        puts ("valid options:");
        puts ("");
        puts ("  cmdargs    - raspictl command line args after script filename");
        puts ("  ctrlc      - control-C has been pressed; must be reset or raspictl will abort");
        puts ("  haltstop   - stop on HLT (else wait for interrupt)");
        puts ("  jmpdotstop - stop on JMP .");
        puts ("  mintimes   - print cycle count every minute");
        puts ("  os8zap     - zap the os8 delay loop");
        puts ("  paddles    - verify paddles at end of each cycle");
        puts ("  printfile  - specify file (or - for stdout) for printinstr, printstate");
        puts ("  printinstr - print each instruction executed");
        puts ("  printstate - print each cycle executed");
        puts ("  quiet      - don't print illegal instruction messages");
        puts ("  randmem    - provide random memory contents");
        puts ("  skipopt    - optimize ioskip/jmp.-1 to blocking");
        puts ("  stopats    - stop when accessing any of the memory addresses");
        puts ("  tubesaver  - run random opcodes during halt/skipopt time");
        puts ("  watchwrite - stop when writing to the memory address (-1 to disable)");
        puts ("");
        return (subcmd[0] == 0) ? TCL_ERROR : TCL_OK;
    }

    // set <optionname> <value>
    if (strcmp (subcmd, "set") == 0) {
        if (strcmp (opname, "cmdargs") == 0) {

            // make a new vector from command arguments
            char **cmdstrs = (char **) malloc ((objc - 2) * sizeof *cmdstrs);
            for (int i = 0; i < objc - 3; i ++) {
                cmdstrs[i] = strdup (Tcl_GetString (objv[i+3]));
            }
            cmdstrs[objc-3] = NULL;

            // maybe free off old arguments
            if (cmdargsfreeable) {
                for (int i = 0; i < cmdargc; i ++) free (cmdargv[i]);
                free (cmdargv);
            }

            // set up new command arg list
            cmdargc = objc - 3;
            cmdargv = cmdstrs;
            cmdargsfreeable = true;

            return TCL_OK;
        }

        if (strcmp (opname, "stopats") == 0) {

            numstopats = 0;
            for (int i = 3; i < objc; i ++) {
                if (numstopats >= MAXSTOPATS) {
                    Tcl_SetResult (interp, (char *) "too many stopat addresses", TCL_STATIC);
                    return TCL_ERROR;
                }
                int addr;
                int rc = Tcl_GetIntFromObj (interp, objv[i], &addr);
                if (rc != TCL_OK) return rc;
                if ((addr < 0) || (addr > 077777)) {
                    Tcl_SetResultF (interp, "stopat address 0%o not in range 0..077777", addr);
                    return TCL_ERROR;
                }
                stopats[numstopats++] = addr;
            }

            return TCL_OK;
        }

        bool *boolptr;
        if (objc == 4) {
            if (strcmp (opname, "ctrlc")      == 0) { boolptr = &ctrlcflag;  goto setbool; }
            if (strcmp (opname, "haltstop")   == 0) { boolptr = &haltstop;   goto setbool; }
            if (strcmp (opname, "jmpdotstop") == 0) { boolptr = &jmpdotstop; goto setbool; }
            if (strcmp (opname, "os8zap")     == 0) { boolptr = &os8zap;     goto setbool; }
            if (strcmp (opname, "printinstr") == 0) { boolptr = &shadow.printinstr; goto setbool; }
            if (strcmp (opname, "printstate") == 0) { boolptr = &shadow.printstate; goto setbool; }
            if (strcmp (opname, "quiet")      == 0) { boolptr = &quiet;      goto setbool; }
            if (strcmp (opname, "randmem")    == 0) { boolptr = &randmem;    goto setbool; }
            if (strcmp (opname, "skipopt")    == 0) { boolptr = &skipopt;    goto setbool; }
            if (strcmp (opname, "tubesaver")  == 0) { boolptr = &tubesaver;  goto setbool; }

            if (strcmp (opname, "mintimes") == 0) {
                int val;
                int rc = Tcl_GetBooleanFromObj (interp, objv[3], &val);
                if (rc == TCL_OK) setmintimes (val != 0);
                return rc;
            }

            if (strcmp (opname, "paddles") == 0) {
                int val;
                int rc = Tcl_GetBooleanFromObj (interp, objv[3], &val);
                if (rc == TCL_OK) {
                    // make sure paddles present before enabling
                    if (val && ! gpio->haspads ()) {
                        Tcl_SetResult (interp, (char *) "paddles not present", TCL_STATIC);
                        return TCL_ERROR;
                    }
                    shadow.paddles = val;
                }
                return rc;
            }

            if (strcmp (opname, "printfile") == 0) {
                char const *name = Tcl_GetString (objv[3]);
                FILE *file = stdout;
                if (strcmp (name, "-") != 0) {
                    file = fopen (name, "w");
                    if (file == NULL) {
                        Tcl_SetResultF (interp, "%m");
                        return TCL_ERROR;
                    }
                }
                if (shadow.printfile != stdout) {
                    fclose (shadow.printfile);
                }
                shadow.printname = name;
                shadow.printfile = file;
                return TCL_OK;
            }


            if (strcmp (opname, "watchwrite") == 0) {
                int val;
                int rc = Tcl_GetIntFromObj (interp, objv[3], &val);
                if (rc == TCL_OK) {
                    if ((val < -1) || (val > 077777)) {
                        Tcl_SetResultF (interp, "watchwrite address 0%o not in range -1..077777", val);
                        return TCL_ERROR;
                    }
                    watchwrite = val;
                }
                return rc;
            }
        }

        Tcl_SetResult (interp, (char *) "option set <optionname> <value>", TCL_STATIC);
        return TCL_ERROR;

    setbool:;
        int val;
        int rc = Tcl_GetBooleanFromObj (interp, objv[3], &val);
        if (rc == TCL_OK) *boolptr = (val != 0);
        return rc;
    }

    Tcl_SetResult (interp, (char *) "bad number of arguments", TCL_STATIC);
    return TCL_ERROR;
}

// read memory location
static int cmd_readmem (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    switch (objc) {
        case 2: {
            int addr;
            int rc = Tcl_GetIntFromObj (interp, objv[1], &addr);
            if (rc == TCL_OK) {
                uint16_t data = memarray[addr&(MEMSIZE-1)];
                Tcl_SetObjResult (interp, Tcl_NewIntObj (data));
            }
            return rc;
        }
        default: {
            Tcl_SetResult (interp, (char *) "bad number of arguments", TCL_STATIC);
            return TCL_ERROR;
        }
    }
}

// halt then reset processor, optionally setting start address
static int cmd_reset (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    int addr;
    switch (objc) {
        case 1: {
            addr = 0;
            break;
        }
        case 2: {
            char const *arg1str = Tcl_GetString (objv[1]);
            if (strcmp (arg1str, "help") == 0) {
                puts ("");
                puts ("  reset         - reset processor, set if.pc=0.0000");
                puts ("  reset <value> - reset processor, set if.pc=15-bit value");
                puts ("");
                return TCL_OK;
            }
            int rc = Tcl_GetIntFromObj (interp, objv[1], &addr);
            if (rc != TCL_OK) return rc;
            addr &= 077777;
            break;
        }
        default: {
            Tcl_SetResult (interp, (char *) "bad number of arguments", TCL_STATIC);
            return TCL_ERROR;
        }
    }

    ctl_halt ();
    if (! ctl_reset (addr)) {
        Tcl_SetResult (interp, (char *) "failed to reset", TCL_STATIC);
        return TCL_ERROR;
    }
    return TCL_OK;
}

// tell processor to start running
// - tells raspictl main to resume clocking the processor
static int cmd_run (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    if (! ctl_run ()) {
        Tcl_SetResult (interp, (char *) "not halted", TCL_STATIC);
        return TCL_ERROR;
    }
    return TCL_OK;
}

// tell processor to step one cycle
// - tells raspictl main to clock the processor a single cycle
// - halts at the end of the next cycle
static int cmd_stepcyc (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    if (! ctl_stepcyc ()) {
        Tcl_SetResult (interp, (char *) "not halted", TCL_STATIC);
        return TCL_ERROR;
    }
    return TCL_OK;
}

// tell processor to step one instruction
// - tells raspictl main to clock the processor a single cycle
// - halts at the end of the next FETCH2 or INTAK1 cycle
static int cmd_stepins (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    if (! ctl_stepins ()) {
        Tcl_SetResult (interp, (char *) "not halted", TCL_STATIC);
        return TCL_ERROR;
    }
    return TCL_OK;
}

// access switch register
//  swreg = return switch register value
//  swreg <value> = set switch register value
static int cmd_swreg (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    switch (objc) {
        case 1: {
            Tcl_SetObjResult (interp, Tcl_NewIntObj (switchregister));
            return TCL_OK;
        }
        case 2: {
            char const *arg1str = Tcl_GetString (objv[1]);
            if (strcmp (arg1str, "help") == 0) {
                puts ("");
                puts ("  swreg         - return switch register as a value");
                puts ("  swreg <value> - set switch register to value");
                puts ("");
                return TCL_OK;
            }
            int data;
            int rc = Tcl_GetIntFromObj (interp, objv[1], &data);
            if (rc == TCL_OK) switchregister = data & 077777;
            return rc;
        }
        default: {
            Tcl_SetResult (interp, (char *) "bad number of arguments", TCL_STATIC);
            return TCL_ERROR;
        }
    }
}

// wait for halt (control-C, haltstop, stopat, etc)
// returns immediately if already halted
static int cmd_wait (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    ctl_wait ();
    return TCL_OK;
}

// write memory location
static int cmd_writemem (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    switch (objc) {
        case 3: {
            int addr, data;
            int rc = Tcl_GetIntFromObj (interp, objv[1], &addr);
            if (rc == TCL_OK) rc = Tcl_GetIntFromObj (interp, objv[2], &data);
            if (rc == TCL_OK) {
                memarray[addr&(MEMSIZE-1)] = data & 07777;
            }
            return rc;
        }
        default: {
            Tcl_SetResult (interp, (char *) "bad number of arguments", TCL_STATIC);
            return TCL_ERROR;
        }
    }
}

/////////////////
//  Utilities  //
/////////////////

static int cmpdevnames (void const *v, void const *w)
{
    return strcmp (*(char const **)v, *(char const **)w);
}

// process return value for cpu or iodev command
// converts it to corresponding TCL value
static int procscret (Tcl_Interp *interp, SCRet *scret)
{
    if (scret == NULL) return TCL_OK;
    switch (scret->gettype ()) {
        case SCRet::SCRT_ERR: {
            Tcl_SetResult (interp, scret->casterr ()->msg, (void (*) (char *)) free);
            delete scret;
            return TCL_ERROR;
        }
        case SCRet::SCRT_INT: {
            Tcl_SetObjResult (interp, Tcl_NewIntObj (scret->castint ()->val));
            break;
        }
        case SCRet::SCRT_LIST: {
            SCRetList *scretlist = scret->castlist ();
            int nelems = scretlist->nelems;
            Tcl_Obj *elemobjs[nelems];
            for (int i = 0; i < nelems; i ++) {
                SCRetStr *scretstr = scretlist->elems[i]->caststr ();
                ASSERT (scretstr != NULL);
                elemobjs[i] = Tcl_NewStringObj (scretstr->str, -1);
            }
            Tcl_SetObjResult (interp, Tcl_NewListObj (nelems, elemobjs));
            break;
        }
        case SCRet::SCRT_LONG: {
            Tcl_SetObjResult (interp, Tcl_NewLongObj (scret->castlong ()->val));
            break;
        }
        case SCRet::SCRT_STR: {
            Tcl_SetObjResult (interp, Tcl_NewStringObj (scret->caststr ()->str, -1));
            break;
        }
        default: ABORT ();
    }
    delete scret;
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

///////////////////////////////////////////////
//  I/O DEVICE SCRIPT COMMAND RETURN VALUES  //
///////////////////////////////////////////////

SCRetErr::SCRetErr (char const *fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);
    char *buf = NULL;
    if (vasprintf (&buf, fmt, ap) < 0) ABORT ();
    va_end (ap);
    this->msg = buf;
}

SCRetInt::SCRetInt (int val)
{
    this->val = val;
}

SCRetList::SCRetList (int nelems)
{
    this->nelems = nelems;
    if (nelems > 0) {
        this->elems = (SCRet **) calloc (nelems, sizeof *this->elems);
        if (this->elems == NULL) ABORT ();
    } else {
        this->elems = NULL;
    }
}

SCRetList::~SCRetList ()
{
    if (this->elems != NULL) {
        for (int i = 0; i < this->nelems; i ++) {
            if (this->elems[i] != NULL) {
                delete this->elems[i];
                this->elems[i] = NULL;
            }
        }
        free (this->elems);
        this->elems = NULL;
    }
}

SCRetLong::SCRetLong (long val)
{
    this->val = val;
}

SCRetStr::SCRetStr (char const *fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);
    char *buf = NULL;
    if (vasprintf (&buf, fmt, ap) < 0) ABORT ();
    va_end (ap);
    this->str = buf;
}

SCRetStr::~SCRetStr ()
{
    free (this->str);
    this->str = NULL;
}
