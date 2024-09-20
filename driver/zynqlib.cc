//    Copyright (C) Mike Rieker, Beverly, MA USA
//    www.outerworldapps.com
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; version 2 of the License.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    EXPECT it to FAIL when someone's HeALTh or PROpeRTy is at RISk.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    http://www.gnu.org/licenses/gpl-2.0.html

// access zynq implementation
// tube-speed simulation - verifies modules files programmed into zynq fpga

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gpiolib.h"
#include "rdcyc.h"
#include "resetprocessorexception.h"

#define GP_INPUT 0
#define GP_OUTPUT 1
#define GP_COMPOS 2
#define GP_VERSION 3
#define GP_BOARDENA 4
#define GP_NTTNTO 5
#define GP_PADRDA 8
#define GP_PADRDB 9
#define GP_PADRDC 10
#define GP_PADRDD 11
#define GP_PADWRA 12
#define GP_PADWRB 13
#define GP_PADWRC 14
#define GP_PADWRD 15
#define GP_FPIN 16
#define GP_FPOUT 17

// sent by frontpanel.v
#define FPI_DATA0 0x00001U      // data word
#define FPI_DATA  0x00FFFU
#define FPI_DEP   0x01000U      // deposit key, write memory
#define FPI_EXAM  0x02000U      // examine key, read memory
#define FPI_JUMP  0x04000U      // along with FPI_CONT, set PC to FPI_DATA
#define FPI_CONT  0x08000U      // continue key
#define FPI_STRT  0x10000U      // along with FPI_CONT, reset I/O
#define FPI_STOP  0x20000U      // step or stop key

// sent to frontpanel.v
#define FPO_DATA0   0x0001U     // data word
#define FPO_DATA    0x0FFFU
#define FPO_ENABLE  0x1000U     // enable frontpanel.v processing
#define FPO_CLOCK   0x2000U     // clocks frontpanel.v processing
#define FPO_STOPPED 0x4000U     // indicates raspictl is stopped

ZynqLib::ZynqLib ()
    : ZynqLib ("aclalumapcrpiseq", NULL)
{ }

ZynqLib::ZynqLib (char const *modname, uint16_t *memarray)
{
    libname = "zynqlib";

    gpiopage = NULL;

    boardena =
        ((strstr (modname, "acl") != NULL) ? 001 : 0) |
        ((strstr (modname, "alu") != NULL) ? 002 : 0) |
        ((strstr (modname, "ma")  != NULL) ? 004 : 0) |
        ((strstr (modname, "pc")  != NULL) ? 010 : 0) |
        ((strstr (modname, "rpi") != NULL) ? 020 : 0) |
        ((strstr (modname, "seq") != NULL) ? 040 : 0);

    this->memarray = memarray;
}

ZynqLib::~ZynqLib ()
{
    this->close ();
}

void ZynqLib::open ()
{
    TimedLib::open ();

    // access gpio page in physical memory
    // /proc/zynqgpio is a simple mapping of the single gpio page of zynq.vhd
    // it is created by loading the km-zynqgpio/zynqgpio.ko kernel module
    gpiopage = (uint32_t volatile *) gpiofile.open ("/proc/zynqgpio");
    fprintf (stderr, "ZynqLib::open: zynq fpga version %08X\n", gpiopage[GP_VERSION]);

    gpiopage[GP_BOARDENA] = boardena;
    uint32_t berb = gpiopage[GP_BOARDENA];
    std::string best;
    if (berb & 001) best.append ("acl ");
    if (berb & 002) best.append ("alu ");
    if (berb & 004) best.append ("ma ");
    if (berb & 010) best.append ("pc ");
    if (berb & 020) best.append ("rpi ");
    if (berb & 040) best.append ("seq ");
    fprintf (stderr, "ZynqLib::open: boardena %02o=%s\n", berb, best.c_str ());
    if (berb != boardena) {
        fprintf (stderr, "ZynqLib::open: boardena %02o read back as %02o\n", boardena, berb);
        ABORT ();
    }

    // enable the i2c console
    gpiopage[GP_FPOUT] = FPO_ENABLE;
}

void ZynqLib::close ()
{
    pthread_mutex_lock (&trismutex);
    gpiopage = NULL;
    pthread_mutex_unlock (&trismutex);
    gpiofile.close ();
}

void ZynqLib::halfcycle (bool aluadd, bool topoloop)
{
    // let the values written to gpio port soak into gates
    TimedLib::halfcycle (aluadd);

    // now read the stats as to how many gates are one = triodes are off
    pthread_mutex_lock (&trismutex);
    if (gpiopage != NULL) {
        uint32_t nttnto = gpiopage[GP_NTTNTO];
        numtrisoff += nttnto & 0xFFFFU;
        ntotaltris += nttnto >> 16;
    }
    pthread_mutex_unlock (&trismutex);

    // if at top of raspictl's main loop, ie not in middle of read/write/io,
    // ...check for step/stop switch from frontpanel.v
    if (topoloop && (gpiopage[GP_FPIN] & FPI_STOP)) {
        fpstopped ();
    }
}

// returns all signals with active high orientation
uint32_t ZynqLib::readgpio ()
{
    uint32_t value = gpiopage[GP_COMPOS];
    if (value & G_QENA) value ^= (G_REVIS & G_INS) | G_REVOS;
    if (value & G_DENA) value ^= G_REVIS | (G_OUTS & G_REVOS);
    return value;
}

