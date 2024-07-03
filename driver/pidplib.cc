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

#include <errno.h>
#include <fcntl.h>
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

#define SWROW1  16
#define SWROW2  17
#define SWROW3  18

#define LEDROW1 20
#define LEDROW2 21
#define LEDROW3 22
#define LEDROW4 23
#define LEDROW5 24
#define LEDROW6 25
#define LEDROW7 26
#define LEDROW8 27

// directions

// - https://www.raspberrypi.org/documentation/hardware/raspberrypi/gpio/gpio_pads_control.md
//   111... means 2mA; 777... would be 14mA

#define IGO 1

// - when writing LEDs
//   all switch rows set to inputs (hi-Z)
//   led row select outputs are active high (just one at a time)
//   column outputs are active low

#define LEDSEL0 (IGO*01111110000)   // GPIO[09:00]
#define LEDSEL1 (IGO*00000111111)   // GPIO[19:10]
#define LEDSEL2 (IGO*00011111111)   // GPIO[29:20]

// - when reading switches
//   switch row select outputs are active low (just one at a time)
//   column inputs are active low

#define SWSEL0  (IGO*00000000000)   // GPIO[09:00]
#define SWSEL1  (IGO*00111000000)   // GPIO[19:10]
#define SWSEL2  (IGO*00011111111)   // GPIO[29:20]

#define GPIO_FSEL0 (0)          // set output current / disable (readwrite)
#define GPIO_SET0 (0x1C/4)      // set gpio output bits (writeonly)
#define GPIO_CLR0 (0x28/4)      // clear gpio output bits (writeonly)
#define GPIO_LEV0 (0x34/4)      // read gpio bit levels (readonly)
#define GPIO_PUDMOD (0x94/4)    // pullup/pulldown enable (0=disable; 1=enable pulldown; 2=enable pullup)
#define GPIO_PUDCLK (0x98/4)    // which pins to change pullup/pulldown mode

static uint32_t ledscram (uint32_t data);
static uint32_t swunscram (uint32_t gpio);

PiDPLib::PiDPLib ()
{
    this->libname  = "pidplib";
    this->gpiopage = NULL;
    this->running  = false;
    this->threadid = 0;
}

PiDPLib::~PiDPLib ()
{
    this->close ();
}

void PiDPLib::open ()
{
    NohwLib::open ();

    gpiopage = (uint32_t volatile *) gpiofile.open ("/dev/gpiomem");

    this->running = true;
    int rc = pthread_create (&this->threadid, NULL, threadwrap, this);
    if (rc != 0) ABORT ();
}

void PiDPLib::close ()
{
    this->running = false;
    if (this->threadid != 0) {
        pthread_join (this->threadid, NULL);
        this->threadid = 0;
    }
    if (gpiopage != NULL) {
        gpiopage[GPIO_FSEL0+0] = 0;
        gpiopage[GPIO_FSEL0+1] = 0;
        gpiopage[GPIO_FSEL0+2] = 0;
        gpiopage = NULL;
    }
    gpiofile.close ();
    NohwLib::close ();
}

uint16_t PiDPLib::readhwsr ()
{
    return this->switchreg;
}

void *PiDPLib::threadwrap (void *zhis)
{
    ((PiDPLib *)zhis)->thread ();
    return NULL;
}

