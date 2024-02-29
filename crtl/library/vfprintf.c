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
/*  Generic output conversion routines                                  */
/*                                                                      */
/*  'Extra' format types:                                               */
/*                                                                      */
/*    %S - printable string - arg is one char *                         */
/*                                                                      */
/*  Extra modifiers for %d, %f, %o, %u, %x:                             */
/*                                                                      */
/*    B - datatype is (u)Byte (8 bits)                                  */
/*    L - datatype is (u)Long (32 bits)                                 */
/*    P - datatype is __size_t                                          */
/*    Q - datatype is (u)Quad (64 bits)                                 */
/*    W - datatype is (u)Word (16 bits)                                 */
/*                                                                      */
/************************************************************************/

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fdfile.h"
#include "vfprintf.h"

/************************************************************************/
/*                                                                      */
/*    Input:                                                            */
/*                                                                      */
/*      stream->put() = write output                                    */
/*      format = pointer to null terminated format string               */
/*      ap     = conversion argument list pointer                       */
/*                                                                      */
/*    Output:                                                           */
/*                                                                      */
/*      returns < 0: as returned by stream->put()                       */
/*             else: number of chars written to output                  */
/*                                                                      */
/************************************************************************/

int vfprintf (FILE *stream, const char *format, va_list ap)
{
  char *cp, fc, ncb[32];
  const char *fp, *sp;
  int i, numsize;
  int siz, sts;
  Par par, *p;

  p = &par;                                                                     // point to param block
  memset (p, 0, sizeof *p);                                                     // clear out param block

  p -> stream = stream;
  p -> ap     = ap;

  for (fp = format; (fc = *fp) != 0;) {                                         // scan each character in the format string
    if (fc != '%') {                                                            // if not a percent, copy as is to output buffer
      sp = strchr (fp, '%');
      if (sp == NULL) i = strlen (fp);
      else i = sp - fp;
      PUTST (i, fp);
      fp += i;
      continue;
    }

    p -> altform   = 0;                                                         // percent, initialize param block values
    p -> zeropad   = 0;
    p -> leftjust  = 0;
    p -> posblank  = 0;
    p -> plussign  = 0;
    p -> minwidth  = 0;
    p -> precision = -1;
    p -> intsize   = sizeof (int);
    p -> fltsize   = sizeof (float);

    fp ++;                                                                      // skip over the percent

    /* Process format item prefix characters */

    while ((fc = *fp) != 0) {
      fp ++;
      switch (fc) {
        case '#': p -> altform ++; break;                                       // 'alternate form'
        case '0': if (!p -> leftjust) p -> zeropad  = 1; break;                 // zero pad the numeric value (ignore precision)
        case '-': p -> leftjust = 1;  p -> zeropad  = 0; break;                 // left justify space fill (otherwise it is right justified)
        case ' ': if (!p -> plussign) p -> posblank = 1; break;                 // leave a space for a plus sign if value is positive
        case '+': p -> plussign = 1;  p -> posblank = 0; break;                 // include a '+' if value is positive
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': 
        case '*': -- fp; GETFMTINT (p -> minwidth); break;                                  // minimum field width
        case '.': GETFMTINT (p -> precision); break;                                        // precision
        case 'h': p -> intsize  = sizeof (short); p -> fltsize = sizeof (float);  break;    // value is a 'short int'
        case 'l': p -> intsize += p -> intsize;   p -> fltsize = sizeof (double); break;    // value is a 'long int' or 'long long int'
        case 'B': p -> intsize  = sizeof (Byte); break;                                     // value is a 'Byte'
        case 'L': p -> intsize  = sizeof (Long);  p -> fltsize = sizeof (float);  break;    // value is a 'Long' or a 'long double'
        case 'P': p -> intsize  = sizeof (__size_t); break;                                 // value is a '__size_t'
        case 'Q': p -> intsize  = sizeof (Quad);  p -> fltsize = sizeof (double); break;    // value is a 'Quad'
        case 'W': p -> intsize  = sizeof (Word); break;                                     // value is a 'Word'

        default: goto gotfinal;                                                             // unknown, assume it's the final character
      }
    }
gotfinal:

    /* Process final format character using those prefixes */

    switch (fc) {

      /* Character and string conversions */

      case 'c': {                                                               // - single character
        fc = va_arg (p -> ap, char);                                            // get character to be output
        if (!(p -> leftjust)) PUTFC (p -> minwidth - 1, ' ');                   // output leading padding
        PUTCH (fc);                                                             // output the character
        PUTRIGHTFILL;                                                           // output trailing padding
        break;
      }

      case 's': {                                                               // - character string
        sp = va_arg (p -> ap, const char *);                                    // point to string to be output
        if (sp == NULL) sp = "(nil)";
        i = strlen (sp);                                                        // get length of the string
        if ((p -> precision < 0) || (p -> precision > i)) p -> precision = i;   // get length to be output
        if (!(p -> leftjust)) PUTFC (p -> minwidth - p -> precision, ' ');      // output leading padding
        PUTST (p -> precision, sp);                                             // output the string itself
        PUTRIGHTFILL;                                                           // output trailing padding
        break;
      }

      case 'S': {                                                               // - printable character string
        sp = va_arg (p -> ap, const char *);                                    // point to string to be output
        if (sp == NULL) sp = "(nil)";
        i = strlen (sp);                                                        // get length of the string
        if ((p -> precision < 0) || (p -> precision > i)) p -> precision = i;   // get length to be output
        if (!(p -> leftjust)) PUTFC (p -> minwidth - p -> precision, ' ');      // output leading padding
        while ((p -> precision > 0) && ((fc = *(sp ++)) != 0)) {
          p -> precision --;
          if ((fc == 0x7F) || (fc & 0x80) || (fc < ' ')) fc = '.';              // convert anything funky to a dot
          PUTCH (fc);                                                           // output the character
        }
        PUTRIGHTFILL;                                                           // output trailing padding
        break;
      }

      /* Integer conversions */

      case 'd':
      case 'i': {                                                               // - signed decimal
        GETSIGNEDNUM;                                                           // get value into 'p -> unum', with sign in 'p -> negative'
        p -> ncp = p -> nce = ncb + sizeof ncb;                                 // point to end of numeric conversion buffer
        do {                                                                    // convert to decimal string in end of ncb
          *(-- (p -> ncp)) = (char) (p -> unum % 10) + '0';
          p -> unum /= 10;
        } while (p -> unum != 0);
        PUTINT ("");
        break;
      }

      case 'o': {                                                               // - unsigned octal
        GETUNSIGNEDNUM;                                                         // get value into 'p -> unum'
        p -> ncp = p -> nce = ncb + sizeof ncb;                                 // point to end of numeric conversion buffer
        do {                                                                    // convert to octal string in end of ncb
          *(-- (p -> ncp)) = ((char) p -> unum & 7) + '0';
          p -> unum >>= 3;
        } while (p -> unum != 0);
        PUTINT ("0");
        break;
      }

      case 'u': {                                                               // - unsigned decimal
        GETUNSIGNEDNUM;                                                         // get value into 'p -> unum'
        p -> ncp = p -> nce = ncb + sizeof ncb;                                 // point to end of numeric conversion buffer
        do {                                                                    // convert to decimal string in end of ncb
          *(-- (p -> ncp)) = (char) (p -> unum % 10) + '0';
          p -> unum /= 10;
        } while (p -> unum != 0);
        PUTINT ("");
        break;
      }

      case 'x': {                                                               // - unsigned hexadecimal (lower case)
        GETUNSIGNEDNUM;                                                         // get value into 'p -> unum'
        p -> ncp = p -> nce = ncb + sizeof ncb;                                 // point to end of numeric conversion buffer
        do {                                                                    // convert to decimal string in end of ncb
          *(-- (p -> ncp)) = "0123456789abcdef"[(__size_t)p->unum&15];
          p -> unum >>= 4;
        } while (p -> unum != 0);
        PUTINT ("0x");
        break;
      }

      case 'X': {                                                               // - unsigned hexadecimal (caps)
        GETUNSIGNEDNUM;                                                         // get value into 'p -> unum'
        p -> ncp = p -> nce = ncb + sizeof ncb;                                 // point to end of numeric conversion buffer
        do {                                                                    // convert to decimal string in end of ncb
          *(-- (p -> ncp)) = "0123456789ABCDEF"[(__size_t)p->unum&15];
          p -> unum >>= 4;
        } while (p -> unum != 0);
        PUTINT ("0X");
        break;
      }

      /* Floating point conversions - in a different module so this one can be compiled without floating point */

      case 'A':
      case 'E':
      case 'F':
      case 'G':
      case 'a':
      case 'e':
      case 'f':
      case 'g': {
        va_list ap_r;
        sts = vfprintf_fp (fc, p, ap, &ap_r);
        if (sts < 0) return (sts);
        ap = ap_r;
        break;
      }

      /* Misc conversions */

      case 'p': {                                                               // - pointer
        p -> unum = (__size_t) va_arg (p -> ap, void *);                        // get value
        p -> negative = 0;                                                      // it's never negative by definition
        p -> ncp = p -> nce = ncb + sizeof ncb;                                 // point to end of numeric conversion buffer
        do {                                                                    // convert to decimal string in end of ncb
          *(-- (p -> ncp)) = "0123456789abcdef"[(__size_t)p->unum&15];
          p -> unum /= 16;
        } while (p -> unum != 0);
        p -> altform = 1;                                                       // always include the 0x
        PUTINT ("0x");
        break;
      }

      case 'n': {                                                               // - number of output characters so far
        switch (p -> intsize) {
          case 2:  *(va_arg (p -> ap, __int16_t *)) = p -> numout;
          case 4:  *(va_arg (p -> ap, __int32_t *)) = p -> numout;
          default: *(va_arg (p -> ap, int *)) = p -> numout;
        }
        break;
      }

      /* Don't know what it is, just output it as is (maybe it was '%%') */

      default: {
        PUTCH (fc);
        break;
      }
    }
  }

  return p -> numout;
}

