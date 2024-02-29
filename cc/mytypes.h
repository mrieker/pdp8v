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
#ifndef _MYTYPES_H
#define _MYTYPES_H

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#define TEXTPSECT ".50.text"
#define DATAPSECT ".70.data"

#define CARROFFS -2         // must match __malloc2()
#define RETBYVALSIZE 4
#define MAXALIGN 2
typedef uint16_t tsize_t;

extern FILE *sfile;
extern bool errorflag;

#define assert_false() do { assert (false); } while (true)

#endif
