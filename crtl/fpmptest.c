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

/*
    compare results of software floatingpoint with raspi computed floatingpoint

    make sure library/magicdefs.asm RASPIFLOAT = 0 (optionally RASPIMULDIV = 0)
    rebuild if changed:
       rm *.hode.o
       make

    rm -f x.*
    ln -s fpmptest.c x.c
    make x.hex
    ./runit.sh x.hex 2>&1 | grep 'BAD RESULT'
*/

#include <assert.h>
#include <hode.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define SCN_ADD_DDD 12
#define SCN_ADD_FFF 13
#define SCN_SUB_DDD 14
#define SCN_SUB_FFF 15
#define SCN_MUL_DDD 16
#define SCN_MUL_FFF 17
#define SCN_DIV_DDD 18
#define SCN_DIV_FFF 19

struct DDD {
    __uint16_t scn;
    double *result;
    double *leftop;
    double *riteop;
};

struct FFF {
    __uint16_t scn;
    float *result;
    float *leftop;
    float *riteop;
};

int error = 0;

void printquadhex (__uint64_t u)
{
    char b[17];
    for (int i = 16; -- i >= 0;) {
        b[i] = "0123456789ABCDEF"[u&15];
        u >>= 4;
    }
    b[16] = 0;
    ass_putstr (b);
}

void printlonghex (__uint32_t u)
{
    char b[9];
    for (int i = 8; -- i >= 0;) {
        b[i] = "0123456789ABCDEF"[u&15];
        u >>= 4;
    }
    b[8] = 0;
    ass_putstr (b);
}

__uint64_t dbltoquad (double d)
{
    union {
        double d;
        __uint64_t u;
    } v;
    v.d = d;
    return v.u;
}

__uint32_t flttolong (float f)
{
    union {
        float f;
        __uint32_t u;
    } v;
    v.f = f;
    return v.u;
}

void verifyd (double a, double b, double t, __uint16_t scn, char const *opstr)
{
    double s;

    DDD raspi = { scn, &s, &a, &b };
    *(DDD **)0xFFF0U = &raspi;

    __uint64_t au = dbltoquad (a);
    __uint64_t bu = dbltoquad (b);
    __uint64_t tu = dbltoquad (t);
    __uint64_t su = dbltoquad (s);

    printquadhex (au); ass_putchr (' '); printquadhex (bu); ass_putchr (' '); printquadhex (tu); ass_putchr (' '); printquadhex (su); ass_putstr ("  ");
    ass_putdbl (a); ass_putstr (opstr); ass_putdbl (b); ass_putstr (" = "); ass_putdbl (t); ass_putstr (" sb "); ass_putdbl (s);

    __uint64_t diffts = tu - su;
    __uint64_t diffst = su - tu;
    if (diffts > diffst) diffts = diffst;

    if ((diffts == 0) || (isnan (t) && isnan (s))) ass_putstr (" : exact\n");
    else if ((diffts < 3) || ((diffts == 0x8000000000000000) && ((tu & 0x7FFFFFFFFFFFFFFF) == 0))) {
        ass_putstr (" : close enuf ");
        printquadhex (diffts);
        ass_putchr ('\n');
    } else {
        ass_putstr (" : BAD RESULT ");
        printquadhex (diffts);
        ass_putchr ('\n');
        error = 1;
    }
}

void verifyf (float a, float b, float t, __uint16_t scn, char const *opstr)
{
    float s;

    FFF raspi = { scn, &s, &a, &b };
    *(FFF **)0xFFF0U = &raspi;

    __uint32_t au = flttolong (a);
    __uint32_t bu = flttolong (b);
    __uint32_t tu = flttolong (t);
    __uint32_t su = flttolong (s);

    printlonghex (au); ass_putchr (' '); printlonghex (bu); ass_putchr (' '); printlonghex (tu); ass_putchr (' '); printlonghex (su); ass_putstr ("  ");
    ass_putflt (a); ass_putstr (opstr); ass_putflt (b); ass_putstr (" = "); ass_putflt (t); ass_putstr (" sb "); ass_putflt (s);

    __uint32_t diffts = tu - su;
    __uint32_t diffst = su - tu;
    if (diffts > diffst) diffts = diffst;

    if ((diffts == 0) || (isnanf (t) && isnanf (s))) ass_putstr (" : exact\n");
    else if ((diffts < 3) || ((diffts == 0x80000000) && ((tu & 0x7FFFFFFF) == 0))) {
        ass_putstr (" : close enuf ");
        printlonghex (diffts);
        ass_putchr ('\n');
    } else {
        ass_putstr (" : BAD RESULT ");
        printlonghex (diffts);
        ass_putchr ('\n');
        error = 1;
    }
}

