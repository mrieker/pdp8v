#ifndef _SCNCALL_H
#define _SCNCALL_H

#include <map>
#include <termios.h>

#define SCN_IO 06300    // base I/O instruction

#include "iodevs.h"
#include "miscdefs.h"

struct ScnCall : IODev {
    virtual void ioreset ();
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input);
    void exithandler ();

private:
    std::map<int,struct termios> savedtermioss;

    uint16_t docall (uint16_t data);
};

struct ScnCall2 : IODev {
    ScnCall2 ();
    virtual void ioreset ();
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input);
};

extern ScnCall scncall;
extern ScnCall2 scncall2;

#endif
