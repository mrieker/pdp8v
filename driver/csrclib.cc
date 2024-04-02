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

// uses C code generated from netgen -csource
// ...to provide for simulation

// module-specific code is loaded from .so files generated via make:
//  make csrcmod_<modname>.so.$(MACH)
//   <modname> = proc : whole processor (ACL + ALU + MA + PC + RPI + SEQ boards) accessed via GPIO connector
//           proc_seq : just SEQ board + RPI board to hook up to GPIO connector
//            seqcirc : just SEQ board, no GPIO simulation, simulates A,B,C,D paddles for testing

#include <dirent.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "csrcmod.h"
#include "gpiolib.h"
#include "makepipelibmod.h"

// construct the library object
CSrcLib::CSrcLib (char const *modname)
{
    libname = "csrclib";

    latestwdata = 0;
    latestwvalu = 0;

    // get executable directory
    char linkbuf[4000];
    char *mach = getexedir (linkbuf, sizeof linkbuf);

    // make sure <modname> is included in ../modules/whole.mod so netgen can generate the csource file
    // this also switches "rpi" to "rpi_testpads"
    modname = makepipelibmod (modname);

    // get .so loaded that contains generated simulation code derived from the module files
    // name is <this-executable-directory>/csrcmod_<modname>.so.<this-executable-machname>
    std::string soname;
    soname.append (linkbuf);
    soname.append ("csrcmod_");
    soname.append (modname);
    soname.append (".so");
    if (mach != NULL) {
        soname.append (mach);
    }

    // if .so file doesn't exist, try to make it
    char const *sonamestr = soname.c_str ();
    if (access (sonamestr, R_OK) < 0) {
        if (errno != ENOENT) {
            fprintf (stderr, "CSrcLib::CSrcLib: error accessing %s: %m\n", sonamestr);
            ABORT ();
        }
        std::string makecmd;
        makecmd.append ("cd ");
        makecmd.append (linkbuf);
        makecmd.append (" ; make csrcmod_");
        makecmd.append (modname);
        makecmd.append (".so");
        if (mach != NULL) {
            makecmd.append (mach);
        }
        fprintf (stderr, "CSrcLib::CSrcLib: %s\n", makecmd.c_str ());
        int rc = system (makecmd.c_str ());
        if (rc != 0) fprintf (stderr, "CSrcLib::CSrcLib: - make status %d, errno %d\n", rc, errno);
    }

    // now try to open it
    modhand = dlopen (sonamestr, RTLD_NOW);
    if (modhand == NULL) {
        fprintf (stderr, "CSrcLib::CSrcLib: error opening %s\n", sonamestr);
        ABORT ();
    }

    // look for CSrcMod_<modname>_ctor() function that instantiates the module
    std::string ctorname;
    ctorname.append ("CSrcMod_");
    ctorname.append (modname);
    ctorname.append ("_ctor");

    CSrcMod *(*ctorfunc) () = (CSrcMod *(*) ()) dlsym (modhand, ctorname.c_str ());
    if (ctorfunc == NULL) {
        fprintf (stderr, "CSrcLib::CSrcLib: file %s does not define %s\n", soname.c_str (), ctorname.c_str ());
        ABORT ();
    }

    // create the module instance
    module = (*ctorfunc) ();
}

CSrcLib::~CSrcLib ()
{
    if (modhand != NULL) {
        dlclose (modhand);
        modhand = NULL;
    }
    if (module != NULL) {
        delete module;
        module = NULL;
    }
}

void CSrcLib::open ()
{
    fprintf (stderr, "CSrcLib::open: module=%s\n", module->modname);
    writecount = 0;
    writeloops = 0;
    maxloops = 0;
}

void CSrcLib::close ()
{
    if (writecount > 0) {
        fprintf (stderr, "CSrcLib::close: writecount=%llu avgloopsperwrite=%3.1f\n", (LLU) writecount, ((double) writeloops / (double) writecount));
    }
}

void CSrcLib::halfcycle ()
{
    bool backing[module->boolcount];
    uint32_t nloops = 0;

    writecount ++;

    // clock in the new state
    module->stepstatework ();

    do {
        writeloops ++;
        nloops ++;

        // save a copy of the booleans
        memcpy (backing, module->boolarray, sizeof backing);

        // step state again
        module->stepstatework ();

        // it should stabilize
    } while (memcmp (backing, module->boolarray, sizeof backing) != 0);

    pthread_mutex_lock (&trismutex);
    numtrisoff += module->nto;
    ntotaltris += module->ntt;
    pthread_mutex_unlock (&trismutex);

    if (maxloops < nloops) {
        maxloops = nloops;
        fprintf (stderr, "CSrcLib::halfcycle: nloops=%u\n", nloops);
    }
}

// read GPIO pins
uint32_t CSrcLib::readgpio ()
{
    uint32_t valu = module->readgpiowork ();
    uint32_t mask = latestwdata ? (G_LINK | G_DATA | G_OUTS) : G_OUTS;
    return (valu & ~ mask) | (latestwvalu & mask);
}

// write GPIO pins
void CSrcLib::writegpio (bool wdata, uint32_t valu)
{
    valu = (valu & ~ (G_QENA | G_DENA)) | (wdata ? G_QENA : G_DENA);

    latestwdata = wdata;
    latestwvalu = valu;

    // update pins such as the clock
    module->writegpiowork (valu);
}

// we have paddles
bool CSrcLib::haspads ()
{
    return true;
}

// read A,B,C,D connectors
// pin 01 of each is grounded so make sure we return 0
void CSrcLib::readpads (uint32_t *pinss)
{
    pinss[0] = module->readaconwork () & ~1U;
    pinss[1] = module->readbconwork () & ~1U;
    pinss[2] = module->readcconwork () & ~1U;
    pinss[3] = module->readdconwork () & ~1U;
}

// write A,B,C,D connectors
void CSrcLib::writepads (uint32_t const *masks, uint32_t const *pinss)
{
    module->writeaconwork (pinss[0]);
    module->writebconwork (pinss[1]);
    module->writecconwork (pinss[2]);
    module->writedconwork (pinss[3]);
}

int CSrcLib::examine (char const *varname, uint32_t *value)
{
    CSrcMod::Var const *var = module->findvar (varname, strlen (varname));
    if (var == NULL) return 0;
    uint32_t retval = 0;
    for (int i = var->hidim - var->lodim; i >= 0; -- i) {
        int bai = var->indxs[i];
        retval += retval + ((bai < 0) ? ~ bai : (module->boolarray[bai] ? 1 : 0));
    }
    *value = retval;
    return var->hidim - var->lodim + 1;
}

bool *CSrcLib::getvarbool (char const *varname, int rbit)
{
    CSrcMod::Var const *var = module->findvar (varname, strlen (varname));
    if (var == NULL) return NULL;
    if ((rbit < 0) || (rbit > var->hidim - var->lodim)) return NULL;
    int bai = var->indxs[rbit];
    return &module->boolarray[bai];
}
