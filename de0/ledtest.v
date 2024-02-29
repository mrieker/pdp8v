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

// top-level module to test DE0-LEDS board
// uses same interface with raspi as computer circuit de0.v
// all raspi does is generate reset signal

//  FIFTYMHZ = connects to DE0's on-board 50MHz clock
//             used to simulate tube-length propagation delays
//  LEDS = connects to DE0's on-board LEDs
//  GP0OUT = connects to GPIO 0 pins for DE0-LEDS board
//  others connect to raspi gpio pins via 33-ohm resistors

module ledtest (FIFTYMHZ, _CLOCK, _DENA, _INTRQ, _IOSKP, _RESET, FLAG, DFRM, JUMP, INTAK, _IOINST, MREAD, MWRITE, DATA, LINK, LEDS, GP0OUT);
    input FIFTYMHZ, _CLOCK, _DENA, _INTRQ, _IOSKP, _RESET, FLAG;
    output DFRM, JUMP, INTAK, _IOINST, MREAD, MWRITE;
    inout[11:00] DATA;
    inout LINK;
    output[7:0] LEDS;
    output[33:00] GP0OUT;

    assign DFRM    = 0;
    assign JUMP    = 0;
    assign INTAK   = 0;
    assign _IOINST = 1;
    assign MREAD   = 0;
    assign MWRITE  = 0;

    reg[23:00] freqdiv;
    reg[33:00] GP0OUT;
    reg[7:0] LEDS;

    always @ (posedge FIFTYMHZ or negedge _RESET) begin
        if (~ _RESET) begin
            freqdiv <= 0;
            GP0OUT  <= 0;
            LEDS    <= 1;
        end else begin
            if (freqdiv == 0) begin
                GP0OUT[06] <= GP0OUT[02];
                GP0OUT[10] <= GP0OUT[06];
                GP0OUT[14] <= GP0OUT[10];
                GP0OUT[18] <= GP0OUT[14];
                GP0OUT[22] <= GP0OUT[18];
                GP0OUT[26] <= GP0OUT[22];
                GP0OUT[30] <= GP0OUT[26];
                GP0OUT[32] <= GP0OUT[30];
                GP0OUT[28] <= GP0OUT[32];
                GP0OUT[24] <= GP0OUT[28];
                GP0OUT[20] <= GP0OUT[24];
                GP0OUT[16] <= GP0OUT[20];
                GP0OUT[12] <= GP0OUT[16];
                GP0OUT[08] <= GP0OUT[12];
                GP0OUT[04] <= GP0OUT[08];
                GP0OUT[00] <= GP0OUT[04];
                GP0OUT[03] <= GP0OUT[00];
                GP0OUT[07] <= GP0OUT[03];
                GP0OUT[11] <= GP0OUT[07];
                GP0OUT[15] <= GP0OUT[11];
                GP0OUT[19] <= GP0OUT[15];
                GP0OUT[23] <= GP0OUT[19];
                GP0OUT[27] <= GP0OUT[23];
                GP0OUT[31] <= GP0OUT[27];
                GP0OUT[33] <= GP0OUT[31];
                GP0OUT[29] <= GP0OUT[33];
                GP0OUT[25] <= GP0OUT[29];
                GP0OUT[21] <= GP0OUT[25];
                GP0OUT[17] <= GP0OUT[21];
                GP0OUT[13] <= GP0OUT[17];
                GP0OUT[09] <= GP0OUT[13];
                GP0OUT[05] <= GP0OUT[09];
                GP0OUT[01] <= GP0OUT[05];
                GP0OUT[02] <= GP0OUT[01] ^ LEDS[7];

                LEDS[1] <= LEDS[0];
                LEDS[2] <= LEDS[1];
                LEDS[3] <= LEDS[2];
                LEDS[4] <= LEDS[3];
                LEDS[5] <= LEDS[4];
                LEDS[6] <= LEDS[5];
                LEDS[7] <= LEDS[6];
                LEDS[0] <= LEDS[6];
            end
            freqdiv <= freqdiv + 1;
        end
    end
endmodule
