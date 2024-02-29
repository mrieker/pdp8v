
// runs on raspi plugged into experiment board (exp)
// measures clock frequency of MQ0->MD1 flipflop going through bi-dir level converters

// ./loadmod.sh
// ./exptimingmd1.armv6l

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "gpiolib.h"
#include "rdcyc.h"

#define G_DATA1 (G_DATA0 * 2)

static GpioLib *gpiolib;
static uint32_t thirdus;

static void waithird ();

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

    thirdus = 10000;
    if (argc > 1) thirdus = atoi (argv[1]);
    while (true) {
        int bad = 0;
        if (clock_gettime (CLOCK_REALTIME, &started) < 0) ABORT ();
        int nloops = 333333 / thirdus;
        for (int i = 0; i < nloops; i ++) {
            uint32_t sent = (i & 1) * G_DATA0;
            gpiolib->writegpio (true, sent);
            waithird ();
            gpiolib->writegpio (true, sent | G_CLOCK);
            waithird ();
            gpiolib->writegpio (false, 0);
            waithird ();
            uint32_t rcvd = gpiolib->readgpio () & G_DATA1;
            bad += (sent / G_DATA0) ^ (rcvd / G_DATA1);
        }
        if (clock_gettime (CLOCK_REALTIME, &finished) < 0) ABORT ();
        uint64_t elapsedns = (uint64_t) (finished.tv_sec - started.tv_sec) * 1000000000 + finished.tv_nsec - started.tv_nsec;
        printf (" thirdus = %u  bad = %5u/%d  %6llu Hz\n", thirdus, bad, nloops, nloops * 1000000000ULL / elapsedns);
    }
    return 0;
}

static void waithird ()
{
    uint32_t start = rdcyc ();
    while (rdcyc () - start < thirdus) { }
}
