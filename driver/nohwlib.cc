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

// simulate processor via shadow
// fastest simulation but doesn't verify modules

#include "abcd.h"
#include "gpiolib.h"
#include "shadow.h"

NohwLib::NohwLib ()
{
    libname = "nohwlib";
}

void NohwLib::open ()
{ }

void NohwLib::close ()
{ }

void NohwLib::halfcycle ()
{ }

// read what gpio connector should be at end of cycle
//  cycle as given by shadow
//  assumes writegpio() has been called mid-cycle
uint32_t NohwLib::readgpio ()
{
    ABCD abcdvals = calcabcd ();
    uint32_t value = gpiowritten & G_OUTS;
    if (  abcdvals.ioinst)  value |= G_IOIN;
    if (! abcdvals._jump)   value |= G_JUMP;
    if (! abcdvals._dfrm)   value |= G_DFRM;
    if (! abcdvals._intak)  value |= G_IAK;
    if (! abcdvals._mread)  value |= G_READ;
    if (! abcdvals._mwrite) value |= G_WRITE;
    if (value & G_DENA) {
        value |= abcdvals._lnq ? 0 : G_LINK;
        value |= (abcdvals._aluq ^ 07777) * G_DATA0;
    }
    if (value & G_QENA) {
        value |= gpiowritten & (G_LINK | G_DATA);
    }
    return value;
}

// save what is being written to gpio connector
void NohwLib::writegpio (bool wdata, uint32_t value)
{
    gpiowritten = (value & ~ G_QENA & ~ G_DENA) | (wdata ? G_QENA : G_DENA);
}

bool NohwLib::haspads ()
{
    return true;
}

// read what paddles should be at end of cycle
//  cycle as given by shadow
//  assumes writegpio() has been called mid-cycle
void NohwLib::readpads (uint32_t *pinss)
{
    ABCD abcdvals = calcabcd ();

    abcdvals.encode ();

    for (int i = 0; i < NPADS; i ++) {
        pinss[i] = abcdvals.cons[i];
    }
}

void NohwLib::writepads (uint32_t const *masks, uint32_t const *pinss)
{
    ABORT ();
}

