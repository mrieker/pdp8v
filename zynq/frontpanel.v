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
//  scl   = i2c bus for MCP23017s
//  sdai  = i2c data to MCP23017s from pipanel/i2clib.cc
//  CLOCK = zynq 100MHz clock
//  RESET = zynq power-on reset
//  paddlrd{a,b,c,d} = spying on paddle output pins going to raspictl
//  boardena = board enable bits from raspictl/zynqlib.cc
//  fpoutput = front panel bits from raspictl/zynqlib.cc
//  gpcompos = spying on composite gpio pins going to raspictl
// outputs:
//  TRIGGR  = trigger singal to G_TRIG to zynq arm
//  DEBUGS  = debug outputs for ILA
//  fpinput = front panel bits going to raspictl/zynqlib.cc
//  sdao    = i2c data from MCP23017s going to pipanel/i2clib.cc
module frontpanel (scl, sdai, sdao,
        CLOCK, RESET, TRIGGR, DEBUGS,
        paddlrda, paddlrdb, paddlrdc, paddlrdd,
        fpinput, fpoutput, gpcompos);
    input scl, sdai;
    output sdao;
    input CLOCK, RESET;
    output TRIGGR;
    output[31:00] DEBUGS;
    input[31:00] paddlrda, paddlrdb, paddlrdc, paddlrdd;
    output[31:00] fpinput;
    input[31:00] fpoutput, gpcompos;

    // data is in bits [11:00]
    localparam FPO_V_ENABLE  = 12;  // raspictl/zynqlib.cc is enabling us
    localparam FPO_V_CLOCK   = 13;  // raspictl/zynqlib.cc is clocking us
    localparam FPO_V_STOPPED = 14;  // raspictl/zynqlib.cc has stopped processing

    // signals to/from pipanel.cc, ie, simulated PDP-8/L front panel, via i2c bus
    wire[79:00] fplites;    // signals going to front panel, ie, lights
    wire[79:00] fpswchs;    // signals coming from front panel, ie, switches
    wire sdao0, sdao1, sdao2, sdao3, sdao4;
    mcp23017 mcp0 (3'b000, CLOCK, RESET, scl, sdai, sdao0, fplites[07:00], fplites[15:08], fpswchs[07:00], fpswchs[15:08]);
    mcp23017 mcp1 (3'b001, CLOCK, RESET, scl, sdai, sdao1, fplites[23:16], fplites[31:24], fpswchs[23:16], fpswchs[31:24]);
    mcp23017 mcp2 (3'b010, CLOCK, RESET, scl, sdai, sdao2, fplites[39:32], fplites[47:40], fpswchs[39:32], fpswchs[47:40]);
    mcp23017 mcp3 (3'b011, CLOCK, RESET, scl, sdai, sdao3, fplites[55:48], fplites[63:56], fpswchs[55:48], fpswchs[63:56]);
    mcp23017 mcp4 (3'b100, CLOCK, RESET, scl, sdai, sdao4, fplites[71:64], fplites[79:72], fpswchs[71:64], fpswchs[79:72]);
    assign sdao = sdao0 & sdao1 & sdao2 & sdao3 & sdao4;

    // front panel state
    reg[2:0] state;
    localparam[2:0] IDLE  = 0;   // processor stopped, waiting for button to be pressed
    localparam[2:0] EXAM1 = 1;   // exam button pressed, waiting for memory contents
    localparam[2:0] CONT  = 2;   // waiting for zynqlib.cc to see continue/start switch
    localparam[2:0] LDAD  = 3;   // ldad button pressed, waiting for memory contents
    localparam[2:0] DEP   = 4;   // dep button pressed, writing memory contents
    localparam[2:0] STOP  = 5;   // processor has just stopped
    localparam[2:0] EXAM2 = 6;

    // data is in bits [11:00]
    reg[31:00] fpinreg;
    localparam FPI_V_DEP  = 12;     // requesting raspictl/zynqlib.cc to write memory
    localparam FPI_V_EXAM = 13;     // requesting raspictl/zynqlib.cc to read memory
    localparam FPI_V_JUMP = 14;     // requesting raspictl/zynqlib.cc to load program counter
    localparam FPI_V_CONT = 15;     // requesting raspictl/zynqlib.cc to continue processing
    localparam FPI_V_STRT = 16;     // requesting raspictl/zynqlib.cc to reset i/o
    localparam FPI_V_STOP = 17;     // requesting raspictl/zynqlib.cc to stop processing
    localparam FPI_V_STATE = 18;    // state for debugging
    localparam FPI_S_STATE = 3;
    assign fpinput[16:00] = fpinreg[16:00];
    assign fpinput[FPI_V_STOP] = ~ fpswchs[8'h4F] | ~ fpswchs[8'h49];  // P_STEP,P_STOP
    assign fpinput[20:18] = state;
    assign fpinput[31:21] = 0;

    // drive PDP-8/L panel lights
    reg[11:00] mareg, mbreg;
    reg fetreg, defreg, exereg, brkreg;

    assign TRIGGR = fpoutput[FPO_V_CLOCK];
    assign DEBUGS[31:29] = state;
    assign DEBUGS[28:17] = fpoutput[11:00];
    assign DEBUGS[16:05] = mbreg[11:00];
    assign DEBUGS[04:00] = fpinreg[16:12];

    // - all light bulbs are active low
    assign fplites[8'h03] = ~ paddlrda[02-1];   // P_ACnn = acq[11-nn]
    assign fplites[8'h05] = ~ paddlrda[09-1];
    assign fplites[8'h1E] = ~ paddlrda[16-1];
    assign fplites[8'h08] = ~ paddlrda[23-1];
    assign fplites[8'h1B] = ~ paddlrda[30-1];
    assign fplites[8'h14] = ~ paddlrdb[06-1];
    assign fplites[8'h19] = ~ paddlrdb[13-1];
    assign fplites[8'h16] = ~ paddlrdb[20-1];
    assign fplites[8'h2C] = ~ paddlrdb[27-1];
    assign fplites[8'h2B] = ~ paddlrdc[02-1];
    assign fplites[8'h3D] = ~ paddlrdc[09-1];
    assign fplites[8'h3E] = ~ paddlrdc[16-1];

    assign fplites[8'h32] = ~ paddlrda[08-1];   // P_IRnn = irq[11-nn]
    assign fplites[8'h36] = ~ paddlrda[15-1];
    assign fplites[8'h40] = ~ paddlrda[22-1];

    assign fplites[8'h0C] = ~ paddlrdd[23-1];   // P_LINK = lnq

    assign fplites[8'h06] = ~ mareg[11];        // P_MAnn = mareg[11-nn]
    assign fplites[8'h09] = ~ mareg[10];
    assign fplites[8'h11] = ~ mareg[09];
    assign fplites[8'h1D] = ~ mareg[08];
    assign fplites[8'h1C] = ~ mareg[07];
    assign fplites[8'h15] = ~ mareg[06];
    assign fplites[8'h20] = ~ mareg[05];
    assign fplites[8'h21] = ~ mareg[04];
    assign fplites[8'h2D] = ~ mareg[03];
    assign fplites[8'h28] = ~ mareg[02];
    assign fplites[8'h3C] = ~ mareg[01];
    assign fplites[8'h30] = ~ mareg[00];

    assign fplites[8'h04] = ~ mbreg[11];        // P_MBnn = mbreg[11-nn]
    assign fplites[8'h0A] = ~ mbreg[10];
    assign fplites[8'h10] = ~ mbreg[09];
    assign fplites[8'h12] = ~ mbreg[08];
    assign fplites[8'h13] = ~ mbreg[07];
    assign fplites[8'h1A] = ~ mbreg[06];
    assign fplites[8'h18] = ~ mbreg[05];
    assign fplites[8'h22] = ~ mbreg[04];
    assign fplites[8'h2E] = ~ mbreg[03];
    assign fplites[8'h29] = ~ mbreg[02];
    assign fplites[8'h3B] = ~ mbreg[01];
    assign fplites[8'h26] = ~ mbreg[00];

    assign fplites[8'h0B] = ~ 0;                // P_EMA
    assign fplites[8'h31] = ~ fetreg;           // P_FET
    assign fplites[8'h33] = ~ 0;                // P_ION
    assign fplites[8'h34] = ~ 0;                // P_PARE
    assign fplites[8'h35] = ~ exereg;           // P_EXE
    assign fplites[8'h41] = ~ 0;                // P_PRTE
    assign fplites[8'h42] = fpoutput[FPO_V_STOPPED];  // P_RUN
    assign fplites[8'h43] = ~ 0;                // P_WCT
    assign fplites[8'h44] = ~ brkreg;           // P_BRK
    assign fplites[8'h4B] = ~ 0;                // P_CAD
    assign fplites[8'h4E] = ~ defreg;           // P_DEF

    // sense PDP-8/L panel switches (active high)
    wire [11:00] srswitches;
    assign srswitches[11] = fpswchs[8'h07];
    assign srswitches[10] = fpswchs[8'h02];
    assign srswitches[09] = fpswchs[8'h01];
    assign srswitches[08] = fpswchs[8'h00];
    assign srswitches[07] = fpswchs[8'h0D];
    assign srswitches[06] = fpswchs[8'h27];
    assign srswitches[05] = fpswchs[8'h25];
    assign srswitches[04] = fpswchs[8'h24];
    assign srswitches[03] = fpswchs[8'h23];
    assign srswitches[02] = fpswchs[8'h2F];
    assign srswitches[01] = fpswchs[8'h17];
    assign srswitches[00] = fpswchs[8'h45];

    // front panel state
    reg incrdep, increxm;
    reg lastclock, lastcont, lastdep, lastexam, lastldad, laststrt;
    reg thisclock, thiscont, thisdep, thisexam, thisldad, thisstrt;
    reg[11:00] pcreg;

    always @(posedge CLOCK or posedge RESET) begin

        // don't do anything if not enabled
        if (RESET | ~ fpoutput[FPO_V_ENABLE]) begin
            state    <= IDLE;
            fpinreg  <= 0;
            incrdep  <= 0;
            increxm  <= 0;
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
        end else begin

            if (~ fpoutput[FPO_V_STOPPED]) begin

                // processor is running, update front panel lights to match
                fetreg <= paddlrdc[22-1] | paddlrdd[05-1];                      // fetch1q,fetch2q
                defreg <= paddlrdd[07-1] | paddlrdd[08-1] | paddlrdd[09-1];     // defer1q,defer2q,defer3q
                exereg <= paddlrdd[10-1] | paddlrdd[12-1] | paddlrdd[17-1];     // exec1q,exec2q,exec3q
                brkreg <= paddlrdc[15-1];                                       // intak1q
                mareg  <= {                                                     // maq[nn]
                    paddlrda[05-1], paddlrda[12-1], paddlrda[19-1], paddlrda[26-1],
                    paddlrdb[02-1], paddlrdb[09-1], paddlrdb[16-1], paddlrdb[23-1],
                    paddlrdb[30-1], paddlrdc[05-1], paddlrdc[12-1], paddlrdc[19-1] };
                mbreg  <= gpcompos[15:04];                                      // G_DATA
            end else begin

                // look for zynqlib.cc to drive FPO_CLOCK high
                // it does this whenever the processor is halted so the panel can do things
                if (~ lastclock & thisclock) begin

                    case (state)

                        // processor is halted waiting for button to be pressed
                    IDLE:
                        begin

                            // CONT switch just released
                            if (lastcont & ~ thiscont) begin
                                fpinreg[FPI_V_CONT] <= 1;
                                fpinreg[FPI_V_JUMP] <= ~ fetreg & ~ defreg & ~ exereg & ~ brkreg;
                                fpinreg[FPI_V_STRT] <= 0;
                                fpinreg[11:00] <= pcreg;
                                incrdep <= 0;
                                increxm <= 0;
                                state <= CONT;
                            end

                            // DEP switch just released
                            // maybe increment MA and tell ARM to write that location
                            else if (lastdep & ~ thisdep) begin
                                fpinreg[11:00] <= mareg + incrdep;
                                fpinreg[FPI_V_DEP] <= 1;
                                mareg   <= mareg + incrdep;
                                incrdep <= 1;
                                increxm <= 0;
                                state   <= DEP;
                                fetreg  <= 0;
                                defreg  <= 0;
                                exereg  <= 0;
                                brkreg  <= 0;
                            end

                            // EXAM switch just released
                            // increment MA and tell ARM to read that memory location
                            else if (lastexam & ~ thisexam) begin
                                fpinreg[11:00] <= mareg + increxm;
                                fpinreg[FPI_V_EXAM] <= 1;
                                mareg   <= mareg + increxm;
                                incrdep <= 0;
                                increxm <= 1;
                                state   <= EXAM1;
                                fetreg  <= 0;
                                defreg  <= 0;
                                exereg  <= 0;
                                brkreg  <= 0;
                            end

                            // LDAD switch just released
                            // set the address in the MA and tell ARM to read the memory location
                            else if (lastldad & ~ thisldad) begin
                                fpinreg[11:00] <= srswitches;
                                fpinreg[FPI_V_EXAM] <= 1;
                                incrdep <= 0;
                                increxm <= 0;
                                mareg   <= srswitches;
                                pcreg   <= srswitches;
                                state   <= EXAM1;
                                fetreg  <= 0;
                                defreg  <= 0;
                                exereg  <= 0;
                                brkreg  <= 0;
                            end

                            // START switch just released
                            else if (laststrt & ~ thisstrt) begin
                                fpinreg[FPI_V_CONT] <= 1;
                                fpinreg[FPI_V_JUMP] <= ~ fetreg & ~ defreg & ~ exereg & ~ brkreg;
                                fpinreg[FPI_V_STRT] <= 1;
                                fpinreg[11:00] <= pcreg;
                                incrdep <= 0;
                                increxm <= 0;
                                state   <= CONT;
                            end

                            // check momentary switch transitions (switches are active low)
                            lastcont <= thiscont;
                            lastdep  <= thisdep;
                            lastexam <= thisexam;
                            lastldad <= thisldad;
                            laststrt <= thisstrt;
                            thiscont <= ~ fpswchs[8'h46];       // P_CONT
                            thisdep  <= ~ fpswchs[8'h3F];       // P_DEP
                            thisexam <= ~ fpswchs[8'h37];       // P_EXAM
                            thisldad <= ~ fpswchs[8'h47];       // P_LDAD
                            thisstrt <= ~ fpswchs[8'h48];       // P_STRT
                        end

                        // zyqlib.cc is sending us memory contents
                    EXAM1:
                        begin
                            fpinreg[FPI_V_EXAM] <= 0;
                            state <= EXAM2;
                        end
                    EXAM2:
                        begin
                            mbreg <= fpoutput[11:00];
                            state <= IDLE;
                        end

                        // send memory contents to zynqlib.cc
                    DEP:
                        begin
                            fpinreg[FPI_V_DEP] <= 0;
                            fpinreg[11:00] <= srswitches;
                            mbreg <= srswitches;
                            state <= IDLE;
                        end

                        // zynqlib.cc has seen us set FPI_CONT and is about to resume processing
                        // clear all the flags and wait for zynqlib.cc to tell us processor has stopped
                        // zynqlib.cc will not clock us until it stops again
                    CONT:
                        begin
                            fpinreg[FPI_V_CONT] <= 0;
                            fpinreg[FPI_V_JUMP] <= 0;
                            fpinreg[FPI_V_STRT] <= 0;
                            fpinreg[FPI_V_DEP]  <= 0;
                            fpinreg[FPI_V_EXAM] <= 0;
                            state <= IDLE;
                        end
                    endcase
                end

                // check FPO_CLOCK clock transition
                lastclock <= thisclock;
                thisclock <= fpoutput[FPO_V_CLOCK];
            end
        end
    end
endmodule
