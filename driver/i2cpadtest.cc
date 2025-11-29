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

#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "gpiolib.h"
#include "readprompt.h"

int main (int argc, char **argv)
{
    setlinebuf (stdout);

    PhysLib *gp = new PhysLib ();
    gp->open ();

    if (! gp->haspads ()) {
        fprintf (stderr, "raspictl: paddles not present\n");
        return 1;
    }

#if 000
    while (true) {
        char const *line = readprompt ("> ");
        if (line == NULL) break;
        char ch;
        while ((ch = *(line ++)) != 0) {
            if (ch > ' ') break;
        }
        switch (ch) {
            case 0: break;
            case 'r': {
                uint8_t num = strtoul (line, (char **)&line, 16);
                uint8_t reg = strtoul (line, (char **)&line, 16);
                uint16_t data = gp->i2cread16 (num | 0x20, reg);
                printf (" %02X %02X => %04X\n", num, reg, data);
                break;
            }
            case 'w': {
                uint8_t num = strtoul (line, (char **)&line, 16);
                uint8_t reg = strtoul (line, (char **)&line, 16);
                uint16_t data = strtoul (line, (char **)&line, 16);
                gp->i2cwrite16 (num | 0x20, reg, data);
                printf (" %02X %02X <= %04X\n", num, reg, data);
                break;
            }
            default: {
                fprintf (stderr, "r <num> <reg>\n");
                fprintf (stderr, "w <num> <reg> <data>\n");
                break;
            }
        }
    }
#endif

    uint32_t mask[4];
    memset (mask, -1, sizeof mask);

    uint32_t pads[4];

    for (int i = 0; i < 128; i ++) {
        memset (pads, -1, sizeof pads);
        pads[i>>5] -= 1U << (i & 31);
        gp->writepads (mask, pads);
        sleep (1);

        gp->readpads (pads);
        printf ("%6d  %08X  %08X  %08X  %08X\n", i, pads[3], pads[2], pads[1], pads[0]);
        usleep (333333);
    }

    memset (pads, -1, sizeof pads);
    gp->writepads (mask, pads);

    int n = 0;
    while (true) {
        gp->readpads (pads);
        printf ("%6d  %08X  %08X  %08X  %08X\n", ++ n, pads[3], pads[2], pads[1], pads[0]);
        usleep (100000);
    }

    return 0;
}
