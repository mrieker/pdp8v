
// cc -std=c99 -o testldexpd testldexpd.c
// ./testldexpd

// rm -f x.*
// ln -s testldexpd.c x.c
// make x.hex
// ./runhw.sh x.hex

#ifdef __HODE__
#include <hode.h>
#define DF "%lg"
#else
#define DF "%g"
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char **argv)
{
    if (argc == 3) {
        double x = strtod (argv[1], NULL);
        int exp = strtol (argv[2], NULL, 0);
#ifdef __HODE__
        printinstr (1);
#endif
        double y = ldexp (x, exp);
#ifdef __HODE__
        printinstr (0);
#endif
        printf (DF" * 2 ** %d = "DF"\n", x, exp, y);
        return 0;
    }

    for (double px = 1e-323; px < 1e308; px *= 10.0) {
        printf (DF"\n", px);
        double qx = px;
        for (int exp = 0; exp < 2050; ++ exp) {
            double py = ldexp (px, exp);
            if (py != qx) {
                printf (DF" * 2 ** %d = "DF" sb "DF"\n", px, exp, py, qx);
            }
            qx *= 2.0;
        }
        qx = px;
        for (int exp = 0; exp > -2050; -- exp) {
            double py = ldexp (px, exp);
            double diff = py / qx;
            if ((diff < 0.999) || (diff > 1.001)) {
                printf (DF" * 2 ** %d = "DF" sb "DF"  diff "DF"\n", px, exp, py, qx, diff);
            }
            qx /= 2.0;
            if (qx < 5e-321) break;
        }
    }
    return 0;
}
