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

// program counter register
// synchronous incrementable, synchronous loadable, asynchronously resettable 12-bit counter

module pccirc (in _aluq[11:00], in clok2, in _pc_aluq, in _pc_inc, out pcq[11:00], in reset)
{
    wire clok0a, clok0b, clok0c, _reseta, _resetb, _resetc, _resetd;
    wire nextpc[11:00], _pcq[11:00], allones[11:01], _allones[11:01];
    wire _clok1, pc_aluqa, pc_aluqb, pc_holda, pc_holdb, pc_inca, pc_incb, pc_incc;

    _clok1  = ~  clok2;
    clok0a  = ~ _clok1;         // each DFF T input has load 2
    clok0b  = ~ _clok1;         // so load 24 requires 3 drivers
    clok0c  = ~ _clok1;

    _reseta = ~ reset;          // each DFF _PC input has load 3
    _resetb = ~ reset;          // so load 36 requires 4 drivers
    _resetc = ~ reset;
    _resetd = ~ reset;

    _allones[01] =  _pcq[00];
    _allones[02] = ~(pcq[00] & pcq[01]);
    _allones[03] = ~(pcq[00] & pcq[01] & pcq[02]);
    _allones[04] = ~(pcq[00] & pcq[01] & pcq[02] & pcq[03]);
    _allones[05] = ~(pcq[00] & pcq[01] & pcq[02] & pcq[03] & pcq[04]);
    _allones[06] = ~(pcq[00] & pcq[01] & pcq[02] & pcq[03] & pcq[04] & pcq[05]);
    _allones[07] = ~(allones[06] & pcq[06]);
    _allones[08] = ~(allones[06] & pcq[06] & pcq[07]);
    _allones[09] = ~(allones[06] & pcq[06] & pcq[07] & pcq[08]);
    _allones[10] = ~(allones[06] & pcq[06] & pcq[07] & pcq[08] & pcq[09]);
    _allones[11] = ~(allones[06] & pcq[06] & pcq[07] & pcq[08] & pcq[09] & pcq[10]);

    allones = ~ _allones;

    pc_aluqa = ~ _pc_aluq;
    pc_aluqb = ~ _pc_aluq;
    pc_inca  = ~ _pc_inc;
    pc_incb  = ~ _pc_inc;
    pc_incc  = ~ _pc_inc;
    pc_holda = ~ (pc_inca | pc_aluqa);
    pc_holdb = ~ (pc_incb | pc_aluqb);

    nextpc[00] = ~(pc_inca &               pcq[00] |                                     pc_aluqa & _aluq[00] | pc_holda & _pcq[00]);
    nextpc[01] = ~(pc_inca & allones[01] & pcq[01] | pc_inca & _allones[01] & _pcq[01] | pc_aluqa & _aluq[01] | pc_holda & _pcq[01]);
    nextpc[02] = ~(pc_inca & allones[02] & pcq[02] | pc_inca & _allones[02] & _pcq[02] | pc_aluqa & _aluq[02] | pc_holda & _pcq[02]);
    nextpc[03] = ~(pc_inca & allones[03] & pcq[03] | pc_inca & _allones[03] & _pcq[03] | pc_aluqa & _aluq[03] | pc_holda & _pcq[03]);
    nextpc[04] = ~(pc_incb & allones[04] & pcq[04] | pc_incb & _allones[04] & _pcq[04] | pc_aluqa & _aluq[04] | pc_holda & _pcq[04]);
    nextpc[05] = ~(pc_incb & allones[05] & pcq[05] | pc_incb & _allones[05] & _pcq[05] | pc_aluqa & _aluq[05] | pc_holda & _pcq[05]);
    nextpc[06] = ~(pc_incb & allones[06] & pcq[06] | pc_incb & _allones[06] & _pcq[06] | pc_aluqb & _aluq[06] | pc_holdb & _pcq[06]);
    nextpc[07] = ~(pc_incb & allones[07] & pcq[07] | pc_incb & _allones[07] & _pcq[07] | pc_aluqb & _aluq[07] | pc_holdb & _pcq[07]);
    nextpc[08] = ~(pc_incc & allones[08] & pcq[08] | pc_incc & _allones[08] & _pcq[08] | pc_aluqb & _aluq[08] | pc_holdb & _pcq[08]);
    nextpc[09] = ~(pc_incc & allones[09] & pcq[09] | pc_incc & _allones[09] & _pcq[09] | pc_aluqb & _aluq[09] | pc_holdb & _pcq[09]);
    nextpc[10] = ~(pc_incc & allones[10] & pcq[10] | pc_incc & _allones[10] & _pcq[10] | pc_aluqb & _aluq[10] | pc_holdb & _pcq[10]);
    nextpc[11] = ~(pc_incc & allones[11] & pcq[11] | pc_incc & _allones[11] & _pcq[11] | pc_aluqb & _aluq[11] | pc_holdb & _pcq[11]);

    pc00: DFF (_PC:_reseta, _PS:1, D:nextpc[00], T:clok0a, Q:pcq[00], _Q:_pcq[00]);
    pc01: DFF (_PC:_reseta, _PS:1, D:nextpc[01], T:clok0a, Q:pcq[01], _Q:_pcq[01]);
    pc02: DFF (_PC:_reseta, _PS:1, D:nextpc[02], T:clok0a, Q:pcq[02], _Q:_pcq[02]);
    pc03: DFF (_PC:_resetb, _PS:1, D:nextpc[03], T:clok0a, Q:pcq[03], _Q:_pcq[03]);
    pc04: DFF (_PC:_resetb, _PS:1, D:nextpc[04], T:clok0b, Q:pcq[04], _Q:_pcq[04]);
    pc05: DFF (_PC:_resetb, _PS:1, D:nextpc[05], T:clok0b, Q:pcq[05], _Q:_pcq[05]);
    pc06: DFF (_PC:_resetc, _PS:1, D:nextpc[06], T:clok0b, Q:pcq[06], _Q:_pcq[06]);
    pc07: DFF (_PC:_resetc, _PS:1, D:nextpc[07], T:clok0b, Q:pcq[07], _Q:_pcq[07]);
    pc08: DFF (_PC:_resetc, _PS:1, D:nextpc[08], T:clok0c, Q:pcq[08], _Q:_pcq[08]);
    pc09: DFF (_PC:_resetd, _PS:1, D:nextpc[09], T:clok0c, Q:pcq[09], _Q:_pcq[09]);
    pc10: DFF (_PC:_resetd, _PS:1, D:nextpc[10], T:clok0c, Q:pcq[10], _Q:_pcq[10]);
    pc11: DFF (_PC:_resetd, _PS:1, D:nextpc[11], T:clok0c, Q:pcq[11], _Q:_pcq[11]);
}

module pc ()
{
//            00000000011111111112222222222333
//            12345678901234567890123456789012
#define amask giiiiioiiiiiioiiiiiioiiiiiioiiii
#define bmask giioiiiiiioiiiiiioiiiiiioiiiiiio
#define cmask giiiiioiiiiiioiiiiiioiiiiiiiiiii
#define dmask giiiiiiiiiiiiiiiiiiiiiiiiiiiiiii
#include "cons.inc"

    pccirc: pccirc (_aluq[11:00], clok2, _pc_aluq, _pc_inc, pcq[11:00], reset);
}
