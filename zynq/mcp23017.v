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

// minimal implementation of MCP23017
//  inputs:
//   adr = low 3 bits of MCP23017 address
//   clk = zynq 100MHz clock
//   res = reset the MCP23017
//   scl = i2c clock
//   sdai = i2c data input
//   ains, bins = input pins
//  outputs:
//   sdao = i2c data output
//   aouts, bouts = output pins
module mcp23017 (adr, clk, res, scl, sdai, sdao, ains, bins, aouts, bouts);
    input[2:0] adr;
    input clk, res, scl, sdai;
    output sdao;
    input[7:0] ains, bins;
    output[7:0] aouts, bouts;

    reg lastscl, lastsda, reading, sdaout, thisscl, thissda;
    reg start, haveix;
    reg[175:0] registers;
    reg[2:0] count;
    reg[2:0] mute;
    reg[4:0] index;
    reg[6:0] addr;
    reg[3:0] state;

    localparam[3:0] IDLE       = 0; // waiting for start bit
    localparam[3:0] GETADR     = 1; // shifting in address and R/W bit
    localparam[3:0] SENDADRACK = 2; // sending ack for address
    localparam[3:0] GETIDX     = 3; // shifting in index
    localparam[3:0] SENDIDXACK = 4; // sending ack for index or received data
    localparam[3:0] RECVBITS   = 5; // receiving write data bits
    localparam[3:0] SENDBITS   = 6; // sending read data bits
    localparam[3:0] RECVDATACK = 7; // receiving ack for data bits sent
    localparam[3:0] RECVACKREL = 8; // waiting for master to stop sending ack

    assign sdao = sdaout;

    // output latches go to output pins
    assign aouts[0] = registers[{5'h14,3'h0}];
    assign aouts[1] = registers[{5'h14,3'h1}];
    assign aouts[2] = registers[{5'h14,3'h2}];
    assign aouts[3] = registers[{5'h14,3'h3}];
    assign aouts[4] = registers[{5'h14,3'h4}];
    assign aouts[5] = registers[{5'h14,3'h5}];
    assign aouts[6] = registers[{5'h14,3'h6}];
    assign aouts[7] = registers[{5'h14,3'h7}];
    assign bouts[0] = registers[{5'h15,3'h0}];
    assign bouts[1] = registers[{5'h15,3'h1}];
    assign bouts[2] = registers[{5'h15,3'h2}];
    assign bouts[3] = registers[{5'h15,3'h3}];
    assign bouts[4] = registers[{5'h15,3'h4}];
    assign bouts[5] = registers[{5'h15,3'h5}];
    assign bouts[6] = registers[{5'h15,3'h6}];
    assign bouts[7] = registers[{5'h15,3'h7}];

    // readbacks come from input pin if input mode, output latch if output mode
    wire[7:0] arbks, brbks;
    assign arbks[0] = registers[{5'h00,3'h0}] ? ains[0] : registers[{5'h14,3'h0}];
    assign arbks[1] = registers[{5'h00,3'h1}] ? ains[1] : registers[{5'h14,3'h1}];
    assign arbks[2] = registers[{5'h00,3'h2}] ? ains[2] : registers[{5'h14,3'h2}];
    assign arbks[3] = registers[{5'h00,3'h3}] ? ains[3] : registers[{5'h14,3'h3}];
    assign arbks[4] = registers[{5'h00,3'h4}] ? ains[4] : registers[{5'h14,3'h4}];
    assign arbks[5] = registers[{5'h00,3'h5}] ? ains[5] : registers[{5'h14,3'h5}];
    assign arbks[6] = registers[{5'h00,3'h6}] ? ains[6] : registers[{5'h14,3'h6}];
    assign arbks[7] = registers[{5'h00,3'h7}] ?   1'b0  : registers[{5'h14,3'h7}];  // output-only pin
    assign brbks[0] = registers[{5'h01,3'h0}] ? bins[0] : registers[{5'h15,3'h0}];
    assign brbks[1] = registers[{5'h01,3'h1}] ? bins[1] : registers[{5'h15,3'h1}];
    assign brbks[2] = registers[{5'h01,3'h2}] ? bins[2] : registers[{5'h15,3'h2}];
    assign brbks[3] = registers[{5'h01,3'h3}] ? bins[3] : registers[{5'h15,3'h3}];
    assign brbks[4] = registers[{5'h01,3'h4}] ? bins[4] : registers[{5'h15,3'h4}];
    assign brbks[5] = registers[{5'h01,3'h5}] ? bins[5] : registers[{5'h15,3'h5}];
    assign brbks[6] = registers[{5'h01,3'h6}] ? bins[6] : registers[{5'h15,3'h6}];
    assign brbks[7] = registers[{5'h01,3'h7}] ?   1'b0  : registers[{5'h15,3'h7}];  // output-only pin

    // splice the readbacks into the registers for reading
    // also we do not implement registers 02..11 so they always readback as zeroes
    wire[255:0] regreads;
    assign regreads[{5'h01,3'h7}:{5'h00,3'h0}] = registers[{5'h01,3'h7}:{5'h00,3'h0}];
    assign regreads[{5'h11,3'h7}:{5'h02,3'h0}] = 0;
    assign regreads[{5'h12,3'h7}:{5'h12,3'h0}] = arbks;
    assign regreads[{5'h13,3'h7}:{5'h13,3'h0}] = brbks;
    assign regreads[{5'h15,3'h7}:{5'h14,3'h0}] = registers[{5'h15,3'h7}:{5'h14,3'h0}];
    assign regreads[{5'h1F,3'h7}:{5'h16,3'h0}] = 0;

    always @(posedge clk or posedge res) begin

        // scl clock during reset is '1'
        // data pins all reset to input
        if (res) begin
            haveix  <= 1'b0;        // don't have register index byte yet
            lastscl <= 1'b1;        // previous clock sample is high
            lastsda <= 1'b1;        // previous data sample is high
            mute    <= 3'b000;      // ignoring ringing on input signals for 80ns
            sdaout  <= 1'b1;        // data output is high meaning open-collector
            start   <= 1'b0;        // haven't seen a start bit yet
            state   <= IDLE;        // waiting for start bit
            thisscl <= 1'b1;        // current clock sample is high
            thissda <= 1'b1;        // current data sample is high
            registers[{5'h00,3'h7}:{5'h00,3'h0}] <= 8'hFF;  // i/o pins default to inputs
            registers[{5'h01,3'h7}:{5'h01,3'h0}] <= 8'hFF;
        end else begin

            // detect data transition when clock is high
            // normally data line stays steady when clock is high
            // high-to-low means start bit (when clock goes low)
            // low-to-high means stop bit (immediately)
            if (thisscl) begin
                if (lastsda & ~ thissda) start <= 1'b1;     // high-to-low when clock high, start when clock goes low
                if (~ lastsda & thissda) begin              // low-to-high when clock high, stop now
                    haveix <= 1'b0;                         // - invalidates register index
                    sdaout <= 1'b1;                         // - float data bus
                    state  <= IDLE;                         // - go to idle so we look for start bit
                end
            end else begin
                start <= 1'b0;                              // clock low, no start bit

                // we sent 8 data bits for a receive
                // we received ack from master and the corresponding clock pulse went high then low
                // we are now waiting for master to release the sda bus so we can send next bit
                // if master is going to send stop bit instead of wanting more,
                //   it will keep sda low then drive clock high then drive sda high
                //   so only do this detection only when clock low so stop detect will work
                if (thissda & (state == RECVACKREL)) begin
                    sdaout <= regreads[{index,3'h7}];
                    state  <= SENDBITS;
                end

                // check for falling i2c clock edge
                else if (lastscl) begin

                    // high to low when clock was high means start regardless of previous state
                    if (start) begin
                        count <= 3'h0;
                        state <= GETADR;
                    end else begin

                        // data bit did not transition while clock was high
                        // process the data bit based on what state we're in
                        case (state)

                            // waiting for a start bit
                        IDLE:
                            begin end

                            // getting address by reading bits from data line
                            // also gets read/write function bit
                        GETADR:
                            begin
                                if (count != 3'h7) begin
                                    addr  <= { addr[5:0], thissda };
                                    count <= count + 1;
                                end else if (addr == { 4'b0100, adr } & (~ thissda | haveix)) begin
                                    reading <= thissda;
                                    sdaout  <= 1'b0;
                                    state   <= SENDADRACK;
                                end else begin
                                    // it's not our address or requesting read with invalid index, don't send ack
                                    state <= IDLE;
                                end
                            end

                            // done sending ack for address
                            // - if we don't have index, receive index bits next (can only happen in writing mode)
                            // - if we have index in writing mode, start receiving data bits
                            // - if we have index in reading mode, start sending data bits
                        SENDADRACK:
                            begin
                                count  <= 3'h0;
                                sdaout <= 1'b1;
                                if (~ haveix) begin
                                    state  <= GETIDX;
                                end else begin
                                    count  <= reading ? 3'h6 : 3'h7;
                                    sdaout <= reading ? regreads[{index,3'h7}] : 1'b1;
                                    state  <= reading ? SENDBITS : RECVBITS;
                                end
                            end

                            // getting index bits
                        GETIDX:
                            begin
                                index <= { index[3:0], thissda };
                                if (count != 3'h7) begin
                                    count  <= count + 1;
                                end else begin
                                    haveix <= 1'b1;
                                    sdaout <= 1'b0;
                                    state  <= SENDIDXACK;
                                end
                            end

                            // done sending ack for index or received data
                            // go on to next register
                        SENDIDXACK:
                            begin
                                count  <= reading ? 3'h6 : 3'h7;
                                sdaout <= reading ? regreads[{index,3'h7}] : 1'b1;
                                state  <= reading ? SENDBITS : RECVBITS;
                            end

                            // receive data bits for writing register
                        RECVBITS:
                            begin
                                registers[{index,count}] <= thissda;
                                if (count != 3'h0) begin
                                    count  <= count - 1;
                                end else begin
                                    sdaout <= 1'b0;
                                    index  <= index + 1;
                                    state  <= SENDIDXACK;
                                end
                            end

                            // send data bits for reading register
                        SENDBITS:
                            begin
                                if (count != 3'h7) begin
                                    count  <= count - 1;
                                    sdaout <= regreads[{index,count}];
                                end else begin
                                    index  <= index + 1;
                                    sdaout <= 1'b1;
                                    state  <= RECVDATACK;
                                end
                            end

                            // got ack bit from master for the register bits we just sent
                        RECVDATACK:
                            begin
                                if (thissda) begin
                                    state <= IDLE;
                                end else begin
                                    count <= 3'h6;
                                    state <= RECVACKREL;
                                end
                            end
                        endcase
                    end
                end
            end

            // synchronize scl and sda to the 100MHz clock
            lastscl <= thisscl;
            lastsda <= thissda;
            if ((lastscl ^ thisscl) | (lastsda ^ thissda)) begin
                // ignore ringing for next 50nS
                mute <= 3'b000;
            end else if (mute != 3'b100) begin
                mute <= mute + 1;
            end else begin
                thisscl <= scl;
                thissda <= sdai;
                // give thisscl,thissda 10nS to stop glitching
            end
        end
    end
endmodule
