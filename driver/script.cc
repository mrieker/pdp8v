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

#include "binloader.h"
#include "controls.h"
#include "iodevs.h"
#include "linkloader.h"
#include "memext.h"
#include "memory.h"
#include "miscdefs.h"
#include "readprompt.h"
#include "rimloader.h"
#include "script.h"
#include "shadow.h"

static Tcl_Interp *interp;

static void *tclthread (void *fnv);
static int cmpdevnames (void const *v, void const *w);
static int procscret (Tcl_Interp *interp, SCRet *scret);

void runscript (char const *argv0, char const *filename)
{
    Tcl_FindExecutable (argv0);
    pthread_t tclpid;
    int rc = pthread_create (&tclpid, NULL, tclthread, (void *) filename);
    if (rc != 0) ABORT ();
    haltflags = HF_HALTIT;
}

static Tcl_ObjCmdProc cmd_alliodevs;
static Tcl_ObjCmdProc cmd_cpu;
static Tcl_ObjCmdProc cmd_halt;
static Tcl_ObjCmdProc cmd_iodev;
static Tcl_ObjCmdProc cmd_loadbin;
static Tcl_ObjCmdProc cmd_loadlink;
static Tcl_ObjCmdProc cmd_loadrim;
static Tcl_ObjCmdProc cmd_randmem;
static Tcl_ObjCmdProc cmd_readmem;
static Tcl_ObjCmdProc cmd_reset;
static Tcl_ObjCmdProc cmd_run;
static Tcl_ObjCmdProc cmd_stepcyc;
static Tcl_ObjCmdProc cmd_stepins;
static Tcl_ObjCmdProc cmd_wait;
static Tcl_ObjCmdProc cmd_writemem;

