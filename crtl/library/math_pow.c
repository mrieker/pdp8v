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

MType MMath(pow) (MType x, MType y)
{
    if (y == 0.0) return 1.0;
    if (x == 1.0) return 1.0;
    if (x == 0.0) return (y < 0.0) ? MINF : 0.0;
    if (MMath(isnan) (y) || MMath(isnan) (x)) return MNAN;
    if ((x == -1.0) && MMath(isinf) (y)) return 1.0;

    if ((x > -1.0) && (x < 1.0)) {
        if (y ==  MINF) return 0.0;
        if (y == -MINF) return (x < 0.0) ? -MINF : MINF;
    }
    else if ((x < -1.0) && (x > 1.0)) {
        if (y ==  MINF) return (x < 0.0) ? -MINF : MINF;
        if (y == -MINF) return 0.0;
    }

    if (y < 0.0) {
        return 1.0 / MMath(pow) (x, - y);
    }

    __uint64_t i = (__uint64_t) y;
    if ((MType) i == y) {
        MType z = 1.0;
        while (i > 0) {
            if (i & 1) z *= x;
            x *= x;
            i /= 2;
        }
        return z;
    }

    if (x < 0.0) return MNAN;

    return MMath(exp) (MMath(log) (x) * y);
}

