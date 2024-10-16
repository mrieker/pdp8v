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
#define IRQ_DTAPE     00400
#define IRQ_VC8       01000

#define MAXSTOPATS 16
#define STOPATWRITE 0x8000

#define UNSUPIO 0177777  // returned by ioinstr() for unsupported i/o instruction

typedef unsigned long long LLU; // for printf %llu format

#include "gpiolib.h"

extern bool ctrlcflag;
extern bool jmpdotstop;
extern bool lincenab;
extern bool os8zap;
extern bool quiet;
extern bool randmem;
extern bool resetio;
extern bool scriptmode;
extern bool skipopt;
extern bool startln;
extern bool tubesaver;
extern bool waitingforinterrupt;
extern char **cmdargv;
extern char const *stopreason;
extern GpioLib *gpio;
extern int cmdargc;
extern int numstopats;
extern uint16_t lastreadaddr;
extern uint16_t startac;
extern uint16_t startpc;
extern uint16_t switchregister;
extern uint16_t stopats[MAXSTOPATS];

uint16_t readswitches (char const *swvar);
void stopcheck ();
void stopordie (char const *reason);
void skipoptwait (uint16_t skipopcode, pthread_mutex_t *lock, bool *flag);
void clrintreqmask (uint16_t mask, bool wake = false);
void setintreqmask (uint16_t mask);
void waitforinterrupt ();
void wfiwake ();
void stopwake ();
uint16_t getintreqmask ();
char *getexedir (char *buf, int buflen);

#define SF_STOPPED  1
#define SF_STOPIT  2
#define SF_RESETIT 4

extern pthread_cond_t stopcond2;
extern pthread_mutex_t stopmutex;
extern uint32_t stopflags;

bool getmintimes ();
void setmintimes (bool mintimes);

#endif
