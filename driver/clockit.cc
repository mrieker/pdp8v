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
 * Program runs on raspberry pi to generate clock signal.
 * sudo insmod km/enabtsc.ko
 * sudo ./clockit [200]
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "gpiolib.h"
#include "miscdefs.h"

int main (int argc, char **argv)
{
    double cpuhz = DEFCPUHZ;

    if (argc > 1) cpuhz = strtod (argv[1], NULL);

    printf ("cpuhz=%f\n", cpuhz);

    GpioLib *gpio = new PhysLib ();
    gpio->open ();

    while (1) {
        gpio->writegpio (false, 0);
        usleep ((useconds_t)(500000 / cpuhz));
        gpio->writegpio (false, G_CLOCK);
        usleep ((useconds_t)(500000 / cpuhz));
    }

    return 0;
}
