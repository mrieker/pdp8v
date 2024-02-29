
// runs on raspi plugged into experiment board (exp)
// measures clock frequency of MQ0,MQ1,MQL,IOS->DFRM

// ./loadmod.sh
// ./exptimingdfrm.armv6l

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "gpiolib.h"
#include "rdcyc.h"

#define G_DATA1 (G_DATA0 * 2)

static GpioLib *gpiolib;
static uint32_t halfus;

static void waithalf ();

int main (int argc, char **argv)
{
    struct timespec finished, started;

    setlinebuf (stdout);

    gpiolib = new PhysLib (10, true);
    gpiolib->open ();
    gpiolib->writegpio (false, G_RESET);
    gpiolib->halfcycle ();
    gpiolib->halfcycle ();
    gpiolib->halfcycle ();
    gpiolib->writegpio (false, 0);

    halfus = 10000;
    if (argc > 1) halfus = atoi (argv[1]);
    uint32_t bits = 0;
    while (true) {
        int bad = 0;
        if (clock_gettime (CLOCK_REALTIME, &started) < 0) ABORT ();
        int nloops = 500000 / halfus;
        for (int i = 0; i < nloops; i ++) {
            int n = (i ^ (i >> 1) ^ (i >> 2) ^ (i >> 3)) & 3;
            bits ^= 0xF - (1 << n);         // flip all but one of the four bits
            bool mq0 =  bits       & 1;
            bool mq1 = (bits >> 1) & 1;
            bool mql = (bits >> 2) & 1;
            bool ios = (bits >> 3) & 1;
            uint32_t sent = (mq0 ? G_DATA0 : 0) | (mq1 ? G_DATA1 : 0) | (mql ? G_LINK : 0) | (ios ? G_IOS : 0);
            gpiolib->writegpio (true, sent);
            waithalf ();
            gpiolib->writegpio (true, sent | G_CLOCK);
            waithalf ();
            uint32_t rcvd = gpiolib->readgpio ();
            bool md0 = mq1;
            bool md1 = mq0;
            bool mdl = mql;
            bool dfrm_expect = (mdl & ios & md1) | md0;
            bool dfrm_actual = (rcvd / G_DFRM) & 1;
            if (dfrm_actual != dfrm_expect) bad ++;
        }
        if (clock_gettime (CLOCK_REALTIME, &finished) < 0) ABORT ();
        uint64_t elapsedns = (uint64_t) (finished.tv_sec - started.tv_sec) * 1000000000 + finished.tv_nsec - started.tv_nsec;
        printf (" halfus = %u  bad = %5u/%d  %6llu Hz\n", halfus, bad, nloops, nloops * 1000000000ULL / elapsedns);
    }
    return 0;
}

static void waithalf ()
{
    uint32_t start = rdcyc ();
    while (rdcyc () - start < halfus) { }
}
