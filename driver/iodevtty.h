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

#ifndef _IODEVTTY_H
#define _IODEVTTY_H

#include "iodevs.h"

struct IODevTTY : IODev {
    IODevTTY (uint16_t iobase = 3);
    virtual ~IODevTTY ();

    bool keystruck ();
    void updintreq ();

    virtual void ioreset ();
    virtual SCRet *scriptcmd (int argc, char const *const *argv);
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input);

private:
    bool debug;
    bool kbflag;
    bool intenab;
    bool prflag;
    bool prfull;
    char ttydevname[8];
    char ttyenvname[24];
    int confd;
    int lisfd;
    int servport;
    uint32_t usperchr;
    uint16_t iobasem3;
    uint16_t lastin;
    uint16_t lastout;
    uint16_t lastpc;
    uint8_t kbbuff;
    uint8_t prbuff;
    uint8_t mask8;

    pthread_t listid;
    pthread_t prtid;

    pthread_cond_t prcond;
    pthread_mutex_t lock;

    void stopthreads ();
    void startdeflisten ();
    void startlisthread (int lfd);
    void listhread ();
    void kbthreadlk ();
    void prthread ();

    void updintreqlk ();

    static void *listhreadwrap (void *zhis);
    static void *prthreadwrap (void *zhis);
};

extern IODevTTY iodevtty;
extern IODevTTY iodevtty40;
extern IODevTTY iodevtty42;
extern IODevTTY iodevtty44;
extern IODevTTY iodevtty46;

#endif
