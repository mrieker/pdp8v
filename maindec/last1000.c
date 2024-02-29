
// pass through last 1000 lines of a file

#include <stdio.h>
#include <stdlib.h>

#define NLINES 1000
#define WIDTH 256

int main ()
{
    char lines[NLINES][WIDTH];
    int i, n;

    for (n = 0; fgets (lines[n%NLINES], WIDTH, stdin) != NULL; n ++) if (n == NLINES * 2) n = NLINES;

    for (i = (n < NLINES) ? 0 : n - NLINES; i < n; i ++) fputs (lines[i%NLINES], stdout);

    return 0;
}
