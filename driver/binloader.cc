
#include <stdio.h>
#include <stdlib.h>

#include "memory.h"

#define debug(...) do { } while (0) // printf

#define ever (;;)

typedef unsigned short uint16_t;

// pdp-8 binloader format
//  input:
//   loadname = name of binloader file
//   loadfile = file opened here
//  output:
//   returns 0: read successful
//              *start_r = 0x8000 : no start address given
//                           else : 15-bit start address
//        else: failure, error message was output
int binloader (char const *loadname, FILE *loadfile, uint16_t *start_r)
{
    bool rubbingout = false;
    bool inleadin   = true;
    int      state  = -1;
    uint16_t addr   =  0;
    uint16_t data   =  0;
    uint16_t chksum =  0;
    uint16_t field  =  0;
    *start_r = 0x8000;
    for ever {
        int ch = fgetc (loadfile);
        if (ch < 0) {
            fprintf (stderr, "raspictl: eof reading loadfile %s\n", loadname);
            return 1;
        }
        debug ("\n %03o", ch);

        // ignore blocks between pairs of rubouts
        if (ch == 0377) {
            debug ("  rubout");
            rubbingout = ! rubbingout;
            continue;
        }
        if (rubbingout) {
            debug ("  rubbedout");
            continue;
        }

        // 03x0 sets field to 'x'
        // not counted in checksum
        if ((ch & 0300) == 0300) {
            field = (ch & 0070) << 9;
            debug ("  field set to %o", field >> 12);
            continue;
        }

        // leader/trailer is just <7>
        if (ch == 0200) {
            if (! inleadin) break;
            debug ("  leader");
            continue;
        }
        inleadin = false;

        // no other frame should have <7> set
        if (ch & 0200) {
            fprintf (stderr, "raspictl: bad char %03o in loadfile %s\n", ch, loadname);
            return 1;
        }

        // add to checksum before stripping <6>
        chksum += ch;
        debug ("  chksum %04o", chksum & 07777);

        // state 4 means we have a data word assembled ready to go to memory
        // it also invalidates the last address as being a start address
        // and it means the next word is the first of a data pair
        if (state == 4) {
            debug ("  store memarray[%05o] = %04o", field | addr, data);
            ASSERT ((field | addr) < MEMSIZE);
            ASSERT (data < 4096);
            memarray[field|addr] = data;
            addr  = (addr + 1) & 07777;
            *start_r = 0x8000;
            state = 2;
        }

        // <6> set means this is first part of an address
        if (ch & 0100) {
            debug ("  address mark");
            state = 0;
            ch -= 0100;
        }

        // process the 6 bits
        switch (state) {
            // top 6 bits of address are followed by bottom 6 bits
            case 0: {
                addr  = ch << 6;
                debug ("  high addr");
                state = 1;
                break;
            }
            // bottom 6 bits of address are followed by top 6 bits data
            // it is also the start address if it is last address on tape and is not followed by any data other than checksum
            case 1: {
                addr |= ch;
                debug ("  low addr %04o", addr);
                *start_r = field | addr;
                state = 2;
                break;
            }
            // top 6 bits of data are followed by bottom 6 bits
            case 2: {
                data  = ch << 6;
                debug ("  high data");
                state = 3;
                break;
            }
            // bottom 6 bits of data are followed by top 6 bits of next word
            // the data is stored in memory when next frame received,
            // as this is the checksum if it is the very last data word
            case 3: {
                data |= ch;
                debug ("  low data %04o", data);
                state =  4;
                break;
            }
            default: ABORT ();
        }
    }
    debug ("  trailer\n");
    debug ("raw chksum %04o, data %04o, start %05o\n", chksum, data, *start_r);

    // trailing byte found, validate checksum
    chksum -= data & 077;
    chksum -= data >> 6;
    chksum &= 07777;
    debug ("cooked chksum %04o\n", chksum);
    if (chksum != data) {
        fprintf (stderr, "raspictl: checksum calculated %04o, given on tape %04o\n", chksum, data);
        return 1;
    }

    return 0;
}
