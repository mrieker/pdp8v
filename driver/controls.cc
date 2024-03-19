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
// - tell raspictl main loop to halt, reset, run, step

#include <pthread.h>
#include <string.h>
#include <tcl.h>

#include "controls.h"
#include "memext.h"
#include "miscdefs.h"
#include "shadow.h"

// halt processor if not already
// - tells raspictl main to suspend clocking the processor
// - halts at the end of a cycle
// returns true: was already halted
//        false: was running, now halted
bool ctl_halt ()
{
    pthread_mutex_lock (&haltmutex);
    if (haltflags & HF_HALTED) {
        pthread_mutex_unlock (&haltmutex);
        return true;
    }
    haltflags |= HF_HALTIT;
    pthread_cond_broadcast (&haltcond);
    while (! (haltflags & HF_HALTED)) {
        pthread_cond_wait (&haltcond2, &haltmutex);
    }
    pthread_mutex_unlock (&haltmutex);
    return false;
}

// reset processor, optionally setting start address
bool ctl_reset (int addr)
{
    pthread_mutex_lock (&haltmutex);

    // processor must already be halted
    if (! (haltflags & HF_HALTED)) {
        pthread_mutex_unlock (&haltmutex);
        return false;
    }

    // tell it to reset but then halt immediately thereafter
    haltflags = HF_RESETIT | HF_HALTIT;
    pthread_cond_broadcast (&haltcond);

    // wait for it to halt again
    while (! (haltflags & HF_HALTED)) {
        pthread_cond_wait (&haltcond2, &haltmutex);
    }

    pthread_mutex_unlock (&haltmutex);

    // raspictl has reset the processor and the tubes are waiting at the end of FETCH1 with the clock still low
    // when resumed, raspictl will sample the GPIO lines, call shadow.clock() and resume processing from there

    // if 15-bit address given, load DF and IF with the frame and JMP to the address
    if (addr >= 0) {
        uint32_t masked_sample, sample, should_be;

        // set DF and IF registers to given frame
        memext.setdfif ((addr >> 12) & 7);

        // clock has been low for half cycle, at end of FETCH1 as a result of the reset()
        // G_DENA has been asserted for half cycle so we can read MDL,MD coming from cpu
        // processor should be asking us to read from memory
        sample = gpio->readgpio ();
        if ((sample & (G_CLOCK|G_RESET|G_IOS|G_QENA|G_IRQ|G_DENA|G_IOIN|G_DFRM|G_READ|G_WRITE|G_IAK)) !=
                (G_DENA|G_READ)) ABORT ();

        // drive clock high to transition to FETCH2
        shadow.clock (sample);
        gpio->writegpio (false, G_CLOCK);
        gpio->halfcycle (shadow.aluadd ());

        // half way through FETCH2 with clock still high
        // drop clock and start sending it a JMP @0 opcode
        gpio->writegpio (true, 05400 * G_DATA0);
        gpio->halfcycle (shadow.aluadd ());

        // end of FETCH2 - processor should be accepting the opcode we sent it
        sample = gpio->readgpio ();
        masked_sample = sample & (G_CLOCK|G_RESET|G_DATA|G_IOS|G_QENA|G_IRQ|G_DENA|G_JUMP|G_IOIN|G_DFRM|G_READ|G_WRITE|G_IAK);
        should_be = 05400 * G_DATA0 | G_QENA;
        if (masked_sample != should_be) ABORT ();

        // drive clock high to transition to DEFER1
        // keep sending opcode so tubes will clock it in
        shadow.clock (sample);
        gpio->writegpio (true, 05400 * G_DATA0 | G_CLOCK);
        gpio->halfcycle (shadow.aluadd ());

        // half way through DEFER1 with clock still high
        // drop clock and step to end of cycle
        gpio->writegpio (false, 0);
        gpio->halfcycle (shadow.aluadd ());

        // end of DEFER1 - processor should be asking us to read from memory location 0
        sample = gpio->readgpio ();
        if ((sample & (G_CLOCK|G_RESET|G_DATA|G_IOS|G_QENA|G_IRQ|G_DENA|G_JUMP|G_IOIN|G_DFRM|G_READ|G_WRITE|G_IAK)) !=
                (G_DENA|G_READ)) ABORT ();

        // drive clock high to transition to DEFER2
        shadow.clock (sample);
        gpio->writegpio (false, G_CLOCK);
        gpio->halfcycle (shadow.aluadd ());

        // half way through DEFER2 with clock still high
        // drop clock and start sending it the PC contents - 1 for the CLA CLL we're about to send
        gpio->writegpio (true, ((addr - 1) & 07777) * G_DATA0);
        gpio->halfcycle (shadow.aluadd ());

        // end of DEFER2 - processor should be accepting the address we sent it
        sample = gpio->readgpio ();
        masked_sample = sample & (G_CLOCK|G_RESET|G_DATA|G_IOS|G_QENA|G_IRQ|G_DENA|G_JUMP|G_IOIN|G_DFRM|G_READ|G_WRITE|G_IAK);
        should_be = ((addr - 1) & 07777) * G_DATA0 | G_QENA;
        if (masked_sample != should_be) ABORT ();

        // drive clock high to transition to EXEC1/JMP
        // keep sending the address - 1 so it gets clocked into MA
        shadow.clock (sample);
        gpio->writegpio (true, ((addr - 1) & 07777) * G_DATA0 | G_CLOCK);
        gpio->halfcycle (shadow.aluadd ());

        // drop clock and run to end of EXEC1/JUMP
        gpio->writegpio (false, 0);
        gpio->halfcycle (shadow.aluadd ());

        // end of EXEC1/JUMP - processor should be reading the opcode at the new address - 1 and about to clock address into PC
        sample = gpio->readgpio ();
        masked_sample = sample & (G_CLOCK|G_RESET|G_DATA|G_IOS|G_QENA|G_IRQ|G_DENA|G_JUMP|G_IOIN|G_DFRM|G_READ|G_WRITE|G_IAK);
        should_be = ((addr - 1) & 07777) * G_DATA0 | G_DENA | G_JUMP | G_READ;
        if (masked_sample != should_be) ABORT ();

        // drive clock high to transition to FETCH2
        shadow.clock (sample);
        gpio->writegpio (false, G_CLOCK);
        gpio->halfcycle (shadow.aluadd ());

        // half way through FETCH2 with clock still high
        // drop clock and start sending it a CLA CLL opcode
        gpio->writegpio (true, 07300 * G_DATA0);
        gpio->halfcycle (shadow.aluadd ());

        // end of FETCH2 - processor should be accepting the opcode we sent it
        sample = gpio->readgpio ();
        masked_sample = sample & (G_CLOCK|G_RESET|G_DATA|G_IOS|G_QENA|G_IRQ|G_DENA|G_JUMP|G_IOIN|G_DFRM|G_READ|G_WRITE|G_IAK);
        should_be = 07300 * G_DATA0 | G_QENA;
        if (masked_sample != should_be) ABORT ();

        // drive clock high to transition to EXEC1/GRP1
        // this also clocks incremented PC into PC, it should have correct address
        // keep sending the opcode so it gets clocked into IR
        shadow.clock (sample);
        gpio->writegpio (true, 07300 * G_DATA0 | G_CLOCK);
        gpio->halfcycle (shadow.aluadd ());

        // half way through EXEC1/GRP1
        // drop clock and step to end of cycle
        gpio->writegpio (false, 0);
        gpio->halfcycle (shadow.aluadd ());

        // end of EXEC1/GRP1 - don't really care what is on the bus
        sample = gpio->readgpio ();

        // drive clock high to transition to FETCH1
        shadow.clock (sample);
        gpio->writegpio (false, G_CLOCK);
        gpio->halfcycle (shadow.aluadd ());

        // half way through FETCH1 with clock still high
        // drop clock
        gpio->writegpio (false, 0);
        gpio->halfcycle (shadow.aluadd ());

        // clock has been low for half cycle, at end of FETCH1 as a result of the reset()
        // G_DENA has been asserted for half cycle so we can read MDL,MD coming from cpu
        // processor should be asking us to read memory at PC address
        sample = gpio->readgpio ();
        masked_sample = sample & (G_CLOCK|G_RESET|G_DATA|G_IOS|G_QENA|G_IRQ|G_DENA|G_JUMP|G_IOIN|G_DFRM|G_READ|G_WRITE|G_IAK);
        should_be = (addr & 07777) * G_DATA0 | G_DENA | G_READ;
        if (masked_sample != should_be) ABORT ();
    }

    // raspictl has reset the processor and the tubes are waiting at the end of FETCH1 with the clock still low
    // when resumed, raspictl will sample the GPIO lines, call shadow.clock() and resume processing from there

    return true;
}

