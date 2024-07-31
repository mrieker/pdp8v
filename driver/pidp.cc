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

// operate pidp panel via gpio connector
// assumes raspberry pi gpio connector is plugged into pidp panel

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "controls.h"
#include "extarith.h"
#include "gpiolib.h"
#include "memext.h"
#include "memory.h"
#include "pidp.h"
#include "rdcyc.h"
#include "shadow.h"

// gpio pins

                    //  LR1 LR2 LR3 LR4 LR5 LR6 LR7 LR8 SR1 SR2 SR3
#define COL1    14  //  pc1 ma1 mb1 ac1 mq1 and cad df1 sw1 df1 start   [MSB]
#define COL2    15  //  pc2 ma2 mb2 ac2 mq2 tad brk df2 sw2 df2 ldaddr
#define COL3     4  //  pc3 ma3 mb3 ac3 mq3 isz ion df3 sw3 df3 dep
#define COL4     5  //   .   .   .   .   .  dca pau if1  .  if1 exam
#define COL5     6  //   .   .   .   .   .  jms run if2  .  if2 cont
#define COL6     7  //   .   .   .   .   .  jmp sc1 if3  .  if3 stop
#define COL7     8  //   .   .   .   .   .  iot sc2 lnk  .      sstep
#define COL8     9  //   .   .   .   .   .  opr sc3      .      sinst
#define COL9    10  //   .   .   .   .   .  fet sc4      .
#define COL10   11  //   .   .   .   .   .  exe sc5      .
#define COL11   12  //   .   .   .   .   .  def          .
#define COL12   13  //   .   .   .   .   .  wct          .              [LSB]

#define COLMASK 0xFFF0

static bool running;
static pthread_t threadid;
static uint16_t dfifswitches;
static uint16_t membuffer;

static void *pidpthread (void *dummy);
static void startbutton (uint32_t buttons);
static void ldaddrbutton ();
static void deposbutton ();
static void exambutton ();
static void contbutton (uint32_t buttons);
static void stopbutton ();
static bool atendofmajorstate ();

static uint32_t ledscram (uint32_t data);
static uint32_t swunscram (uint32_t gpio);

void pidp_start ()
{
    ASSERT (threadid == 0);
    running = true;
    int rc = pthread_create (&threadid, NULL, pidpthread, NULL);
    if (rc != 0) ABORT ();
}

void pidp_stop ()
{
    running = false;
    if (threadid != 0) {
        pthread_join (threadid, NULL);
        threadid = 0;
    }
}

