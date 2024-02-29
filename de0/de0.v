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

// top-level module that interfaces with raspi
// inverts signals like the real level converters
// also switches link,data direction on command by raspi via dena and qena

//  FIFTYMHZ = connects to DE0's on-board 50MHz clock
//             used to simulate tube-length propagation delays
//  LEDS = connects to DE0's on-board LEDs
//         first 3 indicate major state:  fetch, defer, execute
//         last 3 indicate cycle therein
//  GP0OUT = connects to GPIO 0 pins for debugging
//  others connect to raspi gpio pins via 33-ohm resistors

module de0 (FIFTYMHZ, _CLOCK, DENA, _INTRQ, _IOSKP, _RESET, QENA, DFRM, JUMP, INTAK, _IOINST, MREAD, MWRITE, DATA, LINK, LEDS, GP0OUT);
    input FIFTYMHZ, _CLOCK, DENA, _INTRQ, _IOSKP, _RESET, QENA;
    output DFRM, JUMP, INTAK, _IOINST, MREAD, MWRITE;
    inout[11:00] DATA;
    inout LINK;
    output[7:0] LEDS;
    output[33:00] GP0OUT;

    wire clok2, reset, intrq, ioskp, mql;
    wire _dfrm, _jump, _intak, ioinst, _mdl, _mread, _mwrite;
    wire[11:00] mq, _md;

    assign clok2   = ~ _CLOCK;
    assign reset   = ~ _RESET;
    assign intrq   = ~ _INTRQ;
    assign ioskp   = ~ _IOSKP;

    assign DFRM    = ~ _dfrm;
    assign JUMP    = ~ _jump;
    assign INTAK   = ~ _intak | reset;
    assign _IOINST = ~ ioinst;
    assign MREAD   = ~ _mread;
    assign MWRITE  = ~ _mwrite;

    // when DENA is high, gate the md,mdl values to the LINK,DATA lines
    // when QENA is high, let LINK,DATA flow through to mql,mq
    assign DATA    = DENA ? ~  _md  : 12'hZZZ;
    assign LINK    = DENA ? ~  _mdl :  1'bZ;
    assign mq      = QENA ? ~  DATA : 12'h000;
    assign mql     = QENA ? ~  LINK :  1'b0;

    proc proc (.uclk (FIFTYMHZ),
                .CLOK2 (clok2), .INTRQ (intrq), .IOSKP (ioskp), .MQ (mq), .MQL (mql), .RESET (reset), .QENA (QENA), .DENA (DENA),
                ._DFRM (_dfrm), ._JUMP (_jump), ._INTAK (_intak), .IOINST (ioinst), ._MDL (_mdl), ._MD (_md), ._MREAD (_mread), ._MWRITE (_mwrite),
                .LEDS (LEDS), .GP0OLO (GP0OUT[31:00]), .GP0OHI (GP0OUT[33:32]));
endmodule
