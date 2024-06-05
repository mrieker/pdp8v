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

// wire all the boards together

// inputs:
//  CLOCK = FPGA clock (100MHz)
//  paddlwr{a,b,c,d} = paddle values written by ARM
//  boardena = board enable bits
//    <0> = 1 : include ACL board; 0 : ACL board outputs must be provided by paddlwr{a,b,c,d}
//    <1> = 1 : include ALU board; 0 : ALU board outputs must be provided by paddlwr{a,b,c,d}
//    <2> = 1 : include MA  board; 0 : MA  board outputs must be provided by paddlwr{a,b,c,d}
//    <3> = 1 : include PC  board; 0 : PC  board outputs must be provided by paddlwr{a,b,c,d}
//    <4> = 1 : include RPI board; 0 : RPI board outputs must be provided by paddlwr{a,b,c,d}
//    <5> = 1 : include SEQ board; 0 : SEQ board outputs must be provided by paddlwr{a,b,c,d}
//  gpoutput = GPIO bits written by ARM
// outputs:
//  TRIGGR = G_TRIG GPIO bit written by ARM (debugging)
//  DEBUGS = debug signals
//  paddlrd{a,b,c,d} = paddle values can be read by ARM
//  gpinput = GPIO bits can be read by ARM
//  gpcompos = GPIO read bits combined with write bits
//             ...can be read by ARM to show just what RasPI would have
//  nto = number triodes 'one' - a couple 100MHz clock cycles behind
//  ntt = number total triodes - a constant for a given set of boards

