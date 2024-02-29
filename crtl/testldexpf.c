
// cc -std=c99 -o testldexpf testldexpf.c
// ./testldexpf

// rm -f x.*
// ln -s testldexpf.c x.c
// make x.hex
// ./runhw.sh x.hex

#ifdef __HODE__
#include <hode.h>
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char **argv)
{
    if (argc == 3) {
        float x = strtof (argv[1], NULL);
        int exp = strtol (argv[2], NULL, 0);
#ifdef __HODE__
        printinstr (1);
#endif
        float y = ldexpf (x, exp);
#ifdef __HODE__
        printinstr (0);
#endif
        printf ("%g * 2 ** %d = %g\n", x, exp, y);
        return 0;
    }

    for (float px = 1e-44; px < 3e38; px *= 10.0) {
        printf ("%g\n", px);
        float qx = px;
        for (int exp = 0; exp < 260; ++ exp) {
            float py = ldexpf (px, exp);
            if (py != qx) {
                printf ("%g * 2 ** %d = %g sb %g\n", px, exp, py, qx);
            }
            qx *= 2.0;
        }
        qx = px;
        for (int exp = 0; exp > -260; -- exp) {
            float py = ldexpf (px, exp);
            float diff = py / qx;
            if ((diff < 0.93) || (diff > 1.07)) {
                printf ("%g * 2 ** %d = %g sb %g  diff %g\n", px, exp, py, qx, diff);
            }
            qx /= 2.0;
            if (qx < 2e-44) break;
        }
    }
    return 0;
}
