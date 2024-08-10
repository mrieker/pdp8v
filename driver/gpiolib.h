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

#ifndef _GPIOLIB_H
#define _GPIOLIB_H

#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <string>

#define DEFADDHZ 20000  // seems happy with vpp=117.3, vgg=-87.3
#define DEFCPUHZ 32000  // ...giving raspictl -randmem => 25340 Hz

#define G_CLOCK       0x4   // GPIO[02]   out: send clock signal to cpu
#define G_RESET       0x8   // GPIO[03]   out: send reset signal to cpu
#define G_DATA     0xFFF0   // GPIO[15:04] bi: data bus bits
#define G_LINK    0x10000   // GPIO[16]    bi: link bus bit
#define G_IOS     0x20000   // GPIO[17]   out: skip next instruction (valid in IOT.2 only)
#define G_TRIG    0x40000   // GPIO[18]   out: trigger scope
#define G_QENA    0x80000   // GPIO[19]   out: we are sending data out to cpu
#define G_IRQ    0x100000   // GPIO[20]   out: send interrupt request to cpu
#define G_DENA   0x200000   // GPIO[21]   out: we are receiving data from cpu
#define G_JUMP   0x400000   // GPIO[22]    in: doing JMP/JMS instruction
#define G_IOIN   0x800000   // GPIO[23]    in: I/O instruction (in state IOT.1)
#define G_DFRM  0x1000000   // GPIO[24]    in: accompanying READ and/or WRITE address uses data frame
#define G_READ  0x2000000   // GPIO[25]    in: cpu wants to read from us
#define G_WRITE 0x4000000   // GPIO[26]    in: cpu wants to write to us
#define G_IAK   0x8000000   // GPIO[27]    in: interrupt acknowledge

#define G_OUTS  (G_CLOCK|G_RESET|G_IOS|G_IRQ|G_DENA|G_QENA|G_TRIG)  // these are always output pins
#define G_INS   (G_IOIN|G_DFRM|G_JUMP|G_READ|G_WRITE|G_IAK)         // these are always input pins
#define G_DATA0 (G_DATA & -G_DATA)                                  // low-order data pin
#define G_ALL   (G_OUTS | G_INS | G_DATA | G_LINK)                  // all the gpio bits we care about

#define G_REVIS (G_IOIN)                                    // reverse these input pins when reading GPIO connector so app sees only active high signals
            // G_DATA  = _MD = _aluq: active low by alu.mod -> inverted by level converter -> active high to gpio connector -> active high to app
            // G_LINK  = _MDL = _lnq: active low by acl.mod -> inverted by level converter -> active high to gpio connector -> active high to app
            // G_IOIN  = active high by seq.mod -> inverted by level converter -> active low to gpio connector -> inverted by G_REVIS -> active high to app
            // G_DFRM  = active low by seq.mod -> inverted by level converter -> active high to gpio connector -> active high to app
            // G_JUMP  = active low by seq.mod -> inverted by level converter -> active high to gpio connector -> active high to app
            // G_READ  = active low by seq.mod -> inverted by level converter -> active high to gpio connector -> active high to app
            // G_WRITE = active low by seq.mod -> inverted by level converter -> active high to gpio connector -> active high to app
            // G_IAK   = active low by seq.mod -> inverted by level converter -> active high to gpio connector -> active high to app

#define G_REVOS (G_CLOCK|G_RESET|G_DATA|G_LINK|G_IOS|G_IRQ) // reverse these output pins when writing GPIO connector
            // G_CLOCK = active high by app -> inverted by G_REVOS -> active low on gpio connector -> inverted by level converter -> CLOK2 active high signal
            // G_RESET = active high by app -> inverted by G_REVOS -> active low on gpio connector -> inverted by level converter -> RESET active high signal
            // G_DATA  = active high by app -> inverted by G_REVOS -> active low on gpio connector -> inverted by level converter -> MQ active high signal
            // G_LINK  = active high by app -> inverted by G_REVOS -> active low on gpio connector -> inverted by level converter -> MQL active high signal
            // G_IOS   = active high by app -> inverted by G_REVOS -> active low on gpio connector -> inverted by level converter -> IOSKP active high signal
            // G_IRQ   = active high by app -> inverted by G_REVOS -> active low on gpio connector -> inverted by level converter -> INTRQ active high signal

struct GpioLib;
struct Shadow;

#include "abcd.h"
#include "miscdefs.h"
#include "myiowkit.h"

struct GpioFile {
    GpioFile ();
    virtual ~GpioFile ();
    void *open (char const *devname);
    void close ();

private:
    int memfd;
    void *memptr;
};

// base class for GPIO access

#define NPADS 4  // number of paddles
struct GpioLib {
    char const *libname;
    pthread_mutex_t trismutex;
    uint64_t numtrisoff;
    uint64_t ntotaltris;

    GpioLib ();
    virtual ~GpioLib ();
    virtual void open () = 0;
    virtual void close () = 0;
    virtual void halfcycle ();
    virtual void halfcycle (bool aluadd);
    virtual uint32_t readgpio () = 0;
    virtual void writegpio (bool wdata, uint32_t valu) = 0;
    virtual bool haspads () = 0;
    virtual void readpads (uint32_t *pinss) = 0;
    virtual void writepads (uint32_t const *masks, uint32_t const *pinss) = 0;
    virtual int examine (char const *varname, uint32_t *value);
    virtual bool *getvarbool (char const *varname, int rbit);
    virtual uint16_t readhwsr ();
    void doareset ();

