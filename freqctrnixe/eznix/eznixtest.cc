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

// g++ -Wall -O2 -o eznixtest eznixtest.cc eznix.cc -lpthread

#include <stdint.h>
#include <stdlib.h>

#include "eznix.h"

int main (int argc, char **argv)
{
    EZNix eznix;

    int dutypct = (argc > 1) ? atoi (argv[1]) : 100;
    eznix.open (dutypct);

    uint64_t startus = EZNix::getnowus () / 1000000 * 1000000;
    while (1) {
        int i, j;
        for (j = 0; j < 3; j ++) {
            uint16_t led = (0x1000 >> j) ^ 0x1C00;
            eznix.shiftout (0x2000 | led);
            startus += 1000000;
            EZNix::sleepus (startus);
            for (i = 0; i < 10; i ++) {
                eznix.shiftout (0x2000 | led | (1 << i));
                startus += 1000000;
                EZNix::sleepus (startus);
            }
        }
    }
}
