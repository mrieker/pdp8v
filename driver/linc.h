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

#ifndef _LINC_H
#define _LINC_H

#include "iodevs.h"

struct Linc : IODev {
    uint16_t specfuncs; // special function mask <09:05>

    Linc ();

    // implements IODev
    virtual void ioreset ();
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input);

private:
    uint16_t lincac;    // 12-bit accumulator
    uint16_t lincpc;    // 10-bit program counter
    bool disabjumprtn;
    bool link;          // link bit
    bool flow;          // overflow bit

    void execute ();
    void execbeta (uint16_t lastfetchaddr, uint16_t op);
    bool execalpha (uint16_t lastfetchaddr, uint16_t op);
    bool execgamma (uint16_t lastfetchaddr, uint16_t op);
    void execiob (uint16_t lastfetchaddr, uint16_t op);
    void unsupinst (uint16_t lastfetchaddr, uint16_t op);
    uint16_t addition (uint16_t a, uint16_t b, bool lame);
};

extern Linc linc;

#endif
