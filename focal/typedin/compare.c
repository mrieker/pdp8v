
// compare objcode.txt typed-in file
//   to focal12.oct assembled file

// cc -Wall -g -o compare.x86_64 compare.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MEMSIZE 32768

typedef unsigned short uint16_t;

int main ()
{
    char five[5], linbuf[256], *p, *q, objocc[MEMSIZE], txtocc[MEMSIZE];
    FILE *objfile, *txtfile;
    uint16_t addr, data, objmem[MEMSIZE], txtmem[MEMSIZE];

    memset (objocc, 0, sizeof objocc);
    memset (txtocc, 0, sizeof txtocc);
    memset (objmem, 0, sizeof objmem);
    memset (txtmem, 0, sizeof txtmem);

    txtfile = fopen ("objcode.txt", "r");
    if (txtfile == NULL) {
        fprintf (stderr, "error opening objcode.txt: %m\n");
        return 1;
    }
    while (fgets (linbuf, sizeof linbuf, txtfile) != NULL) {
        p = strchr (linbuf, '/');
        if (p != NULL) *p = 0;
        addr = 0xFFFF;
        p = linbuf;
        while (1) {
            while (*p <= ' ') {
                if (*p == 0) goto nexttxtline;
                p ++;
            }
            if ((*p < '0') || (*p > '7')) {
                fprintf (stderr, "txt: bad number %s\n", p);
                return 1;
            }
            data = strtoul (p, &p, 8);
            if (addr == 0xFFFF) {
                addr = data;
                continue;
            }
            if (data >= 4096) {
                fprintf (stderr, "txt: bad data %04o\n", data);
                return 1;
            }
            if (addr >= MEMSIZE) {
                fprintf (stderr, "txt: bad address %05o\n", addr);
                return 1;
            }
            if (txtocc[addr]) {
                fprintf (stderr, "txt: duplicate address %04o\n", addr);
                return 1;
            }
            txtocc[addr] = 1;
            txtmem[addr] = data;
            addr ++;
        }
    nexttxtline:;
    }
    fclose (txtfile);

    objfile = fopen ("focal12.oct", "r");
    if (objfile == NULL) {
        fprintf (stderr, "error opening focal12.oct: %m\n");
        return 1;
    }
    while (fgets (linbuf, sizeof linbuf, objfile) != NULL) {
        addr = strtoul (linbuf, &p, 8);
        if (*p != ':') {
            fprintf (stderr, "obj: bad address in %s\n", linbuf);
            return 1;
        }
        p ++;
        while (*p > ' ') {
            if (*p == '(') break;   // skip relocs (undefined symbol probably)
                                    // will show up as missing in comparison
            memcpy (five, p, 4);
            five[4] = 0;
            data = strtoul (five, &q, 8);
            if (*q != 0) {
                fprintf (stderr, "obj: bad number %s\n", p);
                return 1;
            }
            p += 4;
            if (data >= 4096) {
                fprintf (stderr, "obj: bad data %04o\n", data);
                return 1;
            }
            if (addr >= MEMSIZE) {
                fprintf (stderr, "obj: bad address %05o\n", addr);
                return 1;
            }
            if (objocc[addr]) {
                fprintf (stderr, "obj: duplicate address %04o\n", addr);
                return 1;
            }
            objocc[addr] = 1;
            objmem[addr] = data;
            addr ++;
        }
    }
    fclose (objfile);

    for (addr = 0; addr < MEMSIZE; addr ++) {
        if ((txtocc[addr] != objocc[addr]) || (txtmem[addr] != objmem[addr])) {
            printf ("%05o:  txt=%o.%04o  obj=%o.%04o\n", addr, txtocc[addr], txtmem[addr], objocc[addr], objmem[addr]);
        }
    }
    return 0;
}
