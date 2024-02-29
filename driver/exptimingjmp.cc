
// runs on raspi plugged into experiment board (exp)
// measures clock frequency of IOS,MDL->JMP flipflop

// ./loadmod.sh
// ./exptimingjmp.armv6l

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
        int badjmp = 0;
        int badmwt = 0;
        if (clock_gettime (CLOCK_REALTIME, &started) < 0) ABORT ();
        for (int i = 0; i < 10000; i ++) {
            uint32_t gray = (i & 3) ^ ((i & 3) >> 1);
            uint32_t sent = ((gray & 1) ? G_LINK : 0) | ((gray & 2) ? G_IOS : 0);
            gpiolib->writegpio (true, sent);
            waithalf ();
            gpiolib->writegpio (true, sent | G_CLOCK);
            waithalf ();
            uint32_t rcvd = gpiolib->readgpio ();
            badmwt += (((sent / G_LINK) | (sent / G_IOS)) & 1) ^ ((rcvd / G_WRITE) & 1);
            badjmp += (((sent / G_LINK) | (sent / G_IOS)) & 1) ^ ((rcvd / G_JUMP) & 1);
        }
        if (clock_gettime (CLOCK_REALTIME, &finished) < 0) ABORT ();
        uint64_t elapsedns = (uint64_t) (finished.tv_sec - started.tv_sec) * 1000000000 + finished.tv_nsec - started.tv_nsec;
        printf (" halfus = %2u  badmwt = %5u/10000  badjmp = %5u/10000  %6llu Hz\n", halfus, badmwt, badjmp, 10000000000000ULL / elapsedns);
        if (halfus == 12) halfus = 11;
    }
    return 0;
}

static void waithalf ()
{
    uint32_t start = rdcyc ();
    while (rdcyc () - start < halfus) { }
}
