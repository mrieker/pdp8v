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

// processor control functions
// - tell raspictl.cc main loop to halt, reset, run, step

#include <pthread.h>
#include <string.h>
#include <tcl.h>

#include "controls.h"
#include "memext.h"
#include "miscdefs.h"
#include "shadow.h"

// halt processor if not already
// - tells raspictl.cc main to suspend clocking the processor
// - halts at the end of a cycle, shadow.clock() not called yet
// returns true: was already halted
//        false: was running, now halted
bool ctl_halt ()
{
    bool sigint = ctl_lock ();
    if (haltflags & HF_HALTED) {

        // must be at end of cycle (clock is low)
        uint32_t sample = gpio->readgpio ();
        ASSERT (! (sample & G_CLOCK));

        ctl_unlock (sigint);
        return true;
    }

    // if nothing else has requested halt, request halt and say because of halt button
    if (! (haltflags & HF_HALTIT)) {
        haltreason = "HALTBUTTON";
        haltflags |= HF_HALTIT;
    }

    // if raspictl.cc main sleeping in an HLT instruction, wake it up
    haltwake ();

    // wait for raspictl.cc main to stop cycling processor
    while (! (haltflags & HF_HALTED)) {
        pthread_cond_wait (&haltcond2, &haltmutex);
    }

    // must be at end of cycle (clock is low)
    uint32_t sample = gpio->readgpio ();
    ASSERT (! (sample & G_CLOCK));

    ctl_unlock (sigint);
    return false;
}

bool ctl_ishalted ()
{
    return (haltflags & HF_HALTED) != 0;
}

// reset processor
// returns with processor halted at end of fetch1 with clock still low, shadow.clock() not called yet
// ...and with DF,IF,PC set to the given 15-bit address
bool ctl_reset (uint16_t addr)
{
    bool sigint = ctl_lock ();

    // processor must already be halted
    if (! (haltflags & HF_HALTED)) {
        ctl_unlock (sigint);
        return false;
    }

    // tell it what 15-bit address to reset to
    startpc = addr & 077777;

    // tell main to reset but then halt immediately thereafter
    haltreason = "RESETBUTTON";
    haltflags  = HF_HALTIT | HF_RESETIT;
    haltwake ();

    // wait for raspictl.cc main to finish resetting
    while (! (haltflags & HF_HALTED)) {
        pthread_cond_wait (&haltcond2, &haltmutex);
    }

    // raspictl.cc main has reset the processor and the tubes are waiting at the end of FETCH1 with the clock still low
    // when resumed, raspictl.cc main will sample the GPIO lines, call shadow.clock() and resume processing from there
    ASSERT (shadow.r.state == Shadow::State::FETCH1);
    ASSERT (memext.iframe == (addr & 070000));
    ASSERT (memext.dframe == (addr & 070000));
    uint32_t sample = gpio->readgpio ();
    ASSERT ((sample & (G_READ | G_WRITE | G_DATA | G_CLOCK)) == (G_READ | (addr & 07777U) * G_DATA0));

    ctl_unlock (sigint);

    return true;
}

// tell processor to start running
// - tells raspictl.cc main to resume clocking the processor
bool ctl_run ()
{
    bool sigint = ctl_lock ();

    // maybe already running
    if (! (haltflags & HF_HALTED)) {
        ctl_unlock (sigint);
        return false;
    }

    // clock must be low (assumed by raspictl.cc main)
    // ...and we haven't called shadow.clock() yet
    uint32_t sample = gpio->readgpio ();
    ASSERT (! (sample & G_CLOCK));

    // tell raspictl.cc main to resume cycling the processor
    haltreason = "";
    haltflags  = 0;
    haltwake ();
    ctl_unlock (sigint);
    return true;
}

// tell processor to step one cycle
// - tells raspictl.cc main to clock the processor a single cycle
// - halts at the end of the next cycle
bool ctl_stepcyc ()
{
    bool sigint = ctl_lock ();

    // processor must already be halted
    if (! (haltflags & HF_HALTED)) {
        ctl_unlock (sigint);
        return false;
    }

    // clock must be low (assumed by raspictl.cc main)
    uint32_t sample = gpio->readgpio ();
    ASSERT (! (sample & G_CLOCK));

    // tell it to start, run one cycle, then halt
    haltreason = "";
    haltflags  = HF_HALTIT;
    haltwake ();

    // wait for it to halt again
    while (! (haltflags & HF_HALTED)) {
        pthread_cond_wait (&haltcond2, &haltmutex);
    }

    // must be at end of cycle (clock is low)
    sample = gpio->readgpio ();
    ASSERT (! (sample & G_CLOCK));

    // if normal halt, say it's because of stepping
    if (haltreason[0] == 0) {
        haltreason = "SINGLECYCLE";
    }

    ctl_unlock (sigint);
    return true;
}

// tell processor to step one instruction
// - tells raspictl.cc main to clock the processor a single cycle
// - halts at the end of the next FETCH2 or INTAK1 cycle
bool ctl_stepins ()
{
    bool sigint = ctl_lock ();

    // processor must already be halted
    if (! (haltflags & HF_HALTED)) {
        ctl_unlock (sigint);
        return false;
    }

    // must be at end of cycle (clock is low)
    uint32_t sample = gpio->readgpio ();
    ASSERT (! (sample & G_CLOCK));

    do {

        // tell it to start, run one cycle, then halt
        haltreason = "";
        haltflags  = HF_HALTIT;
        haltwake ();

        // wait for it to halt again
        while (! (haltflags & HF_HALTED)) {
            pthread_cond_wait (&haltcond2, &haltmutex);
        }

        // stop if some error
        if (haltreason[0] != 0) break;

        // repeat until fetching next instruction
    } while ((shadow.r.state != Shadow::State::FETCH2) && (shadow.r.state != Shadow::State::INTAK1));

    // must be at end of cycle (clock is low)
    sample = gpio->readgpio ();
    ASSERT (! (sample & G_CLOCK));

    // if normal halt, say it's because of stepping
    if (haltreason[0] == 0) {
        haltreason = "SINGLEINSTR";
    }

    ctl_unlock (sigint);

    return true;
}

// wait for halted
void ctl_wait ()
{
    bool sigint = ctl_lock ();

    while (! (haltflags & HF_HALTED)) {
        pthread_cond_wait (&haltcond2, &haltmutex);
    }

    // must be at end of cycle (clock is low)
    // assumed when we start it back up
    uint32_t sample = gpio->readgpio ();
    ASSERT (! (sample & G_CLOCK));

    ctl_unlock (sigint);
}

// block SIGINT while mutex is locked
static sigset_t blocksigint;
bool ctl_lock ()
{
    sigset_t oldsigs;
    sigaddset (&blocksigint, SIGINT);
    if (pthread_sigmask (SIG_BLOCK, &blocksigint, &oldsigs) != 0) ABORT ();
    if (pthread_mutex_lock (&haltmutex) != 0) ABORT ();
    return ! sigismember (&oldsigs, SIGINT);
}

void ctl_unlock (bool sigint)
{
    if (pthread_mutex_unlock (&haltmutex) != 0) ABORT ();
    if (sigint && (pthread_sigmask (SIG_UNBLOCK, &blocksigint, NULL) != 0)) ABORT ();
}
