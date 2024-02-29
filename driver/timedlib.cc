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

// provide common addhz/cpuhz processing

#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "gpiolib.h"
#include "rdcyc.h"

void TimedLib::open ()
{
    // initialize rdcyc()
    rdcycinit ();
    double rasphz = rdcycfreq ();

    // get frequency to clock tubes at
    char const *cpuhzenv = getenv ("cpuhz");
    uint32_t cpuhz = ((cpuhzenv == NULL) || (cpuhzenv[0] == 0)) ? DEFCPUHZ : atoi (cpuhzenv);

    memset (&hafcycts, 0, sizeof hafcycts);
    if (cpuhz > 0) hafcycts.tv_nsec = 500000000 / cpuhz;
    fprintf (stderr, "TimedLib::open: cpuhz=%u hafcycns=%u\n", cpuhz, (uint32_t) hafcycts.tv_nsec);

    // 0 means run as fast as raspi can go (eg, connected to fpga)
    if (cpuhz > 0) {
        hafcyclo = hafcychi = rasphz / cpuhz / 2;
        char const *addhzenv = getenv ("addhz");
        int addhz = ((addhzenv != NULL) && (addhzenv[0] != 0)) ? atoi (addhzenv) : DEFADDHZ;
        if (addhz > 0) {
            fprintf (stderr, "TimedLib::open: addhz=%d\n", addhz);
            hafcychi = rasphz / addhz / 2;
        }
    } else {
        hafcychi = 0;
        hafcyclo = 0;
    }
    fprintf (stderr, "TimedLib::open: hafcychi=%u hafcyclo=%u\n", hafcychi, hafcyclo);
}

void TimedLib::halfcycle (bool aluadd)
{
    if (gpiovalu & G_RESET) {
        struct timespec tenthsec;
        memset (&tenthsec, 0, sizeof tenthsec);
        tenthsec.tv_nsec = 100000000;
        if (nanosleep (&tenthsec, NULL) < 0) ABORT ();
    } else if (hafcycts.tv_nsec >= 1000000) {
        if (nanosleep (&hafcycts, NULL) < 0) ABORT ();
    } else {
        uint32_t start = rdcyc ();
        uint32_t delta = aluadd ? hafcychi : hafcyclo;
        while (rdcyc () - start < delta) { }
    }
}