static void *pidpthread (void *dummy)
{
    uint32_t buttonring[8];
    uint32_t lastbuttons = 0;
    int buttonindex = 0;
    memset (buttonring, 0, sizeof buttonring);

    rdcycuninit ();

    // set up sequence number gt anything previously used
    PiDPMsg pidpmsg;
    memset (&pidpmsg, 0, sizeof pidpmsg);
    struct timespec nowts;
    if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) ABORT ();
    pidpmsg.seq = nowts.tv_sec * 1000000000ULL + nowts.tv_nsec;

    while (running) {
        pidpmsg.seq ++;

        if (! ctl_ishalted ()) membuffer = (gpio->readgpio () & G_DATA) / G_DATA0;

        Shadow::State st = (Shadow::State) (shadow.r.state & 15);
        uint32_t ir  = (st == Shadow::State::FETCH2) ? (gpio->readgpio () & G_DATA) / G_DATA0 : shadow.r.ir;
        uint32_t lr6 = ((st == Shadow::State::FETCH1) || (st == Shadow::State::INTAK1)) ? 0 : ledscram (04000 >> ((ir >> 9) & 7));
        if (st == Shadow::State::FETCH1) lr6 |= 1 << COL9;
        if (st == Shadow::State::FETCH2) lr6 |= 1 << COL9;
        if (st == Shadow::State::DEFER1) lr6 |= 1 << COL11;
        if (st == Shadow::State::DEFER2) lr6 |= 1 << COL11;
        if (st == Shadow::State::DEFER3) lr6 |= 1 << COL11;
        if (st == Shadow::State::EXEC1)  lr6 |= 1 << COL10;
        if (st == Shadow::State::EXEC2)  lr6 |= 1 << COL10;
        if (st == Shadow::State::EXEC3)  lr6 |= 1 << COL10;

        uint32_t lr7 =
            ((st == Shadow::State::INTAK1) ? (1 << COL2) : 0) |     // BREAK (we use it for INTAK1 cycle)
            (memext.intenabd ()  ? (1 << COL3) : 0) |               // ION
            (waitingforinterrupt ? (1 << COL4) : 0) |               // PAUSE (we use it for 'wait for interrupt')
            (ctl_ishalted ()     ? 0 : (1 << COL5)) |               // RUN
            ledscram (extarith.stepcount << 2);                     // SC

        uint32_t lr8 =
            ledscram ((memext.dframe >> 3) | (memext.iframe >> 6)) |
            (shadow.r.link ? (1 << COL7) : 0);

        // output LED rows
        pidpmsg.ledrows[0] = ledscram (shadow.r.pc);
        pidpmsg.ledrows[1] = ledscram (shadow.r.ma);
        pidpmsg.ledrows[2] = ledscram (membuffer);
        pidpmsg.ledrows[3] = ledscram (shadow.r.ac);
        pidpmsg.ledrows[4] = ledscram (extarith.multquot);
        pidpmsg.ledrows[5] = lr6;
        pidpmsg.ledrows[6] = lr7;
        pidpmsg.ledrows[7] = lr8;

        if (! pidp_proc (&pidpmsg)) continue;

        // row 1 - switch register switches
        switchregister = swunscram (~ pidpmsg.swrows[0]);

        // row 2 - data field, instr field switches
        dfifswitches = swunscram (~ pidpmsg.swrows[1]);

        // row 3 - control buttons
        uint32_t buttons = ~ pidpmsg.swrows[2] & COLMASK;
        buttonring[buttonindex] = buttons;
        buttonindex = (buttonindex + 1) % 8;

        // if button went from not pressed to pressed, perform function
        uint32_t butpressed = ~ lastbuttons & buttons;

        // if buttons have been steady past 8 iterations, we consider the buttons valid
        for (int i = 0; ++ i < 8;) {
            if (buttonring[i] != buttons) goto badbuttons;
        }

        if (butpressed & (1 << COL1)) startbutton (buttons);
        if (butpressed & (1 << COL2)) ldaddrbutton ();
        if (butpressed & (1 << COL3)) deposbutton ();
        if (butpressed & (1 << COL4)) exambutton ();
        if (butpressed & (1 << COL5)) contbutton (buttons);
        if (butpressed & (1 << COL6)) stopbutton ();
        lastbuttons = buttons;
    badbuttons:;
    }

    return NULL;
}

// small computer handbook 1970 v12/p27

static void startbutton (uint32_t buttons)
{
    if (ctl_ishalted ()) {
        ctl_reset (memext.iframe | shadow.r.pc);
        contbutton (buttons);
    }
}

static void ldaddrbutton ()
{
    if (ctl_ishalted ()) {
        shadow.r.ma = shadow.r.pc = switchregister;
        memext.dframe = (dfifswitches & 07000) << 3;
        memext.iframe = (dfifswitches & 00700) << 6;
        membuffer = memarray[memext.iframe|shadow.r.ma];
    }
}

static void deposbutton ()
{
    if (ctl_ishalted ()) {
        shadow.r.ma = shadow.r.pc;
        memarray[memext.iframe|shadow.r.ma] = membuffer = switchregister;
        shadow.r.pc = (shadow.r.pc + 1) & 07777;
    }
}

static void exambutton ()
{
    if (ctl_ishalted ()) {
        shadow.r.ma = shadow.r.pc;
        membuffer = memarray[memext.iframe|shadow.r.ma];
        shadow.r.pc = (shadow.r.pc + 1) & 07777;
    }
}

