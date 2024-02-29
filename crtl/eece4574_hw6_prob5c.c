
#include <complex.h>
#include <math.h>
#include <stdio.h>

/*
    rs = xl + xc || rl
       = jwl + rl/jwc / (1/jwc + rl)
       = jwl + rl / (1 + jwc * rl)
       = jwl + rl * (1 - jwc * rl) / ((1 + jwc * rl) * (1 - jwc * rl))
       = jwl + rl * (1 - jwc * rl) / (1 - jwc * jwc * rl * rl)
       = jwl + rl * (1 - jwc * rl) / (1 + wc * wc * rl * rl)
       = jwl + (rl - jwc * rl * rl) / (1 + wc * wc * rl * rl)
       = jwl + rl / (1 + wc * wc * rl * rl) - (jwc * rl * rl) / (1 + wc * wc * rl * rl)
       = rl / (1 + wc * wc * rl * rl) + jwl - jwc * rl * rl / (1 + wc * wc * rl * rl)

    rs = rl / (1 + wc * wc * rl * rl)       jwl = jwc * rl * rl / (1 + wc * wc * rl * rl)
    rs = rl / (1 + wc * wc * rl * rl)       l = c * rl * rl / (1 + wc * wc * rl * rl)
    rs * (1 + wc * wc * rl * rl) = rl
    (1 + wc * wc * rl * rl) = rl / rs
    wc * wc * rl * rl = rl / rs - 1
    wc * rl = sqrt (rl / rs - 1)
    c = sqrt (rl / rs - 1) / (rl * w)

*/

int main (int argc, char **argv)
{
    float womega = 2 * M_PIF * 10e6;
    float rs = 20;
    float rl = 200;

    float c = sqrt (rl / rs - 1) / (rl * womega);

    float d = 1 + rl * rl * womega * womega * c * c;
    float l = rl * rl * c / d;

    printf (" c = %g  l = %g\n", c, l);

    ComplexF xl = ComplexF::make (0, womega * l);
    ComplexF xc = ComplexF::make (0, -1 / (womega * c));
    ComplexF z = xl.add (xc.par (ComplexF::make (rl, 0)));
    printf (" z = %g + j %g\n", z.real, z.imag);

    return 0;
}
