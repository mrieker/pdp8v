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

struct CType {
    CData real;
    CData imag;

    static CType make (CData r, CData i);

    CType add (CType x);
    CType sub (CType x);
    CType mul (CType x);
    CType div (CType x);
    CType par (CType x);
    CType mulr (CData x);
    CType divr (CData x);
    CType neg ();
    CType rec ();
    CData abs ();
    CData arg ();
    CType sq  ();

    void addeq (CType x);
    void subeq (CType x);
    void muleq (CType x);
    void diveq (CType x);

    CType pow (CType x);
    CType acos ();
    CType asin ();
    CType atan ();
    CType cos ();
    CType exp ();
    CType log ();
    CType sin ();
    CType sqrt ();
    CType tan ();
};
