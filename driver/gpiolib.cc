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

// common struct between PhysLib and PipeLib

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gpiolib.h"
#include "pindefs.h"

GpioLib::~GpioLib ()
{ }

// by default, no implementation of examine()
// - used by testboard.cc to examine internal variables
int GpioLib::examine (char const *varname, uint32_t *value)
{
    int varnamelen = strlen (varname);

    int nbits = 0;
    *value = 0;

    uint32_t pinss[NPADS+1];
    readpads (pinss);
    pinss[NPADS] = readgpio ();

    for (int i = 0; i <= NPADS; i ++) {
        PinDefs const *ipindefs = pindefss[i];
        for (int j = 0; ipindefs[j].pinmask != 0; j ++) {

            // get pin value
            bool pin = (pinss[i] & ipindefs[j].pinmask) != 0;

            // see if base name matches
            char const *vn = ipindefs[j].varname;
            if (memcmp (vn, varname, varnamelen) == 0) {

                // if whole name matches, it's just that one bit
                if (vn[varnamelen] == 0) {
                    *value = pin ? 1 : 0;
                    return 1;
                }

                // if base matches and entry followed by [bitno],
                // add the bit to the value and keep going
                if (vn[varnamelen] == '[') {
                    int bitno = atoi (vn + varnamelen + 1);
                    if (nbits < bitno + 1) nbits = bitno + 1;
                    if (pin) *value |= 1U << bitno;
                }
            }
        }
    }
    return nbits;
}

// by default, no implementation of getvarbool()
// - used by testboard.cc to examine internal variables
bool *GpioLib::getvarbool (char const *varname, int rbit)
{
    return NULL;
}

// reset processor
void GpioLib::doareset ()
{
    uint32_t readback = 0;
    for (int retry = 0; retry < 5; retry ++) {

        // reset CPU circuit for a couple cycles
        this->writegpio (false, G_RESET);
        this->halfcycle ();
        this->halfcycle ();
        this->halfcycle ();

        // drop reset and leave clock signal low
        // enable reading data from cpu so we get initial fetch address of 0000 from processor
        this->writegpio (false, 0);
        this->halfcycle ();

#if 000
        // de0 takes extra time to reset so wait for it
        // it asserts G_IAK while resetting
        // then wait a half cycle after it goes away so processor can set up fetch
        for (int i = 0; i < 100; i ++) {
            uint32_t sample = this->readgpio ();
            this->halfcycle ();
            if (! (sample & G_IAK)) return;
        }
        fprintf (stderr, "GpioLib::doareset: waited too long for DE0 reset\n");
        ABORT ();
#endif

        // processor should be requesting to read memory location 0 (FETCH1 with PC = 0000)
        readback = this->readgpio ();
        if ((readback & (G_JUMP | G_DATA | G_IOIN | G_DFRM | G_READ | G_WRITE | G_IAK)) == G_READ) {
            if (retry > 0) fprintf (stderr, "GpioLib::doareset: %d retr%s\n", retry, ((retry > 1) ? "ies" : "y"));
            return;
        }
    }
    fprintf (stderr, "GpioLib::doareset: did not reset, readback %08X\n", readback);
    ABORT ();
}

void GpioLib::halfcycle ()
{
    halfcycle (true);
}

void GpioLib::halfcycle (bool aluadd)
{
    halfcycle ();
}

// decode 32-bit value read from the gpio connector
//  input:
//   bits = value read from the gpio connector
//  returns:
//   bits decoded as a string
std::string GpioLib::decogpio (uint32_t bits)
{
    std::string str;

    char buf[40];
    sprintf (buf, "LINK=%d DATA=%04o", (bits & G_LINK) / G_LINK, (bits & G_DATA) / G_DATA0);
    str.append (buf);

    if (bits & G_CLOCK) str.append (" CLOCK");
    if (bits & G_RESET) str.append (" RESET");
    if (bits & G_IOS)   str.append (" IOS");
    if (bits & G_IRQ)   str.append (" IRQ");
    if (bits & G_DENA)  str.append (" DENA");
    if (bits & G_QENA)  str.append (" QENA");
    if (bits & G_IOIN)  str.append (" IOIN");
    if (bits & G_DFRM)  str.append (" DFRM");
    if (bits & G_JUMP)  str.append (" JUMP");
    if (bits & G_READ)  str.append (" READ");
    if (bits & G_WRITE) str.append (" WRITE");
    if (bits & G_IAK)   str.append (" IAK");

    return str;
}