void testcased (double a, double b)
{
    verifyd (a, b, a + b, SCN_ADD_DDD, " + ");
    verifyd (a, b, a - b, SCN_SUB_DDD, " - ");
    double t = a * b;
    verifyd (a, b, t, SCN_MUL_DDD, " * ");
    verifyd (a, b, a / b, SCN_DIV_DDD, " / ");
}

void testcasef (float a, float b)
{
    verifyf (a, b, a + b, SCN_ADD_FFF, " + ");
    verifyf (a, b, a - b, SCN_SUB_FFF, " - ");
    //printinstr (1);
    float t = a * b;
    //printinstr (0);
    verifyf (a, b, t, SCN_MUL_FFF, " * ");
    verifyf (a, b, a / b, SCN_DIV_FFF, " / ");
}

struct DRange {
    double lo, hi;
    double add, mul;
};

static double dspecials[] = { 0, 5e-324, 1.5e+308, 2e+308 };

static DRange const dranges[] = {
    {    -10,     10, 1, 1 },
    { 5e-324, 1e-322, 0, 2 },
    { 1e-310, 1e-305, 0, 2 },
    { 1e-165, 1e-160, 0, 2 },
    { 1e-155, 1e-150, 0, 2 },
    { 1e+150, 1e+155, 0, 2 },
    { 1e+304, 1.5e+308, 0, 2 },
    {      0,      0, 0, 0 } };

struct FRange {
    float lo, hi;
    float add, mul;
};

static float fspecials[] = { 0, 2e-45, 3e+38, 4e+38 };

static FRange const franges[] = {
    {    -10,    10, 1, 1 },
    {  1e-45, 1e-42, 0, 2 },
    {  1e-38, 2e-38, 0, 2 },
    {  1e-25, 1e-20, 0, 2 },
    {  1e-19, 1e-18, 0, 2 },
    {  1e+18, 1e+19, 0, 2 },
    {  1e+37, 3e+38, 0, 2 },
    {      0,     0, 0, 0 } };

#define NEL(a) (sizeof a / sizeof a[0])

int main (int argc, char **argv)
{
    ass_putstr ("\nFLOAT\n\n");

    testcasef (7.0, 3.0);

    for (int i = 0; i < NEL(fspecials); i ++) {
        float a = fspecials[i];
        for (int j = 0; j < NEL(fspecials); j ++) {
            float b = fspecials[j];
            testcasef (a, b);
        }
    }

    for (FRange *i = franges; i->mul != 0; i ++) {
        for (float a = i->lo; a <= i->hi; a = (a + i->add) * i->mul) {
            for (FRange *j = franges; j->mul != 0; j ++) {
                for (float b = j->lo; b <= j->hi; b = (b + j->add) * j->mul) {
                    testcasef (a, b);
                }
            }
        }
    }

#if 000
    ass_putstr ("\nDOUBLE\n\n");

    testcased (7.0, 3.0);

    for (int i = 0; i < NEL(dspecials); i ++) {
        double a = dspecials[i];
        for (int j = 0; j < NEL(dspecials); j ++) {
            double b = dspecials[j];
            testcased (a, b);
        }
    }

    for (DRange *i = dranges; i->mul != 0; i ++) {
        for (double a = i->lo; a <= i->hi; a = (a + i->add) * i->mul) {
            for (DRange *j = dranges; j->mul != 0; j ++) {
                for (double b = j->lo; b <= j->hi; b = (b + j->add) * j->mul) {
                    testcased (a, b);
                }
            }
        }
    }
#endif
    return error;
}
