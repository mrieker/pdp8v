#ifndef _MEMEXT_H
#define _MEMEXT_H

#include "iodevs.h"

struct MemExt : IODev {
    uint16_t iframe;                // IF is in <14:12>, all else is zeroes
                                    // linc mode uses <11:10> but clears it on exit
    uint16_t dframe;                // DF is in <14:12>, all else is zeroes
                                    // linc mode uses <11:10> but clears it on exit
    uint16_t iframeafterjump;       // copied to iframe at every jump (JMP or JMS)
    bool intdisableduntiljump;      // blocks intenabled until next jump (JMP or JMS)
    uint16_t savediframe;           // iframe saved at beginning of last interrupt
    uint16_t saveddframe;           // dframe saved at beginning of last interrupt

    MemExt ();
    void setdfif (uint32_t frame);
    void doingjump ();
    void doingread (uint16_t data);
    void intack ();
    bool intenabd ();
    bool candoio ();

    // implements IODev
    virtual void ioreset ();
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input);

private:
    bool intenableafterfetch;       // enable interrupts after next fetch
    bool intenabled;                // interrupts are enabled (unless intdisableduntiljump is also set)
    bool userflag;                  // false: execmode; true: usermode
    bool userflagafterjump;         // copied to userflag at every jump (JMP or JMS)
    uint8_t saveduserflag;          // userflag saved at beginning of last interrupt
};

extern MemExt memext;

#endif
