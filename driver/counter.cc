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

/**
 * Program runs on the raspberry pi with counter.v loaded into DE0.
 *
 *  ./counter
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "gpiolib.h"

#define ever (;;)

static uint32_t randuint32 ();

int main (int argc, char **argv)
{
    setlinebuf (stdout);

    // access cpu circuitry
    GpioLib *gpio = new PhysLib ();
    gpio->open ();

    // reset CPU circuit for a couple cycles
    gpio->writegpio (false, G_RESET);
    gpio->halfcycle ();
    gpio->halfcycle ();
    gpio->halfcycle ();

    // drop reset and leave clock signal low
    // enable reading data from cpu
    gpio->writegpio (false, 0);
    gpio->halfcycle ();

    uint32_t errors  = 0;
    uint32_t flips   = 0;
    uint32_t countsb = 0x87654321;
    uint32_t torand  = (randuint32 () % 4) + 1;
    for ever {

        // invariant:
        //  clock has been low for half cycle
        //  G_DENA has been asserted for half cycle so we can read MDL,MD coming from cpu

        // get input signals just before raising clock
        uint32_t sample = gpio->readgpio ();

        // clock is fed back through the G_IAK line so should show as low
        if (sample & G_IAK) {
            printf ("clock is high  sample %08X  errors %u\n", sample, ++ errors);
        }

        // counter low bits come back on write,read,dfrm,ioin,link,data lines
        uint32_t countis = (sample & G_WRITE ? 0200000 : 0) | (sample & G_READ ? 0100000 : 0) | (sample & G_DFRM ? 040000 : 0) |
                            (sample & G_IOIN ? 020000 : 0) | (sample & G_LINK ? 010000 : 0) | ((sample / G_DATA0) & 07777) |
                            (countsb & ~ 0377777);

        if (countis != countsb) {
            printf ("count is %06o should be %06o   errors %u\n", countis & 0377777, countsb & 0377777, ++ errors);
        }
        /*
        printf ("countsb %u %u %u %u  %u %u %u %u  %u %u %u %u  %u %u %u %u  %u %u %u %u  %u %u %u %u  %u %u %u %u  %u %u %u %u   countis %u %u %u %u  %u %u %u %u  %u %u %u %u  %u %u %u %u  %u %u %u %u  %u %u %u %u  %u %u %u %u  %u %u %u %u\n",
            (countsb >> 31) & 1, (countsb >> 30) & 1, (countsb >> 29) & 1, (countsb >> 28) & 1, (countsb >> 27) & 1, (countsb >> 26) & 1, (countsb >> 25) & 1, (countsb >> 24) & 1,
            (countsb >> 23) & 1, (countsb >> 22) & 1, (countsb >> 21) & 1, (countsb >> 20) & 1, (countsb >> 19) & 1, (countsb >> 18) & 1, (countsb >> 17) & 1, (countsb >> 16) & 1,
            (countsb >> 15) & 1, (countsb >> 14) & 1, (countsb >> 13) & 1, (countsb >> 12) & 1, (countsb >> 11) & 1, (countsb >> 10) & 1, (countsb >>  9) & 1, (countsb >>  8) & 1,
            (countsb >>  7) & 1, (countsb >>  6) & 1, (countsb >>  5) & 1, (countsb >>  4) & 1, (countsb >>  3) & 1, (countsb >>  2) & 1, (countsb >>  1) & 1, (countsb >>  0) & 1,
            (countis >> 31) & 1, (countis >> 30) & 1, (countis >> 29) & 1, (countis >> 28) & 1, (countis >> 27) & 1, (countis >> 26) & 1, (countis >> 25) & 1, (countis >> 24) & 1,
            (countis >> 23) & 1, (countis >> 22) & 1, (countis >> 21) & 1, (countis >> 20) & 1, (countis >> 19) & 1, (countis >> 18) & 1, (countis >> 17) & 1, (countis >> 16) & 1,
            (countis >> 15) & 1, (countis >> 14) & 1, (countis >> 13) & 1, (countis >> 12) & 1, (countis >> 11) & 1, (countis >> 10) & 1, (countis >>  9) & 1, (countis >>  8) & 1,
            (countis >>  7) & 1, (countis >>  6) & 1, (countis >>  5) & 1, (countis >>  4) & 1, (countis >>  3) & 1, (countis >>  2) & 1, (countis >>  1) & 1, (countis >>  0) & 1);
        printf ("        |                     |                                            | |           |                     |                                            | |\n");
        printf ("        ^---------------------^--------------------------------------------^-^           ^---------------------^--------------------------------------------^-^\n");
        printf ("                                                                             |                                                                                |\n");
        */

        // raise clock (which increments counter) then wait for half a cycle
        gpio->writegpio (false, G_CLOCK);
        gpio->halfcycle ();

        // that clock incremented the counter, so compute what the counter should be holding right now
        countsb = (countis << 1) | (((countis >> 31) ^ (countis >> 21) ^ (countis >> 1) ^ countis) & 1);

        // every now and then, write a random number to the counter
        // this flips the bi-directional bus around to see if it works
        if (-- torand == 0) {
            countsb = (countsb & ~ 037777) | (randuint32 () & 037777);  // generate a random number for counter
            sample  = (countsb & 040000 ? G_IRQ : 0) | (countsb & 020000 ? G_IOS : 0) | (countsb & 010000 ? G_LINK : 0) | ((countsb & 07777) * G_DATA0);
            gpio->writegpio (true, sample);                             // send data, drop clock
            gpio->halfcycle ();                                         // let it soak into counter's D inputs
            gpio->writegpio (true, sample | G_CLOCK);                   // ker-chunk the counter's T input
            gpio->halfcycle ();                                         // hold data on counter's D inputs a little longer
            torand = (randuint32 () % 4) + 1;                           // do it again in a few cycles
            if (++ flips % 10000 == 0) printf ("counter: %u flips (%u errors)\n", flips, errors);
        }

        // drop the clock and enable link,data bus read
        // then wait half a clock cycle while clock is low
        gpio->writegpio (false, 0);
        gpio->halfcycle ();
    }
}

// generate a random number
static uint32_t randuint32 ()
{
    static uint64_t seed = 0x123456789ABCDEF0ULL;

    // https://www.xilinx.com/support/documentation/application_notes/xapp052.pdf
    uint64_t xnor = ~ ((seed >> 63) ^ (seed >> 62) ^ (seed >> 60) ^ (seed >> 59));
    seed = (seed << 1) | (xnor & 1);

    return (uint32_t) seed;
}