// calculate what backplane signals should be at end of cycle
// from shadow state and what was written to gpio connector mid-cycle
ABCD NohwLib::calcabcd ()
{
    ABCD abcdvals;

    // signals from raspi gpio
    abcdvals.mq  = (gpiowritten & G_QENA) ? (gpiowritten & G_DATA) / G_DATA0 : 0;
    abcdvals.mql = (gpiowritten & G_QENA) ? (gpiowritten & G_LINK) != 0 : false;

    abcdvals.ioskp = (gpiowritten & G_IOS)   != 0;
    abcdvals.clok2 = (gpiowritten & G_CLOCK) != 0;
    abcdvals.intrq = (gpiowritten & G_IRQ)   != 0;
    abcdvals.reset = (gpiowritten & G_RESET) != 0;

    // decode state
    abcdvals.fetch1q = (shadow.r.state == Shadow::FETCH1);
    abcdvals.fetch2q = (shadow.r.state == Shadow::FETCH2);
    abcdvals.defer1q = (shadow.r.state == Shadow::DEFER1);
    abcdvals.defer2q = (shadow.r.state == Shadow::DEFER2);
    abcdvals.defer3q = (shadow.r.state == Shadow::DEFER3);
    abcdvals.exec1q  = ((shadow.r.state & 15) == Shadow::EXEC1);
    abcdvals.exec2q  = ((shadow.r.state & 15) == Shadow::EXEC2);
    abcdvals.exec3q  = ((shadow.r.state & 15) == Shadow::EXEC3);
    abcdvals.intak1q = (shadow.r.state == Shadow::INTAK1);

    abcdvals.iot2q   = (shadow.r.state == Shadow::IOT2);
    abcdvals.tad3q   = (shadow.r.state == Shadow::TAD3);
    abcdvals._grpa1q = (shadow.r.state != Shadow::GRPA1);

    // control signals going to raspi gpio
    // (data and link are always _aluq and _lnq)
    abcdvals._jump   = ! ((shadow.r.state == Shadow::JMS1) || (shadow.r.state == Shadow::JMP1));
    abcdvals._dfrm   = ! (abcdvals.exec1q && ((shadow.r.ir & 04400) == 00400));
    abcdvals._intak  = ! abcdvals.intak1q;
    abcdvals.ioinst  = shadow.r.state == Shadow::IOT1;
    abcdvals._mread  = ! (abcdvals.fetch1q || abcdvals.defer1q || (shadow.r.state == Shadow::ARITH1) || (shadow.r.state == Shadow::ISZ1) || ((shadow.r.state == Shadow::JMP1) && ! abcdvals.intrq) || ((shadow.r.state == Shadow::JMS3) && ! abcdvals.intrq) || ((shadow.r.state == Shadow::GRPB1) && ! abcdvals.intrq));
    abcdvals._mwrite = ! ((abcdvals.defer1q && ((shadow.r.ma & 07770) == 00010)) || (shadow.r.state == Shadow::ISZ1) || (shadow.r.state == Shadow::DCA1) || (shadow.r.state == Shadow::JMS1) || (shadow.r.state == Shadow::INTAK1));

    // get register contents
    abcdvals.acq  = shadow.r.ac;
    abcdvals.irq  = shadow.r.ir;
    abcdvals.lnq  = shadow.r.link;
    abcdvals._lnq = ! shadow.r.link;
    abcdvals.maq  = shadow.r.ma;
    abcdvals._maq = shadow.r.ma ^ 07777;
    abcdvals.pcq  = shadow.r.pc;

    // compute skip condition
    abcdvals.acqzero = (shadow.r.ac == 0);
    abcdvals.grpb_skip = false;
    if (shadow.r.ma & 00100) {   // ac neg
        abcdvals.grpb_skip |= (shadow.r.ac & 04000) != 0;
    }
    if (shadow.r.ma & 00040) {   // ac zero
        abcdvals.grpb_skip |= abcdvals.acqzero;
    }
    if (shadow.r.ma & 00020) {   // link set
        abcdvals.grpb_skip |= shadow.r.link;
    }
    if (shadow.r.ma & 00010) {   // reverse sense
        abcdvals.grpb_skip = ! abcdvals.grpb_skip;
    }

    // select alu operands and function
    abcdvals._alua_m1 = ! ((shadow.r.state == Shadow::DCA2) || (shadow.r.state == Shadow::IOT1) || (! abcdvals._grpa1q && (shadow.r.ir & 00040)));
    abcdvals._alua_ma = ! (abcdvals.defer1q || (shadow.r.state == Shadow::DEFER3) || (shadow.r.state == Shadow::ARITH1) || (shadow.r.state == Shadow::ISZ1) || (shadow.r.state == Shadow::DCA1) || (shadow.r.state == Shadow::JMS1) || (shadow.r.state == Shadow::JMP1) || abcdvals.tad3q || (shadow.r.state == Shadow::JMS3) || (shadow.r.state == Shadow::ISZ3));
    abcdvals.alua_mq0600 = (shadow.r.state == Shadow::FETCH2) || (shadow.r.state == Shadow::DEFER2) || (shadow.r.state == Shadow::AND2) || (shadow.r.state == Shadow::TAD2) || (shadow.r.state == Shadow::ISZ2) || abcdvals.iot2q;
    abcdvals.alua_mq1107 = ((shadow.r.state == Shadow::FETCH2) && ((shadow.r.ir & 06000) == 06000)) || (shadow.r.state == Shadow::DEFER2) || (shadow.r.state == Shadow::AND2) || (shadow.r.state == Shadow::TAD2) || (shadow.r.state == Shadow::ISZ2) || abcdvals.iot2q;
    abcdvals.alua_pc0600 = abcdvals.fetch1q || (shadow.r.state == Shadow::JMS2) || (shadow.r.state == Shadow::GRPB1);
    abcdvals.alua_pc1107 = abcdvals.fetch1q || ((shadow.r.state == Shadow::FETCH2) && ((shadow.r.ir & 06000) != 06000) && (abcdvals.mq & 00200)) || (shadow.r.state == Shadow::JMS2) || (shadow.r.state == Shadow::GRPB1);

    abcdvals._alub_m1 = ! (abcdvals.fetch1q || (shadow.r.state == Shadow::FETCH2) || abcdvals.defer1q || (shadow.r.state == Shadow::DEFER2) || (shadow.r.state == Shadow::JMP1) || (shadow.r.state == Shadow::JMS1) || (shadow.r.state == Shadow::JMS2) || (shadow.r.state == Shadow::ARITH1) || (shadow.r.state == Shadow::TAD2) || (shadow.r.state == Shadow::DCA1) || (shadow.r.state == Shadow::ISZ1) || (shadow.r.state == Shadow::ISZ2) || abcdvals.iot2q || ((shadow.r.state == Shadow::AND2) && (shadow.r.ir & 01000)));
    abcdvals._alub_ac = ! ((shadow.r.state == Shadow::AND2) || abcdvals.tad3q || (shadow.r.state == Shadow::DCA2) || (shadow.r.state == Shadow::IOT1) || (! abcdvals._grpa1q & ! (shadow.r.ir & 00200)));
    abcdvals.alub_1   = abcdvals.defer3q || (shadow.r.state == Shadow::JMS3) || (shadow.r.state == Shadow::ISZ3) || ((shadow.r.state == Shadow::GRPB1) && abcdvals.grpb_skip);

    abcdvals._alu_add = ! ((shadow.r.state == Shadow::DEFER3) || (shadow.r.state == Shadow::JMS3) || (shadow.r.state == Shadow::ISZ3) || abcdvals.tad3q || (shadow.r.state == Shadow::GRPB1));
    abcdvals._alu_and = ! (abcdvals._alu_add && abcdvals._grpa1q);
    abcdvals.inc_axb  = ! abcdvals._grpa1q && (shadow.r.ir & 1);

    // get alu operands
    uint16_t alua = (abcdvals._alua_m1 ? 0 : 07777) | (abcdvals._alua_ma ? 0 : shadow.r.ma) | (abcdvals.alua_mq0600 ? abcdvals.mq & 00177 : 0) | (abcdvals.alua_mq1107 ? abcdvals.mq & 07600 : 0) | (abcdvals.alua_pc0600 ? shadow.r.pc & 00177 : 0) | (abcdvals.alua_pc1107 ? shadow.r.pc & 07600 : 0);
    uint16_t alub = (abcdvals._alub_m1 ? 0 : 07777) | (abcdvals._alub_ac ? 0 : shadow.r.ac) | (abcdvals.alub_1 ? 1 : 0);

    // perform alu computation
    uint16_t sum = (alua + alub) + (abcdvals.inc_axb ? 1 : 0);
    uint16_t inc = (alua ^ alub) + (abcdvals.inc_axb ? 1 : 0);
    bool cin_add_12 = sum > 07777;
    bool cin_inc_12 = inc > 07777;

    uint16_t aluq = (abcdvals._alu_and ? 0 : alua & alub) | (abcdvals._alu_add ? 0 : sum & 07777) | (abcdvals._grpa1q ? 0 : inc & 07777);

    abcdvals._aluq = aluq ^ 07777;
    abcdvals._alucout = ! ((! abcdvals._alu_add & cin_add_12) | (! abcdvals._grpa1q & cin_inc_12));

    bool oldlink = (shadow.r.state != Shadow::GRPA1) ? shadow.r.link :
        (((shadow.r.ir & 00120) == 00000) ?    shadow.r.link :
        (((shadow.r.ir & 00120) == 00020) ?  ! shadow.r.link :
        (((shadow.r.ir & 00120) == 00120))));
    abcdvals._newlink = oldlink ^ abcdvals._alucout;

    // load register at end of cycle
    abcdvals._ac_sc   = ! ((shadow.r.state == Shadow::DCA2) || ((shadow.r.state == Shadow::GRPB1) && (shadow.r.ir & 00200)));
    abcdvals._ac_aluq = ! ((shadow.r.state == Shadow::AND2) || abcdvals.tad3q || abcdvals.iot2q || ! abcdvals._grpa1q || ! abcdvals._ac_sc);
    abcdvals._ln_wrt  = ! (abcdvals.tad3q || ! abcdvals._grpa1q || abcdvals.iot2q);
    abcdvals._ma_aluq = ! (abcdvals.fetch2q || abcdvals.defer2q || abcdvals.defer3q || (shadow.r.state == Shadow::TAD2) || (shadow.r.state == Shadow::ISZ2) || abcdvals.intak1q);
    abcdvals._pc_aluq = ! ((shadow.r.state == Shadow::JMP1) || (shadow.r.state == Shadow::JMS3) || (shadow.r.state == Shadow::GRPB1));
    abcdvals._pc_inc  = ! (abcdvals.fetch2q || ((shadow.r.state == Shadow::ISZ3) && ! abcdvals._alucout) || (abcdvals.iot2q && abcdvals.ioskp));

    return abcdvals;
}
