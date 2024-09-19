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
// fastest simulation that verifies module files

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "csrcmod.h"
#include "gpiolib.h"

// construct the library object
CSrcLib::CSrcLib (char const *modname)
{
    libname = "csrclib";

    latestwdata = 0;
    latestwvalu = 0;

    uint32_t boardena = ((modname == NULL) || (strcmp (modname, "proc") == 0)) ? BE_all :
        (((strstr (modname, "acl") != NULL) ? BE_acl : 0) |
         ((strstr (modname, "alu") != NULL) ? BE_alu : 0) |
         ((strstr (modname, "ma")  != NULL) ? BE_ma  : 0) |
         ((strstr (modname, "pc")  != NULL) ? BE_pc  : 0) |
         ((strstr (modname, "rpi") != NULL) ? BE_rpi : 0) |
         ((strstr (modname, "seq") != NULL) ? BE_seq : 0));

    // create the module instance
    module = CSrcMod_proc_ctor ();
    module->boardena = boardena;
}

CSrcLib::~CSrcLib ()
{
    if (module != NULL) {
        delete module;
        module = NULL;
    }
}

void CSrcLib::open ()
{
    char mods[24] = "";
    if (module->boardena & BE_acl) strcat (mods, " acl");
    if (module->boardena & BE_alu) strcat (mods, " alu");
    if (module->boardena & BE_ma)  strcat (mods, " ma");
    if (module->boardena & BE_pc)  strcat (mods, " pc");
    if (module->boardena & BE_rpi) strcat (mods, " rpi");
    if (module->boardena & BE_seq) strcat (mods, " seq");
    fprintf (stderr, "CSrcLib::open: modules%s\n", mods);

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

void CSrcLib::halfcycle (bool aluadd, bool topoloop)
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
    module->writeaconwork (masks[0], pinss[0]);
    module->writebconwork (masks[1], pinss[1]);
    module->writecconwork (masks[2], pinss[2]);
    module->writedconwork (masks[3], pinss[3]);
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
