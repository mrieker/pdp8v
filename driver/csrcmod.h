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

#ifndef _CSRCMOD_H
#define _CSRCMOD_H

#include "miscdefs.h"

struct CSrcMod {
    struct Var {
        char const *name;
        short lodim;
        short hidim;
        short const *indxs;
    };

    bool *boolarray;
    unsigned boolcount;
    Var const *vararray;
    unsigned varcount;
    char const *modname;
    unsigned boardena;  // enabled board bits (BE_*)
    unsigned nto;       // num triodes off (outputting 1)
    unsigned ntt;       // num total triodes

    CSrcMod (bool *ba, unsigned bc, Var const *va, unsigned vc, char const *mn);
    virtual ~CSrcMod ();

    virtual Var const *findvar (char const *namestr, unsigned namelen);
    virtual void stepstatework () = 0;
    virtual uint32_t readgpiowork () = 0;
    virtual uint32_t readaconwork () = 0;
    virtual uint32_t readbconwork () = 0;
    virtual uint32_t readcconwork () = 0;
    virtual uint32_t readdconwork () = 0;
    virtual void writegpiowork (uint32_t valu) = 0;
    virtual void writeaconwork (uint32_t mask, uint32_t valu) = 0;
    virtual void writebconwork (uint32_t mask, uint32_t valu) = 0;
    virtual void writecconwork (uint32_t mask, uint32_t valu) = 0;
    virtual void writedconwork (uint32_t mask, uint32_t valu) = 0;

    void DFFStep (bool d, bool t, bool _pc, bool _ps, bool old_q, bool oldq, bool *new_q, bool *newq, bool *lastt, char const *name);
    void DLatStep (bool d, bool g, bool _pc, bool _ps, bool *q, bool *_q, char const *name);
};

extern "C" {
    CSrcMod *CSrcMod_proc_ctor ();
}

#endif
