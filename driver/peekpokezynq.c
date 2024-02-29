
// very simple program to peek/poke zynq's gpio registers by hand
// cc -o peekpokezynq peekpokezynq.c
// requires the km-zynqgpio module to be loaded

#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#define G_CLOCK       0x4   // GPIO[02]   out: send clock signal to cpu
#define G_RESET       0x8   // GPIO[03]   out: send reset signal to cpu
#define G_DATA     0xFFF0   // GPIO[15:04] bi: data bus bits
#define G_LINK    0x10000   // GPIO[16]    bi: link bus bit
#define G_IOS     0x20000   // GPIO[17]   out: skip next instruction (valid in IOT.2 only)
#define G_QENA    0x80000   // GPIO[19]   out: we are sending data out to cpu
#define G_IRQ    0x100000   // GPIO[20]   out: send interrupt request to cpu
#define G_DENA   0x200000   // GPIO[21]   out: we are receiving data from cpu
#define G_JUMP   0x400000   // GPIO[22]    in: doing JMP/JMS instruction
#define G_IOIN   0x800000   // GPIO[23]    in: I/O instruction (in state IOT.1)
#define G_DFRM  0x1000000   // GPIO[24]    in: accompanying READ and/or WRITE address uses data frame
#define G_READ  0x2000000   // GPIO[25]    in: cpu wants to read from us
#define G_WRITE 0x4000000   // GPIO[26]    in: cpu wants to write to us
#define G_IAK   0x8000000   // GPIO[27]    in: interrupt acknowledge

#define G_OUTS  (G_CLOCK|G_RESET|G_IOS|G_IRQ|G_DENA|G_QENA) // these are always output pins
#define G_INS   (G_IOIN|G_DFRM|G_JUMP|G_READ|G_WRITE|G_IAK) // these are always input pins
#define G_DATA0 (G_DATA & -G_DATA)                          // low-order data pin
#define G_ALL   (G_OUTS | G_INS | G_DATA | G_LINK)          // all the gpio bits we care about

#define G_REVIS (G_IOIN)                                    // reverse these input pins when reading GPIO connector so app sees only active high signals
            // G_DATA  = _MD = _aluq: active low by alu.mod -> inverted by level converter -> active high to gpio connector -> active high to app
            // G_LINK  = _MDL = _lnq: active low by acl.mod -> inverted by level converter -> active high to gpio connector -> active high to app
            // G_IOIN  = active high by seq.mod -> inverted by level converter -> active low to gpio connector -> inverted by G_REVIS -> active high to app
            // G_DFRM  = active low by seq.mod -> inverted by level converter -> active high to gpio connector -> active high to app
            // G_JUMP  = active low by seq.mod -> inverted by level converter -> active high to gpio connector -> active high to app
            // G_READ  = active low by seq.mod -> inverted by level converter -> active high to gpio connector -> active high to app
            // G_WRITE = active low by seq.mod -> inverted by level converter -> active high to gpio connector -> active high to app
            // G_IAK   = active low by seq.mod -> inverted by level converter -> active high to gpio connector -> active high to app

#define G_REVOS (G_CLOCK|G_RESET|G_DATA|G_LINK|G_IOS|G_IRQ) // reverse these output pins when writing GPIO connector
            // G_CLOCK = active high by app -> inverted by G_REVOS -> active low on gpio connector -> inverted by level converter -> CLOK2 active high signal
            // G_RESET = active high by app -> inverted by G_REVOS -> active low on gpio connector -> inverted by level converter -> RESET active high signal
            // G_DATA  = active high by app -> inverted by G_REVOS -> active low on gpio connector -> inverted by level converter -> MQ active high signal
            // G_LINK  = active high by app -> inverted by G_REVOS -> active low on gpio connector -> inverted by level converter -> MQL active high signal
            // G_IOS   = active high by app -> inverted by G_REVOS -> active low on gpio connector -> inverted by level converter -> IOSKP active high signal
            // G_IRQ   = active high by app -> inverted by G_REVOS -> active low on gpio connector -> inverted by level converter -> INTRQ active high signal

static char *interp (uint32_t val, uint32_t index);

int main ()
{
    char buf[80], *p;
    int memfd;
    uint32_t volatile *gpiopage;
    void *memptr;

    // access gpio page in physical memory
    // /proc/zynqgpio is a simple mapping of the single gpio page of zynq.vhd
    // it is created by loading the km-zynqgpio/zynqgpio.ko kernel module
    gpiopage = NULL;
    memfd = open ("/proc/zynqgpio", O_RDWR);
    if (memfd < 0) {
        fprintf (stderr, "error opening /proc/zynqgpio: %m\n");
        return 1;
    }

    memptr = mmap (NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0);
    if (memptr == MAP_FAILED) {
        fprintf (stderr, "error mmapping /proc/zynqgpio: %m\n");
        return 1;
    }

    // save pointer to gpio register page
    gpiopage = (uint32_t volatile *) memptr;

    while (fgets (buf, sizeof buf, stdin)) {
        uint32_t index = strtoul (buf, &p, 16);
        if (index < 1024) {
            if (*p == '\n') {
                uint32_t val = gpiopage[index];
                printf ("gpiopage[%u] => 0x%08X  %s\n", index, val, interp (val, index));
                continue;
            }
            if (*p == '=') {
                uint32_t val = strtoul (p + 1, NULL, 16);
                gpiopage[index] = val;
                printf ("gpiopage[%u] <= 0x%08X  %s\n", index, val, interp (val, index));
                continue;
            }
        }
    }
    return 0;
}

static char *interp (uint32_t val, uint32_t index)
{
    static char buf[256];

    if (index == 0) val = (val ^ G_REVIS) & G_INS;
    if (index == 1) val = (val ^ G_REVOS) & G_OUTS;

    char *p = buf;
    *p = 0;

    if (val & G_CLOCK) p += sprintf (p, " CLOCK");
    if (val & G_RESET) p += sprintf (p, " RESET");
    if (val & G_IOS)   p += sprintf (p, " IOS");
    if (val & G_QENA)  p += sprintf (p, " QENA");
    if (val & G_IRQ)   p += sprintf (p, " IRQ");
    if (val & G_DENA)  p += sprintf (p, " DENA");
    if (val & G_JUMP)  p += sprintf (p, " JUMP");
    if (val & G_IOIN)  p += sprintf (p, " IOIN");
    if (val & G_DFRM)  p += sprintf (p, " DFRM");
    if (val & G_READ)  p += sprintf (p, " READ");
    if (val & G_WRITE) p += sprintf (p, " WRITE");
    if (val & G_IAK)   p += sprintf (p, " IAK");

    return buf;
}