module backplane (CLOCK, TRIGGR, DEBUGS,
        paddlrda, paddlrdb, paddlrdc, paddlrdd,
        paddlwra, paddlwrb, paddlwrc, paddlwrd,
        boardena, gpinput, gpoutput, gpcompos,
        nto, ntt);
    input CLOCK;
    output TRIGGR;
    output[13:00] DEBUGS;
    output[31:00] paddlrda, paddlrdb, paddlrdc, paddlrdd;
    input[31:00] paddlwra, paddlwrb, paddlwrc, paddlwrd;
    input[5:0] boardena;
    output[31:00] gpinput, gpcompos;
    input[31:00] gpoutput;

    wire[12:00] denadata, qenadata;

    // board enables
    wire aclena, aluena, maena, pcena, rpiena, seqena;

    // bus_<signal> = what would be on the backplane in real computer, available as input to boards
    //                driven by board if board is enabled, driven by paddle if board is disabled
    // <board>_<signal> = what is being output by board, whether it is enabled or disabled
    // pad_<signal> = what is being output on the paddles for the given signal

    // signals output by the acl board
    wire[11:00] bus_acq       , acl_acq       , pad_acq      ;
    wire        bus_acqzero   , acl_acqzero   , pad_acqzero  ;
    wire        bus_grpb_skip , acl_grpb_skip , pad_grpb_skip;
    wire        bus__lnq      , acl__lnq      , pad__lnq     ;
    wire        bus_lnq       , acl_lnq       , pad_lnq      ;

    // signals output by the alu board
    wire        bus__alucout , alu__alucout , pad__alucout;
    wire[11:00] bus__aluq    , alu__aluq    , pad__aluq   ;
    wire        bus__newlink , alu__newlink , pad__newlink;

    // signals output by the ma board
    wire[11:00] bus__maq , ma__maq , pad__maq;
    wire[11:00] bus_maq  , ma_maq  , pad_maq ;

    // signals output by the pc board
    wire[11:00] bus_pcq , pc_pcq , pad_pcq;

    // signals output by the rpi board
    wire        bus_clok2 , rpi_clok2 , pad_clok2;
    wire        bus_intrq , rpi_intrq , pad_intrq;
    wire        bus_ioskp , rpi_ioskp , pad_ioskp;
    wire        bus_mql   , rpi_mql   , pad_mql  ;
    wire[11:00] bus_mq    , rpi_mq    , pad_mq   ;
    wire        bus_reset , rpi_reset , pad_reset;

    // signals output by the seq board
    wire        bus__ac_aluq    , seq__ac_aluq    , pad__ac_aluq   ;
    wire        bus__ac_sc      , seq__ac_sc      , pad__ac_sc     ;
    wire        bus__alu_add    , seq__alu_add    , pad__alu_add   ;
    wire        bus__alu_and    , seq__alu_and    , pad__alu_and   ;
    wire        bus__alua_m1    , seq__alua_m1    , pad__alua_m1   ;
    wire        bus__alua_ma    , seq__alua_ma    , pad__alua_ma   ;
    wire        bus_alua_mq0600 , seq_alua_mq0600 , pad_alua_mq0600;
    wire        bus_alua_mq1107 , seq_alua_mq1107 , pad_alua_mq1107;
    wire        bus_alua_pc0600 , seq_alua_pc0600 , pad_alua_pc0600;
    wire        bus_alua_pc1107 , seq_alua_pc1107 , pad_alua_pc1107;
    wire        bus_alub_1      , seq_alub_1      , pad_alub_1     ;
    wire        bus__alub_ac    , seq__alub_ac    , pad__alub_ac   ;
    wire        bus__alub_m1    , seq__alub_m1    , pad__alub_m1   ;
    wire        bus__grpa1q     , seq__grpa1q     , pad__grpa1q    ;
    wire        bus__dfrm       , seq__dfrm       , pad__dfrm      ;
    wire        bus__jump       , seq__jump       , pad__jump      ;
    wire        bus_inc_axb     , seq_inc_axb     , pad_inc_axb    ;
    wire        bus__intak      , seq__intak      , pad__intak     ;
    wire        bus_intak1q     , seq_intak1q     , pad_intak1q    ;
    wire        bus_ioinst      , seq_ioinst      , pad_ioinst     ;
    wire        bus_iot2q       , seq_iot2q       , pad_iot2q      ;
    wire        bus__ln_wrt     , seq__ln_wrt     , pad__ln_wrt    ;
    wire        bus__ma_aluq    , seq__ma_aluq    , pad__ma_aluq   ;
    wire        bus__mread      , seq__mread      , pad__mread     ;
    wire        bus__mwrite     , seq__mwrite     , pad__mwrite    ;
    wire        bus__pc_aluq    , seq__pc_aluq    , pad__pc_aluq   ;
    wire        bus__pc_inc     , seq__pc_inc     , pad__pc_inc    ;
    wire        bus_tad3q       , seq_tad3q       , pad_tad3q      ;
    wire        bus_fetch1q     , seq_fetch1q     , pad_fetch1q    ;
    wire        bus_fetch2q     , seq_fetch2q     , pad_fetch2q    ;
    wire        bus_defer1q     , seq_defer1q     , pad_defer1q    ;
    wire        bus_defer2q     , seq_defer2q     , pad_defer2q    ;
    wire        bus_defer3q     , seq_defer3q     , pad_defer3q    ;
    wire        bus_exec1q      , seq_exec1q      , pad_exec1q     ;
    wire        bus_exec2q      , seq_exec2q      , pad_exec2q     ;
    wire        bus_exec3q      , seq_exec3q      , pad_exec3q     ;
    wire[11:09] bus_irq         , seq_irq         , pad_irq        ;

    assign DEBUGS = 14'b0;

    // count total number of gates outputting one = total number of triodes off
    output reg[9:0] nto, ntt;
    wire[9:0] aclnto, alunto, manto, pcnto, seqnto;
    wire[9:0] aclntt, aluntt, mantt, pcntt, seqntt;
    always @(posedge CLOCK) begin
        nto <= (aclena ? aclnto : 0) + (aluena ? alunto : 0) + (maena ? manto : 0) + (pcena ? pcnto : 0) + (seqena ? seqnto : 0);
        ntt <= (aclena ? aclntt : 0) + (aluena ? aluntt : 0) + (maena ? mantt : 0) + (pcena ? pcntt : 0) + (seqena ? seqntt : 0);
    end

    // paddles reading the bus bits

    assign paddlrda[01-1] = 1'b0            ;
    assign paddlrda[02-1] = bus_acq[11]     ;
    assign paddlrda[03-1] = bus__aluq[11]   ;
    assign paddlrda[04-1] = bus__maq[11]    ;
    assign paddlrda[05-1] = bus_maq[11]     ;
    assign paddlrda[06-1] = bus_mq[11]      ;
    assign paddlrda[07-1] = bus_pcq[11]     ;
    assign paddlrda[08-1] = bus_irq[11]     ;
    assign paddlrda[09-1] = bus_acq[10]     ;
    assign paddlrda[10-1] = bus__aluq[10]   ;
    assign paddlrda[11-1] = bus__maq[10]    ;
    assign paddlrda[12-1] = bus_maq[10]     ;
    assign paddlrda[13-1] = bus_mq[10]      ;
    assign paddlrda[14-1] = bus_pcq[10]     ;
    assign paddlrda[15-1] = bus_irq[10]     ;
    assign paddlrda[16-1] = bus_acq[09]     ;
    assign paddlrda[17-1] = bus__aluq[09]   ;
    assign paddlrda[18-1] = bus__maq[09]    ;
    assign paddlrda[19-1] = bus_maq[09]     ;
    assign paddlrda[20-1] = bus_mq[09]      ;
    assign paddlrda[21-1] = bus_pcq[09]     ;
    assign paddlrda[22-1] = bus_irq[09]     ;
    assign paddlrda[23-1] = bus_acq[08]     ;
    assign paddlrda[24-1] = bus__aluq[08]   ;
    assign paddlrda[25-1] = bus__maq[08]    ;
    assign paddlrda[26-1] = bus_maq[08]     ;
    assign paddlrda[27-1] = bus_mq[08]      ;
    assign paddlrda[28-1] = bus_pcq[08]     ;
    assign paddlrda[30-1] = bus_acq[07]     ;
    assign paddlrda[31-1] = bus__aluq[07]   ;
    assign paddlrda[32-1] = bus__maq[07]    ;

    assign paddlrdb[01-1] = 1'b0            ;
    assign paddlrdb[02-1] = bus_maq[07]     ;
    assign paddlrdb[03-1] = bus_mq[07]      ;
    assign paddlrdb[04-1] = bus_pcq[07]     ;
    assign paddlrdb[06-1] = bus_acq[06]     ;
    assign paddlrdb[07-1] = bus__aluq[06]   ;
    assign paddlrdb[08-1] = bus__maq[06]    ;
    assign paddlrdb[09-1] = bus_maq[06]     ;
    assign paddlrdb[10-1] = bus_mq[06]      ;
    assign paddlrdb[11-1] = bus_pcq[06]     ;
    assign paddlrdb[12-1] = bus__jump       ;
    assign paddlrdb[13-1] = bus_acq[05]     ;
    assign paddlrdb[14-1] = bus__aluq[05]   ;
    assign paddlrdb[15-1] = bus__maq[05]    ;
    assign paddlrdb[16-1] = bus_maq[05]     ;
    assign paddlrdb[17-1] = bus_mq[05]      ;
    assign paddlrdb[18-1] = bus_pcq[05]     ;
    assign paddlrdb[19-1] = bus__alub_m1    ;
    assign paddlrdb[20-1] = bus_acq[04]     ;
    assign paddlrdb[21-1] = bus__aluq[04]   ;
    assign paddlrdb[22-1] = bus__maq[04]    ;
    assign paddlrdb[23-1] = bus_maq[04]     ;
    assign paddlrdb[24-1] = bus_mq[04]      ;
    assign paddlrdb[25-1] = bus_pcq[04]     ;
    assign paddlrdb[26-1] = bus_acqzero     ;
    assign paddlrdb[27-1] = bus_acq[03]     ;
    assign paddlrdb[28-1] = bus__aluq[03]   ;
    assign paddlrdb[29-1] = bus__maq[03]    ;
    assign paddlrdb[30-1] = bus_maq[03]     ;
    assign paddlrdb[31-1] = bus_mq[03]      ;
    assign paddlrdb[32-1] = bus_pcq[03]     ;

    assign paddlrdc[01-1] = 1'b0            ;
    assign paddlrdc[02-1] = bus_acq[02]     ;
    assign paddlrdc[03-1] = bus__aluq[02]   ;
    assign paddlrdc[04-1] = bus__maq[02]    ;
    assign paddlrdc[05-1] = bus_maq[02]     ;
    assign paddlrdc[06-1] = bus_mq[02]      ;
    assign paddlrdc[07-1] = bus_pcq[02]     ;
    assign paddlrdc[08-1] = bus__ac_sc      ;
    assign paddlrdc[09-1] = bus_acq[01]     ;
    assign paddlrdc[10-1] = bus__aluq[01]   ;
    assign paddlrdc[11-1] = bus__maq[01]    ;
    assign paddlrdc[12-1] = bus_maq[01]     ;
    assign paddlrdc[13-1] = bus_mq[01]      ;
    assign paddlrdc[14-1] = bus_pcq[01]     ;
    assign paddlrdc[15-1] = bus_intak1q     ;
    assign paddlrdc[16-1] = bus_acq[00]     ;
    assign paddlrdc[17-1] = bus__aluq[00]   ;
    assign paddlrdc[18-1] = bus__maq[00]    ;
    assign paddlrdc[19-1] = bus_maq[00]     ;
    assign paddlrdc[20-1] = bus_mq[00]      ;
    assign paddlrdc[21-1] = bus_pcq[00]     ;
    assign paddlrdc[22-1] = bus_fetch1q     ;
    assign paddlrdc[23-1] = bus__ac_aluq    ;
    assign paddlrdc[24-1] = bus__alu_add    ;
    assign paddlrdc[25-1] = bus__alu_and    ;
    assign paddlrdc[26-1] = bus__alua_m1    ;
    assign paddlrdc[27-1] = bus__alucout    ;
    assign paddlrdc[28-1] = bus__alua_ma    ;
    assign paddlrdc[29-1] = bus_alua_mq0600 ;
    assign paddlrdc[30-1] = bus_alua_mq1107 ;
    assign paddlrdc[31-1] = bus_alua_pc0600 ;
    assign paddlrdc[32-1] = bus_alua_pc1107 ;

    assign paddlrdd[01-1] = 1'b0            ;
    assign paddlrdd[02-1] = bus_alub_1      ;
    assign paddlrdd[03-1] = bus__alub_ac    ;
    assign paddlrdd[04-1] = bus_clok2       ;
    assign paddlrdd[05-1] = bus_fetch2q     ;
    assign paddlrdd[06-1] = bus__grpa1q     ;
    assign paddlrdd[07-1] = bus_defer1q     ;
    assign paddlrdd[08-1] = bus_defer2q     ;
    assign paddlrdd[09-1] = bus_defer3q     ;
    assign paddlrdd[10-1] = bus_exec1q      ;
    assign paddlrdd[11-1] = bus_grpb_skip   ;
    assign paddlrdd[12-1] = bus_exec2q      ;
    assign paddlrdd[13-1] = bus__dfrm       ;
    assign paddlrdd[14-1] = bus_inc_axb     ;
    assign paddlrdd[15-1] = bus__intak      ;
    assign paddlrdd[16-1] = bus_intrq       ;
    assign paddlrdd[17-1] = bus_exec3q      ;
    assign paddlrdd[18-1] = bus_ioinst      ;
    assign paddlrdd[19-1] = bus_ioskp       ;
    assign paddlrdd[20-1] = bus_iot2q       ;
    assign paddlrdd[21-1] = bus__ln_wrt     ;
    assign paddlrdd[22-1] = bus__lnq        ;
    assign paddlrdd[23-1] = bus_lnq         ;
    assign paddlrdd[24-1] = bus__ma_aluq    ;
    assign paddlrdd[25-1] = bus_mql         ;
    assign paddlrdd[26-1] = bus__mread      ;
    assign paddlrdd[27-1] = bus__mwrite     ;
    assign paddlrdd[28-1] = bus__pc_aluq    ;
    assign paddlrdd[29-1] = bus__pc_inc     ;
    assign paddlrdd[30-1] = bus_reset       ;
    assign paddlrdd[31-1] = bus__newlink    ;
    assign paddlrdd[32-1] = bus_tad3q       ;

    // paddle bits written by the zynq ps [raspictl zynqlib]
    // to make up for missing boards when board is disabled via boardena bit

    assign pad_acq[11]     = paddlwra[02-1];
    assign pad__aluq[11]   = paddlwra[03-1];
    assign pad__maq[11]    = paddlwra[04-1];
    assign pad_maq[11]     = paddlwra[05-1];
    assign pad_mq[11]      = paddlwra[06-1];
    assign pad_pcq[11]     = paddlwra[07-1];
    assign pad_irq[11]     = paddlwra[08-1];
    assign pad_acq[10]     = paddlwra[09-1];
    assign pad__aluq[10]   = paddlwra[10-1];
    assign pad__maq[10]    = paddlwra[11-1];
    assign pad_maq[10]     = paddlwra[12-1];
    assign pad_mq[10]      = paddlwra[13-1];
    assign pad_pcq[10]     = paddlwra[14-1];
    assign pad_irq[10]     = paddlwra[15-1];
    assign pad_acq[09]     = paddlwra[16-1];
    assign pad__aluq[09]   = paddlwra[17-1];
    assign pad__maq[09]    = paddlwra[18-1];
    assign pad_maq[09]     = paddlwra[19-1];
    assign pad_mq[09]      = paddlwra[20-1];
    assign pad_pcq[09]     = paddlwra[21-1];
    assign pad_irq[09]     = paddlwra[22-1];
    assign pad_acq[08]     = paddlwra[23-1];
    assign pad__aluq[08]   = paddlwra[24-1];
    assign pad__maq[08]    = paddlwra[25-1];
    assign pad_maq[08]     = paddlwra[26-1];
    assign pad_mq[08]      = paddlwra[27-1];
    assign pad_pcq[08]     = paddlwra[28-1];
    assign pad_acq[07]     = paddlwra[30-1];
    assign pad__aluq[07]   = paddlwra[31-1];
    assign pad__maq[07]    = paddlwra[32-1];

    assign pad_maq[07]     = paddlwrb[02-1];
    assign pad_mq[07]      = paddlwrb[03-1];
    assign pad_pcq[07]     = paddlwrb[04-1];
    assign pad_acq[06]     = paddlwrb[06-1];
    assign pad__aluq[06]   = paddlwrb[07-1];
    assign pad__maq[06]    = paddlwrb[08-1];
    assign pad_maq[06]     = paddlwrb[09-1];
    assign pad_mq[06]      = paddlwrb[10-1];
    assign pad_pcq[06]     = paddlwrb[11-1];
    assign pad__jump       = paddlwrb[12-1];
    assign pad_acq[05]     = paddlwrb[13-1];
    assign pad__aluq[05]   = paddlwrb[14-1];
    assign pad__maq[05]    = paddlwrb[15-1];
    assign pad_maq[05]     = paddlwrb[16-1];
    assign pad_mq[05]      = paddlwrb[17-1];
    assign pad_pcq[05]     = paddlwrb[18-1];
    assign pad__alub_m1    = paddlwrb[19-1];
    assign pad_acq[04]     = paddlwrb[20-1];
    assign pad__aluq[04]   = paddlwrb[21-1];
    assign pad__maq[04]    = paddlwrb[22-1];
    assign pad_maq[04]     = paddlwrb[23-1];
    assign pad_mq[04]      = paddlwrb[24-1];
    assign pad_pcq[04]     = paddlwrb[25-1];
    assign pad_acqzero     = paddlwrb[26-1];
    assign pad_acq[03]     = paddlwrb[27-1];
    assign pad__aluq[03]   = paddlwrb[28-1];
    assign pad__maq[03]    = paddlwrb[29-1];
    assign pad_maq[03]     = paddlwrb[30-1];
    assign pad_mq[03]      = paddlwrb[31-1];
    assign pad_pcq[03]     = paddlwrb[32-1];

    assign pad_acq[02]     = paddlwrc[02-1];
    assign pad__aluq[02]   = paddlwrc[03-1];
    assign pad__maq[02]    = paddlwrc[04-1];
    assign pad_maq[02]     = paddlwrc[05-1];
    assign pad_mq[02]      = paddlwrc[06-1];
    assign pad_pcq[02]     = paddlwrc[07-1];
    assign pad__ac_sc      = paddlwrc[08-1];
    assign pad_acq[01]     = paddlwrc[09-1];
    assign pad__aluq[01]   = paddlwrc[10-1];
    assign pad__maq[01]    = paddlwrc[11-1];
    assign pad_maq[01]     = paddlwrc[12-1];
    assign pad_mq[01]      = paddlwrc[13-1];
    assign pad_pcq[01]     = paddlwrc[14-1];
    assign pad_intak1q     = paddlwrc[15-1];
    assign pad_acq[00]     = paddlwrc[16-1];
    assign pad__aluq[00]   = paddlwrc[17-1];
    assign pad__maq[00]    = paddlwrc[18-1];
    assign pad_maq[00]     = paddlwrc[19-1];
    assign pad_mq[00]      = paddlwrc[20-1];
    assign pad_pcq[00]     = paddlwrc[21-1];
    assign pad_fetch1q     = paddlwrc[22-1];
    assign pad__ac_aluq    = paddlwrc[23-1];
    assign pad__alu_add    = paddlwrc[24-1];
    assign pad__alu_and    = paddlwrc[25-1];
    assign pad__alua_m1    = paddlwrc[26-1];
    assign pad__alucout    = paddlwrc[27-1];
    assign pad__alua_ma    = paddlwrc[28-1];
    assign pad_alua_mq0600 = paddlwrc[29-1];
    assign pad_alua_mq1107 = paddlwrc[30-1];
    assign pad_alua_pc0600 = paddlwrc[31-1];
    assign pad_alua_pc1107 = paddlwrc[32-1];

    assign pad_alub_1      = paddlwrd[02-1];
    assign pad__alub_ac    = paddlwrd[03-1];
    assign pad_clok2       = paddlwrd[04-1];
    assign pad_fetch2q     = paddlwrd[05-1];
    assign pad__grpa1q     = paddlwrd[06-1];
    assign pad_defer1q     = paddlwrd[07-1];
    assign pad_defer2q     = paddlwrd[08-1];
    assign pad_defer3q     = paddlwrd[09-1];
    assign pad_exec1q      = paddlwrd[10-1];
    assign pad_grpb_skip   = paddlwrd[11-1];
    assign pad_exec2q      = paddlwrd[12-1];
    assign pad__dfrm       = paddlwrd[13-1];
    assign pad_inc_axb     = paddlwrd[14-1];
    assign pad__intak      = paddlwrd[15-1];
    assign pad_intrq       = paddlwrd[16-1];
    assign pad_exec3q      = paddlwrd[17-1];
    assign pad_ioinst      = paddlwrd[18-1];
    assign pad_ioskp       = paddlwrd[19-1];
    assign pad_iot2q       = paddlwrd[20-1];
    assign pad__ln_wrt     = paddlwrd[21-1];
    assign pad__lnq        = paddlwrd[22-1];
    assign pad_lnq         = paddlwrd[23-1];
    assign pad__ma_aluq    = paddlwrd[24-1];
    assign pad_mql         = paddlwrd[25-1];
    assign pad__mread      = paddlwrd[26-1];
    assign pad__mwrite     = paddlwrd[27-1];
    assign pad__pc_aluq    = paddlwrd[28-1];
    assign pad__pc_inc     = paddlwrd[29-1];
    assign pad_reset       = paddlwrd[30-1];
    assign pad__newlink    = paddlwrd[31-1];
    assign pad_tad3q       = paddlwrd[32-1];

    // split board enable register into its various bits
    // when bit is set, gate the corresponding module outputs to the bus
    // when bit is clear, gate the corresponding paddle outputs to the bus
    assign aclena = boardena[0];
    assign aluena = boardena[1];
    assign maena  = boardena[2];
    assign pcena  = boardena[3];
    assign rpiena = boardena[4];
    assign seqena = boardena[5];

    // for bus signals output by the acl board, select either the module or the paddles
    assign bus_acq       = aclena ? acl_acq       : pad_acq;
    assign bus_acqzero   = aclena ? acl_acqzero   : pad_acqzero;
    assign bus_grpb_skip = aclena ? acl_grpb_skip : pad_grpb_skip;
    assign bus__lnq      = aclena ? acl__lnq      : pad__lnq;
    assign bus_lnq       = aclena ? acl_lnq       : pad_lnq;

    // for bus signals output by the alu board, select either the module or the paddles
    assign bus__alucout = aluena ? alu__alucout : pad__alucout;
    assign bus__aluq    = aluena ? alu__aluq    : pad__aluq;
    assign bus__newlink = aluena ? alu__newlink : pad__newlink;

    // for bus signals output by the ma board, select either the module or the paddles
    assign bus__maq  = maena ? ma__maq  : pad__maq;
    assign bus_maq   = maena ? ma_maq   : pad_maq;

    // for bus signals output by the pc board, select either the module or the paddles
    assign bus_pcq = pcena ? pc_pcq : pad_pcq;

    // for bus signals output by the rpi board, select either the gpio signals or the paddles
    assign bus_clok2 = rpiena ? rpi_clok2 : pad_clok2;
    assign bus_intrq = rpiena ? rpi_intrq : pad_intrq;
    assign bus_ioskp = rpiena ? rpi_ioskp : pad_ioskp;
    assign bus_mql   = rpiena ? rpi_mql   : pad_mql;
    assign bus_mq    = rpiena ? rpi_mq    : pad_mq;
    assign bus_reset = rpiena ? rpi_reset : pad_reset;

    // for bus signals output by the seq board, select either the module or the paddles
    assign bus__ac_aluq    = seqena ? seq__ac_aluq    : pad__ac_aluq;
    assign bus__ac_sc      = seqena ? seq__ac_sc      : pad__ac_sc;
    assign bus__alu_add    = seqena ? seq__alu_add    : pad__alu_add;
    assign bus__alu_and    = seqena ? seq__alu_and    : pad__alu_and;
    assign bus__alua_m1    = seqena ? seq__alua_m1    : pad__alua_m1;
    assign bus__alua_ma    = seqena ? seq__alua_ma    : pad__alua_ma;
    assign bus_alua_mq0600 = seqena ? seq_alua_mq0600 : pad_alua_mq0600;
    assign bus_alua_mq1107 = seqena ? seq_alua_mq1107 : pad_alua_mq1107;
    assign bus_alua_pc0600 = seqena ? seq_alua_pc0600 : pad_alua_pc0600;
    assign bus_alua_pc1107 = seqena ? seq_alua_pc1107 : pad_alua_pc1107;
    assign bus_alub_1      = seqena ? seq_alub_1      : pad_alub_1;
    assign bus__alub_ac    = seqena ? seq__alub_ac    : pad__alub_ac;
    assign bus__alub_m1    = seqena ? seq__alub_m1    : pad__alub_m1;
    assign bus__grpa1q     = seqena ? seq__grpa1q     : pad__grpa1q;
    assign bus__dfrm       = seqena ? seq__dfrm       : pad__dfrm;
    assign bus__jump       = seqena ? seq__jump       : pad__jump;
    assign bus_inc_axb     = seqena ? seq_inc_axb     : pad_inc_axb;
    assign bus__intak      = seqena ? seq__intak      : pad__intak;
    assign bus_intak1q     = seqena ? seq_intak1q     : pad_intak1q;
    assign bus_ioinst      = seqena ? seq_ioinst      : pad_ioinst;
    assign bus_iot2q       = seqena ? seq_iot2q       : pad_iot2q;
    assign bus__ln_wrt     = seqena ? seq__ln_wrt     : pad__ln_wrt;
    assign bus__ma_aluq    = seqena ? seq__ma_aluq    : pad__ma_aluq;
    assign bus__mread      = seqena ? seq__mread      : pad__mread;
    assign bus__mwrite     = seqena ? seq__mwrite     : pad__mwrite;
    assign bus__pc_aluq    = seqena ? seq__pc_aluq    : pad__pc_aluq;
    assign bus__pc_inc     = seqena ? seq__pc_inc     : pad__pc_inc;
    assign bus_tad3q       = seqena ? seq_tad3q       : pad_tad3q;
    assign bus_fetch1q     = seqena ? seq_fetch1q     : pad_fetch1q;
    assign bus_fetch2q     = seqena ? seq_fetch2q     : pad_fetch2q;
    assign bus_defer1q     = seqena ? seq_defer1q     : pad_defer1q;
    assign bus_defer2q     = seqena ? seq_defer2q     : pad_defer2q;
    assign bus_defer3q     = seqena ? seq_defer3q     : pad_defer3q;
    assign bus_exec1q      = seqena ? seq_exec1q      : pad_exec1q;
    assign bus_exec2q      = seqena ? seq_exec2q      : pad_exec2q;
    assign bus_exec3q      = seqena ? seq_exec3q      : pad_exec3q;
    assign bus_irq         = seqena ? seq_irq         : pad_irq;

    // instantiate the modules
    // for input pins, use the bus signals
    // for output pins, use the module-specific signal

    aclcirc_nto aclinst (
        .uclk (CLOCK),
        ._ac_aluq    (bus__ac_aluq),
        ._ac_sc      (bus__ac_sc),
        .acq         (acl_acq),
        .acqzero     (acl_acqzero),
        ._aluq       (bus__aluq),
        .clok2       (bus_clok2),
        ._grpa1q     (bus__grpa1q),
        .grpb_skip   (acl_grpb_skip),
        .iot2q       (bus_iot2q),
        ._ln_wrt     (bus__ln_wrt),
        ._lnq        (acl__lnq),
        .lnq         (acl_lnq),
        ._maq        (bus__maq),
        .maq         (bus_maq),
        .mql         (bus_mql),
        ._newlink    (bus__newlink),
        .reset       (bus_reset),
        .tad3q       (bus_tad3q),
        .nto         (aclnto),
        .ntt         (aclntt)
    );

    alucirc_nto aluinst (
        .uclk (CLOCK),
        .acq         (bus_acq),
        ._alu_add    (bus__alu_add),
        ._alu_and    (bus__alu_and),
        ._alua_m1    (bus__alua_m1),
        ._alua_ma    (bus__alua_ma),
        .alua_mq0600 (bus_alua_mq0600),
        .alua_mq1107 (bus_alua_mq1107),
        .alua_pc0600 (bus_alua_pc0600),
        .alua_pc1107 (bus_alua_pc1107),
        .alub_1      (bus_alub_1),
        ._alub_ac    (bus__alub_ac),
        ._alub_m1    (bus__alub_m1),
        ._alucout    (alu__alucout),
        ._aluq       (alu__aluq),
        ._grpa1q     (bus__grpa1q),
        .inc_axb     (bus_inc_axb),
        ._lnq        (bus__lnq),
        .lnq         (bus_lnq),
        ._maq        (bus__maq),
        .maq         (bus_maq),
        .mq          (bus_mq),
        ._newlink    (alu__newlink),
        .pcq         (bus_pcq),
        .nto         (alunto),
        .ntt         (aluntt)
    );

    macirc_nto mainst (
        .uclk (CLOCK),
        ._aluq       (bus__aluq),
        .clok2       (bus_clok2),
        ._ma_aluq    (bus__ma_aluq),
        ._maq        (ma__maq),
        .maq         (ma_maq),
        .reset       (bus_reset),
        .nto         (manto),
        .ntt         (mantt)
    );

    rpicirc rpiinst (
        .gpinput     (gpinput),
        .gpcompos    (gpcompos),
        .gpoutput    (gpoutput),
        ._aluq       (bus__aluq),
        ._lnq        (bus__lnq),
        ._jump       (bus__jump),
        .ioinst      (bus_ioinst),
        ._dfrm       (bus__dfrm),
        ._mread      (bus__mread),
        ._mwrite     (bus__mwrite),
        ._intak      (bus__intak),
        .clok2       (rpi_clok2),
        .reset       (rpi_reset),
        .mq          (rpi_mq),
        .mql         (rpi_mql),
        .ioskp       (rpi_ioskp),
        .intrq       (rpi_intrq),
        .TRIGGR      (TRIGGR)
    );

    pccirc_nto pcinst (
        .uclk (CLOCK),
        ._aluq       (bus__aluq),
        .clok2       (bus_clok2),
        ._pc_aluq    (bus__pc_aluq),
        ._pc_inc     (bus__pc_inc),
        .pcq         (pc_pcq),
        .reset       (bus_reset),
        .nto         (pcnto),
        .ntt         (pcntt)
    );

    seqcirc_nto seqinst (
        .uclk (CLOCK),
        ._ac_aluq    (seq__ac_aluq),
        ._ac_sc      (seq__ac_sc),
        .acqzero     (bus_acqzero),
        ._alu_add    (seq__alu_add),
        ._alu_and    (seq__alu_and),
        ._alua_m1    (seq__alua_m1),
        ._alua_ma    (seq__alua_ma),
        .alua_mq0600 (seq_alua_mq0600),
        .alua_mq1107 (seq_alua_mq1107),
        .alua_pc0600 (seq_alua_pc0600),
        .alua_pc1107 (seq_alua_pc1107),
        .alub_1      (seq_alub_1),
        ._alub_ac    (seq__alub_ac),
        ._alub_m1    (seq__alub_m1),
        ._alucout    (bus__alucout),
        .clok2       (bus_clok2),
        ._grpa1q     (seq__grpa1q),
        .grpb_skip   (bus_grpb_skip),
        ._dfrm       (seq__dfrm),
        ._jump       (seq__jump),
        .inc_axb     (seq_inc_axb),
        ._intak      (seq__intak),
        .intrq       (bus_intrq),
        .intak1q     (seq_intak1q),
        .ioinst      (seq_ioinst),
        .ioskp       (bus_ioskp),
        .iot2q       (seq_iot2q),
        ._ln_wrt     (seq__ln_wrt),
        ._ma_aluq    (seq__ma_aluq),
        ._maq        (bus__maq),
        .maq         (bus_maq),
        .mq          (bus_mq),
        ._mread      (seq__mread),
        ._mwrite     (seq__mwrite),
        ._pc_aluq    (seq__pc_aluq),
        ._pc_inc     (seq__pc_inc),
        .reset       (bus_reset),
        .tad3q       (seq_tad3q),
        .fetch1q     (seq_fetch1q),
        .fetch2q     (seq_fetch2q),
        .defer1q     (seq_defer1q),
        .defer2q     (seq_defer2q),
        .defer3q     (seq_defer3q),
        .exec1q      (seq_exec1q),
        .exec2q      (seq_exec2q),
        .exec3q      (seq_exec3q),
        .irq         (seq_irq),
        .nto         (seqnto),
        .ntt         (seqntt)
    );
