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

// turn all pidp panel leds on

#include <signal.h>
#include "pidp.h"

static bool volatile ctrlcflag;

static void sighand (int signum);

int main ()
{
    signal (SIGINT, sighand);
    while (! ctrlcflag) {
        PiDPMsg pidpmsg;
        for (int i = 0; i < 8; i ++) {
            pidpmsg.ledrows[i] = 0xFFF0;
        }
        pidp_proc (&pidpmsg);
    }
    return 0;
}

static void sighand (int signum)
{
    ctrlcflag = true;
}
