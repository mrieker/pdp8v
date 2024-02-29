#ifndef _EXTARITH_H
#define _EXTARITH_H

#include "iodevs.h"
#include "miscdefs.h"

struct ExtArith : IODev {
    bool getgtflag ();
    void setgtflag (bool gt);

    virtual void ioreset ();
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input);

    uint16_t multquot;

private:
    uint16_t eae_modea (uint16_t opcode, uint16_t input);
    uint16_t eae_modeb (uint16_t opcode, uint16_t input);

    uint16_t getdeferredaddr ();

    uint16_t multiply (uint16_t input, uint16_t multiplier);
    uint16_t divide (uint16_t input, uint16_t divisor);
    uint16_t normalize (uint16_t input);

    bool gtflag;
    bool modeb;
    uint16_t nextreadaddr;
    uint16_t stepcount;
};

extern ExtArith extarith;

#endif
