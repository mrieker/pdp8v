#ifndef _IODEVPTAPE_H
#define _IODEVPTAPE_H

#include <termios.h>

#include "iodevs.h"

struct IODevPTape : IODev {
    IODevPTape ();

    virtual void ioreset ();
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input);

private:
    bool intenab;
    bool punflag;
    bool punfull;
    bool punrun;
    bool rdrdebug;
    bool rdrflag;
    bool rdrrun;
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
    void punthread ();
    void rdrstart ();
    void rdrthread ();

    void updintreq ();

    static void *punthreadwrap (void *zhis);
    static void *rdrthreadwrap (void *zhis);
};

extern IODevPTape iodevptape;

#endif
