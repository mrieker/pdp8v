
// standard i/o port assignments
// from pdp-12 p 153 v 5-43
//  00      interrupt
//  01..02  high speed reader/punchtype PR12
//  03..04  teletype keyboard/printer
//  05..07  displays types VC8/I and KV8/I, light pen
//  10      power fail option KP12
//  11..12  teletype system type PT08
//  13      real time clock type KW12
//  14      linc mode change (IOT 6141)
//  15      tape maintenance
//  16..17  unused
//  20..27  memory extension control
//  30..37  user interfaces << 30 = system calls
//  40..47  teletype system type DP12/PT08
//  50..52  incremental plotter type XY12
//  53..54  general purpose A/D converters and multiplexers
//  55..57  D/A converter
//  60..62  random access disk file / synchronous modem
//  63      card reader
//  64..66  synchronous modem
//  67      card reader / synchronous modem
//  70..74  automatic mag tape
//  74      rk8-je
//  75      unused << 73,74,75 = RK8 (pdp-8 p 365 v 7-143)
//  76..77  dectape

#include <stdio.h>

#include "extarith.h"
#include "iodevs.h"
#include "iodevptape.h"
#include "iodevrk8.h"
#include "iodevrk8je.h"
#include "iodevrtc.h"
#include "iodevtty.h"
#include "linc.h"
#include "memext.h"
#include "memory.h"
#include "miscdefs.h"
#include "scncall.h"

pthread_cond_t  intreqcond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t intreqlock = PTHREAD_MUTEX_INITIALIZER;

static bool initted;
static IODev *iodevs[64];

static void initiodevs ();

// execute IO instruction
// in middle of EXEC2 state of IOT instruction
//  input:
//   opcode = all 12 bits of opcode (top 3 bits must be 6)
//   input<12> = link bit
//     <11:00> = accumulator
//  output:
//   returns UNSUPIO: nothing processes the given opcode
//              else: <13> = 0: resume with next instruction
//                           1: skip next instruction
//                    <12> = updated link bit
//                 <11:00> = updated accumulator
uint16_t ioinstr (uint16_t opcode, uint16_t input)
{
    if (memext.candoio ()) {
        uint16_t idx = (opcode >> 3) & 63;
        IODev *iodev = iodevs[idx];
        if (iodev == NULL) {
            initiodevs ();
            iodev = iodevs[idx];
            if (iodev == NULL) return UNSUPIO;
        }
        input = iodev->ioinstr (opcode, input);
    }
    return input;
}

// reset all IO devices
void ioreset ()
{
    extarith.ioreset ();
    iodevptape.ioreset ();
    iodevrk8je.ioreset ();
    iodevrtc.ioreset ();
    iodevtty.ioreset ();
    memext.ioreset ();
    scncall.ioreset ();
}

// disassemble IO instruction
//  input:
//   opcode = all 12 bits of opcode
//  output:
//   returns NULL: opcode unknown
//           else: description string
char const *iodisas (uint16_t opcode)
{
    if ((opcode & 07000) == 06000) {
        initiodevs ();
        uint16_t idx = (opcode >> 3) & 63;
        IODev *iodev = iodevs[idx];
        if (iodev != NULL) {
            for (uint16_t i = 0; i < iodev->opscount; i ++) {
                if (iodev->opsarray[i].opcd == opcode) {
                    return iodev->opsarray[i].desc;
                }
            }
        }
    }
    return NULL;
}

// initialize iodevs[] array if not already
static void initiodevs ()
{
    if (! initted) {
        iodevs[000] = &memext;
        iodevs[010] = &mempar;
        iodevs[020] = &memext;
        iodevs[021] = &memext;
        iodevs[022] = &memext;
        iodevs[023] = &memext;
        iodevs[024] = &memext;
        iodevs[025] = &memext;
        iodevs[026] = &memext;
        iodevs[027] = &memext;
        if (! randmem) {
            iodevs[001] = &iodevptape;
            iodevs[002] = &iodevptape;
            iodevs[003] = &iodevtty;
            iodevs[004] = &iodevtty;
            iodevs[013] = &iodevrtc;
            if (lincenab) iodevs[014] = &linc;
            iodevs[030] = &scncall;
            iodevs[031] = &scncall2;
            iodevs[074] = &iodevrk8je;
            //iodevs[073] = &iodevrk8;
            //iodevs[074] = &iodevrk8;
            //iodevs[075] = &iodevrk8;
        }
        initted = true;
    }
}

// by default, devices don't have any opcode disassembly
IODev::IODev ()
{
    opscount = 0;
    opsarray = NULL;
}