//  wdata = false: reading G_LINK,DATA pins; true: writing G_LINK,DATA pins
//  value = value to write, all active high orientation, excluding G_DENA and G_QENA
void ZynqLib::writegpio (bool wdata, uint32_t value)
{
    // insert G_DENA or G_QENA as appropriate for mask
    uint32_t mask;
    if (wdata) {
        mask = G_OUTS | G_LINK | G_DATA;

        // caller is writing data to the GPIO pins and wants it to go to MQ
        // clearing G_DENA blocks MD from interfering with GPIO pins
        // setting G_QENA enables MQ going to processor
        value &= ~ G_DENA;
        value |=   G_QENA;
    } else {
        mask = G_OUTS;

        // caller isn't writing and may want to read the GPIO data pins
        // setting G_DENA enables data from MD to get to GPIO data pins
        // clearing G_QENA blocks MQ from loading GPIO data pins
        value |=   G_DENA;
        value &= ~ G_QENA;
    }

    // flip some bits going outbound
    value ^= G_REVOS;

    // send out the value
    gpiopage[GP_OUTPUT] = value;

    // verify that it got the value
    uint32_t readback = gpiopage[GP_COMPOS];
    uint32_t diff = (readback ^ value) & mask;
    if (diff != 0) {
        fprintf (stderr, "ZynqLib::writegpio: wrote %08X mask %08X => %08X, readback %08X => %08X, diff %08X\n",
                    value, mask, value & mask, readback, readback & mask, diff);
        ABORT ();
    }
}

// we always have paddles
// physlib open-collectors everything so do equivalent here
bool ZynqLib::haspads ()
{
    gpiopage[GP_PADWRA] = 0xFFFFFFFFU;
    gpiopage[GP_PADWRB] = 0xFFFFFFFFU;
    gpiopage[GP_PADWRC] = 0xFFFFFFFFU;
    gpiopage[GP_PADWRD] = 0xFFFFFFFFU;
    return true;
}

// read abcd connector pins
//  output:
//    returns pins read from connectors
void ZynqLib::readpads (uint32_t *pinss)
{
    pinss[0] = gpiopage[GP_PADRDA];
    pinss[1] = gpiopage[GP_PADRDB];
    pinss[2] = gpiopage[GP_PADRDC];
    pinss[3] = gpiopage[GP_PADRDD];
}

// write abcd connector pins
//  input:
//    masks = which pins are output pins (others are inputs or not connected)
//    pinss = values for the pins listed in mask (others are don't care)
void ZynqLib::writepads (uint32_t const *masks, uint32_t const *pinss)
{
    gpiopage[GP_PADWRA] = pinss[0] | ~ masks[0];
    gpiopage[GP_PADWRB] = pinss[1] | ~ masks[1];
    gpiopage[GP_PADWRC] = pinss[2] | ~ masks[2];
    gpiopage[GP_PADWRD] = pinss[3] | ~ masks[3];
}

// frontpanel.v has told us to stop
// in middle of cycle with clock still high
// do front panel things as directed by frontpanel.v
// return to resume processing
void ZynqLib::fpstopped ()
{
    // let frontpanel.v know we have stopped
    gpiopage[GP_FPOUT] = FPO_ENABLE | FPO_STOPPED;

    while (true) {

        // FPO_CLOCK is low, wait a bit for fpga to process last thing sent
        usleep (1000);

        // get fpga request value
        uint32_t sample = gpiopage[GP_FPIN];

        // tell fpga we got it by driving clock high then wait a bit before sending reply
        gpiopage[GP_FPOUT] = FPO_ENABLE | FPO_STOPPED | FPO_CLOCK;
        usleep (1000);

        // process request to write memory
        if (sample & FPI_DEP) {
            uint16_t addr = (sample & FPI_DATA) / FPI_DATA0;
            gpiopage[GP_FPOUT] = FPO_ENABLE | FPO_STOPPED;
            usleep (1000);
            memarray[addr] = (gpiopage[GP_FPIN] & FPI_DATA) / FPI_DATA0;
            gpiopage[GP_FPOUT] = FPO_ENABLE | FPO_STOPPED | FPO_CLOCK;
            usleep (1000);
        }

        // process request to read memory
        if (sample & FPI_EXAM) {
            uint16_t addr = (sample & FPI_DATA) / FPI_DATA0;
            uint16_t data = memarray[addr];
            gpiopage[GP_FPOUT] = FPO_ENABLE | FPO_STOPPED | data * FPO_DATA0;
            usleep (1000);
            gpiopage[GP_FPOUT] = FPO_ENABLE | FPO_STOPPED | data * FPO_DATA0 | FPO_CLOCK;
            usleep (1000);
        }

        // drive clock low cuz that's what's expected at top of loop
        gpiopage[GP_FPOUT] = FPO_ENABLE | FPO_STOPPED;

        // process request to continue processing
        if (sample & FPI_CONT) {
            gpiopage[GP_FPOUT] = FPO_ENABLE;                // indicate that processor is no longer stopped
            if (! (sample & FPI_JUMP)) break;               // if no 'load address' done, just continue in current state
            ResetProcessorException rpe;                    // reset to load program counter and resume processing
            rpe.startpc = (sample & FPI_DATA) / FPI_DATA0;  // load address wipes state and writes program counter
            rpe.resetio = true;                             // assume it was start switch
            rpe.startac = 0;
            rpe.startln = 0;
            if (! (sample & FPI_STRT)) {                    // start switch resets i/o and clears link and accum
                ABCD abcd;                                  // otherwise, preserve link and accum
                readpads (abcd.cons);
                abcd.decode ();
                rpe.startac = abcd.acq;
                rpe.startln = abcd.lnq;
                rpe.resetio = false;                        // ...and don't reset io
            }
            throw rpe;                                      // tink!
        }
    }
}
