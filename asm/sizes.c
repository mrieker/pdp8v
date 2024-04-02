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

// reads an assembly listin on stdin and prints out page sizes
// assumes each page begins with .align
//  <pcatendofpage>  <numberofwordsinpage>  <totalwordsused>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main ()
{
    char c, linebuf[256], *p;
    int i, j, k;
    unsigned size, align, newpc, oldpc, used;

    newpc = 0;
    oldpc = 0;
    used  = 0;

    while (fgets (linebuf, sizeof linebuf, stdin) != NULL) {

        // remove redundant spaces
        k = 0;
        j = 0;
        for (i = 0; (c = linebuf[i]) != 0; i ++) {
            if (c <= ' ') {
                k = 1;
            } else {
                if (k) linebuf[j++] = ' ';
                linebuf[j++] = c;
                k = 0;
            }
        }
        linebuf[j] = 0;

        // if begins with a digit, that's the PC
        if ((linebuf[0] >= '0') && (linebuf[0] <= '7')) {
            newpc = strtoul (linebuf, &p, 8);
        }

        // if contains .align, print the PC
        p = strstr (linebuf, ".align");
        if (p != NULL) {
            size  = newpc + 1 - oldpc;
            used += size;
            printf ("%05o  %05o  %05o\n", newpc, size, used);
            align = strtoul (p + 7, &p, 8);
            oldpc = (newpc + align) & - align;
        }
    }

    return 0;
}
