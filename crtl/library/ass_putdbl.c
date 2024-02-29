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

#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void ass_putdbl (double dbl)
{
    if (isnan (dbl)) {
        ass_putstr ("nan");
        return;
    }
    if (dbl < 0.0) {
        ass_putchr ('-');
        dbl = - dbl;
    }
    if (! isfinite (dbl)) {
        ass_putstr ("inf");
    } else if (dbl == 0.0) {
        ass_putstr ("0.0");
    } else if (dbl < 0.001) {
        dbl *= 10000.0;
        int exp = 0;
        while (dbl < 0.001) { exp -= 5; dbl *= 100000.0; }
        while (dbl < 1000.0) { -- exp; dbl *= 10.0; }
        int num = (int) (dbl + 0.5);
        ass_putint (num / 10000);
        ass_putchr ('.');
        ass_putint (num % 10000);
        ass_putchr ('e');
        ass_putint (exp);
    } else if (dbl > 1000.0) {
        int exp = 0;
        while (dbl >= 1000000.0) { exp += 5; dbl /= 100000.0; }
        while (dbl >= 1.0) { exp ++; dbl /= 10.0; }
        dbl *= 10000.0;
        int num = (int) (dbl + 0.5);
        ass_putint (num / 10000);
        ass_putchr ('.');
        ass_putint (num % 10000);
        ass_putstr ("e+");
        ass_putint (exp);
    } else {
        dbl += 0.00005;
        int i = (int) dbl;
        ass_putint (i);
        ass_putchr ('.');
        for (int j = 4; -- j >= 0;) {
            dbl -= i;
            dbl *= 10.0;
            i = (int) dbl;
            ass_putchr ((char) ('0' + i));
        }
    }
}

