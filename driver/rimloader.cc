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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "memory.h"
#include "rimloader.h"

#define ever (;;)

// pdp-8 rimloader format
//  input:
//   loadname = name of rimloader file
//   loadfile = file opened here
//  output:
//   returns 0: read successful
//        else: failure, error message was output
int rimloader (char const *loadname, FILE *loadfile)
{
    for ever {
        int ch = fgetc (loadfile);
        if (ch < 0) break;

        if (ch == 0377) continue;

        if (ch & 0177600) {
            fprintf (stderr, "rimloader: error reading 1st address byte %03o\n", ch);
            return 1;
        }

        // keep skipping until we have an address
        if (! (ch & 0100)) continue;

        uint16_t addr = (ch & 077) << 6;
        ch = fgetc (loadfile);
        if (ch & 0177700) {
            fprintf (stderr, "rimloader: error reading 2nd address byte %03o\n", ch);
            return 1;
        }
        addr |= ch & 077;

        ch = fgetc (loadfile);
        if (ch & 0177700) {
            fprintf (stderr, "rimloader: error reading 1st data byte %03o\n", ch);
            return 1;
        }
        uint16_t data = (ch & 077) << 6;

        ch = fgetc (loadfile);
        if (ch & 0177700) {
            fprintf (stderr, "rimloader: error reading 2nd data byte %03o\n", ch);
            return 1;
        }
        data |= ch & 077;

        memarray[addr] = data;
    }

    return 0;
}
