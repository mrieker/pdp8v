
// test program to dump zynq gpio registers once a second
// used mostly to see that the test dma circuit is working

// requires the km-zynqgpio/zynkqpio.ko kernel module is loaded
// ...which simply gives non-root access to the zynq gpio register page

/*
  # fill test0..test9 with some values
  wr 200 11 22 33 44 55 66 77 88 99 aa

  # write those values to test memory page
  # twice in a row, using 2 10-word bursts
  wr 101 3

  # confirm the write is complete
  # - should show reg[101] = 3ED91050
  # - 3ED91 is example phys page number
  rr 101

  # confirm memory was written
  # - should show the numbers twice 11..AA 11..AA then zeroes
  rm 0 1f

  # fill memory with opposite numbers
  wm 0 aa 99 88 77 66 55 44 33 22 11

  # read memory into temp0..9
  wr 100 1

  # confirm the read is complete
  # - should show reg[100] = 3ED91028
  rr 100

  # confirm temp0..9 contents
  # - should show AA..11 for contents
  rr 200 209
*/

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "readprompt.h"

static bool readit (uint32_t volatile *it, char const *cmd, char const *des, char **args, int nargs);
static bool writeit (uint32_t volatile *it, char const *cmd, char const *des, char **args, int nargs, uint32_t dmapa);

int main ()
{
    // access gpio page in physical memory
    // /proc/zynqgpio is a simple mapping of the single gpio page of zynq.vhd
    // it is created by loading the km-zynqgpio/zynqgpio.ko kernel module
    int memfd = ::open ("/proc/zynqgpio", O_RDWR);
    if (memfd < 0) {
        fprintf (stderr, "ZynqLib::open: error opening /proc/zynqgpio: %m\n");
        abort ();
    }

    void *memptr = mmap (NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0);
    if (memptr == MAP_FAILED) {
        fprintf (stderr, "ZynqLib::open: error mmapping /proc/zynqgpio: %m\n");
        abort ();
    }

    // save pointer to gpio register page
    uint32_t volatile *gpiopage = (uint32_t volatile *) memptr;

    // allocate a locked memory page
    void *dmaptr = mmap (NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 4096);
    if (dmaptr == MAP_FAILED) {
        fprintf (stderr, "zynctest: error mmapping dmapage: %m\n");
        abort ();
    }

    // get physical address
    unsigned long dmava = (unsigned long) dmaptr;
    unsigned long dmapa = -1;
    if (ioctl (memfd, 13423, &dmapa) < 0) {
        fprintf (stderr, "zynctest: error getting phys addr: %m\n");
        abort ();
    }
    fprintf (stderr, "zynctest: va %lX => pa %lX\n", dmava, dmapa);
    uint32_t volatile *dmapage = (uint32_t volatile *) dmaptr;

    while (true) {
        char const *buff = readprompt (" > ");
        if (buff == NULL) break;
        char *line = strdup (buff);

        char *args[80];
        int nargs = 0;
        for (int i = 0;; i ++) {
            char c = line[i];
            if (c == 0) break;
            if (c <= ' ') {
                line[i] = 0;
            } else if ((i == 0) || (line[i-1] == 0)) {
                args[nargs++] = &line[i];
            }
        }
        if (nargs == 0) continue;
        args[nargs] = NULL;

        // ReadMemory <start> [<endat>]
        if (readit (dmapage, "rm", "mem", args, nargs)) continue;

        // ReadRegister <start> [<endat>]
        if (readit (gpiopage, "rr", "reg", args, nargs)) continue;

        // WriteMemory <index> <value> ...
        if (writeit (dmapage, "wm", "mem", args, nargs, 0)) continue;

        // WriteRegister <index> <value>
        if (writeit (gpiopage, "wr", "reg", args, nargs, dmapa)) continue;

        printf ("bad command\n");
    }

    printf ("\n");

/***
    gpiopage[1] = 0x123456;
    printf ("gpio[0] = %08X %08X %08X %08X\n", gpiopage[0], gpiopage[1], gpiopage[2], gpiopage[3]);
    gpiopage[1] = 0x654321;
    printf ("gpio[0] = %08X %08X %08X %08X\n", gpiopage[0], gpiopage[1], gpiopage[2], gpiopage[3]);

    // fill first 10 words of memory
    uint32_t volatile *dmapage = (uint32_t volatile *) dmaptr;
    for (int i = 0; i < 10; i ++) {
        dmapage[i] = 1111 * (i + 1);
    }

    // read memory into temp0..9 registers
    gpiopage[0x100] = dmapa + 1;
    for (int i = 0; i < 10; i ++) {
        uint32_t dmardaddr = gpiopage[0x100];
        printf ("gpio 0x100 = %08X\n", dmardaddr);
        if ((dmardaddr & 1) == 0) break;
    }
    for (int i = -2; i < 4; i ++) {
        printf ("gpio 0x%X = %08X\n", 0x100 + i, gpiopage[0x100+i]);
    }
    for (int i = -2; i < 12; i ++) {
        printf ("gpio 0x%X = %08X\n", 0x200 + i, gpiopage[0x200+i]);
    }
***/

    return 0;
}

static bool readit (uint32_t volatile *it, char const *cmd, char const *des, char **args, int nargs)
{
    if (((nargs == 2) || (nargs == 3)) && (strcasecmp (args[0], cmd) == 0)) {
        char *p;
        uint32_t start = strtoul (args[1], &p, 16);
        if ((*p != 0) || (start >= 0x400)) {
            printf ("bad index %s\n", args[1]);
            return true;
        }
        uint32_t endat = start;
        if (nargs == 3) {
            endat = strtoul (args[2], &p, 16);
            if ((*p != 0) || (endat < start) || (endat >= 0x400)) {
                printf ("bad index %s\n", args[2]);
                return true;
            }
        }
        for (uint32_t index = start; index <= endat; index ++) {
            printf ("  %s[%03X] = %08X\n", des, index, it[index]);
        }
        return true;
    }
    return false;
}

static bool writeit (uint32_t volatile *it, char const *cmd, char const *des, char **args, int nargs, uint32_t dmapa)
{
    if ((nargs >= 3) && (strcasecmp (args[0], cmd) == 0)) {
        char *p;
        uint32_t index = strtoul (args[1], &p, 16);
        if ((*p != 0) || (index >= (uint32_t)(0x400 - (nargs - 3)))) {
            printf ("bad index %s\n", args[1]);
            return true;
        }
        for (int i = 2; i < nargs; i ++) {
            uint32_t value = strtoul (args[i], &p, 16);
            if (*p != 0) {
                printf ("bad value %s\n", args[i]);
                return true;
            }
            if ((dmapa != 0) && (index >= 0x100) && (index <= 0x1FF)) {
                value = (value & 3) | dmapa;
            }
            it[index] = value;
            printf ("  %s[%03X] = %08X\n", des, index, it[index]);
            index ++;
        }
        return true;
    }
    return false;
}
