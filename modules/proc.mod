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

// processor put together for simulation via proc.sim script
// the inputs are forceable in the simulation script
// must not contain any logic, not even an inverter,
// ...as it is modelling just wiring between the boards

// also used for DE0-nano implementation
// use 'make ../de0/proc-synth.v' create ../de0/proc-synth.v file
// ...then compile with qvartvs and load into DE0-nano

//  LEDS   = wired to DE0's leds
//  GP0OUT = wired to DE0's GPIO 0 as external LED outputs
//  PADDL{A,B,C,D} = read-only paddles

module proc
    (in CLOK2, in INTRQ, in IOSKP, in MQ[11:00], in MQL, in RESET, in QENA, in DENA,
    out _DFRM, out _JUMP, out _INTAK, out IOINST, out _MDL, out _MD[11:00], out _MREAD, out _MWRITE,
    out LEDS[7:0], out GP0OLO[31:00], out GP0OHI[1:0],
    out PADDLA[31:0], out PADDLB[31:0], out PADDLC[31:0], out PADDLD[31:0])
{
//            00000000011111111112222222222333
//            12345678901234567890123456789012
#define amask goooooooooooooooooooooooooooiooo
#define bmask goooiooooooooooooooooooooooooooo
#define cmask gooooooooooooooooooooooooooooooo
#define dmask gooooooooooooooooooooooooooooooo
#include "cons.inc"

    clok2 = CLOK2;
    intrq = INTRQ;
    ioskp = IOSKP;
    mq    = MQ;
    mql   = MQL;
    reset = RESET;

    _DFRM   = _dfrm;
    _JUMP   = _jump;
    _INTAK  = _intak;
    IOINST  = ioinst;
    _MDL    = _lnq;
    _MD     = _aluq;
    _MREAD  = _mread;
    _MWRITE = _mwrite;

    PADDLA[ 0] =  0;
    PADDLA[ 1] =  O2/acon;
    PADDLA[ 2] =  O3/acon;
    PADDLA[ 3] =  O4/acon;
    PADDLA[ 4] =  O5/acon;
    PADDLA[ 5] =  O6/acon;
    PADDLA[ 6] =  O7/acon;
    PADDLA[ 7] =  O8/acon;
    PADDLA[ 8] =  O9/acon;
    PADDLA[ 9] = O10/acon;
    PADDLA[10] = O11/acon;
    PADDLA[11] = O12/acon;
    PADDLA[12] = O13/acon;
    PADDLA[13] = O14/acon;
    PADDLA[14] = O15/acon;
    PADDLA[15] = O16/acon;
    PADDLA[16] = O17/acon;
    PADDLA[17] = O18/acon;
    PADDLA[18] = O19/acon;
    PADDLA[19] = O20/acon;
    PADDLA[20] = O21/acon;
    PADDLA[21] = O22/acon;
    PADDLA[22] = O23/acon;
    PADDLA[23] = O24/acon;
    PADDLA[24] = O25/acon;
    PADDLA[25] = O26/acon;
    PADDLA[26] = O27/acon;
    PADDLA[27] = O28/acon;
    PADDLA[28] =  1;    // __nc1
    PADDLA[29] = O30/acon;
    PADDLA[30] = O31/acon;
    PADDLA[31] = O32/acon;

    PADDLB[ 0] =  0;
    PADDLB[ 1] =  O2/bcon;
    PADDLB[ 2] =  O3/bcon;
    PADDLB[ 3] =  O4/bcon;
    PADDLB[ 4] =  1;    // __nc2
    PADDLB[ 5] =  O6/bcon;
    PADDLB[ 6] =  O7/bcon;
    PADDLB[ 7] =  O8/bcon;
    PADDLB[ 8] =  O9/bcon;
    PADDLB[ 9] = O10/bcon;
    PADDLB[10] = O11/bcon;
    PADDLB[11] = O12/bcon;
    PADDLB[12] = O13/bcon;
    PADDLB[13] = O14/bcon;
    PADDLB[14] = O15/bcon;
    PADDLB[15] = O16/bcon;
    PADDLB[16] = O17/bcon;
    PADDLB[17] = O18/bcon;
    PADDLB[18] = O19/bcon;
    PADDLB[19] = O20/bcon;
    PADDLB[20] = O21/bcon;
    PADDLB[21] = O22/bcon;
    PADDLB[22] = O23/bcon;
    PADDLB[23] = O24/bcon;
    PADDLB[24] = O25/bcon;
    PADDLB[25] = O26/bcon;
    PADDLB[26] = O27/bcon;
    PADDLB[27] = O28/bcon;
    PADDLB[28] = O29/bcon;
    PADDLB[29] = O30/bcon;
    PADDLB[30] = O31/bcon;
    PADDLB[31] = O32/bcon;

    PADDLC[ 0] =  0;
    PADDLC[ 1] =  O2/ccon;
    PADDLC[ 2] =  O3/ccon;
    PADDLC[ 3] =  O4/ccon;
    PADDLC[ 4] =  O5/ccon;
    PADDLC[ 5] =  O6/ccon;
    PADDLC[ 6] =  O7/ccon;
    PADDLC[ 7] =  O8/ccon;
    PADDLC[ 8] =  O9/ccon;
    PADDLC[ 9] = O10/ccon;
    PADDLC[10] = O11/ccon;
    PADDLC[11] = O12/ccon;
    PADDLC[12] = O13/ccon;
    PADDLC[13] = O14/ccon;
    PADDLC[14] = O15/ccon;
    PADDLC[15] = O16/ccon;
    PADDLC[16] = O17/ccon;
    PADDLC[17] = O18/ccon;
    PADDLC[18] = O19/ccon;
    PADDLC[19] = O20/ccon;
    PADDLC[20] = O21/ccon;
    PADDLC[21] = O22/ccon;
    PADDLC[22] = O23/ccon;
    PADDLC[23] = O24/ccon;
    PADDLC[24] = O25/ccon;
    PADDLC[25] = O26/ccon;
    PADDLC[26] = O27/ccon;
    PADDLC[27] = O28/ccon;
    PADDLC[28] = O29/ccon;
    PADDLC[29] = O30/ccon;
    PADDLC[30] = O31/ccon;
    PADDLC[31] = O32/ccon;

    PADDLD[ 0] =  0;
    PADDLD[ 1] =  O2/dcon;
    PADDLD[ 2] =  O3/dcon;
    PADDLD[ 3] =  O4/dcon;
    PADDLD[ 4] =  O5/dcon;
    PADDLD[ 5] =  O6/dcon;
    PADDLD[ 6] =  O7/dcon;
    PADDLD[ 7] =  O8/dcon;
    PADDLD[ 8] =  O9/dcon;
    PADDLD[ 9] = O10/dcon;
    PADDLD[10] = O11/dcon;
    PADDLD[11] = O12/dcon;
    PADDLD[12] = O13/dcon;
    PADDLD[13] = O14/dcon;
    PADDLD[14] = O15/dcon;
    PADDLD[15] = O16/dcon;
    PADDLD[16] = O17/dcon;
    PADDLD[17] = O18/dcon;
    PADDLD[18] = O19/dcon;
    PADDLD[19] = O20/dcon;
    PADDLD[20] = O21/dcon;
    PADDLD[21] = O22/dcon;
    PADDLD[22] = O23/dcon;
    PADDLD[23] = O24/dcon;
    PADDLD[24] = O25/dcon;
    PADDLD[25] = O26/dcon;
    PADDLD[26] = O27/dcon;
    PADDLD[27] = O28/dcon;
    PADDLD[28] = O29/dcon;
    PADDLD[29] = O30/dcon;
    PADDLD[30] = O31/dcon;
    PADDLD[31] = O32/dcon;

    acl: aclcirc (
        _ac_aluq, _ac_sc, acq[11:00], acqzero, _aluq[11:00], clok2, _grpa1q, grpb_skip,
        iot2q, _ln_wrt, _lnq, lnq, _maq[11:00], maq[11:00], mql,
        _newlink, reset, tad3q);

    alu: alucirc (
        acq[11:00], _alu_add, _alu_and, _alua_m1, _alua_ma, alua_mq0600, alua_mq1107,
        alua_pc0600, alua_pc1107, alub_1, _alub_ac, _alub_m1, _alucout, _aluq[11:00], _grpa1q,
        inc_axb, _lnq, lnq, _maq[11:00], maq[11:00], mq[11:00], _newlink, pcq[11:00]);

    ma: macirc (_aluq, clok2, _ma_aluq, _maq[11:00], maq[11:00], reset);

    pc: pccirc (_aluq, clok2, _pc_aluq, _pc_inc, pcq[11:00], reset);

    seq: seqcirc (
        _ac_aluq, _ac_sc, acqzero, _alu_add, _alu_and, _alua_m1, _alua_ma, alua_mq0600, alua_mq1107,
        alua_pc0600, alua_pc1107, alub_1, _alub_ac, _alub_m1, _alucout, clok2, _grpa1q,
        grpb_skip, _dfrm, _jump, inc_axb, _intak, intrq, intak1q, ioinst, ioskp,
        iot2q, _ln_wrt, _ma_aluq, _maq[11:00], maq[11:00], mq[11:00], _mread, _mwrite, _pc_aluq,
        _pc_inc, reset, tad3q,
        fetch1q, fetch2q, defer1q, defer2q, defer3q, exec1q, exec2q, exec3q, irq[11:09]);

    // DE0s on-board LEDs

    LEDS[0] = fetch1q; // fetch1q | fetch2q;
    LEDS[1] = fetch2q; // defer1q | defer2q | defer3q;
    LEDS[2] = defer1q; //  exec1q |  exec2q |  exec3q;
    LEDS[3] = defer2q; // intak1q;
    LEDS[4] = defer3q; // 1;
    LEDS[5] = exec1q;  // fetch1q | defer1q | exec1q;
    LEDS[6] = exec2q;  // fetch2q | defer2q | exec2q;
    LEDS[7] = exec3q;  //           defer3q | exec3q;

    // external DE0-LEDS board plugged into GPIO 0 connector

    GP0OHI[01] = pcq[11];/*GPIO[33]*/           GP0OHI[00] = acq[11];/*GPIO[32]*/
                        GP0OLO[31] = pcq[05];                       GP0OLO[30] = acq[05];
    GP0OLO[29] = pcq[10];                       GP0OLO[28] = acq[10];
                        GP0OLO[27] = pcq[04];                       GP0OLO[26] = acq[04];
    GP0OLO[25] = pcq[09];                       GP0OLO[24] = acq[09];

                        GP0OLO[23] = pcq[03];                       GP0OLO[22] = acq[03];
    GP0OLO[21] = 0;                             GP0OLO[20] = 0;
                        GP0OLO[19] = 0;                             GP0OLO[18] = 0;
    GP0OLO[17] = pcq[08];                       GP0OLO[16] = acq[08];
                        GP0OLO[15] = pcq[02];                       GP0OLO[14] = acq[02];
    GP0OLO[13] = pcq[07];                       GP0OLO[12] = acq[07];
                        GP0OLO[11] = pcq[01];                       GP0OLO[10] = acq[01];
    GP0OLO[09] = pcq[06];                       GP0OLO[08] = acq[06];

                        GP0OLO[07] = pcq[00];                       GP0OLO[06] = acq[00];
    GP0OLO[05] = irq[11];                       GP0OLO[04] = lnq;
                        GP0OLO[03] = 0;                             GP0OLO[02] = 0;
    GP0OLO[01] = irq[10];
                        GP0OLO[00] = irq[09];
}
