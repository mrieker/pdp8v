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

// print backplane pin loading
// prints pin number, load count, equivalent load resistance, signal name

// cc -O2 -Wall -o loadings loadings.c
// cat ../*/*.rep | ./loadings > loadings.out

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define XX(s) #s

char const *const names[4] = { 

XX(( abody ,\
         acq[11],           /* 02 */ \
        _aluq[11],          /* 03 */ \
        _maq[11],           /* 04 */ \
         maq[11],           /* 05 */ \
         mq[11],            /* 06 */ \
         pcq[11],           /* 07 */ \
         irq[11],           /* 08 */ \
         acq[10],           /* 09 */ \
        _aluq[10],          /* 10 */ \
        _maq[10],           /* 11 */ \
         maq[10],           /* 12 */ \
         mq[10],            /* 13 */ \
         pcq[10],           /* 14 */ \
         irq[10],           /* 15 */ \
         acq[09],           /* 16 */ \
        _aluq[09],          /* 17 */ \
        _maq[09],           /* 18 */ \
         maq[09],           /* 19 */ \
         mq[09],            /* 20 */ \
         pcq[09],           /* 21 */ \
         irq[09],           /* 22 */ \
         acq[08],           /* 23 */ \
        _aluq[08],          /* 24 */ \
        _maq[08],           /* 25 */ \
         maq[08],           /* 26 */ \
         mq[08],            /* 27 */ \
         pcq[08],           /* 28 */ \
         __nc1,             /* 29 */ \
         acq[07],           /* 30 */ \
        _aluq[07],          /* 31 */ \
        _maq[07]            /* 32 */
)),

XX(( bbody ,\
         maq[07],           /* 02 */ \
         mq[07],            /* 03 */ \
         pcq[07],           /* 04 */ \
         __nc2,             /* 05 */ \
         acq[06],           /* 06 */ \
        _aluq[06],          /* 07 */ \
        _maq[06],           /* 08 */ \
         maq[06],           /* 09 */ \
         mq[06],            /* 10 */ \
         pcq[06],           /* 11 */ \
        _jump,              /* 12 */ \
         acq[05],           /* 13 */ \
        _aluq[05],          /* 14 */ \
        _maq[05],           /* 15 */ \
         maq[05],           /* 16 */ \
         mq[05],            /* 17 */ \
         pcq[05],           /* 18 */ \
        _alub_m1,           /* 19 */ \
         acq[04],           /* 20 */ \
        _aluq[04],          /* 21 */ \
        _maq[04],           /* 22 */ \
         maq[04],           /* 23 */ \
         mq[04],            /* 24 */ \
         pcq[04],           /* 25 */ \
         acqzero,           /* 26 */ \
         acq[03],           /* 27 */ \
        _aluq[03],          /* 28 */ \
        _maq[03],           /* 29 */ \
         maq[03],           /* 30 */ \
         mq[03],            /* 31 */ \
         pcq[03]            /* 32 */
)),

XX(( cbody ,\
         acq[02],           /* 02 */ \
        _aluq[02],          /* 03 */ \
        _maq[02],           /* 04 */ \
         maq[02],           /* 05 */ \
         mq[02],            /* 06 */ \
         pcq[02],           /* 07 */ \
        _ac_sc,             /* 08 */ \
         acq[01],           /* 09 */ \
        _aluq[01],          /* 10 */ \
        _maq[01],           /* 11 */ \
         maq[01],           /* 12 */ \
         mq[01],            /* 13 */ \
         pcq[01],           /* 14 */ \
         intak1q,           /* 15 */ \
         acq[00],           /* 16 */ \
        _aluq[00],          /* 17 */ \
        _maq[00],           /* 18 */ \
         maq[00],           /* 19 */ \
         mq[00],            /* 20 */ \
         pcq[00],           /* 21 */ \
         fetch1q,           /* 22 */ \
        _ac_aluq,           /* 23 */ \
        _alu_add,           /* 24 */ \
        _alu_and,           /* 25 */ \
        _alua_m1,           /* 26 */ \
        _alucout,           /* 27 */ \
        _alua_ma,           /* 28 */ \
         alua_mq0600,       /* 29 */ \
         alua_mq1107,       /* 30 */ \
         alua_pc0600,       /* 31 */ \
         alua_pc1107        /* 32 */
)),

XX(( dbody ,\
         alub_1,            /* 02 */ \
        _alub_ac,           /* 03 */ \
         clok2,             /* 04 */ \
         fetch2q,           /* 05 */ \
        _grpa1q,            /* 06 */ \
         defer1q,           /* 07 */ \
         defer2q,           /* 08 */ \
         defer3q,           /* 09 */ \
         exec1q,            /* 10 */ \
         grpb_skip,         /* 11 */ \
         exec2q,            /* 12 */ \
        _dfrm,              /* 13 */ \
         inc_axb,           /* 14 */ \
        _intak,             /* 15 */ \
         intrq,             /* 16 */ \
         exec3q,            /* 17 */ \
         ioinst,            /* 18 */ \
         ioskp,             /* 19 */ \
         iot2q,             /* 20 */ \
        _ln_wrt,            /* 21 */ \
        _lnq,               /* 22 */ \
         lnq,               /* 23 */ \
        _ma_aluq,           /* 24 */ \
         mql,               /* 25 */ \
        _mread,             /* 26 */ \
        _mwrite,            /* 27 */ \
        _pc_aluq,           /* 28 */ \
        _pc_inc,            /* 29 */ \
         reset,             /* 30 */ \
        _newlink,           /* 31 */ \
         tad3q              /* 32 */
)) };


int main ()
{
    char line[256];
    char *p;
    int con, load, loads[4][32], pin;
    char const *q;

    memset (loads, 0, sizeof loads);

    while (fgets (line, sizeof line, stdin) != NULL) {
        // Network: I3/acon (load 6)
        if (memcmp (line, "Network: I", 10) != 0) continue;
        pin = strtol (line + 10, &p, 10);
        if (*(p ++) != '/') abort ();
        if ((pin < 1) || (pin > 32)) abort ();
        con = *(p ++) - 'a';
        if ((con < 0) || (con > 3)) abort ();
        if (memcmp (p, "con (load ", 10) != 0) abort ();
        load = strtol (p + 10, &p, 10);
        if (*p != ')') abort ();
        loads[con][pin-1] += load;
    }

    for (con = 0; con < 4; con ++) {
        q = strchr (names[con], ',') + 1;
        for (pin = 1; pin < 32; pin ++) {
            if (pin % 5 == 1) printf ("\n");
            while (*q == ' ') q ++;
            p = strchr (q, ',');
            if (p == NULL) p = strchr (q, ')');
            load = loads[con][pin];
            if (load > 0) {
                printf ("%c%02d : %2d  %4.1fK  %.*s\n", con + 'A', pin + 1, load, 47.0 / load, (int)(p - q), q);
            } else {
                printf ("%c%02d : %2d  -----  %.*s\n", con + 'A', pin + 1, load, (int)(p - q), q);
            }
            q = ++ p;
        }
        printf ("\n");
    }

    return 0;
}
