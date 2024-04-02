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

// emulate latch constructed the way the tube latches are

module DLatModule (U, Q, _Q, D, G, _PC, _PS, nto);
    input   U;
    output  Q;
    output _Q;
    input   D;
    input   G;
    input _PC;
    input _PS;

    wire ga, gb, gc, gd;

    StandCell anand (U, ga, G  & D);
    StandCell bnand (U, gb, G  & ga);
    StandCell cnand (U, gc, ga & gd & _PS);
    StandCell dnand (U, gd, gb & gc & _PC);

    assign  Q = gc;
    assign _Q = gd;

    // number of gates that are one = number of triodes that are off
    output reg[2:0] nto;
    always @(posedge U) begin
        nto <= ga + gb + gc + gd;
    end
endmodule
