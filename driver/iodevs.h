#ifndef _IODEVS_H
#define _IODEVS_H

#include <pthread.h>

#define IO_SKIP 0020000  // returned by ioinstr() to skip next instruction
#define IO_LINK 0010000  // link bit state passed into and returned by ioinstr()
#define IO_DATA 0007777  // accumulator passed into and returned by ioinstr()
#define UNSUPIO 0177777  // returned by ioinstr() for unsupported i/o instruction

#include "miscdefs.h"

struct IODevOps {
    uint16_t opcd;
    char const *desc;
};

struct IODev {
    uint16_t opscount;
    IODevOps const *opsarray;

    IODev ();

    virtual void ioreset () = 0;
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input) = 0;
};

extern pthread_cond_t intreqcond;
extern pthread_mutex_t intreqlock;

uint16_t ioinstr (uint16_t opcode, uint16_t input);
char const *iodisas (uint16_t opcode);
void ioreset ();

void clrintreqmask (uint16_t mask);
void setintreqmask (uint16_t mask);
uint16_t getintreqmask ();

#endif
