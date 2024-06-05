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

#ifndef _IODEVRTC_H
#define _IODEVRTC_H

#include "iodevs.h"
#include "miscdefs.h"

#define NEXTOFLONSOFF 01777777777777777777777ULL

struct IODevRTC : IODev {
    IODevRTC ();
    virtual void ioreset ();
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input);

private:
    bool wait;              // waiting in skipoptwait()
    pthread_cond_t cond;    // thread sleeps waiting for counter overflow
    pthread_mutex_t lock;   // freezes everything
    pthread_t threadid;     // non-zero when thread running
    uint16_t buffer;        // buffer register
    uint16_t enable;        // enable register
    uint16_t status;        // status register
    uint16_t zcount;        // last value written to counter by i/o instruction
    uint32_t factor;        // time factor given by envar iodevrtc_factor
    uint64_t nextoflons;    // next counter overflow at this time (or NEXTOFLONSOFF if off)
    uint64_t basens;        // time counter was written extrapolated back to when it would have been zero
    uint64_t nowns;         // current time
    uint64_t nspertick;     // nanoseconds per clock tick including factor

    void update ();
    static void *threadwrap (void *zhis);
    void thread ();
    bool checkoflo ();
    bool calcnextoflo ();
    uint16_t getcounter ();
    void setcounter (uint16_t counter);
    void updintreq ();
    static uint64_t getnowns ();
};

extern IODevRTC iodevrtc;

#endif
