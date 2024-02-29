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
    ./trigtest.sh

    ./trigtest [comparefile]
        if given, compares generated values to those in comparefile
        so use this program on x86 without parameter to create comparefile from stdout
        then run this program on hode with that file to compare to x86 numbers
*/

#include <assert.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifdef __HODE__
#define DF "%lA"
#define FF "%A"
#else
#define DF "%A"
#define FF "%A"
int isnanf (float x);
#endif

#define TESTD(f)  { double y = f (x);   printdouble (" "#f" = "DF"\n", y); }
#define TESTF(f)  { float  y = f (x);   printfloat  (" "#f" = "FF"\n", y); }
#define TEST2D(f) { double z = f (x,y); printdouble (" "#f" = "DF"\n", z); }
#define TEST2F(f) { float  z = f (x,y); printfloat  (" "#f" = "FF"\n", z); }

static FILE *oldfile;

static void printexact (char const *fmt, ...);
static void printdouble (char const *fmt, double val);
static void printfloat (char const *fmt, float val);

int main (int argc, char **argv)
{
    if (argc > 1) {
        oldfile = fopen (argv[1], "r");
        if (oldfile == NULL) {
            fprintf (stderr, "error %d opening %s\n", errno, argv[1]);
            return 1;
        }
    }

    for (int deg = -360; deg <= 360; deg += 15) {
        double x = deg * M_PI / 180.0;
        printexact ("DBL  %d "DF"\n", deg, x);
        TESTD (acos);
        TESTD (asin);
        TESTD (atan);
        TESTD (cos);
        TESTD (exp);
        TESTD (fabs);
        TESTD (log);
        TESTD (sin);
        TESTD (sqrt);
        TESTD (tan);
        for (int dgy = -360; dgy <= 360; dgy += 15) {
            double y = dgy / 30.0;
            printexact ("DBL  %d,%d "DF","DF"\n", deg, dgy, x, y);
            TEST2D (atan2);
            TEST2D (pow);
        }
    }

    for (int deg = -360; deg <= 360; deg += 15) {
        float x = deg * M_PI / 180.0;
        printexact ("FLT  %d "FF"\n", deg, x);
        TESTF (acosf);
        TESTF (asinf);
        TESTF (atanf);
        TESTF (cosf);
        TESTF (expf);
        TESTF (fabsf);
        TESTF (logf);
        TESTF (sinf);
        TESTF (sqrtf);
        TESTF (tanf);
        for (int dgy = -360; dgy <= 360; dgy += 15) {
            float y = dgy / 30.0;
            printexact ("FLT  %d,%d "FF","FF"\n", deg, dgy, x, y);
            TEST2F (atan2f);
            TEST2F (powf);
        }
    }

    return 0;
}

static void printexact (char const *fmt, ...)
{
    char outbuf[256];
    va_list ap;
    va_start (ap, fmt);
    vsnprintf (outbuf, sizeof outbuf, fmt, ap);
    va_end (ap);

    fputs (outbuf, stdout);

    if (oldfile != NULL) {
        char inbuf[256];
        if (fgets (inbuf, sizeof inbuf, oldfile) == NULL) {
            printf ("ERROR EOF\n");
            return;
        }
        if (memcmp (inbuf, outbuf, 4) != 0) {
            printf ("ERROR mismatch string %s", inbuf);
        }
    }
}

static void printdouble (char const *fmt, double val)
{
    printf (fmt, val);

    if (oldfile != NULL) {
        char inbuf[256];
        if (fgets (inbuf, sizeof inbuf, oldfile) == NULL) {
            printf ("ERROR EOF\n");
            return;
        }
        char *p = strchr (inbuf, '=');
        if (p == NULL) {
            printf ("ERROR missing = %s", inbuf);
            return;
        }
        if (memcmp (inbuf, fmt, p - inbuf) != 0) {
            printf ("ERROR mismatch prefix %s", inbuf);
            return;
        }
        double inval = strtod (++ p, NULL);
        if (isnan (val) && isnan (inval)) return;
        if ((fabs (val) < 1e-9) && (fabs (inval) < 1e-9)) return;
        double ratio = val / inval;
        if ((ratio > 1.0001) && (ratio < 0.9999)) {
            printf ("ERROR mismatch value %s", inbuf);
        }
    }
}

static void printfloat (char const *fmt, float val)
{
    printf (fmt, val);

    if (oldfile != NULL) {
        char inbuf[256];
        if (fgets (inbuf, sizeof inbuf, oldfile) == NULL) {
            printf ("ERROR EOF\n");
            return;
        }
        char *p = strchr (inbuf, '=');
        if (p == NULL) {
            printf ("ERROR missing = %s", inbuf);
            return;
        }
        if (memcmp (inbuf, fmt, p - inbuf) != 0) {
            printf ("ERROR mismatch prefix %s", inbuf);
            return;
        }
        float inval = strtof (++ p, NULL);
        if (isnanf (val) && isnanf (inval)) return;
        if ((fabsf (val) < 1e-9) && (fabsf (inval) < 1e-9)) return;
        float ratio = val / inval;
        if ((ratio > 1.0001) && (ratio < 0.9999)) {
            printf ("ERROR mismatch value %s", inbuf);
        }
    }
}
