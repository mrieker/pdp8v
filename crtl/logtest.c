
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main (int argc, char **argv)
{
    double x = strtod (argv[1], NULL);
    double y = log (x);
    double z = exp (y);
    printf ("x=%lg y=%lg z=%lg\n", x, y, z);
    return 0;
}
