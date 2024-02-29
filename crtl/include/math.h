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

#ifndef _MATH_H
#define _MATH_H

/* Declarations for math constants.
   Copyright (C) 1991-1993, 1995-1999, 2001, 2002, 2004, 2006, 2009, 2011, 2012
   Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <http://www.gnu.org/licenses/>.  */
#define M_E         2.7182818284590452354   /* e */
#define M_LOG2E     1.4426950408889634074   /* log_2 e */
#define M_LOG10E    0.43429448190325182765  /* log_10 e */
#define M_LN2       0.69314718055994530942  /* log_e 2 */
#define M_LN10      2.30258509299404568402  /* log_e 10 */
#define M_PI        3.14159265358979323846  /* pi */
#define M_PI_2      1.57079632679489661923  /* pi/2 */
#define M_PI_4      0.78539816339744830962  /* pi/4 */
#define M_1_PI      0.31830988618379067154  /* 1/pi */
#define M_2_PI      0.63661977236758134308  /* 2/pi */
#define M_2_SQRTPI  1.12837916709551257390  /* 2/sqrt(pi) */
#define M_SQRT2     1.41421356237309504880  /* sqrt(2) */
#define M_SQRT1_2   0.70710678118654752440  /* 1/sqrt(2) */

#define M_NAN (0.0/0.0)
#define M_INF (1.0/0.0)

bool isfinite (double x);
bool isinf (double x);
bool isnan (double x);

double acos (double x);
double asin (double x);
double atan (double x);
double atan2 (double y, double x);
double cos (double x);
double exp (double x);
double fabs (double x);
double frexp (double x, int *exp);
double ldexp (double x, int exp);
double log (double x);
double pow (double x, double y);
double sin (double x);
double sqrt (double x);
double tan (double x);

#define M_EF        ((float)M_E)
#define M_LOG2EF    ((float)M_LOG2E)
#define M_LOG10EF   ((float)M_LOG10E)
#define M_LN2F      ((float)M_LN2)
#define M_LN10F     ((float)M_LN10)
#define M_PIF       ((float)M_PI)
#define M_PI_2F     ((float)M_PI_2)
#define M_PI_4F     ((float)M_PI_4)
#define M_1_PIF     ((float)M_1_PI)
#define M_2_PIF     ((float)M_2_PI)
#define M_2_SQRTPIF ((float)M_2_SQRTPI)
#define M_SQRT2F    ((float)M_SQRT2)
#define M_SQRT1_2F  ((float)M_SQRT1_2)

#define M_NANF ((float)(0.0/0.0))
#define M_INFF ((float)(1.0/0.0))

bool isfinitef (float x);
bool isinff (float x);
bool isnanf (float x);

float acosf (float x);
float asinf (float x);
float atanf (float x);
float atan2f (float y, float x);
float cosf (float x);
float expf (float x);
float fabsf (float x);
float frexpf (float x, int *exp);
float ldexpf (float x, int exp);
float logf (float x);
float powf (float x, float y);
float sinf (float x);
float sqrtf (float x);
float tanf (float x);

#endif
