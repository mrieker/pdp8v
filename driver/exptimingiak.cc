
// runs on raspi plugged into experiment board (exp)
// measures clock frequency of IRQ->IAK flipflop going through uni-dir level converters

// ./loadmod.sh
// ./exptimingiak.armv6l

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "gpiolib.h"
#include "rdcyc.h"

static GpioLib *gpiolib;
static uint32_t halfus;

static void waithalf ();

int main ()
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

    for (halfus = 1; halfus <= 20; halfus ++) {
        int bad = 0;
        if (clock_gettime (CLOCK_REALTIME, &started) < 0) ABORT ();
        for (int i = 0; i < 10000; i ++) {
            uint32_t sent = (i & 1) * G_IRQ;
            gpiolib->writegpio (false, sent);
            waithalf ();
            gpiolib->writegpio (false, sent | G_CLOCK);
            waithalf ();
            uint32_t rcvd = gpiolib->readgpio () & G_IAK;
            bad += sent / G_IRQ ^ rcvd / G_IAK;
        }
        if (clock_gettime (CLOCK_REALTIME, &finished) < 0) ABORT ();
        uint64_t elapsedns = (uint64_t) (finished.tv_sec - started.tv_sec) * 1000000000 + finished.tv_nsec - started.tv_nsec;
        printf (" halfus = %2u  bad = %5u/10000  %6llu Hz\n", halfus, bad, 10000000000000ULL / elapsedns);
    }
    return 0;
}

static void waithalf ()
{
    uint32_t start = rdcyc ();
    while (rdcyc () - start < halfus) { }
}
