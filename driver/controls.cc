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
// - tell raspictl.cc main loop to stop, reset, run, step

#include <pthread.h>
#include <string.h>
#include <tcl.h>

#include "controls.h"
#include "memext.h"
#include "miscdefs.h"
#include "shadow.h"

// stop processor if not already
// - tells raspictl.cc main to suspend clocking the processor
// - stops in middle of a cycle with clock still high
// returns true: was already stopped
//        false: was running, now stopped
bool ctl_stop ()
{
    bool sigint = ctl_lock ();
    if (stopflags & SF_STOPPED) {

        // must be in middle of cycle with clock still high
        uint32_t sample = gpio->readgpio ();
        ASSERT (sample & G_CLOCK);

        ctl_unlock (sigint);
        return true;
    }

    // if nothing else has requested stop, request stop and say because of stop button
    if (! (stopflags & SF_STOPIT)) {
        stopreason = "STOPBUTTON";
        stopflags |= SF_STOPIT;
    }

    // if raspictl.cc main sleeping in an WFI instruction, wake it up
    wfiwake ();

    // wait for raspictl.cc main to stop cycling processor
    while (! (stopflags & SF_STOPPED)) {
        pthread_cond_wait (&stopcond2, &stopmutex);
    }

    // must be in middle of cycle with clock still high
    uint32_t sample = gpio->readgpio ();
    ASSERT (sample & G_CLOCK);

    ctl_unlock (sigint);
    return false;
}

// flag processor to stop for the given reason
// does not wait for processor to stop
bool ctl_stopfor (char const *reason)
{
    bool sigint = ctl_lock ();
    if (stopflags & SF_STOPIT) {
        ctl_unlock (sigint);
        return true;
    }

    // if nothing else has requested stop, request stop and say because of stop button
    stopreason = reason;
    stopflags |= SF_STOPIT;

    // if raspictl.cc main sleeping in an WFI instruction, wake it up
    wfiwake ();

    ctl_unlock (sigint);
    return false;
}

bool ctl_isstopped ()
{
    return (stopflags & SF_STOPPED) != 0;
}

// reset processor
// returns with processor stopped in middle of FETCH2 with clock still high
// ...and with DF,IF,PC set to the given 15-bit address
// returns false: processor running on entry, still running
//          true: processor was stopped on entry, now stopped in FETCH2
bool ctl_reset (uint16_t addr, bool resio)
{
    bool sigint = ctl_lock ();

    // processor must already be stopped
    if (! (stopflags & SF_STOPPED)) {
        ctl_unlock (sigint);
        return false;
    }

    // tell it what 15-bit address to reset to
    startpc = addr & 077777;
    resetio = resio;

    // tell main to reset but then stop immediately thereafter
    stopreason = "RESETBUTTON";
    stopflags  = SF_STOPIT | SF_RESETIT;
    stopwake ();

    // wait for raspictl.cc main to finish resetting
    while (! (stopflags & SF_STOPPED)) {
        pthread_cond_wait (&stopcond2, &stopmutex);
    }

    // raspictl.cc main has reset the processor and the tubes are waiting in the middle of FETCH2 with the clock still high
    // when resumed, raspictl.cc main will drive the clock low and resume processing from there
    ASSERT (shadow.r.state == Shadow::State::FETCH2);
    ASSERT (memext.iframe == (addr & 070000));
    ASSERT (memext.dframe == (addr & 070000));

    ctl_unlock (sigint);

    return true;
}

// tell processor to start running
// - tells raspictl.cc main to resume clocking the processor
bool ctl_run ()
{
    bool sigint = ctl_lock ();

    // maybe already running
    if (! (stopflags & SF_STOPPED)) {
        ctl_unlock (sigint);
        return false;
    }

    // maybe control-C was just pressed
    if (ctrlcflag) {
        stopreason = "CONTROL_C";
        ctl_unlock (sigint);
        return true;
    }

    // clock must be high (assumed by raspictl.cc stopped())
    uint32_t sample = gpio->readgpio ();
    ASSERT (sample & G_CLOCK);

    // tell raspictl.cc stopped() to resume cycling the processor
    stopreason = "";
    stopflags  = 0;
    stopwake ();
    ctl_unlock (sigint);
    return true;
}

