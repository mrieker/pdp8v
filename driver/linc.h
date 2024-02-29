#ifndef _LINC_H
#define _LINC_H

#include "iodevs.h"

struct Linc : IODev {
    uint16_t specfuncs; // special function mask <09:05>

    Linc ();

    // implements IODev
    virtual void ioreset ();
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input);

private:
    uint16_t lincac;    // 12-bit accumulator
    uint16_t lincpc;    // 10-bit program counter
    bool disabjumprtn;
    bool link;          // link bit
    bool flow;          // overflow bit

    void execute ();
    void execbeta (uint16_t lastfetchaddr, uint16_t op);
    bool execalpha (uint16_t lastfetchaddr, uint16_t op);
    bool execgamma (uint16_t lastfetchaddr, uint16_t op);
    void execiob (uint16_t lastfetchaddr, uint16_t op);
    void unsupinst (uint16_t lastfetchaddr, uint16_t op);
    uint16_t addition (uint16_t a, uint16_t b, bool lame);
};

extern Linc linc;

#endif
