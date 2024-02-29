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

#include <stdio.h>

int main (int argc, char **argv)
{
    double piapprox = 3.0;
    for (__uint16_t denom = 3; denom < 37600U; denom += 2) {
        double diff = 4.0 / ((denom - 1) * (double) denom * (denom + 1));
        if (denom & 2) {
            piapprox += diff;
        } else {
            piapprox -= diff;
        }
        printf ("\r%6u : pi=%16.13lf  ", denom, piapprox);
        fflush (stdout);
    }
    printf ("\n");
    return 0;
}
