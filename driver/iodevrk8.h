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

#ifndef _IODEVRK8_H
#define _IODEVRK8_H

// ref: small computer handbook 1972  pp 7-142..146

#include "iodevs.h"
#include "miscdefs.h"

struct IODevRK8 : IODev {
    enum RK8IO {
        RK8IO_EXIT,
        RK8IO_IDLE,
        RK8IO_READ,
        RK8IO_WRITE,
        RK8IO_CHECK,
        RK8IO_CLEAR
    };

    IODevRK8 ();
    virtual void ioreset ();
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input);

private:
    bool ros[4];
    int fds[4];

    pthread_cond_t cond;
    pthread_cond_t cond2;
    pthread_mutex_t lock;
    pthread_t threadid;

    RK8IO rk8io;

    uint16_t command;       // <11> : enable change in interrupt status
                            // <10> : enable program interrupt on transfer done
                            // <09> : enable interrupt on error
                            // <08> : unused
                            // <07> : seek track and surface only
                            // <06> : enable read/write of 2 header words
                            // <05> : extended memory address <14>
                            // <04> : extended memory address <13>
                            // <03> : extended memory address <12>
                            // <02> : disk file number <1>
                            // <01> : disk file number <0>
                            // <00> : unused

    uint16_t diskaddr;      // <11:04> : cylinder number
                            //    <03> : surface number
                            // <02:00> : sector number

    uint16_t memaddr;       // memory address <11:00>

    uint16_t status;        // <11> : error
                            // <10> : transfer done
                            // <09> : control busy error
                            // <08> : time out error
                            // <07> : parity or timing error
                            // <06> : data rate error
                            // <05> : track address error
                            // <04> : sector no good error
                            // <03> : write lock error
                            // <02> : track capacity exceeded error
                            // <01> : select error
                            // <00> : busy

    uint16_t wordcount;     // 2's complement word count

    uint16_t lastdas[4];

    void startio (RK8IO rk8io);

    static void *threadwrap (void *zhis);
    void thread ();
    void updateirq ();
};

extern IODevRK8 iodevrk8;

#endif
