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
// also switches link,data direction on command by raspi via dena

//  FIFTYMHZ = connects to DE0's on-board 50MHz clock
//             used to simulate tube-length propagation delays
//  LEDS = connects to DE0's on-board LEDs
//         first 3 indicate major state:  fetch, defer, execute
//         last 3 indicate cycle therein
//  others connect to raspi gpio pins via 33-ohm resistors

module counter (FIFTYMHZ, _CLOCK, _DENA, _INTRQ, _IOSKP, _RESET, HALT, INTAK, _IOINST, MREAD, MWRITE, DATA, LINK, LEDS);
    input FIFTYMHZ, _CLOCK, _DENA, _INTRQ, _IOSKP, _RESET;
    output HALT, INTAK, _IOINST, MREAD, MWRITE;
    inout[11:00] DATA;
    inout LINK;
    output[7:0] LEDS;

    wire clok2, intrq, ioskp, reset, mql;
    wire[11:00] mq, _md;
    reg[31:00] count;

    assign clok2   = ~ _CLOCK;                                      // PhysLib::writegpio() flips G_CLOCK
    assign intrq   = ~ _INTRQ;                                      // PhysLib::writegpio() flips G_IRQ
    assign ioskp   = ~ _IOSKP;                                      // PhysLib::writegpio() flips G_IOS
    assign reset   = ~ _RESET;                                      // PhysLib::writegpio() flips G_RESET
    assign mq      = ~ DATA;                                        // PhysLib::writegpio() flips G_DATA
    assign mql     = ~ LINK;                                        // PhysLib::writegpio() flips G_LINK

    // when _DENA is low (DENA high), gate the md,mdl values to the LINK,DATA lines
    // when _DENA is high (DENA low), let LINK,DATA flow through to mql,mq
    assign DATA    = _DENA ? 12'hZZZ : count[11:00];                // PhysLib::readgpio() preserves G_DATA
    assign LINK    = _DENA ? 1'bZ : count[12];                      // PhysLib::readgpio() preserves G_LINK
    assign _IOINST = ~ count[13];                                   // PhysLib::readgpio() reverses G_IOIN
    assign HALT    =   count[14];                                   // PhysLib::readgpio() preserves G_HALT
    assign MREAD   =   count[15];                                   // PhysLib::readgpio() preserves G_READ
    assign MWRITE  =   count[16];                                   // PhysLib::readgpio() preserves G_WRITE
    assign INTAK   =   clok2;                                       // PhysLib::readgpio() preserves G_IAK

    assign LEDS[7] = count[5];
    assign LEDS[6] = count[4];
    assign LEDS[5] = count[3];
    assign LEDS[4] = count[2];
    assign LEDS[3] = count[1];
    assign LEDS[2] = count[0];
    assign LEDS[1] = _DENA;
    assign LEDS[0] = clok2;

    always @ (posedge clok2 or posedge reset) begin
        if (reset) begin
            count <= 32'h87654321;
        end else if (_DENA) begin
            count <= { count[31:15], intrq, ioskp, mql, mq };
        end else begin
            count <= { count[30:00], count[31] ^ count[21] ^ count[01] ^ count[00] };
        end
    end
endmodule
