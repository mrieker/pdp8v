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

#include <fcntl.h>
#include <stdio.h>
#include <sys/mman.h>
#include <time.h>

#include "rdcyc.h"

static uint32_t const *ctrpt;

void rdcycinit ()
{
    ASSERT (UNIPROC);
    if (ctrpt == NULL) {
        int memfd = open ("/proc/armmhz", O_RDONLY);
        if (memfd < 0) {
            fprintf (stderr, "error opening /proc/armmhz: %m\n");
            ABORT ();
        }
        void *mempt = mmap (NULL, 4096, PROT_READ, MAP_SHARED, memfd, 0);
        if (mempt == MAP_FAILED) {
            fprintf (stderr, "error mapping /proc/armmhz: %m\n");
            ABORT ();
        }
        ctrpt = &((uint32_t *)mempt)[1];
    }
}

void rdcycuninit ()
{ }

double rdcycfreq ()
{
    return 1000000;
}

// read raspi cycle counter
uint32_t rdcyc (void)
{
    return *ctrpt;
}
