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

#ifndef _IODEVRK8JE_H
#define _IODEVRK8JE_H

// ref: minicomputer handbook 1976 pp 9-115(224)..120(229)

#include "iodevs.h"
#include "miscdefs.h"

struct IODevRK8JE : IODev {
    IODevRK8JE ();
    virtual ~IODevRK8JE ();
    virtual void ioreset ();
    virtual SCRet *scriptcmd (int argc, char const *const *argv);
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input);

private:
    bool dskpwait;
    bool resetting;
    bool ros[4];
    int debug;
    int fds[4];
    uint32_t nsperus;
    uint16_t lastdebst;

    pthread_cond_t cond;
    pthread_cond_t cond2;
    pthread_mutex_t lock;
    pthread_t threadid;

    uint16_t dmapc;

    uint16_t command;       // <11:09> : func code: 0=read data; 1=read all; 2=set write prot; 3=seek only; 4=write data; 5=write all; 6/7=unused
                            //    <08> : interrupt on done
                            //    <07> : set done on seek done
                            //    <06> : block length: 0=256 words; 1=128 words
                            // <05:03> : extended memory address <14:12>
                            // <02:01> : drive select
                            //    <00> : cylinder number<07>

    uint16_t diskaddr;      // <11:05> : cylinder number<06:00>
                            //    <04> : surface number
                            // <03:00> : sector number<03:00>

    uint16_t memaddr;       // memory address <11:00>

    uint16_t status;        // <11> : status: 0=done; 1=busy
                            // <10> : motion: 0=stationary; 1=head in motion
                            // <09> : xfr capacity exceeded
                            // <08> : seek fail
                            // <07> : file not ready
                            // <06> : control busy
                            // <05> : timing error
                            // <04> : write lock error
                            // <03> : crc error
                            // <02> : data request late
                            // <01> : drive status error
                            // <00> : cylinder address error

    uint16_t lastdas[4];

    void startio ();

    static void *threadwrap (void *zhis);
    void thread ();
    void updateirq ();
};

extern IODevRK8JE iodevrk8je;

#endif
