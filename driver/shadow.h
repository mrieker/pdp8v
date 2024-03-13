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

#ifndef _SHADOW_H
#define _SHADOW_H

#include <exception>

#include "gpiolib.h"
#include "miscdefs.h"

#define NSAVEREGS 16

struct Shadow {
    enum State {
        FETCH1 = 0x00,
        FETCH2 = 0x01,
        DEFER1 = 0x02,
        DEFER2 = 0x03,
        DEFER3 = 0x04,
        ARITH1 = 0x05,
        AND2   = 0x06,
        TAD2   = 0x16,
        TAD3   = 0x17,
        ISZ1   = 0x25,
        ISZ2   = 0x26,
        ISZ3   = 0x27,
        DCA1   = 0x35,
        DCA2   = 0x36,
        JMP1   = 0x45,
        JMS1   = 0x55,
        JMS2   = 0x56,
        JMS3   = 0x57,
        IOT1   = 0x65,
        IOT2   = 0x66,
        GRPA1  = 0x75,
        GRPB1  = 0x85,
        INTAK1 = 0x08
    };

    struct Regs {
        State state;
        bool link;
        uint16_t ac, ir, ma, pc;
    };

    struct StateMismatchException : std::exception {
        virtual char const *what ();
    };

    Regs r;

    bool paddles;
    bool printinstr;
    bool printstate;

    Shadow ();
    void open (GpioLib *gpiolib);
    void reset ();
    void clock (uint32_t sample);
    bool aluadd ();
    uint64_t getcycles ();
    uint32_t readgpio (bool irq);

    static char const *statestr (State s);

private:
    bool acknown;
    bool irknown;
    bool lnknown;
    bool maknown;
    GpioLib *gpiolib;
    uint16_t alu;
    uint64_t cycle;

    Regs saveregs[NSAVEREGS];

    void checkgpio (uint32_t sample, uint32_t expect);
    uint16_t computegrpa (uint16_t *aluq);
    bool doesgrpbskip ();
    State firstexecstate ();
    State endOfInst (bool irq);

    static char const *boolstr (bool b);
};

extern Shadow shadow;

#endif