static void contbutton (uint32_t buttons)
{
    if (ctl_ishalted ()) {
        if (buttons & (1 << COL7)) {
            do ctl_stepcyc ();  // single step - step one major state
            while (! atendofmajorstate ());
            membuffer = (gpio->readgpio () & G_DATA) / G_DATA0;
        } else if (buttons & (1 << COL8)) {
            ctl_stepins ();     // single instr - step one instruction
                                // stops at end of FETCH2 or end of INTAK1
            membuffer = (gpio->readgpio () & G_DATA) / G_DATA0;
        } else {
            ctl_run ();         // neither - continuous run
        }
    }
}

static void stopbutton ()
{
    if (! ctl_ishalted ()) {
        ctl_halt ();            // stops at end of any cycle
        while (! atendofmajorstate ()) {
            ctl_stepcyc ();     // cycle to end of major state
        }
        membuffer = (gpio->readgpio () & G_DATA) / G_DATA0;
    }
}

// halted at end of some cycle - see if it is the last cycle of a major state
static bool atendofmajorstate ()
{
    Shadow::State st = shadow.r.state;
    switch (st) {
        case Shadow::State::FETCH1: return false;
        case Shadow::State::FETCH2: return true;
        case Shadow::State::DEFER1: return false;
        case Shadow::State::DEFER2: return (shadow.r.ma & 07770) != 00010;
        case Shadow::State::DEFER3: return true;
        case Shadow::State::ARITH1: return false;
        case Shadow::State::AND2  : return true;
        case Shadow::State::TAD2  : return false;
        case Shadow::State::TAD3  : return true;
        case Shadow::State::ISZ1  : return false;
        case Shadow::State::ISZ2  : return false;
        case Shadow::State::ISZ3  : return true;
        case Shadow::State::DCA1  : return false;
        case Shadow::State::DCA2  : return true;
        case Shadow::State::JMP1  : return true;
        case Shadow::State::JMS1  : return false;
        case Shadow::State::JMS2  : return false;
        case Shadow::State::JMS3  : return true;
        case Shadow::State::IOT1  : return false;
        case Shadow::State::IOT2  : return true;
        case Shadow::State::GRPA1 : return true;
        case Shadow::State::GRPB1 : return true;
        case Shadow::State::INTAK1: return true;
        default: ABORT ();
    }
}

// scramble the given 12-bit data word to gpio pins
static uint32_t ledscram (uint32_t data)
{
    uint32_t gpio = 0;
    if (data & 04000) gpio |= 1 << COL1;
    if (data & 02000) gpio |= 1 << COL2;
    if (data & 01000) gpio |= 1 << COL3;
    if (data & 00400) gpio |= 1 << COL4;
    if (data & 00200) gpio |= 1 << COL5;
    if (data & 00100) gpio |= 1 << COL6;
    if (data & 00040) gpio |= 1 << COL7;
    if (data & 00020) gpio |= 1 << COL8;
    if (data & 00010) gpio |= 1 << COL9;
    if (data & 00004) gpio |= 1 << COL10;
    if (data & 00002) gpio |= 1 << COL11;
    if (data & 00001) gpio |= 1 << COL12;
    return gpio;
}

// unscramble the given gpio pins to 12-bit data word
static uint32_t swunscram (uint32_t gpio)
{
    uint32_t data = 0;
    if (gpio & (1 << COL1))  data |= 04000;
    if (gpio & (1 << COL2))  data |= 02000;
    if (gpio & (1 << COL3))  data |= 01000;
    if (gpio & (1 << COL4))  data |= 00400;
    if (gpio & (1 << COL5))  data |= 00200;
    if (gpio & (1 << COL6))  data |= 00100;
    if (gpio & (1 << COL7))  data |= 00040;
    if (gpio & (1 << COL8))  data |= 00020;
    if (gpio & (1 << COL9))  data |= 00010;
    if (gpio & (1 << COL10)) data |= 00004;
    if (gpio & (1 << COL11)) data |= 00002;
    if (gpio & (1 << COL12)) data |= 00001;
    return data;
}
