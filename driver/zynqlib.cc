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

#include "gpiolib.h"
#include "rdcyc.h"

ZynqLib::ZynqLib ()
    : ZynqLib ("aclalumapcrpiseq")
{ }

ZynqLib::ZynqLib (char const *modname)
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
    fprintf (stderr, "ZynqLib::open: zynq fpga version %08X\n", gpiopage[3]);

    gpiopage[4] = boardena;
    uint32_t berb = gpiopage[4];
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
}

void ZynqLib::close ()
{
    pthread_mutex_lock (&trismutex);
    gpiopage = NULL;
    pthread_mutex_unlock (&trismutex);
    gpiofile.close ();
}

void ZynqLib::halfcycle (bool aluadd)
{
    // let the values written to gpio port soak into gates
    TimedLib::halfcycle (aluadd);

    // now read the stats as to how many gates are one = triodes are off
    pthread_mutex_lock (&trismutex);
    if (gpiopage != NULL) {
        uint32_t nttnto = gpiopage[5];
        numtrisoff += nttnto & 0xFFFFU;
        ntotaltris += nttnto >> 16;
    }
    pthread_mutex_unlock (&trismutex);
}

// returns all signals with active high orientation
uint32_t ZynqLib::readgpio ()
{
    uint32_t value = gpiopage[2];
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
    gpiopage[1] = value;

    // verify that it got the value
    uint32_t readback = gpiopage[2];
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
    gpiopage[12] = 0xFFFFFFFFU;
    gpiopage[13] = 0xFFFFFFFFU;
    gpiopage[14] = 0xFFFFFFFFU;
    gpiopage[15] = 0xFFFFFFFFU;
    return true;
}

// read abcd connector pins
//  output:
//    returns pins read from connectors
void ZynqLib::readpads (uint32_t *pinss)
{
    pinss[0] = gpiopage[ 8];
    pinss[1] = gpiopage[ 9];
    pinss[2] = gpiopage[10];
    pinss[3] = gpiopage[11];
}

// write abcd connector pins
//  input:
//    masks = which pins are output pins (others are inputs or not connected)
//    pinss = values for the pins listed in mask (others are don't care)
void ZynqLib::writepads (uint32_t const *masks, uint32_t const *pinss)
{
    gpiopage[12] = pinss[0] | ~ masks[0];
    gpiopage[13] = pinss[1] | ~ masks[1];
    gpiopage[14] = pinss[2] | ~ masks[2];
    gpiopage[15] = pinss[3] | ~ masks[3];
}
