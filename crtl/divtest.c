
// sign of quotient  = xor of signs of both operands
// sign of remainder = sign of dividend
// abs (quotient)    = abs (dividend) / abs (divisor)
// abs (remainder)   = abs (dividend) % abs (divisor)
// dividend = quotient * divisor + remainder

#include <stdio.h>
#include <stdlib.h>

int main (int argc, char **argv)
{
    int a = strtol (argv[1], NULL, 0);
    int b = strtol (argv[2], NULL, 0);
    int q = a / b;
    int r = a % b;
    printf ("%5d / %5d = %5d r %5d => %5d\n", a, b, q, r, q * b + r);
    return 0;
}