static void *tclthread (void *fnv)
{
    char const *fn = (char const *) fnv;

    interp = Tcl_CreateInterp ();
    if (Tcl_Init (interp) != TCL_OK) ABORT ();

    // https://www.tcl-lang.org/man/tcl/TclLib/CrtObjCmd.htm

    if (Tcl_CreateObjCommand (interp, "alliodevs", cmd_alliodevs, NULL, NULL) == NULL) ABORT ();
    if (Tcl_CreateObjCommand (interp, "cpu",       cmd_cpu,       NULL, NULL) == NULL) ABORT ();
    if (Tcl_CreateObjCommand (interp, "halt",      cmd_halt,      NULL, NULL) == NULL) ABORT ();
    if (Tcl_CreateObjCommand (interp, "iodev",     cmd_iodev,     NULL, NULL) == NULL) ABORT ();
    if (Tcl_CreateObjCommand (interp, "loadbin",   cmd_loadbin,   NULL, NULL) == NULL) ABORT ();
    if (Tcl_CreateObjCommand (interp, "loadlink",  cmd_loadlink,  NULL, NULL) == NULL) ABORT ();
    if (Tcl_CreateObjCommand (interp, "loadrim",   cmd_loadrim,   NULL, NULL) == NULL) ABORT ();
    if (Tcl_CreateObjCommand (interp, "readmem",   cmd_readmem,   NULL, NULL) == NULL) ABORT ();
    if (Tcl_CreateObjCommand (interp, "randmem",   cmd_randmem,   NULL, NULL) == NULL) ABORT ();
    if (Tcl_CreateObjCommand (interp, "reset",     cmd_reset,     NULL, NULL) == NULL) ABORT ();
    if (Tcl_CreateObjCommand (interp, "run",       cmd_run,       NULL, NULL) == NULL) ABORT ();
    if (Tcl_CreateObjCommand (interp, "stepcyc",   cmd_stepcyc,   NULL, NULL) == NULL) ABORT ();
    if (Tcl_CreateObjCommand (interp, "stepins",   cmd_stepins,   NULL, NULL) == NULL) ABORT ();
    if (Tcl_CreateObjCommand (interp, "wait",      cmd_wait,      NULL, NULL) == NULL) ABORT ();
    if (Tcl_CreateObjCommand (interp, "writemem",  cmd_writemem,  NULL, NULL) == NULL) ABORT ();

    ctl_halt ();    // wait for raspictl main initialization to complete

    if (strcmp (fn, "-") != 0) {
        int rc = Tcl_EvalFile (interp, fn);
        if (rc != TCL_OK) {
            char const *res = Tcl_GetStringResult (interp);
            if ((res == NULL) || (res[0] == 0)) fprintf (stderr, "script: error %d evaluating script %s\n", rc, fn);
                                  else fprintf (stderr, "script: error %d evaluating script %s: %s\n", rc, fn, res);
            ABORT ();
        }
    }

    for (char const *line; (line = readprompt ("raspictl> ")) != NULL;) {
        int rc = Tcl_EvalEx (interp, line, -1, TCL_EVAL_GLOBAL);
        char const *res = Tcl_GetStringResult (interp);
        if (rc != TCL_OK) {
            if ((res == NULL) || (res[0] == 0)) fprintf (stderr, "script: error %d evaluating command\n", rc);
                                  else fprintf (stderr, "script: error %d evaluating command: %s\n", rc, res);
        }
        else if ((res != NULL) && (res[0] != 0)) fprintf (stderr, "%s\n", res);
    }

    Tcl_Finalize ();
    exit (0);

    return NULL;
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

static int cmpdevnames (void const *v, void const *w)
{
    return strcmp (*(char const **)v, *(char const **)w);
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
        case SCRet::SCRT_STR: {
            Tcl_SetObjResult (interp, Tcl_NewStringObj (scret->caststr ()->str, -1));
            break;
        }
        default: ABORT ();
    }
    delete scret;
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

// send command to i/o device
//  iodev <devicename> <command> ...
static int cmd_iodev (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    // must have at least one argument (<command>) to pass to device
    if (objc < 3) {
        Tcl_SetResult (interp, (char *) "missing i/o device name and sub-command", TCL_STATIC);
        return TCL_ERROR;
    }

    // device name is 1st parameter
    char const *devname = Tcl_GetString (objv[1]);
    IODev *iodev;
    for (iodev = alliodevs; iodev != NULL; iodev = iodev->nextiodev) {
        if ((iodev->iodevname != NULL) && (strcmp (iodev->iodevname, devname) == 0)) goto gotit;
    }
    {
        SCRetErr dnerr("unknown i/o device %s", devname);
        Tcl_SetResult (interp, dnerr.msg, (void (*) (char *)) free);
        return TCL_ERROR;
    }

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
            int rc = binloader (filename, loadfile, &start);
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
            int rc = linkloader (filename, loadfile);
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
            int rc = rimloader (filename, loadfile);
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

// set/clear/read randmem state
static int cmd_randmem (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    switch (objc) {
        case 1: {
            Tcl_SetObjResult (interp, Tcl_NewBooleanObj ((int) randmem));
            return TCL_OK;
        }
        case 2: {
            int val;
            int rc = Tcl_GetBooleanFromObj (interp, objv[1], &val);
            if (rc == TCL_OK) {
                randmem = val != 0;
            }
            return rc;
        }
        default: {
            Tcl_SetResult (interp, (char *) "bad number of arguments", TCL_STATIC);
            return TCL_ERROR;
        }
    }
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
            addr = -1;
            break;
        }
        case 2: {
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
        Tcl_SetResult (interp, (char *) "failed to halt", TCL_STATIC);
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
// - halts at the end of the next FETCH2 cycle
static int cmd_stepins (ClientData clientdata, Tcl_Interp *interp, int objc, Tcl_Obj *const objv[])
{
    if (! ctl_stepins ()) {
        Tcl_SetResult (interp, (char *) "not halted", TCL_STATIC);
        return TCL_ERROR;
    }
    return TCL_OK;
}

// wait for control-C (raspictl halts processing on SIGINT when scripting)
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
