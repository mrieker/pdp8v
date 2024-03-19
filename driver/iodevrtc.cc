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

// DK8-EP programmable real time clock
// small-computer-handbook-1972.pdf p7-25/p245

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/time.h>
#include <unistd.h>

#include "iodevrtc.h"
#include "memory.h"
#include "shadow.h"

IODevRTC iodevrtc;

static IODevOps const iodevops[] = {
    { 06130, "CLZE (RTC) clear the given enable bits" },
    { 06131, "CLSK (RTC) skip if requesting interrupt" },
    { 06132, "CLDE (RTC) set the given enable bits" },
    { 06133, "CLAB (RTC) set buffer and counter to given value" },
    { 06134, "CLEN (RTC) read enable register" },
    { 06135, "CLSA (RTC) read status register and reset it" },
    { 06136, "CLBA (RTC) read buffer register" },
    { 06137, "CLCA (RTC) read counter into buffer and read buffer" },
};

IODevRTC::IODevRTC ()
{
    opscount = sizeof iodevops / sizeof iodevops[0];
    opsarray = iodevops;

    iodevname = "rtc";

    pthread_cond_init (&this->cond, NULL);
    pthread_mutex_init (&this->lock, NULL);
    this->threadid = 0;
}

// reset the device
// - clear flags
// - kill threads
void IODevRTC::ioreset ()
{
    char const *cpsenv = getenv ("iodevrtc_factor");
    this->factor = (cpsenv == NULL) ? 1 : atoi (cpsenv);

    pthread_mutex_lock (&this->lock);

    this->enable = 0;
    this->status = 0;
    this->nextoflons = NEXTOFLONSOFF;
    pthread_cond_broadcast (&this->cond);

    clrintreqmask (IRQ_RTC);

    pthread_mutex_unlock (&this->lock);

    if (this->threadid != 0) {
        pthread_join (this->threadid, NULL);
    }
}

// perform i/o instruction
uint16_t IODevRTC::ioinstr (uint16_t opcode, uint16_t input)
{
    pthread_mutex_lock (&this->lock);

    // maybe thread was about to wake up and notice an overflow
    // we don't want to miss one in case this i/o resets the counter
    this->nowns = getnowns ();
    this->checkoflo ();

    // process opcode
    switch (opcode) {

        // clear the given enable bits
        case 06130: {
            this->enable &= ~ input;
            this->update ();
            break;
        }

        // skip if requesting interrupt
        case 06131: {
            if (getintreqmask () & IRQ_RTC) input |= IO_SKIP;
            break;
        }

        // set the given enable bits
        case 06132: {
            this->enable |= input & IO_DATA;
            this->update ();
            break;
        }

        // set buffer and counter to given value
        case 06133: {
            this->buffer = input & IO_DATA;
            this->setcounter (this->buffer);
            this->update ();
            break;
        }

        // read enable register
        case 06134: {
            input &= ~ IO_DATA;
            input |= this->enable;
            break;
        }

        // read status register and reset it
        case 06135: {
            input |= this->status;
            this->status = 0;
            clrintreqmask (IRQ_RTC);
            break;
        }

        // read buffer register
        case 06136: {
            input &= ~ IO_DATA;
            input |= this->buffer;
            break;
        }

        // read counter into buffer and read buffer
        case 06137: {
            input &= ~ IO_DATA;
            input |= this->buffer = this->getcounter ();
            break;
        }

        default: {
            input = UNSUPIO;
            break;
        }
    }
    pthread_mutex_unlock (&this->lock);
    return input;
}

// update divider chain and counter given that an i/o was done to change something
void IODevRTC::update ()
{
    uint32_t newnspertick = 0;
    switch ((this->enable >> 6) & 7) {
        case 2: newnspertick = 10000000; break;     // 100Hz
        case 3: newnspertick =  1000000; break;     //   1KHz
        case 4: newnspertick =   100000; break;     //  10KHz
        case 5: newnspertick =    10000; break;     // 100KHz
        case 6: newnspertick =     1000; break;     //   1MHz
    }
    newnspertick *= this->factor;
    if (this->nspertick != newnspertick) {
        uint16_t oldcounter = this->getcounter ();
        this->nspertick = newnspertick;
        this->setcounter (oldcounter);
    }

    // compute next overflow
    // start thread if needed to post interrupt
    if (this->calcnextoflo ()) {
        pthread_cond_broadcast (&this->cond);
        if ((this->threadid == 0) && (this->enable & 04000) && (this->nextoflons != NEXTOFLONSOFF)) {
            int rc = pthread_create (&this->threadid, NULL, threadwrap, this);
            if (rc != 0) {
                fprintf (stderr, "IODevRTC::update: error %d creating thread: %s\n", rc, strerror (rc));
                ABORT ();
            }
        }
    }

    // update interrupt request
    if (this->status & this->enable & 04000) {
        setintreqmask (IRQ_RTC);
    } else {
        clrintreqmask (IRQ_RTC);
    }
}

