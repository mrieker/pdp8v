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

// returns -PI/2..+PI/2
MType MMath(atan) (MType x)
{
    if (MMath(isnan) (x)) return x;
    if (x == 0.0) return x;
    if (x < 0.0) return - MMath(atan) (- x);
    if (MMath(isinf) (x)) return MPI / 2.0;
    if (x > 1.0) return MPI / 2.0 - MMath(atan) (1.0 / x);
    if (x == 1.0) return MPI / 4.0;

    // y = x - x**3/3 + x**5/5 - x**7/7 + x**9/9 ...
    MType mxx = - x * x;
    MType d   = 1.0;
    MType y1  = x;
    MType y2  = x;
    while (1) {
        x *= mxx;
        d += 2.0;
        MType y = y1 + x / d;
        if ((y == y1) || (y == y2)) return y;
        y2 = y1;
        y1 = y;
    }
}

