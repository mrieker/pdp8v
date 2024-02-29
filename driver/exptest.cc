
// runs on raspi plugged into experiment board (exp)

// ./loadmod.sh
// ./exptest.armv6l

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "gpiolib.h"

#define G_DATA1 (G_DATA0 * 2)

static GpioLib *gpiolib;

int main ()
{
    setlinebuf (stdout);

    gpiolib = new PhysLib (1, true);
    gpiolib->open ();
    gpiolib->writegpio (false, G_RESET);
    gpiolib->halfcycle ();
    gpiolib->halfcycle ();
    gpiolib->halfcycle ();
    gpiolib->writegpio (false, 0);

    bool wdata = false;
    uint32_t outpins = 0;

    while (true) {
        char inbuff[80];
        if (isatty (0) > 0) write (0, "> ", 2);
        if (fgets (inbuff, sizeof inbuff, stdin) == NULL) break;

        char c, *invec[80], *p;
        bool skipping = true;
        int inlen = 0;
        for (p = inbuff; (c = *p) != 0; p ++) {
            if (c == '\n') break;
            if (c > ' ') {
                if (skipping) {
                    invec[inlen++] = p;
                    skipping = false;
                }
            } else if (! skipping) {
                *p = 0;
                skipping = true;
            }
        }
        *p = 0;
        invec[inlen] = NULL;

        for (int i = 0; i < inlen; i ++) {
            p = invec[i];
            uint32_t bitmask = 0;
            if (strcasecmp (p, "RD") == 0) {
                wdata = false;
                continue;
            }
            if (strcasecmp (p, "WD") == 0) {
                wdata = true;
                continue;
            }
            if (strcasecmp (p, "CLK") == 0) bitmask = G_CLOCK;
            if (strcasecmp (p, "IRQ") == 0) bitmask = G_IRQ;
            if (strcasecmp (p, "IOS") == 0) bitmask = G_IOS;
            if (strcasecmp (p, "MQ0") == 0) bitmask = G_DATA0;
            if (strcasecmp (p, "MQ1") == 0) bitmask = G_DATA1;
            if (strcasecmp (p, "MQL") == 0) bitmask = G_LINK;
            if (strcasecmp (p, "RES") == 0) bitmask = G_RESET;
            if (bitmask == 0) {
                fprintf (stderr, "unknown bitname %s\n", p);
                break;
            }
            p = invec[++i];
            if (p == NULL) {
                fprintf (stderr, "missing value for %s\n", invec[i-1]);
                break;
            }
            if (strcmp (p, "0") == 0) {
                outpins &= ~ bitmask;
                continue;
            }
            if (strcmp (p, "1") == 0) {
                outpins |=   bitmask;
                continue;
            }
            fprintf (stderr, "bad value %s for %s\n", p, invec[i-1]);
            break;
        }

        printf ("          CLK DEN IOS IRQ MQ0 MQ1 MQL QEN RES > DFR IAK IOI JMP MD0 MD1 MDL MRD MWT\n");
        printf (" sending   %o   %o   %o   %o   %c   %c   %c   %o   %o\n",
            (outpins / G_CLOCK) & 1,
            ! wdata,    // dena
            (outpins / G_IOS) & 1,
            (outpins / G_IRQ) & 1,
            wdata ? (((outpins / G_DATA0) & 1) + '0') : ' ',
            wdata ? (((outpins / G_DATA1) & 1) + '0') : ' ',
            wdata ? (((outpins / G_LINK)  & 1) + '0') : ' ',
              wdata,    // qena
            (outpins / G_RESET) & 1);
        gpiolib->writegpio (wdata, outpins);
        gpiolib->halfcycle ();

        uint32_t gpi = gpiolib->readgpio ();
        printf ("received                                         %o   %o   %o   %o   %c   %c   %c   %o   %o\n",
            (gpi / G_DFRM) & 1,
            (gpi / G_IAK) & 1,
            (gpi / G_IOIN) & 1,
            (gpi / G_JUMP) & 1,
            wdata ? ' ' : (((gpi / G_DATA0) & 1) + '0'),
            wdata ? ' ' : (((gpi / G_DATA1) & 1) + '0'),
            wdata ? ' ' : (((gpi / G_LINK)  & 1) + '0'),
            (gpi / G_READ) & 1,
            (gpi / G_WRITE) & 1);
    }
    return 0;
}
