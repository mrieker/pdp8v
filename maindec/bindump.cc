
// dump a bin loader format paper tape to .asm file
//  g++ -Wall -O2 -o bindump bindump.cc ../driver/disassemble.o.x86_64
//  ./bindump < binfile > asmfile

#include <stdio.h>
#include <stdlib.h>

#include "../driver/disassemble.h"

#define debug(...) do { } while (0)

int main ()
{
    bool rubbingout = false;
    bool rubatbol   = false;
    bool inleadin   = true;
    int      state  = -1;
    uint16_t addr   =  0;
    uint16_t data   =  0;
    uint16_t chksum =  0;
    uint16_t field  =  0;
    uint16_t nextaddr = 0;
    uint16_t start = 0x8000;

    while (true) {
        int ch = fgetc (stdin);
        if (ch < 0) {
            fprintf (stderr, "raspictl: eof reading stdin\n");
            return 1;
        }
        debug ("\n %03o", ch);

        // ignore blocks between pairs of rubouts
        if (ch == 0377) {
            debug ("  rubout");
            rubbingout = ! rubbingout;
            if (rubbingout) rubatbol = true;
            if (! rubbingout && ! rubatbol) putchar ('\n');
            continue;
        }
        if (rubbingout) {
            ch &= 0177;
            if ((ch == '\r') || (ch == '\n')) {
                if (! rubatbol) putchar ('\n');
                rubatbol = true;
            } else {
                if (rubatbol) putchar (';');
                rubatbol = false;
                putchar (ch);
            }
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
            fprintf (stderr, "raspictl: bad char %03o in stdin\n", ch);
            return 1;
        }

        // add to checksum before stripping <6>
        chksum += ch;
        debug ("  chksum %04o", chksum & 07777);

        // state 4 means we have a data word assembled ready to go to memory
        // it also invalidates the last address as being a start address
        // and it means the next word is the first of a data pair
        if (state == 4) {
            uint16_t fulladdr = field | addr;
            debug ("  store memarray[%05o] = %04o", fulladdr, data);
            if (nextaddr != fulladdr) {
                nextaddr = fulladdr;
                printf (". = 0%05o\n", nextaddr);
            }
            printf ("\t.word\t0%04o\t;%05o  %s\n", data, addr, disassemble (data, addr).c_str ());
            nextaddr ++;
            addr  = (addr + 1) & 07777;
            start = 0x8000;
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
                start = field | addr;
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
            default: abort ();
        }
    }
    debug ("  trailer\n");
    debug ("raw chksum %04o, data %04o, start %05o\n", chksum, data, start);

    // trailing byte found, validate checksum
    chksum -= data & 077;
    chksum -= data >> 6;
    chksum &= 07777;
    debug ("cooked chksum %04o\n", chksum);
    if (chksum != data) {
        fprintf (stderr, "raspictl: checksum calculated %04o, given on tape %04o\n", chksum, data);
        return 1;
    }

    if (start == 0x8000) printf ("\t.end\n");
      else printf ("\t.end\t0%05o\n", start);
    return 0;
}
