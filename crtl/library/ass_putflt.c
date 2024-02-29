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

void ass_putflt (float flt)
{
    if (isnanf (flt)) {
        ass_putstr ("nan");
        return;
    }
    if (flt < 0.0) {
        ass_putchr ('-');
        flt = - flt;
    }
    if (! isfinitef (flt)) {
        ass_putstr ("inf");
    } else if (flt == 0.0) {
        ass_putstr ("0.0");
    } else if (flt < 0.001) {
        flt *= 10000.0;
        int exp = 0;
        while (flt < 0.001) { exp -= 5; flt *= 100000.0; }
        while (flt < 1000.0) { -- exp; flt *= 10.0; }
        int num = (int) (flt + 0.5);
        ass_putint (num / 10000);
        ass_putchr ('.');
        ass_putint (num % 10000);
        ass_putchr ('e');
        ass_putint (exp);
    } else if (flt > 1000.0) {
        int exp = 0;
        while (flt >= 1000000.0) { exp += 5; flt /= 100000.0; }
        while (flt >= 1.0) { exp ++; flt /= 10.0; }
        flt *= 10000.0;
        int num = (int) (flt + 0.5);
        ass_putint (num / 10000);
        ass_putchr ('.');
        ass_putint (num % 10000);
        ass_putstr ("e+");
        ass_putint (exp);
    } else {
        flt += 0.00005;
        int i = (int) flt;
        ass_putint (i);
        ass_putchr ('.');
        for (int j = 4; -- j >= 0;) {
            flt -= i;
            flt *= 10.0;
            i = (int) flt;
            ass_putchr ((char) ('0' + i));
        }
    }
}

