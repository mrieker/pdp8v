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
 * Test Memory Address register hardware
 * Assumes RPI, SEQ and MA boards only in place
 * ...along with MQ->_ALUQ jumpers
 * Counts MA register down starting at 0477
 */

#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "disassemble.h"
#include "gpiolib.h"
#include "miscdefs.h"
#include "rdcyc.h"

#define ever (;;)

int main (int argc, char **argv)
{
    setlinebuf (stdout);

    // access tube circuitry and simulator
    PhysLib *hardware = new PhysLib ();
    hardware->open ();

    uint16_t nextopcode = 07300;
    for ever {

        // reset into FETCH1
        hardware->writegpio (false, G_RESET);
        hardware->halfcycle ();
        hardware->writegpio (false, 0);
        hardware->halfcycle ();

        // clock into FETCH2, give it opcode halfway through
        hardware->writegpio (false, G_CLOCK);
        hardware->halfcycle ();
        hardware->writegpio (true, nextopcode * G_DATA0);
        hardware->halfcycle ();

        // clock into next cycle, whatever that is
        // keep sending opcode during first half so it has hold time
        hardware->writegpio (true, G_CLOCK | nextopcode * G_DATA0);
        hardware->halfcycle ();
        hardware->writegpio (false, 0);
        hardware->halfcycle ();

        // set up next opcode
        nextopcode = (nextopcode + 1) & 07777;
    }
}
