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

// emulate standard cell (and-or-invert) constructed the way the tube A-O-Is are

module StandCell (U, _Q, D);
    input   U;
    output _Q;
    input   D;

    reg _Q;
    reg[8:0] counter;

    always @(posedge U) begin
        if (D) begin
            // tPHL = 0.28uS (28 * 10nS)
            if (counter > 27) begin
                counter <= 27;
            end else if (counter != 0) begin
                counter <= counter - 1;
            end else begin
                _Q <= 0;
            end
        end else begin
            // tPLH = 2.80uS (280 * 10nS)
            if (counter > 279) begin
                _Q <= 1;
            end else begin
                counter <= counter + 1;
            end
        end
    end

endmodule
