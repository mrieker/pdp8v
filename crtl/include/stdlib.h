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
#ifndef _STDLIB_H
#define _STDLIB_H

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef int qsortcompar_t (void const *a, void const *b);

void          abort   ();
__size_t      allocsz (void *ptr);
void         *calloc  (__size_t nmem, __size_t size);
void          exit    (int status);
void          free    (void *ptr);
char         *getenv  (char const *name);
int           main    (int argc, char **argv);
void         *malloc  (__size_t size);
void          qsort   (void *base, __size_t nmemb, __size_t size, qsortcompar_t *compar);
void         *realloc (void *ptr, __size_t size);
double        strtod  (char const *nptr, char **endptr);
float         strtof  (char const *nptr, char **endptr);
long          strtol  (char const *nptr, char **endptr, unsigned int base);
unsigned long strtoul (char const *nptr, char **endptr, unsigned int base);

#endif
