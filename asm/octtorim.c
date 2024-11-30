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

// convert a link octal file to rim loader format

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main (int argc, char **argv)
{
    char linebuf[1024], *p;
    int i;
    uint16_t addr, frame, lsb, msb;

    for (i = 0; i < 64; i ++) putchar (0200);

    while (fgets (linebuf, sizeof linebuf, stdin) != NULL) {
        addr = strtoul (linebuf, &p, 8);
        if (*(p ++) != ':') {
            fprintf (stderr, "bad address in line %s\n", linebuf);
            return 1;
        }
        frame = ((addr & 070000) >> 9);
        if (frame != 0) {
            fprintf (stderr, "bad frame %u\n", frame);
            return 1;
        }

        while (*p != '\n') {
            msb = 0100 | ((addr & 007700) >> 6);
            lsb = 0000 | ((addr & 000077) >> 0);
            putchar (msb);
            putchar (lsb);
            for (int i = 0; i < 2; i ++) {
                msb = *(p ++) - '0';
                lsb = *(p ++) - '0';
                if ((msb > 7) || (lsb > 7)) {
                    fprintf (stderr, "bad data in line %s\n", linebuf);
                    return 1;
                }
                lsb |= msb << 3;
                putchar (lsb);
            }
            addr = (addr + 1) & 07777;
        }
    }

    for (i = 0; i < 64; i ++) putchar (0200);
    fflush (stdout);

    return 0;
}
