
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char **argv)
{
    double x = strtod (argv[1], NULL);
    int exp;
    double y = frexp (x, &exp);
#ifdef __HODE__
    printf ("%lg = %lg * 2 ** %d\n", x, y, exp);
#else
    printf ("%g = %g * 2 ** %d\n", x, y, exp);
#endif
    return 0;
}
