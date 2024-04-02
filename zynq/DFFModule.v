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

// emulate DFF constructed the way the tube DFFs are

module DFFModule (U, Q, _Q, T, D, _PC, _PS, _SC, ga, gb, gc, gd, ge, gf, nto);
    input   U;
    output  Q;
    output _Q;
    input   T;
    input   D;
    input _PC;
    input _PS;
    input _SC;

    wire x = D & _SC;

    // edge-triggered using 6 nand gate standcells
    output ga, gb, gc, gd, ge, gf;

    assign _Q = ge;
    assign  Q = gf;

    StandCell anand (U, ga, x  & gb & _PC);
    StandCell bnand (U, gb, ga & T  & gc);
    StandCell cnand (U, gc, T  & gd & _PC);
    StandCell dnand (U, gd, gc & ga & _PS);
    StandCell enand (U, ge, gb & gf & _PC);
    StandCell fnand (U, gf, gc & ge & _PS);

    // number of gates that are one = number of triodes that are off
    output reg[2:0] nto;
    always @(posedge U) begin
        nto <= ga + gb + gc + gd + ge + gf;
    end
endmodule
