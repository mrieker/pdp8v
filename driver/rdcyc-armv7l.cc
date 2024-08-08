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

#define HZ2835 1400000000       // raspi rdcyc frequency
#define HZ2711 1500000000       // raspi rdcyc frequency

#include "rdcyc.h"

static double rasphz;
static int numcores = -1;
static pthread_attr_t allcpuattr;

void rdcycinit ()
{
    if (numcores < 0) {
        char infoline[256];
        FILE *infofile = fopen ("/proc/cpuinfo", "r");
        if (infofile == NULL) {
            fprintf (stderr, "error opening /proc/cpuinfo: %m\n");
            ABORT ();
        }
        double bogomips = 0.0;
        while (fgets (infoline, sizeof infoline, infofile) != NULL) {
            if (memcmp (infoline, "processor\t:", 11) == 0) {
                int n = atoi (infoline + 11) + 1;
                if (numcores < n) numcores = n;
            }
            if (memcmp (infoline, "BogoMIPS\t:", 10) == 0) {
                bogomips = strtod (infoline + 10, NULL);
            }
            if ((strstr (infoline, "Hardware") != NULL) && (strstr (infoline, "BCM2835") != NULL)) {
                rasphz = HZ2835;
            }
            if ((strstr (infoline, "Hardware") != NULL) && (strstr (infoline, "BCM2711") != NULL)) {
                rasphz = HZ2711;
            }
            if ((strstr (infoline, "Hardware") != NULL) && (strstr (infoline, "Zynq") != NULL)) {
                rasphz = bogomips * 1000000;
            }
        }
        fclose (infofile);
        fprintf (stderr, "rdcycinit: numcores %d; rasphz %0.0f\n", numcores, rasphz);
        if ((numcores <= 0) || (rasphz == 0.0)) ABORT ();
        ASSERT ((numcores == 1) || ! UNIPROC);

        // all created threads can run on any cpu
        // only the main thread what calls rdcyc() gets locked to one cpu
        if (pthread_attr_init (&allcpuattr) != 0) ABORT ();
        if (numcores > 1) {
            cpu_set_t cpuset;
            CPU_ZERO (&cpuset);
            for (int i = 0; i < numcores; i ++) CPU_SET (i, &cpuset);
            if (pthread_attr_setaffinity_np (&allcpuattr, sizeof cpuset, &cpuset) != 0) ABORT ();
        }
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

double rdcycfreq ()
{
    return rasphz;
}

// read raspi cycle counter
uint32_t rdcyc (void)
{
#ifdef __aarch64__
    uint64_t cyc;
    asm volatile ("mrs %0,cntvct_e10" : "=r" (cyc));
    return cyc;
#endif
#ifdef __ARM_ARCH
    uint32_t cyc;
    asm volatile ("mrc p15,0,%0,c9,c13,0" : "=r" (cyc));
    return cyc;
#endif
}

// create thread to run on any cpu
int createthread (pthread_t *tid, void *(*entry) (void *param), void *param)
{
    return pthread_create (tid, (numcores <= 1) ? NULL : &allcpuattr, entry, param);
}
