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

#ifndef _IODEVTC08_H
#define _IODEVTC08_H

#include "iodevs.h"
#include "miscdefs.h"

struct IODevTC08Drive;
struct IODevTC08Shm;

struct IODevTC08 : IODev {
    IODevTC08 ();
    virtual ~IODevTC08 ();
    virtual void ioreset ();
    virtual SCRet *scriptcmd (int argc, char const *const *argv);
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input);

    static constexpr char const *shmname = "/raspictl-tc08";

private:
    IODevTC08Shm *shm;
    int shmfd;
    bool allowskipopt;
    bool dskpwait;

    static void *threadwrap (void *zhis);
    void thread ();
    bool stepskip (IODevTC08Drive *drive);
    bool stepxfer (IODevTC08Drive *drive);
    void dumpbuf (IODevTC08Drive *drive, char const *label, uint16_t const *buff);
    bool delayblk ();
    bool delayloop (int usec);
    void updateirq ();
};

struct IODevTC08Drive {
    int dtfd;
    uint16_t tapepos;
    bool rdonly;
    char fname[256];
};

struct IODevTC08Shm {
    IODevTC08Drive drives[8];
    bool iopend;
    bool resetting;
    bool startdelay;
    bool volatile exiting;
    bool volatile initted;
    int debug;
    int dtpid;
    pthread_cond_t cond;
    pthread_mutex_t lock;
    pthread_t threadid;
    uint16_t dmapc;
    uint16_t status_a;
    uint16_t status_b;
    uint64_t cycles;
};

extern IODevTC08 iodevtc08;

#endif
