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

#ifndef _IODEVDTAPE_H
#define _IODEVDTAPE_H

#include "iodevs.h"
#include "miscdefs.h"

struct IODevDTapeDrive;
struct IODevDTapeShm;

struct IODevDTape : IODev {
    IODevDTape ();
    virtual ~IODevDTape ();
    virtual void ioreset ();
    virtual SCRet *scriptcmd (int argc, char const *const *argv);
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input);

    static constexpr char const *shmname = "/raspictl-dtape";

private:
    IODevDTapeShm *shm;
    int shmfd;

    static void *threadwrap (void *zhis);
    void thread ();
    bool stepskip (IODevDTapeDrive *drive);
    bool stepxfer (IODevDTapeDrive *drive);
    void dumpbuf (IODevDTapeDrive *drive, char const *label, uint16_t const *buff);
    bool delayus (int usec);
    void updateirq ();
};

struct IODevDTapeDrive {
    int dtfd;
    uint16_t tapepos;
    bool rdonly;
    char fname[256];
};

struct IODevDTapeShm {
    IODevDTapeDrive drives[8];
    bool iopend;
    bool resetting;
    bool startdelay;
    bool volatile exiting;
    bool volatile initted;
    int debug;
    pthread_cond_t cond;
    pthread_mutex_t lock;
    pthread_t threadid;
    uint16_t status_a;
    uint16_t status_b;
    uint64_t cycles;
};

extern IODevDTape iodevdtape;

#endif
