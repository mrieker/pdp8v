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

// MA register

module macirc (in _aluq[11:00], in clok2, in _ma_aluq, out _maq[11:00], out maq[11:00], in reset)
{
    wire _mag, mata, matb, matc;
    wire _clok1, _reset;

    _clok1 = ~ clok2;
    _reset = ~ reset;

    mawff: DFF (_PS:_reset, _PC:1, D:_ma_aluq, T:_clok1, Q:_mag);

    mata = ~ (_mag | _clok1);
    matb = ~ (_mag | _clok1);
    matc = ~ (_mag | _clok1);

    malo:  DFF 4 (_PC:1, _PS:1, D:_aluq[03:00], Q:_maq[03:00], _Q:maq[03:00], T:mata);
    mamid: DFF 4 (_PC:1, _PS:1, D:_aluq[07:04], Q:_maq[07:04], _Q:maq[07:04], T:matb);
    mahi:  DFF 4 (_PC:1, _PS:1, D:_aluq[11:08], Q:_maq[11:08], _Q:maq[11:08], T:matc);
}

module ma ()
{
//            00000000011111111112222222222333
//            12345678901234567890123456789012
#define amask giiooiiiiiooiiiiiooiiiiiooiiiiio
#define bmask goiiiiiooiiiiiooiiiiiooiiiiiooii
#define cmask giiooiiiiiooiiiiiooiiiiiiiiiiiii
#define dmask giiiiiiiiiiiiiiiiiiiiiiiiiiiiiii
#include "cons.inc"

    macirc: macirc (_aluq, clok2, _ma_aluq, _maq[11:00], maq[11:00], reset);
}
