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

// inputs:
//  adr = low 3 bits of MCP23017 address
//  clk = zynq 100MHz clock
//  res = reset the MCP23017
//  scl = i2c clock
// bidirs:
//  sda = i2c data
module mcp23017 (adr, clk, res, scl, sdax, sday, ains, bins, aouts, bouts);
    input[2:0] adr;
    input clk, res, scl, sdax;
    output sday;
    input[7:0] ains, bins;
    output[7:0] aouts, bouts;

    reg lastscl, lastsda, reading, sdaout, start, thisscl, thissda;
    reg[255:0] registers;
    reg[2:0] count;
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
    
    assign sday = sdaout & sdax;

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
    assign arbks[7] = registers[{5'h00,3'h7}] ?   1'b0  : registers[{5'h14,3'h7}];
    assign brbks[0] = registers[{5'h01,3'h0}] ? bins[0] : registers[{5'h15,3'h0}];
    assign brbks[1] = registers[{5'h01,3'h1}] ? bins[1] : registers[{5'h15,3'h1}];
    assign brbks[2] = registers[{5'h01,3'h2}] ? bins[2] : registers[{5'h15,3'h2}];
    assign brbks[3] = registers[{5'h01,3'h3}] ? bins[3] : registers[{5'h15,3'h3}];
    assign brbks[4] = registers[{5'h01,3'h4}] ? bins[4] : registers[{5'h15,3'h4}];
    assign brbks[5] = registers[{5'h01,3'h5}] ? bins[5] : registers[{5'h15,3'h5}];
    assign brbks[6] = registers[{5'h01,3'h6}] ? bins[6] : registers[{5'h15,3'h6}];
    assign brbks[7] = registers[{5'h01,3'h7}] ?   1'b0  : registers[{5'h15,3'h7}];

    // splice the readbacks into the registers for reading
    wire[255:0] regreads;
    assign regreads[{5'h11,3'h7}:{5'h00,3'h0}] = registers[{5'h11,3'h7}:{5'h00,3'h0}];
    assign regreads[{5'h12,3'h7}:{5'h12,3'h0}] = arbks;
    assign regreads[{5'h13,3'h7}:{5'h13,3'h0}] = brbks;
    assign regreads[{5'h15,3'h7}:{5'h14,3'h0}] = registers[{5'h15,3'h7}:{5'h14,3'h0}];

    always @(posedge clk or posedge res) begin

        // scl clock during reset is '1'
        // data pins all reset to input
        if (res) begin
            lastscl <= 1'b1;
            lastsda <= 1'b1;
            sdaout  <= 1'b1;
            start   <= 1'b0;
            state   <= IDLE;
            thisscl <= 1'b1;
            thissda <= 1'b1;
            registers[{5'h00,3'h7}:{5'h00,3'h0}] <= 8'hFF;
            registers[{5'h01,3'h7}:{5'h01,3'h0}] <= 8'hFF;
        end else begin

            // detect data transition when clock is high
            // normally data line stays steady when clock is high
            // high-to-low means start bit (when clock goes low)
            // low-to-high means stop bit (immediately)
            if (thisscl) begin
                if (lastsda & ~ thissda) start <= 1'b1;     // high-to-low when clock high, start when clock goes low
                if (~ lastsda & thissda) begin              // low-to-high when clock high, stop now
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

                            // getting address and reading bits
                        GETADR:
                            begin
                                if (count != 3'h7) begin
                                    addr  <= { addr[5:0], thissda };
                                    count <= count + 1;
                                end else if (addr == { 4'b0100, adr }) begin
                                    reading <= thissda;
                                    sdaout  <= 1'b0;
                                    state   <= SENDADRACK;
                                end else begin
                                    state <= IDLE;
                                end
                            end

                            // done sending ack for address
                        SENDADRACK:
                            begin
                                count  <= 3'h0;
                                sdaout <= 1'b1;
                                state  <= GETIDX;
                            end

                            // getting index bits
                        GETIDX:
                            begin
                                index <= { index[3:0], thissda };
                                if (count != 3'h7) begin
                                    count  <= count + 1;
                                end else begin
                                    sdaout <= 1'b0;
                                    state  <= SENDIDXACK;
                                end
                            end
                            // done sending ack for index or received data
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
            thisscl <= scl;
            thissda <= sdax;
            // give thisscl,sda 10nS to stop glitching
        end
    end
endmodule

// inputs:
//  scl   = i2s bus for MCP23017s
//  CLOCK = zynq 100MHz clock
//  paddlrd{a,b,c,d} = spying on paddle output pins going to raspictl
//  boardena = board enable bits from raspictl
//  fpoutput = front panel bits from raspictl
//  gpcompos = spying on composite gpio pins going to raspictl
// bidirs:
//  sda = i2s bus for MCP23017s
// outputs:
//  TRIGGR  = trigger singal to G_TRIG to zynq arm
//  DEBUGS  = debug outputs for ILA
//  fpinput = front panel bits going to raspictl
module frontpanel (scl, sdax, sday,
        CLOCK, TRIGGR, DEBUGS,
        paddlrda, paddlrdb, paddlrdc, paddlrdd,
        fpinput, fpoutput, gpcompos);
    input scl, sdax;
    output sday;
    input CLOCK;
    output TRIGGR;
    output[13:00] DEBUGS;
    input[31:00] paddlrda, paddlrdb, paddlrdc, paddlrdd;
    output reg[31:00] fpinput;
    input[31:00] fpoutput, gpcompos;
    
    // data is in bits [11:00]
    localparam FPO_V_ENABLE  = 12;
    localparam FPO_V_CLOCK   = 13;
    localparam FPO_V_STOPPED = 14;

    // data is in bits [11:00]
    localparam FPI_V_DEP  = 12;
    localparam FPI_V_EXAM = 13;
    localparam FPI_V_JUMP = 14;
    localparam FPI_V_CONT = 15;
    localparam FPI_V_STRT = 16;
    localparam FPI_V_STOP = 17;

    // signals to/from pipanel.cc, ie, PDP-8/L front panel
    wire[79:00] fpins, fpouts;
    wire sda01, sda12, sda23, sda34;
    mcp23017 mcp0 (3'b000, CLOCK, ~ enabled, scl, sdax,  sda01, fpins[07:00], fpins[15:08], fpouts[07:00], fpouts[15:08]);
    mcp23017 mcp1 (3'b001, CLOCK, ~ enabled, scl, sda01, sda12, fpins[23:16], fpins[31:24], fpouts[23:16], fpouts[31:24]);
    mcp23017 mcp2 (3'b010, CLOCK, ~ enabled, scl, sda12, sda23, fpins[39:32], fpins[47:40], fpouts[39:32], fpouts[47:40]);
    mcp23017 mcp3 (3'b011, CLOCK, ~ enabled, scl, sda23, sda34, fpins[55:48], fpins[63:56], fpouts[55:48], fpouts[63:56]);
    mcp23017 mcp4 (3'b100, CLOCK, ~ enabled, scl, sda34, sday,  fpins[71:64], fpins[79:72], fpouts[71:64], fpouts[79:72]);

    // front panel state
    reg[2:0] state;
    localparam[2:0] IDLE = 0;   // processor halted, waiting for button to be pressed
    localparam[2:0] EXAM = 1;   // exam button pressed, waiting for memory contents
    localparam[2:0] CONT = 2;   // waiting for zynqlib.cc to see continue/start switch
    localparam[2:0] LDAD = 3;   // ldad button pressed, waiting for memory contents
    localparam[2:0] DEP  = 4;   // dep button pressed, writing memory contents
    localparam[2:0] STOP = 5;   // processor has just stopped

    // drive PDP-8/L panel lights
    reg[11:00] mareg, mbreg;
    reg fetreg, defreg, exereg, brkreg;

    assign fpins[8'h03] = paddlrda[02];     // P_ACnn = acq[11-nn]
    assign fpins[8'h05] = paddlrda[09];
    assign fpins[8'h1E] = paddlrda[16];
    assign fpins[8'h08] = paddlrda[23];
    assign fpins[8'h1B] = paddlrda[30];
    assign fpins[8'h14] = paddlrdb[06];
    assign fpins[8'h19] = paddlrdb[13];
    assign fpins[8'h16] = paddlrdb[20];
    assign fpins[8'h2C] = paddlrdb[27];
    assign fpins[8'h2B] = paddlrdc[02];
    assign fpins[8'h3D] = paddlrdc[09];
    assign fpins[8'h3E] = paddlrdc[16];

    assign fpins[8'h32] = paddlrda[08];     // P_IRnn = irq[11-nn]
    assign fpins[8'h36] = paddlrda[15];
    assign fpins[8'h40] = paddlrda[22];

    assign fpins[8'h0C] = paddlrdd[23];     // P_LINK = lnq

    assign fpins[8'h06] = mareg[11];        // P_MAnn = mareg[11-nn]
    assign fpins[8'h09] = mareg[10];
    assign fpins[8'h11] = mareg[09];
    assign fpins[8'h1D] = mareg[08];
    assign fpins[8'h1C] = mareg[07];
    assign fpins[8'h15] = mareg[06];
    assign fpins[8'h20] = mareg[05];
    assign fpins[8'h21] = mareg[04];
    assign fpins[8'h2D] = mareg[03];
    assign fpins[8'h28] = mareg[02];
    assign fpins[8'h3C] = mareg[01];
    assign fpins[8'h30] = mareg[00];

    assign fpins[8'h04] = mbreg[00];        // P_MBnn
    assign fpins[8'h0A] = mbreg[01];
    assign fpins[8'h10] = mbreg[02];
    assign fpins[8'h12] = mbreg[03];
    assign fpins[8'h13] = mbreg[04];
    assign fpins[8'h1A] = mbreg[05];
    assign fpins[8'h18] = mbreg[06];
    assign fpins[8'h22] = mbreg[07];
    assign fpins[8'h2E] = mbreg[08];
    assign fpins[8'h29] = mbreg[09];
    assign fpins[8'h3B] = mbreg[10];
    assign fpins[8'h26] = mbreg[11];

    assign fpins[8'h0B] = 0;                // P_EMA
    assign fpins[8'h31] = fetreg;           // P_FET
    assign fpins[8'h33] = 0;                // P_ION
    assign fpins[8'h34] = 0;                // P_PARE
    assign fpins[8'h35] = exereg;           // P_EXE
    assign fpins[8'h41] = 0;                // P_PRTE
    assign fpins[8'h42] = ~ fpoutput[FPO_V_STOPPED];  // P_RUN
    assign fpins[8'h43] = 0;                // P_WCT
    assign fpins[8'h44] = brkreg;           // P_BRK
    assign fpins[8'h4B] = 0;                // P_CAD
    assign fpins[8'h4E] = defreg;           // P_DEF

    // sense PDP-8/L panel switches
    wire [11:00] srswitches;

    assign srswitches[11] = fpouts[8'h07];
    assign srswitches[10] = fpouts[8'h02];
    assign srswitches[09] = fpouts[8'h01];
    assign srswitches[08] = fpouts[8'h00];
    assign srswitches[07] = fpouts[8'h0D];
    assign srswitches[06] = fpouts[8'h27];
    assign srswitches[05] = fpouts[8'h25];
    assign srswitches[04] = fpouts[8'h24];
    assign srswitches[03] = fpouts[8'h23];
    assign srswitches[02] = fpouts[8'h2F];
    assign srswitches[01] = fpouts[8'h17];
    assign srswitches[00] = fpouts[8'h45];

    // front panel state
    reg incrdep;
    reg lastclock, lastcont, lastdep, lastexam, lastldad, laststrt;
    reg thisclock, thiscont, thisdep, thisexam, thisldad, thisstrt;
    reg[11:00] pcreg;

    always @(posedge CLOCK) begin

        // continuously update stop signal going to zynqlib.cc    
        fpinput[FPI_V_STOP] <= fpouts[8'h4F] | fpouts[8'h49];   // P_STEP,P_STOP
        
        // don't do anything if not enabled
        if (~ fpoutput[FPO_V_ENABLE]) begin
            state    <= IDLE;
            incrdep  <= 0;
            lastcont <= 0;
            lastdep  <= 0;
            lastexam <= 0;
            lastldad <= 0;
            laststrt <= 0;
            thiscont <= 0;
            thisdep  <= 0;
            thisexam <= 0;
            thisldad <= 0;
            thisstrt <= 0;
            fetreg   <= 0;
            defreg   <= 0;
            exereg   <= 0;
            brkreg   <= 0;
        end else if (~ fpoutput[FPO_V_STOPPED]) begin

            // processor is running, update front panel lights to match
            fetreg <= paddlrdc[22] | paddlrdd[05];                          // fetch1q,fetch2q
            defreg <= paddlrdd[07] | paddlrdd[08] | paddlrdd[09];           // defer1q,defer2q,defer3q
            exereg <= paddlrdd[10] | paddlrdd[12] | paddlrdd[17];           // exec1q,exec2q,exec3q
            brkreg <= paddlrdc[15];                                         // intak1q
            mareg  <= {                                                     // maq[nn]
                paddlrda[05], paddlrda[12], paddlrda[19], paddlrda[26],
                paddlrdb[02], paddlrdb[09], paddlrdb[16], paddlrdb[23],
                paddlrdb[30], paddlrdc[05], paddlrdc[12], paddlrdc[19] };
            mbreg  <= gpcompos[15:04];                                      // G_DATA
        end else begin

            // look for zynqlib.cc to drive FPO_CLOCK high
            if (~ lastclock & thisclock) begin

                case (state)

                    // processor is halted and doing front panel things
                IDLE:
                    begin

                        // CONT switch just released
                        if (lastcont & ~ thiscont) begin
                            fpinput[FPI_V_CONT] <= 1;
                            fpinput[FPI_V_JUMP] <= ~ fetreg & ~ defreg & ~ exereg & ~ brkreg;
                            fpinput[FPI_V_STRT] <= 0;
                            fpinput[11:00] <= pcreg;
                            incrdep <= 0;
                            state <= CONT;
                        end

                        // DEP switch just released
                        // maybe increment MA and tell ARM to write that location
                        else if (lastdep & ~ thisdep) begin
                            fpinput[11:00] <= mareg + incrdep;
                            fpinput[FPI_V_DEP] <= 1;
                            incrdep <= 0;
                            mareg   <= mareg + incrdep;
                            state   <= DEP;
                            fetreg  <= 0;
                            defreg  <= 0;
                            exereg  <= 0;
                            brkreg  <= 0;
                        end

                        // EXAM switch just released
                        // increment MA and tell ARM to read that memory location
                        else if (lastexam & ~ thisexam) begin
                            fpinput[11:00] <= mareg + 1;
                            fpinput[FPI_V_EXAM] <= 1;
                            incrdep <= 0;
                            mareg   <= mareg + 1;
                            state   <= EXAM;
                            fetreg  <= 0;
                            defreg  <= 0;
                            exereg  <= 0;
                            brkreg  <= 0;
                        end

                        // LDAD switch just released
                        // set the address in the MA and tell ARM to read the memory location
                        else if (lastldad & ~ thisldad) begin
                            fpinput[11:00] <= srswitches;
                            fpinput[FPI_V_EXAM] <= 1;
                            incrdep <= 0;
                            mareg   <= srswitches;
                            pcreg   <= srswitches;
                            state   <= EXAM;
                            fetreg  <= 0;
                            defreg  <= 0;
                            exereg  <= 0;
                            brkreg  <= 0;
                        end

                        // START switch just released
                        else if (laststrt & ~ thisstrt) begin
                            fpinput[FPI_V_CONT] <= 1;
                            fpinput[FPI_V_JUMP] <= ~ fetreg & ~ defreg & ~ exereg & ~ brkreg;
                            fpinput[FPI_V_STRT] <= 1;
                            fpinput[11:00] <= pcreg;
                            incrdep <= 0;
                            state   <= CONT;
                        end

                        // check momentary switch transitions
                        lastcont <= thiscont;
                        lastdep  <= thisdep;
                        lastexam <= thisexam;
                        lastldad <= thisldad;
                        laststrt <= thisstrt;
                        thiscont <= fpouts[8'h46];          // P_CONT
                        thisdep  <= fpouts[8'h3F];          // P_DEP
                        thisexam <= fpouts[8'h37];          // P_EXAM
                        thisldad <= fpouts[8'h47];          // P_LDAD
                        thisstrt <= fpouts[8'h48];          // P_STRT
                    end

                    // zyqlib.cc is sending us memory contents
                EXAM:
                    begin
                        fpinput[FPI_V_EXAM] <= 0;
                        mbreg <= fpoutput[11:00];
                        state <= IDLE;
                    end

                    // send memory contents to zynqlib.cc
                DEP:
                    begin
                        fpinput[FPI_V_DEP] <= 0;
                        fpinput[11:00] <= srswitches;
                        state <= IDLE;
                    end

                    // zynqlib.cc has seen us set FPI_CONT and is about to resume processing
                    // clear all the flags and wait for zynqlib.cc to tell us processor has stopped
                    // zynqlib.cc will not clock us until it stops again
                CONT:
                    begin
                        fpinput[FPI_V_CONT] <= 0;
                        fpinput[FPI_V_JUMP] <= 0;
                        fpinput[FPI_V_STRT] <= 0;
                        fpinput[FPI_V_DEP]  <= 0;
                        fpinput[FPI_V_EXAM] <= 0;
                        state <= IDLE;
                    end
                endcase
            end

            // check FPO_CLOCK clock transition
            lastclock <= thisclock;
            thisclock <= fpoutput[FPO_V_CLOCK];
        end
    end
endmodule
