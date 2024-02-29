
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char **argv)
{
    float x = strtof (argv[1], NULL);
    int exp;
    float y = frexpf (x, &exp);
    printf ("%g = %g * 2 ** %d\n", x, y, exp);
    return 0;
}
