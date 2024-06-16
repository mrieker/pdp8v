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

// raspberry pi interface

module rpicirc (
    in _aluq[11:00], in _dfrm, in _jump, in _intak, in ioinst, in _lnq, in _mread, in _mwrite,
    out clok2, out intrq, out ioskp, out mql, out mq[11:00], out reset)
{
    // all magic done in RasPiModule.java
    rpi: RasPi (
              _MD:_aluq,
            _DFRM:_dfrm,
            _JUMP:_jump,
             _IAK:_intak,
             IOIN:ioinst,
             _MDL:_lnq,
             _MRD:_mread,
             _MWT:_mwrite,
              CLK:clok2,
              IRQ:intrq,
              IOS:ioskp,
              MQL:mql,
               MQ:mq,
              RES:reset);
}

module rpi ()
{
//            00000000011111111112222222222333
//            12345678901234567890123456789012
#define amask giiiioiiiiiioiiiiiioiiiiiioiiiii
#define bmask gioiiiiiioiiiiiioiiiiiioiiiiiioi
#define cmask giiiioiiiiiioiiiiiioiiiiiiiiiiii
#define dmask giioiiiiiiiiiiioiioiiiiioiiiioii
#include "cons.inc"

    rpicirc: rpicirc (
        _aluq[11:00], _dfrm, _jump, _intak, ioinst, _lnq, _mread, _mwrite,
        clok2, intrq, ioskp, mql, mq[11:00], reset);

    aclbl:  Label 4 "AC" ();
    malbl:  Label 4 "MA" ();
    pclbl:  Label 4 "PC" ();
    irlbl:  Label 3 "IR" ();
    fetlbl: Label 3 "FETCH" ();
    dfrlbl: Label 3 "DEFER" ();
    exelbl: Label 3 "EXEC"  ();
    iaklbl: Label 3 "INTAK" ();
    lnklbl: Label 3 "LINK"  ();

    LNQ:         BufLED    (lnq);
    ACQ:         BufLED 12 (acq);
    MAQ:         BufLED 12 (maq);
    PCQ:         BufLED 12 (pcq);

    IRQ11:       BufLED    (irq[11]);
    IRQ10:       BufLED    (irq[10]);
    IRQ09:       BufLED    (irq[09]);
    FETCH1Q:     BufLED    (fetch1q);
    FETCH2Q:     BufLED    (fetch2q);
    DEFER1Q:     BufLED    (defer1q);
    DEFER2Q:     BufLED    (defer2q);
    DEFER3Q:     BufLED    (defer3q);
    EXEC1Q:      BufLED    (exec1q);
    EXEC2Q:      BufLED    (exec2q);
    EXEC3Q:      BufLED    (exec3q);
    INTAK:       BufLED    (intak1q);
}

module rpi_testpads
    (in CLOK2, in INTRQ, in IOSKP, in MQ[11:00], in MQL, in RESET, in QENA, in DENA,
    out _DFRM, out _JUMP, out _INTAK, out IOINST, out _MDL, out _MD[11:00], out _MREAD, out _MWRITE)
{
//            00000000011111111112222222222333
//            12345678901234567890123456789012
#define amask giiiioiiiiiioiiiiiioiiiiiioiiiii
#define bmask gioiiiiiioiiiiiioiiiiiioiiiiiioi
#define cmask giiiioiiiiiioiiiiiioiiiiiiiiiiii
#define dmask giioiiiiiiiiiiioiioiiiiioiiiioii
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
}
