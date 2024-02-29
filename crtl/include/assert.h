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
#ifndef _ASSERT_H
#define _ASSERT_H

#define assert(p) do { if (! (p)) assertfail (__FILE__, __LINE__); } while (0)

void assertfail (char const *file, int line);
void ass_putdbl (double dbl);
void ass_putflt (float flt);
void ass_putstr (char const *str);
void ass_putint (int val);
void ass_putptr (void *ptr);
void ass_putuint (unsigned int val);
void ass_putuinthex (unsigned int val);
void ass_putchr (char chr);
void ass_putbuf (char const *str, int len);

#endif
