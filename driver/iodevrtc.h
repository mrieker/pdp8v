#ifndef _IODEVRTC_H
#define _IODEVRTC_H

#include "iodevs.h"
#include "miscdefs.h"

#define NEXTOFLONSOFF 01777777777777777777777ULL

struct IODevRTC : IODev {
    IODevRTC ();
    void ioreset ();
    uint16_t ioinstr (uint16_t opcode, uint16_t input);

private:
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
    static uint64_t getnowns ();
};

extern IODevRTC iodevrtc;

#endif
