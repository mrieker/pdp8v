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

// list of standard interconnection connectors
// define amask,bmask,cmask,dmask before including
// ...to give the input/output designation for each pin

// pin assignments must match ../driver/gpiolib.h, pindefs.h

    wire _ac_aluq;
    wire _ac_sc;
    wire  acq[11:00];
    wire  acqzero;
    wire _alu_add;
    wire _alu_and;
    wire _alua_m1;
    wire _alua_ma;
    wire  alua_mq0600;
    wire  alua_mq1107;
    wire  alua_pc0600;
    wire  alua_pc1107;
    wire  alub_1;
    wire _alub_ac;
    wire _alub_m1;
    wire _alucout;
    wire _aluq[11:00];
    wire  clok2;
    wire  defer1q;
    wire  defer2q;
    wire  defer3q;
    wire _dfrm;
    wire  exec1q;
    wire  exec2q;
    wire  exec3q;
    wire  fetch1q;
    wire  fetch2q;
    wire _grpa1q;
    wire  grpb_skip;
    wire  inc_axb;
    wire _intak;
    wire  intrq;
    wire  intak1q;
    wire  ioinst;
    wire  ioskp;
    wire  iot2q;
    wire  irq[11:09];
    wire _jump;
    wire _ln_wrt;
    wire _lnq;
    wire  lnq;
    wire _ma_aluq;
    wire _maq[11:00];
    wire  maq[11:00];
    wire  mql;
    wire  mq[11:00];
    wire _mread;
    wire _mwrite;
    wire _newlink;
    wire _pc_aluq;
    wire _pc_inc;
    wire  pcq[11:00];
    wire  reset;
    wire  tad3q;

    wire  __nc1, __nc2;

#define abody \
         acq[11],           /* 02 */ \
        _aluq[11],          /* 03 */ \
        _maq[11],           /* 04 */ \
         maq[11],           /* 05 */ \
         mq[11],            /* 06 */ \
         pcq[11],           /* 07 */ \
         irq[11],           /* 08 */ \
         acq[10],           /* 09 */ \
        _aluq[10],          /* 10 */ \
        _maq[10],           /* 11 */ \
         maq[10],           /* 12 */ \
         mq[10],            /* 13 */ \
         pcq[10],           /* 14 */ \
         irq[10],           /* 15 */ \
         acq[09],           /* 16 */ \
        _aluq[09],          /* 17 */ \
        _maq[09],           /* 18 */ \
         maq[09],           /* 19 */ \
         mq[09],            /* 20 */ \
         pcq[09],           /* 21 */ \
         irq[09],           /* 22 */ \
         acq[08],           /* 23 */ \
        _aluq[08],          /* 24 */ \
        _maq[08],           /* 25 */ \
         maq[08],           /* 26 */ \
         mq[08],            /* 27 */ \
         pcq[08],           /* 28 */ \
         __nc1,             /* 29 */ \
         acq[07],           /* 30 */ \
        _aluq[07],          /* 31 */ \
        _maq[07]            /* 32 */

#define bbody \
         maq[07],           /* 02 */ \
         mq[07],            /* 03 */ \
         pcq[07],           /* 04 */ \
         __nc2,             /* 05 */ \
         acq[06],           /* 06 */ \
        _aluq[06],          /* 07 */ \
        _maq[06],           /* 08 */ \
         maq[06],           /* 09 */ \
         mq[06],            /* 10 */ \
         pcq[06],           /* 11 */ \
        _jump,              /* 12 */ \
         acq[05],           /* 13 */ \
        _aluq[05],          /* 14 */ \
        _maq[05],           /* 15 */ \
         maq[05],           /* 16 */ \
         mq[05],            /* 17 */ \
         pcq[05],           /* 18 */ \
        _alub_m1,           /* 19 */ \
         acq[04],           /* 20 */ \
        _aluq[04],          /* 21 */ \
        _maq[04],           /* 22 */ \
         maq[04],           /* 23 */ \
         mq[04],            /* 24 */ \
         pcq[04],           /* 25 */ \
         acqzero,           /* 26 */ \
         acq[03],           /* 27 */ \
        _aluq[03],          /* 28 */ \
        _maq[03],           /* 29 */ \
         maq[03],           /* 30 */ \
         mq[03],            /* 31 */ \
         pcq[03]            /* 32 */

#define cbody \
         acq[02],           /* 02 */ \
        _aluq[02],          /* 03 */ \
        _maq[02],           /* 04 */ \
         maq[02],           /* 05 */ \
         mq[02],            /* 06 */ \
         pcq[02],           /* 07 */ \
        _ac_sc,             /* 08 */ \
         acq[01],           /* 09 */ \
        _aluq[01],          /* 10 */ \
        _maq[01],           /* 11 */ \
         maq[01],           /* 12 */ \
         mq[01],            /* 13 */ \
         pcq[01],           /* 14 */ \
         intak1q,           /* 15 */ \
         acq[00],           /* 16 */ \
        _aluq[00],          /* 17 */ \
        _maq[00],           /* 18 */ \
         maq[00],           /* 19 */ \
         mq[00],            /* 20 */ \
         pcq[00],           /* 21 */ \
         fetch1q,           /* 22 */ \
        _ac_aluq,           /* 23 */ \
        _alu_add,           /* 24 */ \
        _alu_and,           /* 25 */ \
        _alua_m1,           /* 26 */ \
        _alucout,           /* 27 */ \
        _alua_ma,           /* 28 */ \
         alua_mq0600,       /* 29 */ \
         alua_mq1107,       /* 30 */ \
         alua_pc0600,       /* 31 */ \
         alua_pc1107        /* 32 */

#define dbody \
         alub_1,            /* 02 */ \
        _alub_ac,           /* 03 */ \
         clok2,             /* 04 */ \
         fetch2q,           /* 05 */ \
        _grpa1q,            /* 06 */ \
         defer1q,           /* 07 */ \
         defer2q,           /* 08 */ \
         defer3q,           /* 09 */ \
         exec1q,            /* 10 */ \
         grpb_skip,         /* 11 */ \
         exec2q,            /* 12 */ \
        _dfrm,              /* 13 */ \
         inc_axb,           /* 14 */ \
        _intak,             /* 15 */ \
         intrq,             /* 16 */ \
         exec3q,            /* 17 */ \
         ioinst,            /* 18 */ \
         ioskp,             /* 19 */ \
         iot2q,             /* 20 */ \
        _ln_wrt,            /* 21 */ \
        _lnq,               /* 22 */ \
         lnq,               /* 23 */ \
        _ma_aluq,           /* 24 */ \
         mql,               /* 25 */ \
        _mread,             /* 26 */ \
        _mwrite,            /* 27 */ \
        _pc_aluq,           /* 28 */ \
        _pc_inc,            /* 29 */ \
         reset,             /* 30 */ \
        _newlink,           /* 31 */ \
         tad3q              /* 32 */

    acon: Conn amask (abody);

    pwr1: PwrConn VF1 VF1 GND GND VF2 VF2 ();

    bcon: Conn bmask (bbody);

    pwr2: PwrConn VPP GND VGG GND VCC VCC ();

    ccon: Conn cmask (cbody);

    pwr3: PwrConn VF3 VF3 GND GND VF4 VF4 ();

    dcon: Conn dmask (dbody);

#undef amask
#undef bmask
#undef cmask
#undef dmask
