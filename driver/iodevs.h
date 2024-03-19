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

#ifndef _IODEVS_H
#define _IODEVS_H

#include <pthread.h>

#define IO_SKIP 0020000  // returned by ioinstr() to skip next instruction
#define IO_LINK 0010000  // link bit state passed into and returned by ioinstr()
#define IO_DATA 0007777  // accumulator passed into and returned by ioinstr()
#define UNSUPIO 0177777  // returned by ioinstr() for unsupported i/o instruction

#include "miscdefs.h"

struct IODevOps {
    uint16_t opcd;
    char const *desc;
};

struct SCRet;
struct SCRetErr;
struct SCRetInt;
struct SCRetStr;

struct IODev {
    uint16_t opscount;
    IODevOps const *opsarray;

    char const *iodevname;

    IODev ();
    virtual ~IODev ();

    virtual void ioreset () = 0;
    virtual SCRet *scriptcmd (int argc, char const *const *argv);
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input) = 0;

    IODev *nextiodev;
};

struct SCRet {
    enum Type {
        SCRT_ERR,
        SCRT_INT,
        SCRT_STR
    };

    virtual ~SCRet () { }
    virtual Type gettype () = 0;
    virtual SCRetErr *casterr () { return NULL; }
    virtual SCRetInt *castint () { return NULL; }
    virtual SCRetStr *caststr () { return NULL; }
};

struct SCRetErr : SCRet {
    SCRetErr (char const *fmt, ...);
    virtual Type gettype () { return SCRT_ERR; }
    virtual SCRetErr *casterr () { return this; }

    char *msg;
};

struct SCRetInt : SCRet {
    SCRetInt (int val);
    virtual Type gettype () { return SCRT_INT; }
    virtual SCRetInt *castint () { return this; }

    int val;
};

struct SCRetStr : SCRet {
    SCRetStr (char const *fmt, ...);
    virtual ~SCRetStr ();
    virtual Type gettype () { return SCRT_STR; }
    virtual SCRetStr *caststr () { return this; }

    char *str;
};

extern IODev *alliodevs;

uint16_t ioinstr (uint16_t opcode, uint16_t input);
char const *iodisas (uint16_t opcode);
void ioreset ();

void clrintreqmask (uint16_t mask);
void setintreqmask (uint16_t mask);
uint16_t getintreqmask ();

#endif
