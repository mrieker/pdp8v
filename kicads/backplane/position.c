
// J1 pin 1 :  1.1
// J2 pin 1 :  4.5
// J3 pin 1 :  5.3
// J4 pin 1 :  8.7
// J5 pin 1 :  9.6
// J6 pin 1 : 13.0
// J7 pin 1 : 13.8

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static double yins[] = { 1.1, 4.5, 5.3, 8.7, 9.6, 13.0, 13.8 };

int main ()
{
    char line[4096];
    double xin, yin;
    int jnum;

    jnum = 0;
    while (fgets (line, sizeof line, stdin) != NULL) {
        fputs (line, stdout);
        if (memcmp (line, "  (module Pin_Headers", 21) == 0) {
            jnum ++;
            fgets (line, sizeof line, stdin);
            if (memcmp (line, "    (at ", 8) != 0) abort ();
            xin = (jnum - 1) / 7 * 1.75 + 1.0;
            yin = yins[((jnum-1)%7)];
            printf ("    (at %.1f %.1f)\n", 25.4 * xin, 25.4 * yin);
            continue;
        }
    }
    return 0;
}
