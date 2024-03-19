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

#ifndef _SCNCALL_H
#define _SCNCALL_H

#include <map>
#include <termios.h>

#define SCN_IO 06300    // base I/O instruction

#include "iodevs.h"
#include "miscdefs.h"

struct ScnCall : IODev {
    virtual void ioreset ();
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input);
    void exithandler ();

private:
    std::map<int,struct termios> savedtermioss;

    uint16_t docall (uint16_t data);
};

struct ScnCall2 : IODev {
    ScnCall2 ();
    virtual void ioreset ();
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input);
};

extern ScnCall scncall;
extern ScnCall2 scncall2;

#endif