// tell processor to start running
// - tells raspictl main to resume clocking the processor
bool ctl_run ()
{
    pthread_mutex_lock (&haltmutex);
    if (! (haltflags & HF_HALTED)) {
        pthread_mutex_unlock (&haltmutex);
        return false;
    }
    haltflags = 0;
    pthread_cond_broadcast (&haltcond);
    pthread_mutex_unlock (&haltmutex);
    return true;
}

// tell processor to step one cycle
// - tells raspictl main to clock the processor a single cycle
// - halts at the end of the next cycle
bool ctl_stepcyc ()
{
    pthread_mutex_lock (&haltmutex);

    // processor must already be halted
    if (! (haltflags & HF_HALTED)) {
        pthread_mutex_unlock (&haltmutex);
        return false;
    }

    // tell it to start, run one cycle, then halt
    haltflags = HF_HALTIT;
    pthread_cond_broadcast (&haltcond);

    // wait for it to halt again
    while (! (haltflags & HF_HALTED)) {
        pthread_cond_wait (&haltcond2, &haltmutex);
    }

    pthread_mutex_unlock (&haltmutex);
    return true;
}

// tell processor to step one instruction
// - tells raspictl main to clock the processor a single cycle
// - halts at the end of the next FETCH2 cycle
bool ctl_stepins ()
{
    pthread_mutex_lock (&haltmutex);

    // processor must already be halted
    if (! (haltflags & HF_HALTED)) {
        pthread_mutex_unlock (&haltmutex);
        return false;
    }

    do {

        // tell it to start, run one cycle, then halt
        haltflags = HF_HALTIT;
        pthread_cond_broadcast (&haltcond);

        // wait for it to halt again
        while (! (haltflags & HF_HALTED)) {
            pthread_cond_wait (&haltcond2, &haltmutex);
        }

    } while (shadow.r.state != Shadow::State::FETCH2);

    pthread_mutex_unlock (&haltmutex);

    return true;
}

// wait for control-C (raspictl halts processing on SIGINT when scripting)
void ctl_wait ()
{
    pthread_mutex_lock (&haltmutex);
    while (! (haltflags & HF_HALTED)) {
        pthread_cond_wait (&haltcond2, &haltmutex);
    }
    pthread_mutex_unlock (&haltmutex);
}
