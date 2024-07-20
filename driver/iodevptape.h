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

#ifndef _IODEVPTAPE_H
#define _IODEVPTAPE_H

#include "iodevs.h"

struct IODevPTape : IODev {
    IODevPTape ();
    virtual ~IODevPTape ();

    virtual void ioreset ();
    virtual SCRet *scriptcmd (int argc, char const *const *argv);
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input);

private:
    bool intenab;       // interrupts enabled
    bool psfwait;       // waiting on optimized PSF
    bool punascii;      // punch file is ascii format
    bool punflag;       // punch ready to accept next byte
    bool punfull;       // punch busy punching a byte
    bool punquick;      // punch as fast as possible
    bool punrun;        // punch thread running
    bool punwarn;
    bool rdrascii;      // reader file is ascii format
    bool rdrflag;       // reader has byte ready to read
    bool rdrnext;       // reader should read next byte
    bool rdrquick;      // read as fast as possible
    bool rdrrun;        // reader thread running
    bool rdrwarn;
    bool rsfwait;       // waiting on optimized RSF
    int punfd;
    int rdrfd;
    uint8_t punbuff;
    uint8_t rdrbuff;
    uint8_t rdrinsert;  // insert this char into reader stream
    uint8_t rdrlastch;  // last character passed to processor

    pthread_t puntid;
    pthread_t rdrtid;

    pthread_cond_t puncond;
    pthread_cond_t rdrcond;
    pthread_mutex_t lock;

    void punstart ();
    void stoppunthread ();
    void startpunthread (int fd);
    void punthread ();

    void rdrstart ();
    void stoprdrthread ();
    void startrdrthread (int fd);
    void rdrthread ();

    void updintreq ();

    static void *punthreadwrap (void *zhis);
    static void *rdrthreadwrap (void *zhis);
};

extern IODevPTape iodevptape;

#endif