/* Put the integer in p -> ncp..p -> nce, optionally prepending the alternate string and padding as required */

int putint (Par *p, char const *alt)
{
  int altneeded, leftfill, numdigs, numzeroes, signneeded;
  int sts;

  numdigs = p -> nce - p -> ncp;                                                // get number of digits in p -> ncp..p -> nce string

  signneeded = (p -> negative || p -> plussign || p -> posblank);               // maybe another char will be required for sign position

  altneeded = 0;                                                                // maybe need the alternate string
  if (p -> altform) altneeded = strlen (alt);

  numzeroes = 0;                                                                // maybe it needs zeroes on front for precision padding
  if (numdigs < p -> precision) numzeroes = p -> precision - numdigs;

  leftfill = signneeded + altneeded + numzeroes + numdigs;                      // output any left blank filling
  PUTLEFTFILL (leftfill);
  PUTSIGN;                                                                      // output the sign character, if any
  PUTST (altneeded, alt);                                                       // output the alternate string, if any
  PUTLEFTZERO (numzeroes + numdigs);                                            // put left zero padding (for minimum width)
  PUTFC (numzeroes, '0');                                                       // put left zeroes for precision
  PUTST (numdigs, p -> ncp);                                                    // output the digits as a string
  PUTRIGHTFILL;                                                                 // pad on the right with blanks if needed (for minimum width)

  return (0);
}

