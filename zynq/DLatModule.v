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

module DLatModule (U, Q, _Q, D, G, _PC, _PS);
    input   U;
    output  Q;
    output _Q;
    input   D;
    input   G;
    input _PC;
    input _PS;

    //reg Q, _Q;

    wire a, b, c, d;

    StandCell anand (U, a, G & D);
    StandCell bnand (U, b, G & a);
    StandCell cnand (U, c, a & d & _PS);
    StandCell dnand (U, d, b & c & _PC);

    assign  Q = c;
    assign _Q = d;

    /*
    always @(*) begin
        if (~ _PC | ~ _PS) begin
             Q <= ~ _PS;
            _Q <= ~ _PC;
        end else if (G) begin
             Q <=   D;
            _Q <= ~ D;
        end
    end
    */
endmodule
