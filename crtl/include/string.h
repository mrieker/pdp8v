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
#ifndef _STRING_H
#define _STRING_H

void *memchr (void const *src, char chr, __size_t len);
int memcmp (void const *src, void const *dst, __size_t len);
void memcpy (void *dst, void const *src, __size_t len);
void memmove (void *dst, void const *src, __size_t len);
void memset (void *dst, char chr, __size_t len);
int strcasecmp (char const *a, char const *b);
int strncasecmp (char const *a, char const *b, __size_t len);
char *strchr (char const *s, char c);
int strcmp (char const *a, char const *b);
void strcpy (char *d, char const *s);
char *strdup (char const *s);
__size_t strlen (char const *s);

#endif