// tell processor to step one cycle
// - tells raspictl.cc main to clock the processor a single cycle
// - stops in middle of next cycle with clock still high
bool ctl_stepcyc ()
{
    bool sigint = ctl_lock ();

    // processor must already be stopped
    if (! (stopflags & SF_STOPPED)) {
        ctl_unlock (sigint);
        return false;
    }

    // clock must be high (assumed by raspictl.cc stopped())
    uint32_t sample = gpio->readgpio ();
    ASSERT (sample & G_CLOCK);

    // tell it to start, run one cycle, then stop
    stopreason = "";
    stopflags  = SF_STOPIT;
    stopwake ();

    // wait for it to stop again
    while (! (stopflags & SF_STOPPED)) {
        pthread_cond_wait (&stopcond2, &stopmutex);
    }

    // clock should be high
    sample = gpio->readgpio ();
    ASSERT (sample & G_CLOCK);

    // if normal stop, say it's because of stepping
    if (stopreason[0] == 0) {
        stopreason = "SINGLECYCLE";
    }

    ctl_unlock (sigint);
    return true;
}

// tell processor to step one instruction
// - tells raspictl.cc main to clock the processor a single cycle
// - stops at the end of the next FETCH2 or INTAK1 cycle
bool ctl_stepins ()
{
    bool sigint = ctl_lock ();

    // processor must already be stopped
    if (! (stopflags & SF_STOPPED)) {
        ctl_unlock (sigint);
        return false;
    }

    // clock must be high (assumed by raspictl.cc stopped())
    uint32_t sample = gpio->readgpio ();
    ASSERT (sample & G_CLOCK);

    do {

        // tell it to start, run one cycle, then stop
        stopreason = "";
        stopflags  = SF_STOPIT;
        stopwake ();

        // wait for it to stop again
        while (! (stopflags & SF_STOPPED)) {
            pthread_cond_wait (&stopcond2, &stopmutex);
        }

        // clock should be high
        sample = gpio->readgpio ();
        ASSERT (sample & G_CLOCK);

        // stop if some error
        if (stopreason[0] != 0) break;

        // repeat until fetching next instruction
    } while ((shadow.r.state != Shadow::State::FETCH2) && (shadow.r.state != Shadow::State::INTAK1));

    // if normal stop, say it's because of stepping
    if (stopreason[0] == 0) {
        stopreason = "SINGLEINSTR";
    }

    ctl_unlock (sigint);

    return true;
}

// wait for stopped
void ctl_wait ()
{
    bool sigint = ctl_lock ();

    while (! (stopflags & SF_STOPPED)) {
        pthread_cond_wait (&stopcond2, &stopmutex);
    }

    if (strcmp (stopreason, "CONTROL_C") == 0) {
        ctrlcflag = false;
    }

    // must be in middle of cycle (clock is high)
    // assumed when we start it back up
    uint32_t sample = gpio->readgpio ();
    ASSERT (sample & G_CLOCK);

    ctl_unlock (sigint);
}

// load the given values into the tube's registers
// leaves the tubes stopped in the middle of FETCH2 with clock still high
// returns true: registers loaded
//        false: processor was not stopped
bool ctl_ldregs (bool newlink, uint16_t newac, uint16_t newma, uint16_t newpc)
{
    // reset processor and load newpc-2
    // this puts raspictl main() at wherever it would be for stopped in middle of FETCH2 with clock still high
    uint16_t savedframe = memext.dframe;
    startac = newac;
    startln = newlink;
    if (! ctl_reset (memext.iframe | newpc, false)) return false;
    memext.dframe = savedframe;

    // raspictl main() is still wherever it would be for stopped in middle of FETCH2 with clock still high
    ASSERT (shadow.r.state == Shadow::State::FETCH2);

    // verify results
    ASSERT (shadow.r.link == newlink);
    ASSERT (shadow.r.ac   == newac);
    ASSERT (shadow.r.ma   == newma);
    ASSERT (shadow.r.pc   == newpc);

    return true;
}

// block SIGINT while mutex is locked
static sigset_t blocksigint;
bool ctl_lock ()
{
    sigset_t oldsigs;
    sigaddset (&blocksigint, SIGINT);
    if (pthread_sigmask (SIG_BLOCK, &blocksigint, &oldsigs) != 0) ABORT ();
    if (pthread_mutex_lock (&stopmutex) != 0) ABORT ();
    return ! sigismember (&oldsigs, SIGINT);
}

void ctl_unlock (bool sigint)
{
    if (pthread_mutex_unlock (&stopmutex) != 0) ABORT ();
    if (sigint && (pthread_sigmask (SIG_UNBLOCK, &blocksigint, NULL) != 0)) ABORT ();
}