// thread what sets status and posts interrupt when counter overflows
void *IODevRTC::threadwrap (void *zhis)
{
    ((IODevRTC *)zhis)->thread ();
    return NULL;
}
void IODevRTC::thread ()
{
    struct timespec timeout;
    memset (&timeout, 0, sizeof timeout);
    pthread_mutex_lock (&this->lock);

    // keep going until/unless we are reset
    while (this->nextoflons != NEXTOFLONSOFF) {
        this->nowns = getnowns ();

        // see if counter has overflowed
        // ...and post interrupt if so
        if (! this->checkoflo ()) {

            // it hasn't, wait until next overflow will happen
            if (this->nextoflons >= NEXTOFLONSOFF - 1) {
                int rc = pthread_cond_wait (&this->cond, &this->lock);
                if ((rc != 0) && (rc != ETIMEDOUT)) ABORT ();
            } else {
                timeout.tv_sec  = this->nextoflons / 1000000000;
                timeout.tv_nsec = this->nextoflons % 1000000000;
                int rc = pthread_cond_timedwait (&this->cond, &this->lock, &timeout);
                if ((rc != 0) && (rc != ETIMEDOUT)) ABORT ();
            }
        }
    }

    // all done, say we are exiting
    this->threadid = 0;
    pthread_mutex_unlock (&this->lock);
}

// check counter overflow, set flag and post interrupt if so and reset counter
bool IODevRTC::checkoflo ()
{
    // if next overflow happens in future, don't post interrupt yet but wait for correct time
    if (this->nextoflons > this->nowns) return false;

    // overflow happened in the past, so set the status bit and request interrupt
    this->status |= 04000;
    if (this->enable & 04000) setintreqmask (IRQ_RTC);

    // calculate next overflow supposedly in the future
    this->calcnextoflo ();
    ASSERT (this->nextoflons > this->nowns);
    return true;
}

// calculate next timer overflow
// returns true: new time was calculated
//        false: same time as before
bool IODevRTC::calcnextoflo ()
{
    // default to very far in future, but not far enough so thread exits
    uint64_t newnextoflons = NEXTOFLONSOFF - 1;

    // make sure timing scaling has been set up
    if (this->nspertick != 0) {
        uint64_t tickssincebase = (this->nowns - this->basens) / this->nspertick;

        switch ((this->enable >> 9) & 3) {

            // 4096 ticks per cycle that started at this->basens
            case 0: {
                uint64_t completedcycles = tickssincebase / 4096;
                uint64_t nextoflotickssincebase = (completedcycles + 1) * 4096;
                newnextoflons = this->basens + this->nspertick * nextoflotickssincebase;
                break;
            }

            // 4096-buffer ticks per cycle that started at this->basens
            // but the first cycle has 4096 ticks
            case 1: {

                // first cycle is full 4096 ticks since basens
                // so if before then, next overflow is at that time
                if (tickssincebase < 4096) {
                    newnextoflons = this->basens + this->nspertick * 4096;
                    break;
                }

                // the remaining cycles are 4096 - this->buffer ticks
                uint32_t tickspercycle = 4096 - this->buffer;
                uint64_t completedshortcycles = (tickssincebase - 4096) / tickspercycle;
                uint64_t nextoflotickssinceendoffirstcycle = (completedshortcycles + 1) * tickspercycle;
                uint64_t nextoflotickssincebase = nextoflotickssinceendoffirstcycle + 4096;
                newnextoflons = this->basens + this->nspertick * nextoflotickssincebase;
                break;
            }
        }
    }

    // tell timer thread when new overflow will be
    if (this->nextoflons == newnextoflons) return false;
    this->nextoflons = newnextoflons;
    return true;
}

// get what counter value should be now
uint16_t IODevRTC::getcounter ()
{
    if (this->nspertick == 0) return this->zcount;
    uint64_t deltans = this->nowns - this->basens;
    uint64_t ticks   = deltans / this->nspertick;
    switch ((this->enable >> 9) & 3) {
        case 0: return ticks & 07777;
        case 1: {
            if (ticks <= 07777) return ticks;
            ticks -= 010000;
            ticks %= 010000 - this->buffer;
            return ticks + this->buffer;
        }
    }
    return this->zcount;
}

// set a new counter value
void IODevRTC::setcounter (uint16_t counter)
{
    this->zcount = counter;
    uint64_t deltans = counter * this->nspertick;
    this->basens = this->nowns - deltans;
}

// get time that pthread_cond_timedwait() is based on
uint64_t IODevRTC::getnowns ()
{
    struct timespec nowts;
    if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) ABORT ();
    return ((uint64_t) nowts.tv_sec) * 1000000000 + nowts.tv_nsec;
}
