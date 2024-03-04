
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "memory.h"
#include "rimloader.h"

#define ever (;;)

// pdp-8 rimloader format
//  input:
//   loadname = name of rimloader file
//   loadfile = file opened here
//  output:
//   returns 0: read successful
//              *start_r = 0x8000 : no start address given
//                           else : 15-bit start address
//        else: failure, error message was output
int rimloader (char const *loadname, FILE *loadfile, uint16_t *start_r)
{
    *start_r = 0x8000;

    for ever {
        int ch = fgetc (loadfile);
        if (ch < 0) break;

        if (ch == 0377) continue;

        if (ch & 0177600) {
            fprintf (stderr, "rimloader: error reading 1st address byte %03o\n", ch);
            return 1;
        }

        // keep skipping until we have an address
        if (! (ch & 0100)) continue;

        uint16_t addr = (ch & 077) << 6;
        ch = fgetc (loadfile);
        if (ch & 0177700) {
            fprintf (stderr, "rimloader: error reading 2nd address byte %03o\n", ch);
            return 1;
        }
        addr |= ch & 077;

        ch = fgetc (loadfile);
        if (ch & 0177700) {
            fprintf (stderr, "rimloader: error reading 1st data byte %03o\n", ch);
            return 1;
        }
        uint16_t data = (ch & 077) << 6;

        ch = fgetc (loadfile);
        if (ch & 0177700) {
            fprintf (stderr, "rimloader: error reading 2nd data byte %03o\n", ch);
            return 1;
        }
        data |= ch & 077;

        memarray[addr] = data;
    }

    return 0;
}
