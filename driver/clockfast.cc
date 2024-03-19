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

// cc -O2 -o clockfast.armv7l clockfast.cc

#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "gpiolib.h"

#define IGO 1

#define GPIO_FSEL0 (0)          // set output current / disable (readwrite)
#define GPIO_SET0 (0x1C/4)      // set gpio output bits (writeonly)
#define GPIO_CLR0 (0x28/4)      // clear gpio output bits (writeonly)
#define GPIO_LEV0 (0x34/4)      // read gpio bit levels (readonly)
#define GPIO_PUDMOD (0x94/4)    // pullup/pulldown enable (0=disable; 1=enable pulldown; 2=enable pullup)
#define GPIO_PUDCLK (0x98/4)    // which pins to change pullup/pulldown mode

int main ()
{
    struct flock flockit;

    // access gpio page in physical memory
    int memfd = ::open ("/dev/gpiomem", O_RDWR | O_SYNC);
    if (memfd < 0) {
        fprintf (stderr, "PhysLib::open: error opening /dev/gpiomem: %m\n");
        ABORT ();
    }

trylk:;
    memset (&flockit, 0, sizeof flockit);
    flockit.l_type   = F_WRLCK;
    flockit.l_whence = SEEK_SET;
    flockit.l_len    = 4096;
    if (fcntl (memfd, F_SETLK, &flockit) < 0) {
        if (((errno == EACCES) || (errno == EAGAIN)) && (fcntl (memfd, F_GETLK, &flockit) >= 0)) {
            if (flockit.l_type == F_UNLCK) goto trylk;
            fprintf (stderr, "PhysLib::open: error locking /dev/gpiomem: locked by pid %d\n", (int) flockit.l_pid);
            ABORT ();
        }
        fprintf (stderr, "PhysLib::open: error locking /dev/gpiomem: %m\n");
        ABORT ();
    }

    void *memptr = mmap (NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0);
    if (memptr == MAP_FAILED) {
        fprintf (stderr, "PhysLib::open: error mmapping /dev/gpiomem: %m\n");
        ABORT ();
    }

    // save pointer to gpio register page
    uint32_t volatile *gpiopage = (uint32_t volatile *) memptr;

    // https://www.raspberrypi.org/documentation/hardware/raspberrypi/gpio/gpio_pads_control.md
    // 111... means 2mA; 777... would be 14mA

    gpiopage[GPIO_FSEL0+0] = IGO * 00000001100; // gpio<09:00> = data<05:00>, reset, clock, unused<1:0>
    gpiopage[GPIO_FSEL0+1] = IGO * 01010000000; // gpio<19:10> = qena, unused, ioskip, link, data<11:06>
    gpiopage[GPIO_FSEL0+2] = IGO * 00000000011; // gpio<27:20> = intak, write, read, dfrm, ioinst, jump, dena, intrq
    uint32_t gpiovalu = 0;                      // make sure gpio pin state matches what we have cached
    gpiopage[GPIO_CLR0] = ~ G_REVOS & G_ALL;
    gpiopage[GPIO_SET0] =   G_REVOS & G_ALL;

    // allow link,data bits to be read
    gpiopage[GPIO_FSEL0+0] = IGO * 00000001100;     // enable reset,clock outputs, data<05:00> are inputs
    gpiopage[GPIO_FSEL0+1] = IGO * 01010000000;     // enable qena,ioskip outputs, link,data<11:06> are inputs

    uint32_t lasttime = time (NULL);
    uint32_t lastcount = 0;
    uint32_t thiscount = 0;

    uint32_t speed = 20000;

    while (true) {
        for (int i = 0; i < speed; i ++) { asm volatile (""); }

        gpiovalu ^= G_CLOCK;

        // write gpio pins
        gpiopage[GPIO_CLR0] = ~ gpiovalu & G_ALL;
        gpiopage[GPIO_SET0] =   gpiovalu & G_ALL;

        thiscount ++;

        uint32_t thistime = time (NULL);
        if (lasttime != thistime) {
            printf ("speed=%u  count=%u\n", speed, thiscount - lastcount);
            lasttime = thistime;
            lastcount = thiscount;
            speed -= 100;
        }
    }
}
