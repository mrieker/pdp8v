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

#ifndef _IODEVVC8_H
#define _IODEVVC8_H

#include "iodevs.h"

enum VC8Type {
    VC8TypeN = 0,
    VC8TypeE = 1,
    VC8TypeI = 2
};

struct VC8Pt {
    uint16_t x;
    uint16_t y;
    uint16_t t;
};

struct IODevVC8 : IODev {
    IODevVC8 ();
    virtual ~IODevVC8 ();

    virtual void ioreset ();
    virtual SCRet *scriptcmd (int argc, char const *const *argv);
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input);

private:
    bool resetting;
    bool waiting;
    int insert;
    int remove;
    uint16_t eflags;
    uint16_t xcobuf;
    uint16_t ycobuf;
    uint16_t intens;
    uint16_t pms;
    VC8Pt *buffer;
    VC8Type type;

    pthread_t threadid;
    pthread_cond_t cond;
    pthread_mutex_t lock;

    void insertpoint (uint16_t t);
    void wakethread ();
    void thread ();
    void updintreq ();

    static void *threadwrap (void *zhis);
};

extern IODevVC8 iodevvc8;

#endif
