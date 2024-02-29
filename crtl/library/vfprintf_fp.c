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

/************************************************************************/
/*                                                                      */
/*  Floatingpoint conversions for oz_sys_vfprintf                       */
/*                                                                      */
/*  In a different module so oz_sys_xprintf.c can be compiled without   */
/*  references to floating point data                                   */
/*                                                                      */
/************************************************************************/

#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <string.h>

#include "vfprintf.h"

int vfprintf_fp (char fc, Par *p, va_list ap, va_list *ap_r)
{
  int i, numsize, sts;

  switch (fc) {
    case 'G':
    case 'g': {                                                                 // - either 'e'/'E' or 'f', depending on value
      GETFLOATNUM (1);                                                          // get floating point number to be converted
      if (p -> precision < 0) p -> precision = 6;                               // fix up the given precision
      if (p -> precision == 0) p -> precision = 1;
      ROUNDFLOAT (p -> precision - 1);                                          // round to given precision
      if ((p -> expon < -4) || (p -> expon >= p -> precision)) {                // use 'e'/'E' format if p -> expon < -4 or >= precision
        -- (p -> precision);
        fc -= 'g' - 'e';
        goto format_e;
      }
      p -> precision -= p -> expon + 1;                                         // otherwise, use 'f' format
      goto format_f;
    }

    case 'E':
    case 'e': {                                                                 // - floating point p -> expon notation
      GETFLOATNUM (1);                                                          // get floating point number to be converted
      if (p -> precision < 0) p -> precision = 6;                               // default of 6 places after decimal point
      ROUNDFLOAT (p -> precision);                                              // round to given precision
      goto format_e;
    }

    case 'F':
    case 'f': {                                                                 // - floating point standard notation
      GETFLOATNUM (1);                                                          // get floating point number to be converted
      if (p -> precision < 0) p -> precision = 6;                               // default of 6 places after decimal point
      ROUNDFLOAT (p -> precision + p -> expon);                                 // round to given precision
      goto format_f;
    }

    case 'A':
    case 'a': {                                                                 // - hexadecimal floating point
      GETFLOATNUM (0);
      PUTSIGN;
      PUTCH ('0');
      PUTCH ('x');
      union { __flt64_t d; __uint64_t u; __uint16_t a[4]; } v;
      v.d = p -> fnum;
      int exp = v.a[3] / 16 % 0x800;
      PUTCH ('0' + (exp != 0));
      unsigned int i, j;
      for (j = 0; j < 12; j ++) {
        if (((v.u >> (j * 4)) & 15) != 0) break;
      }
      i = 13;
      if (exp == 0) {
        exp = 1;
        do {
          unsigned int k = i - 1;
          unsigned int d = (v.u >> (k * 4)) & 15;
          if (d != 0) break;
          exp -= 4;
          i = k;
        } while (i > j);
      }
      if (i > j) {
        PUTCH ('.');
        do {
          -- i;
          unsigned int d = (v.u >> (i * 4)) & 15;
          char c = (d < 10) ? '0' + d : fc + d - 10;
          PUTCH (c);
        } while (i > j);
      }
      PUTCH ('p');
      exp -= 1023;
      if (exp < 0) {
        PUTCH ('-');
        exp = - exp;
      }
      char buf[4];
      i = 4;
      do {
        buf[--i] = exp % 10 + '0';
        exp /= 10;
      } while (exp > 0);
      do {
        PUTCH (buf[i]);
      } while (++ i < 4);
      goto done;
    }
  }

  // p->nfe = "inf" or "nan"
infnan:
  i = strlen (p -> nfe);                                                        // same as main vfprintf.c 's' processing
  if ((p -> precision < 0) || (p -> precision > i)) p -> precision = i;
  if (!(p ->leftjust)) PUTFC (p -> minwidth - p -> precision, ' ');
  PUTST (p -> precision, p -> nfe);
  PUTRIGHTFILL;

done:
  *ap_r = ap;
  return 0;

format_e:
  numsize = p -> precision;                                                     // size of number = number of digits after decimal point
  if ((numsize > 0) || p -> altform) numsize ++;                                //                + maybe it includes the decimal point
  numsize ++;                                                                   //                + always includes digit before decimal point
  if (p -> negative || p -> plussign || p -> posblank) numsize ++;              //                + maybe it includes room for the sign
  numsize += 3;                                                                 //                + e/E and two p -> expon digits
  if ((p -> expon < -99) || (p -> expon > 99)) numsize ++;                      //                + maybe another exponent digit
  if ((p -> expon < 0) || p -> plussign || p -> posblank) numsize ++;           //                + maybe it includes room for p -> expon sign
  PUTLEFTFILL (numsize);                                                        // output leading space padding
  PUTSIGN;                                                                      // output the sign character, if any
  PUTLEFTZERO (numsize);                                                        // output leading zero padding
  i = (int) p -> fnum;                                                          // get single digit in front of decimal point
  PUTCH ((char) i + '0');                                                       // output it
  if (p -> altform || (p -> precision > 0)) PUTCH ('.');                        // decimal point if altform or any digits after decimal pt
  while (p -> precision > 0) {                                                  // keep cranking out digits as long as precision says to
    p -> fnum -= i;
    p -> fnum *= 10.0;
    -- (p -> precision);
    i = (int) p -> fnum;
    PUTCH ((char) i + '0');
  }
  PUTCH (fc);                                                                   // output the 'e' or 'E'
  p -> negative = (p -> expon < 0);                                             // get p -> expon sign
  if (p -> negative) p -> expon = - p -> expon;                                 // make p -> expon positive
  PUTSIGN;                                                                      // output the p -> expon sign character, if any
  if (p -> expon > 99) PUTCH ((char) (p -> expon / 100) + '0');                 // output p -> expon hundreds digit
  PUTCH (((p -> expon / 10) % 10) + '0');                                       // output p -> expon tens digit
  PUTCH ((p -> expon % 10) + '0');                                              // output p -> expon units digit
  PUTRIGHTFILL;                                                                 // pad on right with spaces
  goto done;

format_f:
  numsize = p -> precision;                                                     // size of number = number of digits after decimal point
  if ((numsize > 0) || p -> altform) numsize ++;                                //                + maybe it includes the decimal point
  numsize ++;                                                                   //                + always includes digit before decimal point
  if (p -> expon > 0) numsize += p -> expon;                                    //                + this many more before the decimal point
  if (p -> negative || p -> plussign || p -> posblank) numsize ++;              //                + maybe it includes room for the sign
  PUTLEFTFILL (numsize);                                                        // output leading space padding
  PUTSIGN;                                                                      // output the sign character, if any
  PUTLEFTZERO (numsize);                                                        // output leading zero padding
  // format       = %11.6f
  // value        = 0.010638
  // p->expon     = -2
  // p->precision = 6
  // p->fnum      = 1.0638
  if (p -> expon < 0) PUTCH ('0');                                              // solitary '0' before decimal point
  else do {
    i = (int) p -> fnum;                                                        // get digit in front of decimal point
    PUTCH ((char) i + '0');                                                     // output it
    p -> fnum = (p -> fnum - i) * 10.0;                                         // remove from number and shift to get next digit
  } while (-- (p -> expon) >= 0);
  if (p -> altform || (p -> precision > 0)) {
    PUTCH ('.');                                                                // decimal point if altform or any digits after decimal pt
    while (-- (p -> precision) >= 0) {                                          // keep cranking out digits as long as precision says to
      if (++ (p -> expon) < 0) i = 0;
      else {
        i = (int) p -> fnum;
        p -> fnum = (p -> fnum - i) * 10.0;
      }
      PUTCH ((char) i + '0');
    }
  }
  PUTRIGHTFILL;                                                                 // pad on right with spaces
  goto done;
}

