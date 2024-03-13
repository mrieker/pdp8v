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

#ifndef _MISCDEFS_H
#define _MISCDEFS_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define ABORT() do { fprintf (stderr, "abort() %s:%d\n", __FILE__, __LINE__); abort (); } while (0)
#define ASSERT(cond) do { if (__builtin_constant_p (cond)) { if (!(cond)) asm volatile ("assert failure line %c0" :: "i"(__LINE__)); } else { if (!(cond)) ABORT (); } } while (0)

#define IRQ_SCNINTREQ 00001
#define IRQ_LINECLOCK 00002
#define IRQ_RANDMEM   00004
#define IRQ_USERINT   00010
#define IRQ_TTYKBPR   00020
#define IRQ_PTAPE     00040
#define IRQ_RTC       00100
#define IRQ_RK8       00200

typedef unsigned long long LLU; // for printf %llu format

#include "gpiolib.h"

extern bool lincenab;
extern bool quiet;
extern bool randmem;
extern char **cmdargv;
extern GpioLib *gpio;
extern int cmdargc;
extern uint16_t lastreadaddr;
extern uint32_t watchwrite;

uint16_t readswitches (char const *swvar);
void haltinstr (char const *fmt, ...);
void clrintreqmask (uint16_t mask);
void setintreqmask (uint16_t mask);
uint16_t getintreqmask ();
char *getexedir (char *buf, int buflen);

#define HF_HALTED  1
#define HF_HALTIT  2
#define HF_RESETIT 4

extern pthread_cond_t haltcond;
extern pthread_cond_t haltcond2;
extern pthread_mutex_t haltmutex;
extern uint32_t haltflags;
extern uint32_t haltsample;

#include <exception>

struct ResetProcessorException : std::exception {
    virtual char const *what ();
};

#endif
