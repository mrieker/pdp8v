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

MType MMath(cos) (MType x)
{
    if (! MMath(isfinite) (x)) return MNAN;
    if (x < 0.0) x = -x;
    while (x >= MPI) x -= MPI * 2.0;
    MType mxx = - x * x;
    MType m   = 0.0;
    MType y   = 1.0;
    MType y1  = 1.0;
    MType y2  = 1.0;
    x = 1.0;
    while (1) {
        x *= mxx;
        x /= ++ m;
        x /= ++ m;
        y += x;
        if ((y == y1) || (y == y2)) return y;
        y2 = y1;
        y1 = y;
    }
}