/* Get floating point into p -> fnum and p -> expon:  p -> fnum normalised in range 1.0 (inclusive) to 10.0 (exclusive) such that original_value = p -> fnum * 10 ** p -> expon */

int vfprintf_getfnum (Par *p, bool scale)
{
  switch (p -> fltsize) {
    case sizeof float: {
      float flt = va_arg (p -> ap, float);
      if (isnanf (flt)) {
        p -> nfe = "nan";
        return 0;
      }
      if (isinff (flt)) {
        p -> nfe = (flt < 0.0) ? "-inf" : p -> plussign ? "+inf" : p -> posblank ? " inf" : "inf";
        return 0;
      }
      p -> fnum = flt;
      break;
    }
    case sizeof double: {
      p -> fnum = va_arg (p -> ap, double);
      if (isnan (p -> fnum)) {
        p -> nfe = "nan";
        return 0;
      }
      if (isinf (p -> fnum)) {
        p -> nfe = (p -> fnum < 0.0) ? "-inf" : p -> plussign ? "+inf" : p -> posblank ? " inf" : "inf";
        return 0;
      }
      break;
    }
    default: assert (0);
  }
  p -> negative = (p -> fnum < 0.0);
  if (p -> negative) p -> fnum = - p -> fnum;
  p -> expon = 0;
  if (scale && (p -> fnum > 0.0)) {
    while (p -> fnum < 1.0) {
      p -> fnum *= 10.0;
      p -> expon --;
    }
    while (p -> fnum >= 10.0) {
      p -> fnum /= 10.0;
      p -> expon ++;
    }
  }
  return 1;
}

/* Round fnum to the given number of digits */

void vfprintf_roundflt (Par *p, int prec)
{
  double roundfact = 0.5;
  while (-- prec >= 0) roundfact /= 10.0;
  p -> fnum += roundfact;
  if (p -> fnum >= 10.0) {
    p -> fnum = 1.0;
    p -> expon ++;
  }
}