void PiDPLib::thread ()
{
    uint32_t buttonring[8];
    uint32_t lastbuttons = 0;
    int buttonindex = 0;
    memset (buttonring, 0, sizeof buttonring);
    this->retries = 0;

    while (this->running) {

        // set pins for outputting to LEDs
        gpiopage[GPIO_FSEL0+0] = LEDSEL0;
        gpiopage[GPIO_FSEL0+1] = LEDSEL1;
        gpiopage[GPIO_FSEL0+2] = LEDSEL2;

        // output LED rows
        uint32_t pc = ledscram (shadow.r.pc);
        writegpio ((0xFF << LEDROW1) | COLMASK, (1 << LEDROW1) | (pc ^ COLMASK));
        usleep (1000);

        uint32_t ma = ledscram (shadow.r.ma);
        writegpio ((0xFF << LEDROW1) | COLMASK, (1 << LEDROW2) | (ma ^ COLMASK));
        usleep (1000);

        if (! ctl_ishalted ()) this->membuffer = (this->readgpio () & G_DATA) / G_DATA0;
        uint32_t mb = ledscram (this->membuffer);
        writegpio ((0xFF << LEDROW1) | COLMASK, (1 << LEDROW3) | (mb ^ COLMASK));
        usleep (1000);

        uint32_t ac = ledscram (shadow.r.ac);
        writegpio ((0xFF << LEDROW1) | COLMASK, (1 << LEDROW4) | (ac ^ COLMASK));
        usleep (1000);

        uint32_t mq = ledscram (extarith.multquot);
        writegpio ((0xFF << LEDROW1) | COLMASK, (1 << LEDROW5) | (mq ^ COLMASK));
        usleep (1000);

        Shadow::State st = (Shadow::State) (shadow.r.state & 15);
        uint32_t ir  = (st == Shadow::State::FETCH2) ? (this->readgpio () & G_DATA) / G_DATA0 : shadow.r.ir;
        uint32_t lr6 = ((st == Shadow::State::FETCH1) || (st == Shadow::State::INTAK1)) ? 0 : ledscram (04000 >> ((ir >> 9) & 7));
        if (st == Shadow::State::FETCH1) lr6 |= 1 << COL9;
        if (st == Shadow::State::FETCH2) lr6 |= 1 << COL9;
        if (st == Shadow::State::DEFER1) lr6 |= 1 << COL11;
        if (st == Shadow::State::DEFER2) lr6 |= 1 << COL11;
        if (st == Shadow::State::DEFER3) lr6 |= 1 << COL11;
        if (st == Shadow::State::EXEC1)  lr6 |= 1 << COL10;
        if (st == Shadow::State::EXEC2)  lr6 |= 1 << COL10;
        if (st == Shadow::State::EXEC3)  lr6 |= 1 << COL10;
        writegpio ((0xFF << LEDROW1) | COLMASK, (1 << LEDROW6) | (lr6 ^ COLMASK));
        usleep (1000);

        uint32_t lr7 =
            ((st == Shadow::State::INTAK1) ? (1 << COL2) : 0) |     // BREAK (we use it for INTAK1 cycle)
            (memext.intenabd ()  ? (1 << COL3) : 0) |               // ION
            (waitingforinterrupt ? (1 << COL4) : 0) |               // PAUSE (we use it for 'wait for interrupt')
            (ctl_ishalted ()     ? 0 : (1 << COL5)) |               // RUN
            (extarith.stepcount << COL6);                           // SC
        writegpio ((0xFF << LEDROW1) | COLMASK, (1 << LEDROW7) | (lr7 ^ COLMASK));
        usleep (1000);

        uint32_t lr8 =
            ledscram ((memext.dframe >> 3) | (memext.iframe >> 6)) |
            (shadow.r.link ? (1 << COL7) : 0);
        writegpio ((0xFF << LEDROW1) | COLMASK, (1 << LEDROW8) | (lr8 ^ COLMASK));
        usleep (1000);

        // set pins for reading switches
        gpiopage[GPIO_FSEL0+0] = SWSEL0;
        gpiopage[GPIO_FSEL0+1] = SWSEL1;
        gpiopage[GPIO_FSEL0+2] = SWSEL2;

        // row 1 - switch register switches
        writegpio ((0xFF << LEDROW1) | (7 << SWROW1), 6 << SWROW1);
        usleep (100);
        switchreg = swunscram (~ gpiopage[GPIO_LEV0]);

        // row 2 - data field, instr field switches
        writegpio ((0xFF << LEDROW1) | (7 << SWROW1), 5 << SWROW1);
        usleep (100);
        this->dfifswitches = swunscram (~ gpiopage[GPIO_LEV0]);

        // row 3 - control buttons
        writegpio ((0xFF << LEDROW1) | (7 << SWROW1), 3 << SWROW1);
        usleep (100);
        uint32_t buttons = ~ gpiopage[GPIO_LEV0] & COLMASK;
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
}

// write value to GPIO pins
void PiDPLib::writegpio (uint32_t mask, uint32_t data)
{
    gpiopage[GPIO_SET0] = data;
    gpiopage[GPIO_CLR0] = data ^ mask;
}

// small computer handbook 1970 v12/p27

void PiDPLib::startbutton (uint32_t buttons)
{
    if (ctl_ishalted ()) {
        ctl_reset (memext.iframe | shadow.r.pc);
        contbutton (buttons);
    }
}

void PiDPLib::ldaddrbutton ()
{
    if (ctl_ishalted ()) {
        shadow.r.ma = shadow.r.pc = switchreg;
        memext.dframe = (dfifswitches & 07000) << 3;
        memext.iframe = (dfifswitches & 00700) << 6;
        membuffer = memarray[memext.iframe|shadow.r.ma];
    }
}

void PiDPLib::deposbutton ()
{
    if (ctl_ishalted ()) {
        shadow.r.ma = shadow.r.pc;
        memarray[memext.iframe|shadow.r.ma] = membuffer = switchreg;
        shadow.r.pc = (shadow.r.pc + 1) & 07777;
    }
}

void PiDPLib::exambutton ()
{
    if (ctl_ishalted ()) {
        shadow.r.ma = shadow.r.pc;
        membuffer = memarray[memext.iframe|shadow.r.ma];
        shadow.r.pc = (shadow.r.pc + 1) & 07777;
    }
}

void PiDPLib::contbutton (uint32_t buttons)
{
    if (ctl_ishalted ()) {
        if (buttons & (1 << COL7)) {
            do ctl_stepcyc ();  // single step - step one major state
            while (! atendofmajorstate ());
            this->membuffer = (this->readgpio () & G_DATA) / G_DATA0;
        } else if (buttons & (1 << COL8)) {
            ctl_stepins ();     // single instr - step one instruction
                                // stops at end of FETCH2 or end of INTAK1
            this->membuffer = (this->readgpio () & G_DATA) / G_DATA0;
        } else {
            ctl_run ();         // neither - continuous run
        }
    }
}

void PiDPLib::stopbutton ()
{
    if (! ctl_ishalted ()) {
        ctl_halt ();            // stops at end of any cycle
        while (! atendofmajorstate ()) {
            ctl_stepcyc ();     // cycle to end of major state
        }
        this->membuffer = (this->readgpio () & G_DATA) / G_DATA0;
    }
}

// halted at end of some cycle - see if it is the last cycle of a major state
bool PiDPLib::atendofmajorstate ()
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