    static std::string decogpio (uint32_t bits);
};

struct CSrcMod;

struct CSrcLib : GpioLib {
    CSrcLib (char const *modname);

    virtual ~CSrcLib ();
    virtual void open ();
    virtual void close ();
    virtual void halfcycle ();
    virtual uint32_t readgpio ();
    virtual void writegpio (bool wdata, uint32_t valu);
    virtual bool haspads ();
    virtual void readpads (uint32_t *pinss);
    virtual void writepads (uint32_t const *masks, uint32_t const *pinss);
    virtual int examine (char const *varname, uint32_t *value);
    virtual bool *getvarbool (char const *varname, int rbit);

private:
    CSrcMod *module;
    bool latestwdata;
    uint32_t latestwvalu;
    uint64_t waituntilns;
    uint64_t writecount;
    uint64_t writeloops;
    uint32_t maxloops;
    void *modhand;
};

struct NohwLib : GpioLib {
    NohwLib ();
    virtual void open ();
    virtual void close ();
    virtual void halfcycle ();
    virtual uint32_t readgpio ();
    virtual void writegpio (bool wdata, uint32_t valu);
    virtual bool haspads ();
    virtual void readpads (uint32_t *pinss);
    virtual void writepads (uint32_t const *masks, uint32_t const *pinss);

private:
    uint32_t gpiowritten;

    ABCD calcabcd ();
};

struct PipeLib : GpioLib {
    PipeLib (char const *modname);
    virtual void open ();
    virtual void close ();
    virtual void halfcycle ();
    virtual uint32_t readgpio ();
    virtual void writegpio (bool wdata, uint32_t valu);
    virtual bool haspads ();
    virtual void readpads (uint32_t *pinss);
    virtual void writepads (uint32_t const *masks, uint32_t const *pinss);
    virtual int examine (char const *varname, uint32_t *value);

private:
    char const *modname;
    char const *runcount;
    int netgenpid;
    FILE *recvfile;
    FILE *sendfile;
    uint32_t lastwritegpio;
    uint32_t sendseq;

    uint32_t examine (char const *varname);
};

struct TimedLib : GpioLib {
    virtual void open ();
    virtual void halfcycle (bool aluadd);

protected:
    uint32_t gpiovalu;
private:
    struct timespec hafcycts;
    uint32_t hafcychi, hafcyclo;
};

struct PhysLib : TimedLib {
    PhysLib ();
    virtual ~PhysLib ();
    virtual void open ();
    virtual void close ();
    virtual uint32_t readgpio ();
    virtual void writegpio (bool wdata, uint32_t valu);
    virtual bool haspads ();
    virtual void readpads (uint32_t *pinss);
    virtual void writepads (uint32_t const *masks, uint32_t const *pinss);

    uint32_t read32 (int pad);
    void write32 (int pad, uint32_t mask, uint32_t pins);

    static short const gpioudpport = 1357;
    struct UDPMsg {
        uint64_t seq;
        uint32_t val;
    };

private:
    bool paddlesopen;
    GpioFile gpiofile;
    int gpioudpfd;
    int gpioudptm;
    IOWKIT_HANDLE iowhandles[NPADS];
    sigset_t allowedsigs;
    sigset_t sigintmask;
    struct sockaddr_in gpioudpsa;
    uint32_t gpiomask;
    uint32_t gpioreadflip;
    uint32_t volatile *gpiopage;
    uint32_t lastretries;
    uint64_t writecount;
    uint64_t lastudpseq;
    uint64_t writecycles;

    void opengpio ();
    void opengpioudp (char const *server);
    uint32_t sendrecvudp (uint32_t value);
    static void floateach (void *ctx, char const *dn, int pipe, char const *sn, int pid, int rev);
    void openpaddles ();
    static void write56 (IOWKIT_HANDLE iowh, uint64_t allpins);
    static uint64_t read56 (IOWKIT_HANDLE iowh);
    static void write100 (IOWKIT_HANDLE iowh, uint64_t allpins);
    static uint64_t read100 (IOWKIT_HANDLE iowh);
    void delayiow ();
    void blocksigint ();
    void allowsigint ();
};

struct ZynqLib : TimedLib {
    ZynqLib ();
    ZynqLib (char const *modnames);
    virtual ~ZynqLib ();
    virtual void open ();
    virtual void close ();
    virtual void halfcycle (bool aluadd);
    virtual uint32_t readgpio ();
    virtual void writegpio (bool wdata, uint32_t valu);
    virtual bool haspads ();
    virtual void readpads (uint32_t *pinss);
    virtual void writepads (uint32_t const *masks, uint32_t const *pinss);

private:
    GpioFile gpiofile;
    uint32_t boardena;
    uint32_t gpioreadflip;
    uint32_t volatile *gpiopage;
    uint64_t writecount;

    void opengpio ();
};

char *lockfile (int fd, int how);
#endif
