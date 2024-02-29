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

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "rdcyc.h"

static double rasphz;
static int numcores = -1;

void rdcycinit ()
{
    if (numcores < 0) {
        char infoline[256];
        FILE *infofile = fopen ("/proc/cpuinfo", "r");
        if (infofile == NULL) {
            fprintf (stderr, "error opening /proc/cpuinfo: %m\n");
            ABORT ();
        }
        while (fgets (infoline, sizeof infoline, infofile) != NULL) {
            if (memcmp (infoline, "processor\t:", 11) == 0) {
                int n = atoi (infoline + 11) + 1;
                if (numcores < n) numcores = n;
            }
            if (strstr (infoline, "cpu MHz") != NULL) {
                char const *colon = strchr (infoline, ':');
                if (colon != NULL) {
                    rasphz = strtod (colon + 1, NULL) * 1000000.0;
                }
            }
        }
        fclose (infofile);
        fprintf (stderr, "rdcycinit: numcores %d; rasphz %0.0f\n", numcores, rasphz);
        if ((numcores <= 0) || (rasphz == 0.0)) ABORT ();
        ASSERT ((numcores == 1) || ! UNIPROC);
    }

    // stay on one core so rdcyc() will work
    if (numcores > 1) {
        cpu_set_t cpuset;
        CPU_ZERO (&cpuset);
        CPU_SET (1, &cpuset);
        if (sched_setaffinity (0, sizeof cpuset, &cpuset) < 0) {
            ABORT ();
        }
    }
}

void rdcycuninit ()
{
    // not main thread, any core will do
    if (numcores > 1) {
        cpu_set_t cpuset;
        CPU_ZERO (&cpuset);
        for (int i = numcores; -- i >= 0;) {
            CPU_SET (i, &cpuset);
        }
        if (sched_setaffinity (0, sizeof cpuset, &cpuset) < 0) {
            ABORT ();
        }
    }
}

double rdcycfreq ()
{
    return rasphz;
}

// read raspi cycle counter
uint32_t rdcyc (void)
{
    uint32_t cyc;
    asm volatile ("rdtsc" : "=a" (cyc) :: "edx");
    return cyc;
}