/* Output sign character, given p -> negative, plussign, posblank */

int putsign (Par *p)
{
  int sts;

  if (p -> negative) PUTCH ('-');
  else if (p -> plussign) PUTCH ('+');
  else if (p -> posblank) PUTCH (' ');

  return (0);
}

/* Output fill characters, flush if full */

int putfc (Par *p, int s, char c)
{
  int sts;

  if (s > 0) {
    p -> numout   += s;                                                         // this much more will have been output
    p -> minwidth -= s;                                                         // this fewer chars needed to fill output field minimum width
    do {
      sts = p -> stream -> put (&c, 1);
      if (sts < 0) return sts;
    } while (-- s > 0);
  }

  return (0);
}

/* Output string */

int putst (Par *p, int s, char const *b)
{
  if (s > 0) {
    p -> numout   += s;                                                         // this much more will have been output
    p -> minwidth -= s;                                                         // this fewer chars needed to fill output field minimum width
    int sts = p -> stream -> put (b, s);
    if (sts < 0) return sts;
  }

  return (0);
}

/* Output single character */

int putch (Par *p, char c)
{
  p -> numout ++;                                                               // one more char has been output
  p -> minwidth --;                                                             // one less char needed to fill output field minimum width
  return p -> stream -> put (&c, 1);
}

/* Get unsigned short/long/int into 'unum' */

void vfprintf_getunum (Par *p)
{
  switch (p -> intsize) {
    case sizeof uByte: p -> unum = va_arg (p -> ap, uByte); break;
    case sizeof uWord: p -> unum = va_arg (p -> ap, uWord); break;
    case sizeof uLong: p -> unum = va_arg (p -> ap, uLong); break;
    case sizeof uQuad: p -> unum = va_arg (p -> ap, uQuad); break;
    default: assert (0);
  }
  p -> negative = 0;
}

/* Get short/long/int into 'p -> unum', put the sign in 'p -> negative' */

void vfprintf_getsnum (Par *p)
{
  Quad snum;
  switch (p -> intsize) {
    case sizeof Byte: snum = va_arg (p -> ap, Byte); break;
    case sizeof Word: snum = va_arg (p -> ap, Word); break;
    case sizeof Long: snum = va_arg (p -> ap, Long); break;
    case sizeof Quad: snum = va_arg (p -> ap, Quad); break;
    default: assert (0);
  }
  p -> negative = (snum < 0);
  p -> unum = p -> negative ? - snum : snum;
}

/* Get integer in format string.  If *, use next integer argument, otherwise use decimal string */

char const *vfprintf_getfmtint (Par *p, char const *fp, int *fi_r)
{
  char fc;
  int fi;

  if (*fp == '*') {
    fp ++;
    fi = va_arg (p -> ap, int);
  } else {
    char *ep;
    fi = (int) strtoul (fp, &ep, 10);
    fp = ep;
  }

  *fi_r = fi;
  return fp;
}
