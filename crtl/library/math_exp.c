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

MType MMath(exp) (MType x)
{
    if (! MMath(isfinite) (x)) {
        if (MMath(isnan) (x)) return MNAN;
        return (x < 0.0) ? 0.0 : MINF;
    }

    if (x == 0.0) return 1.0;
    if (x < 0.0) {
        return 1.0 / MMath (exp) (- x);
    }

    MType n  = x;
    MType f  = 1.0;
    MType y  = x + 1.0;
    MType y1 = y;
    MType y2 = y;
    while (1) {
        n *= x / ++ f;
        y += n;
        if ((y == y1) || (y == y2)) return y;
        y2 = y1;
        y1 = y;
    }
}

