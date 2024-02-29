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

#ifndef _MEMORY_H
#define _MEMORY_H

#define MEMSIZE 0100000

#include "iodevs.h"
#include "miscdefs.h"

struct MemPar : IODev {
    virtual void ioreset ();
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input);
};

double   readdoub  (uint64_t i48);
float    readflt   (uint32_t i24);
uint64_t writedoub (double   f64);
uint32_t writeflt  (float    f32);

uint16_t readmemword (uint16_t addr);
uint32_t readmemlong (uint16_t addr);
uint64_t readmemquad (uint16_t addr);
double   readmemdoub (uint16_t addr);
float    readmemflt  (uint16_t addr);

void writememword (uint16_t addr, uint16_t data);
void writememlong (uint16_t addr, uint32_t data);
void writememquad (uint16_t addr, uint64_t data);
void writememdoub (uint16_t addr, double   data);
void writememflt  (uint16_t addr, float    data);

typedef void GetBuf  (uint16_t size, uint16_t addr, uint8_t *buff);
typedef void GetStrz (uint16_t addr, uint8_t *buff);
typedef void PutBuf  (uint16_t size, uint16_t addr, uint8_t const *buff);

GetBuf  getbuf6,  getbuf8,  getbuf12;
GetStrz getstrz6, getstrz8, getstrz12;
PutBuf  putbuf6,  putbuf8,  putbuf12;

extern MemPar mempar;
extern uint16_t *memarray;

#endif
