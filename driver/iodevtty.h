#ifndef _IODEVH
#define _IODEVH

#include <termios.h>

#include "iodevs.h"

struct IODevTTY : IODev {
    IODevTTY ();

    bool keystruck ();
    void updintreq ();

    virtual void ioreset ();
    virtual uint16_t ioinstr (uint16_t opcode, uint16_t input);

private:
    bool doublcr;
    bool kbflag;
    bool intenab;
    bool prflag;
    bool prfull;
    bool running;
    int chpersec;
    int debug;
    int ttykbfd;
    int ttyprfd;
    struct termios oldttyattrs;
    uint8_t kbbuff;
    uint8_t prbuff;
    uint8_t mask8;

    pthread_t kbtid;
    pthread_t prtid;

    pthread_cond_t prcond;
    pthread_mutex_t lock;

    void startitup ();
    void kbthread ();
    void prthread ();

    void updintreqlk ();

    static void *kbthreadwrap (void *zhis);
    static void *prthreadwrap (void *zhis);
};

extern IODevTTY iodevtty;

#endif
