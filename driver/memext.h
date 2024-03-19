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

#ifndef _MEMEXT_H
#define _MEMEXT_H

#include "iodevs.h"

struct MemExt : IODev {
    uint16_t iframe;                // IF is in <14:12>, all else is zeroes
                                    // linc mode uses <11:10> but clears it on exit
    uint16_t dframe;                // DF is in <14:12>, all else is zeroes
                                    // linc mode uses <11:10> but clears it on exit
    uint16_t iframeafterjump;       // copied to iframe at every jump (JMP or JMS)
    bool intdisableduntiljump;      // blocks intenabled until next jump (JMP or JMS)
    uint16_t savediframe;           // iframe saved at beginning of last interrupt
    uint16_t saveddframe;           // dframe saved at beginning of last interrupt

    MemExt ();
    void setdfif (uint32_t frame);
    void doingjump ();
    void doingread (uint16_t data);
    void intack ();
    bool intenabd ();
    bool candoio ();

    // implements IODev
    virtual void ioreset ();
    virtual SCRet *scriptcmd (int argc, char const *const *argv);
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input);

private:
    bool intenableafterfetch;       // enable interrupts after next fetch
    bool intenabled;                // interrupts are enabled (unless intdisableduntiljump is also set)
    bool userflag;                  // false: execmode; true: usermode
    bool userflagafterjump;         // copied to userflag at every jump (JMP or JMS)
    bool saveduserflag;             // userflag saved at beginning of last interrupt
};

extern MemExt memext;

#endif
