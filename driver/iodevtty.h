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

#include <termios.h>

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
    bool kbflag;        // there is a char in kbbuff that processor hasn't read yet
    bool kbsetup;       // processor has tried to do i/o on this device since the beginning or since last connection dropped
                        //   if kbfd <  0, keyboard needs tcsetattr() immediately on connect if it's a tty
                        //   if kbfd >= 0, the tcsetattr() has already been done for the dev if it's a tty
    bool kbstdin;       // keyboard is using stdin
    bool intenab;       // interrupts enabled
    bool prflag;        // processor has written a char to prbuff that hasn't been printed yet (processor can set/clear at will)
    bool prfull;        // processor has written a char to prbuff that hasn't been printed yet
    bool stopping;      // threads are being stopped
    bool telnetd;       // being serviced in telnet daemon mode
    char ttydevname[8];
    int debug;
    int kbfd;           // -1: kb shut down or being shut down; else: fd for keyboard i/o
    int prfd;           // -1: pr shut down or being shut down; else: fd for printer i/o
    int tlfd;           // -1: tcp listener shut down or being shut down; else: fd for listening
    int servport;       // tcp port number
    pthread_t kbtid;    // non-zero iff keyboard thread running (set by creator, cleared by joiner)
    pthread_t prtid;    // non-zero iff printer thread running (set by creator, cleared by joiner)
    struct termios oldattr;
    uint32_t usperchr;
    uint16_t iobasem3;
    uint16_t lastin;
    uint16_t lastout;
    uint16_t lastpc;
    uint8_t kbbuff;
    uint8_t prbuff;
    uint8_t mask8;

    pthread_cond_t prcond;
    pthread_mutex_t lock;

    SCRetErr *openpipes (char const *kbname, char const *prname);
    void stopthreads ();
    void startdeflisten ();
    void tcpthread ();
    void kbthreadlk ();
    void prthread ();
    void dokbsetup ();
    void setkbrawmode ();
    void updintreqlk ();

    static void *tcpthreadwrap (void *zhis);
    static void *kbthreadwrap (void *zhis);
    static void *prthreadwrap (void *zhis);
};

extern IODevTTY iodevtty;
extern IODevTTY iodevtty40;
extern IODevTTY iodevtty42;
extern IODevTTY iodevtty44;
extern IODevTTY iodevtty46;

#endif
