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

// contains base methods for netgen -csource generated code that simulates a module

#include <string.h>

#include "csrcmod.h"

// constructor saves module-specific boolarray and vararray
CSrcMod::CSrcMod (bool *ba, unsigned bc, Var const *va, unsigned vc, char const *mn)
{
    memset (ba, 0, bc * sizeof *ba);
    boolarray = ba;
    boolcount = bc;
    vararray  = va;
    varcount  = vc;
    modname   = mn;
}

CSrcMod::~CSrcMod ()
{ }

// look for a variable by name string
CSrcMod::Var const *CSrcMod::findvar (char const *namestr, unsigned namelen)
{
    int i = 0;
    int j = varcount - 1;
    do {
        int k = (i + j) / 2;
        CSrcMod::Var const *var = &vararray[k];
        unsigned varlen = strlen (var->name);
        unsigned cmplen = (varlen < namelen) ? varlen : namelen;
        int cmp = memcmp (var->name, namestr, cmplen);
        if (cmp == 0) cmp = varlen - namelen;
        if (cmp == 0) return var;
        if (cmp > 0) j = k - 1;
        if (cmp < 0) i = k + 1;
    } while (i <= j);
    return NULL;
}

// step a DFF state
//  input:
//        d = data signal
//        t = clock signal
//      _pc = preclear
//      _ps = preset
//    old_q = previous _Q output
//     oldq = previous Q output
//   *lastt = previous T input
//     name = flipflop name
//  output:
//   *new_q = newly computed _Q output
//    *newq = newly computed Q output
//   *lastt = t
void CSrcMod::DFFStep (bool d, bool t, bool _pc, bool _ps, bool old_q, bool oldq, bool *new_q, bool *newq, bool *lastt, char const *name)
{
    if (!_pc | !_ps) {
         *newq = !_ps;
        *new_q = !_pc;

        // _PC=1 _PS=0 : D=1 F=1 C=~t B=1 A=~d E=0
        if (_pc) nto += (t ? 3 : 4) +   (d ? 0 : 1);

        // _PC=0 : A=1 C=1 E=1 B=~t D=~_ps F=~_ps
            else nto += (t ? 3 : 4) + (_ps ? 0 : 2);
    } else {
        if (t & !*lastt) {
             *newq =  d;
            *new_q = !d;
        } else {
             *newq =  oldq;
            *new_q = old_q;
        }

        if (! t) {
            nto += 4;           // B=1, C=1
                                // exactly 1 of A,D=1
                                // exactly 1 of E,F=1
        } else {

            // handle case of flipflop now q=0
            // since B=0 because d input was zero before clock edge,
            // ...any change in d input will be ignored, A stays 1
            if (! *newq) {
                nto += 3;       // A=1
                                // C=1
                                // E=1
            }

            // handle case of flipflop now q=1
            else {

                // handle case of flipflop now q=1 and still d=1
                if (d) {
                    nto += 3;   // B=1
                                // D=1
                                // F=1
                }

                // handle case of flipflop now q=1 but now d=0
                else {
                    nto += 4;   // A=1
                                // B=1
                                // D=1
                                // F=1
                }
            }
        }
    }
    *lastt = t;
}

// step a DLat state
void CSrcMod::DLatStep (bool d, bool g, bool _pc, bool _ps, bool *q, bool *_q, char const *name)
{
    if (!_pc | !_ps) {
         *q = !_ps;
        *_q = !_pc;
        nto += (_pc ? 0 : 1) + (_ps ? 0 : 1) + (g ? 1 : 2);
    } else {
        if (g) {
             *q =  d;
            *_q = !d;
            nto += 2;
        } else {
            nto += 3;
        }
    }
}
