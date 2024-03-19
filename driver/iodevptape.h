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
    bool intenab;
    bool punflag;
    bool punfull;
    bool punrun;
    bool punwarn;
    bool rdrflag;
    bool rdrinprog;
    bool rdrnext;
    bool rdrrun;
    bool rdrwarn;
    int punfd;
    int rdrfd;
    uint8_t punbuff;
    uint8_t rdrbuff;

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