endmodule

// RPI (raspberry pi) circuit board
// it is just a bunch of level-converter transistors
// ...with no logic other than to switch bi-directional gpio bus
//  inputs:
//   gpoutput = signals sent from gpio to the tubes
//   _aluq,_lnq,_jmp,ioinst,_dfrm,_mread,_mwrite,_intak = signals coming from backplane
//  outputs:
//   gpinput = signals sent from tubes to the gpio
//   gpcompos = mixture of gpinput & gpoutput, just like would be read by RasPI from tubes
//   clok2,reset,mq,mql,ioskp,intrq = signals going to backplane
//   TRIGGR = trigger signal for debugging
module rpicirc (gpinput, gpcompos, gpoutput,
    _aluq, _lnq, _jump, ioinst, _dfrm, _mread, _mwrite, _intak,
    clok2, reset, mq, mql, ioskp, intrq,
    TRIGGR);

    output[31:00] gpinput, gpcompos;
    input[31:00] gpoutput;

    input[11:00] _aluq;
    input _lnq, _jump, ioinst, _dfrm, _mread, _mwrite, _intak;
    output clok2, reset, mql, ioskp, intrq, TRIGGR;
    output[11:00] mq;

    // read gpio pins - they read signals from the bus
    assign gpinput[03:00] = 4'b0000;
    assign gpinput[15:04] = ~ _aluq;      // pin isn't in G_REVIS
    assign gpinput[16]    = ~ _lnq;       // pin isn't in G_REVIS
    assign gpinput[21:17] = 5'b00000;
    assign gpinput[22]    = ~ _jump;      // pin isn't in G_REVIS
    assign gpinput[23]    = ~ ioinst;     // pin is in G_REVIS
    assign gpinput[24]    = ~ _dfrm;      // pin isn't in G_REVIS
    assign gpinput[25]    = ~ _mread;     // pin isn't in G_REVIS
    assign gpinput[26]    = ~ _mwrite;    // pin isn't in G_REVIS
    assign gpinput[27]    = ~ _intak;     // pin isn't in G_REVIS
    assign gpinput[31:28] = 4'b0000;

    // write gpio pins - they write signals to the bus only when rpi board is enabled
    assign clok2  = ~ gpoutput[02];       // pin is in G_REVOS
    assign reset  = ~ gpoutput[03];       // pin is in G_REVOS
    assign mq     = ~ gpoutput[15:04];    // pins are in G_REVOS
    assign mql    = ~ gpoutput[16];       // pin is in G_REVOS
    assign ioskp  = ~ gpoutput[17];       // pin is in G_REVOS
    assign TRIGGR =   gpoutput[18];       // trigger scope
    wire   qena   =   gpoutput[19];       // pin isn't in G_REVOS
    assign intrq  = ~ gpoutput[20];       // pin is in G_REVOS
    wire   dena   =   gpoutput[21];       // pin isn't in G_REVOS

    // composite readback - gpio connector just as would be seen on raspi gpio connector plugged into tubes
    wire[12:00] denadata = dena ? gpinput [16:04] : 13'b0000000000000;  // tubes->raspi enabled for link,data pins
    wire[12:00] qenadata = qena ? gpoutput[16:04] : 13'b0000000000000;  // raspi->tubes enabled for link,data pins

    assign gpcompos[0]     = gpoutput[0];           // debugging
    assign gpcompos[1]     = 1'b0;
    assign gpcompos[03:02] = gpoutput[3:2];         // CLOCK,RESET always raspi->tubes
    assign gpcompos[16:04] = denadata | qenadata;   // LINK,DATA bi-directional
    assign gpcompos[21:17] = gpoutput[21:17];       // other raspi->tubes signals
    assign gpcompos[27:22] = gpinput[27:22];        // tubes->raspi signals
    assign gpcompos[31:28] = 4'b0000;
endmodule
