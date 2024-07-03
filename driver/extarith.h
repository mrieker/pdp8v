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

#ifndef _EXTARITH_H
#define _EXTARITH_H

#include "iodevs.h"
#include "miscdefs.h"

struct ExtArith : IODev {
    ExtArith ();
    bool getgtflag ();
    void setgtflag (bool gt);

    virtual void ioreset ();
    virtual SCRet *scriptcmd (int argc, char const *const *argv);
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input);

    uint16_t multquot;
    uint16_t stepcount;

private:
    uint16_t eae_modea (uint16_t opcode, uint16_t input);
    uint16_t eae_modeb (uint16_t opcode, uint16_t input);

    uint16_t getdeferredaddr ();

    uint16_t multiply (uint16_t input, uint16_t multiplier);
    uint16_t divide (uint16_t input, uint16_t divisor);
    uint16_t normalize (uint16_t input);

    bool gtflag;
    bool modeb;
    uint16_t nextreadaddr;
};

extern ExtArith extarith;

#endif
