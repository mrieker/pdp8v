
// runs on raspi plugged into experiment board (exp)
// tests timing of all circuits

// ./loadmod.sh
// ./exptimingjmp.armv6l

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "gpiolib.h"
#include "rdcyc.h"

#define G_DATA1 (G_DATA0 * 2)

#define M_CYC 100       // give 100uS for signals to settle
#define N_ITER 10000    // try 10,000 random numbers

static void printsig (char const *name, uint32_t const *counts);

int main ()
{
    setlinebuf (stdout);
    srand (1145);

    GpioLib *gpiolib = new PhysLib (10, true);
    gpiolib->open ();
    gpiolib->writegpio (false, G_RESET);
    gpiolib->halfcycle ();
    gpiolib->halfcycle ();
    gpiolib->halfcycle ();
    gpiolib->writegpio (false, 0);

    while (true) {
        uint32_t max_ioin [M_CYC+1];
        uint32_t max_mwt  [M_CYC+1];
        uint32_t max_iak  [M_CYC+1];
        uint32_t max_dfrm [M_CYC+1];
        uint32_t max_mrd  [M_CYC+1];
        uint32_t max_jump [M_CYC+1];
        uint32_t max_md0  [M_CYC+1];
        uint32_t max_md1  [M_CYC+1];
        uint32_t max_mdl  [M_CYC+1];

        memset (max_ioin, 0, sizeof max_ioin);
        memset (max_mwt,  0, sizeof max_mwt);
        memset (max_iak,  0, sizeof max_iak);
        memset (max_dfrm, 0, sizeof max_dfrm);
        memset (max_mrd,  0, sizeof max_mrd);
        memset (max_jump, 0, sizeof max_jump);
        memset (max_md0,  0, sizeof max_md0);
        memset (max_md1,  0, sizeof max_md1);
        memset (max_mdl,  0, sizeof max_mdl);

        for (int i = 0; i < N_ITER; i ++) {
            uint32_t delta;

            // send random outputs
            uint32_t r = rand ();

            bool mq0 = r & 1;
            bool mq1 = (r >> 1) & 1;
            bool irq = (r >> 2) & 1;
            bool mql = (r >> 3) & 1;
            bool ios = (r >> 4) & 1;

            //printf ("%6d/%6d mq0=%d mq1=%d irq=%d mql=%d ios=%d\n", i, N_ITER, mq0, mq1, irq, mql, ios);

            uint32_t gpout = 0;
            if (mq0) gpout |= G_DATA0;
            if (mq1) gpout |= G_DATA1;
            if (irq) gpout |= G_IRQ;
            if (mql) gpout |= G_LINK;
            if (ios) gpout |= G_IOS;

            gpiolib->writegpio (true, gpout);
            uint32_t started = rdcyc ();

            // uni-directional combinational outputs should be ready first
            uint32_t fin_ioin = 0;                              // time last seen in unsettled state
            uint32_t fin_mwt  = 0;
            do {
                uint32_t gpin = gpiolib->readgpio ();
                delta = rdcyc () - started;
                bool ioin = (gpin / G_IOIN)  & 1;
                bool mwt  = (gpin / G_WRITE) & 1;

                if (ioin != (mql ^ ios)) fin_ioin = delta;
                if (mwt  != (mql | ios)) fin_mwt  = delta;
            } while (delta < M_CYC);

            if (fin_ioin > M_CYC) fin_ioin = M_CYC;
            if (fin_mwt  > M_CYC) fin_mwt  = M_CYC;
            max_ioin[fin_ioin] ++;
            max_mwt[fin_mwt]   ++;

            // clock the random values into the flip-flops
            gpiolib->writegpio (true, gpout | G_CLOCK);
            started = rdcyc ();

            // uni-directional flipflop outputs should be ready now
            uint32_t fin_iak  = 0;
            uint32_t fin_dfrm = 0;
            uint32_t fin_mrd  = 0;
            uint32_t fin_jump = 0;
            do {
                uint32_t gpin = gpiolib->readgpio ();
                delta = rdcyc () - started;

                bool iak  = (gpin / G_IAK)   & 1;
                bool dfrm = (gpin / G_DFRM)  & 1;
                bool mrd  = (gpin / G_READ)  & 1;
                bool jump = (gpin / G_JUMP)  & 1;

                if (iak  != irq)                       fin_iak  = delta;
                if (dfrm != ((mql & ios & mq0) | mq1)) fin_dfrm = delta;
                if (mrd  != (mq0 & mq1))               fin_mrd  = delta;
                if (jump != (mql | ios))               fin_jump = delta;
            } while (delta < M_CYC);

            if (fin_iak  > M_CYC) fin_iak  = M_CYC;
            if (fin_dfrm > M_CYC) fin_dfrm = M_CYC;
            if (fin_jump > M_CYC) fin_jump = M_CYC;
            if (fin_mrd  > M_CYC) fin_mrd  = M_CYC;
            max_iak[fin_iak]   ++;
            max_dfrm[fin_dfrm] ++;
            max_jump[fin_jump] ++;
            max_mrd[fin_mrd]   ++;

            // flip bi-directional bus to be readable
            gpiolib->writegpio (false, gpout);
            started = rdcyc ();

            // bi-directional should be ready now
            uint32_t fin_md0 = 0;
            uint32_t fin_md1 = 0;
            uint32_t fin_mdl = 0;
            do {
                uint32_t gpin = gpiolib->readgpio ();
                delta = rdcyc () - started;

                bool md0 = (gpin / G_DATA0) & 1;
                bool md1 = (gpin / G_DATA1) & 1;
                bool mdl = (gpin / G_LINK)  & 1;

                if (md0 != mq1)         fin_md0 = delta;
                if (md1 != mq0)         fin_md1 = delta;
                if (mdl != (mq0 ^ mq1)) fin_mdl = delta;
            } while (delta < M_CYC);

            if (fin_md0 > M_CYC) fin_md0 = M_CYC;
            if (fin_md1 > M_CYC) fin_md1 = M_CYC;
            if (fin_mdl > M_CYC) fin_mdl = M_CYC;
            max_md0[fin_md0] ++;
            max_md1[fin_md1] ++;
            max_mdl[fin_mdl] ++;
        }

        // print max settle time for each signal
        printf ("\n");
        printsig ("ioin", max_ioin);
        printsig ("mwt ", max_mwt);
        printf ("\n");
        printsig ("iak ", max_iak);
        printsig ("dfrm", max_dfrm);
        printsig ("mrd ", max_mrd);
        printsig ("jump", max_jump);
        printf ("\n");
        printsig ("md0 ", max_md0);
        printsig ("md1 ", max_md1);
        printsig ("mdl ", max_mdl);
        printf ("\n");
    }
    return 0;
}

static void printsig (char const *name, uint32_t const *counts)
{
    printf ("%s", name);
    for (int i = 0; i <= M_CYC; i ++) {
        if (counts[i] != 0) printf (" %d=%u", i, counts[i]);
    }
    printf ("\n");
}
