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

// turn all pidp panel leds on and walk a zero through
// then turn them all off and walk a one through

// main switches xor into df,if,pc leds
// control switches xor into instr leds

#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "pidp.h"

static bool volatile ctrlcflag;

static void sighand (int signum);

static uint16_t lamps[90] = {
    0x70E,    // df msb
    0x70F,
    0x704,    // df lsb
    0x705,    // if msb
    0x706,
    0x707,    // if lsb
    0x00E,    // pc msb
    0x00F,
    0x004,
    0x005,
    0x006,
    0x007,
    0x008,
    0x009,
    0x00A,
    0x00B,
    0x00C,
    0x00D,    // pc lsb
    0x10E,    // ma msb
    0x10F,
    0x104,
    0x105,
    0x106,
    0x107,
    0x108,
    0x109,
    0x10A,
    0x10B,
    0x10C,
    0x10D,    // ma lsb
    0x20E,    // mb msb
    0x20F,
    0x204,
    0x205,
    0x206,
    0x207,
    0x208,
    0x209,
    0x20A,
    0x20B,
    0x20C,
    0x20D,    // mb lsb
    0x708,    // link
    0x30E,    // ac msb
    0x30F,
    0x304,
    0x305,
    0x306,
    0x307,
    0x308,
    0x309,
    0x30A,
    0x30B,
    0x30C,
    0x30D,    // ac lsb
    0x607,    // sc msb
    0x608,
    0x609,
    0x60A,
    0x60B,    // sc lsb
    0x40E,    // mq msb
    0x40F,
    0x404,
    0x405,
    0x406,
    0x407,
    0x408,
    0x409,
    0x40A,
    0x40B,
    0x40C,
    0x40D,    // mq lsb
    0x50E,    // and
    0x50F,    // tad
    0x504,    // isz
    0x505,    // dca
    0x506,    // jms
    0x507,    // jmp
    0x508,    // iot
    0x509,    // opr
    0x50A,    // fet
    0x50B,    // exe
    0x50C,    // def
    0x50D,    // wct
    0x60E,    // cad
    0x60F,    // brk
    0x604,    // ion
    0x605,    // pau
    0x606,    // run
    0
};

int main ()
{
    setlinebuf (stdout);

    PiDPMsg pidpmsg;
    memset (&pidpmsg, 0, sizeof pidpmsg);

    signal (SIGINT, sighand);

    while (! ctrlcflag) {

        time_t now = time (NULL);
        uint16_t inv = 0; //// ((now / 90) & 1) ? 0xFFF0 : 0x0000;

        for (int i = 0; i < 8; i ++) {
            pidpmsg.ledrows[i] = inv;
        }

        int l = now % 90;
        uint16_t word = lamps[l];
        if (word != 0) {
            int r = word / 256;
            int b = word % 256;
            pidpmsg.ledrows[r] ^= 1 << b;
        }

        // ledrow 0 = PC
        // swrow  0 = main switches
        pidpmsg.ledrows[0] ^= pidpmsg.swrows[0];

        // ledrow 7 = DFIF
        // swrow  1 = field switches
        pidpmsg.ledrows[7] ^= pidpmsg.swrows[1];

        // ledrow 5 = and,tad,isz,dca,jms,jmp,iot,opr
        // swrow  2 = start,ldad,dep,exam,cont,sstep,sinst
        pidpmsg.ledrows[5] ^= pidpmsg.swrows[2];

        pidp_proc (&pidpmsg);

        printf ("\rswrows  %04X  %04X  %04X ", pidpmsg.swrows[0], pidpmsg.swrows[1], pidpmsg.swrows[2]);
        fflush (stdout);
    }
    return 0;
}

static void sighand (int signum)
{
    ctrlcflag = true;
}
