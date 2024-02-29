
// get error codes from listing

//  cc -o errors errors.c
//  ./errors < focal12.lis

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main ()
{
    char line[256];
    unsigned pc, msbs, lsbs;

    while (fgets (line, sizeof line, stdin) != NULL) {
        if ((line[0] != ' ') && (memcmp (line + 7, "4566", 4) == 0)) {
            pc = strtoul (line, NULL, 8) & 4095;
            msbs = pc >> 7;
            lsbs = pc & 127;
            printf ("?%02u.%c%c  %s", msbs, '0' + lsbs / 10, '0' + lsbs % 10, line);
        }
    }
    return 0;
}
