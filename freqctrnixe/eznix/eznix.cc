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

// access eznixie board plugged into gpio connector

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/time.h>
#include <unistd.h>

#include "eznix.h"

#define IGO 1

#define GPIO_FSEL0 (0)          // set output current / disable (readwrite)
#define GPIO_SET0 (0x1C/4)      // set gpio output bits (writeonly)
#define GPIO_CLR0 (0x28/4)      // clear gpio output bits (writeonly)
#define GPIO_LEV0 (0x34/4)      // read gpio bit levels (readonly)
#define GPIO_PUDMOD (0x94/4)    // pullup/pulldown enable (0=disable; 1=enable pulldown; 2=enable pullup)
#define GPIO_PUDCLK (0x98/4)    // which pins to change pullup/pulldown mode

#define DSIN 14     // assumes eznixie board is connected to these gpio pins
#define DSOUT 15
#define STCP 18
#define SHCP 17
#define _OUTEN 4

EZNix::EZNix ()
{
    memfd = -1;
    memptr = NULL;
    gpiopage = NULL;
    pwmtid = 0;
    pthread_mutex_init (&gpiomutex, NULL);
}

EZNix::~EZNix ()
{
    this->close ();
}

void EZNix::open (int dutypct)
{
    // make sure not already open
    if (memfd >= 0) abort ();

    // access gpio page in physical memory
    memfd = ::open ("/dev/gpiomem", O_RDWR | O_SYNC);
    if (memfd < 0) {
        fprintf (stderr, "EZNix::open: error opening /dev/gpiomem: %m\n");
        abort ();
    }

    // lock access to gpio
trylk:;
    struct flock flockit;
    memset (&flockit, 0, sizeof flockit);
    flockit.l_type   = F_WRLCK;
    flockit.l_whence = SEEK_SET;
    flockit.l_len    = 4096;
    if (fcntl (memfd, F_SETLK, &flockit) < 0) {
        if (((errno == EACCES) || (errno == EAGAIN)) && (fcntl (memfd, F_GETLK, &flockit) >= 0)) {
            if (flockit.l_type == F_UNLCK) goto trylk;
            fprintf (stderr, "EZNix::open: error locking /dev/gpiomem: locked by pid %d\n", (int) flockit.l_pid);
            abort ();
        }
        fprintf (stderr, "EZNix::open: error locking /dev/gpiomem: %m\n");
        abort ();
    }

    // map gpio page to memory
    memptr = mmap (NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0);
    if (memptr == MAP_FAILED) {
        fprintf (stderr, "EZNix::open: error mmapping /dev/gpiomem: %m\n");
        abort ();
    }

    // save pointer to gpio register page
    gpiopage = (uint32_t volatile *) memptr;

    // https://www.raspberrypi.org/documentation/hardware/raspberrypi/gpio/gpio_pads_control.md
    // 111... means 2mA; 777... would be 14mA

    gpiopage[GPIO_FSEL0+0] = IGO * 00000010000; // gpio<04> = output enable
    gpiopage[GPIO_FSEL0+1] = IGO * 00110110000; // gpio<14> = data in; <17> = shift clock; <18> = output clock
    gpiopage[GPIO_FSEL0+2] = IGO * 00000000000;

    // turn output off to begin with
    gpout (_OUTEN, 1);

    // spawn a thread to work the output enable to modulate output duty
    this->dutypct = dutypct;
    if (pthread_create (&pwmtid, NULL, threadwrap, this) != 0) abort ();
}

void EZNix::close ()
{
    pthread_mutex_lock (&gpiomutex);
    munmap (memptr, 4096);
    ::close (memfd);
    memfd = -1;
    memptr = NULL;
    gpiopage = NULL;
    pthread_mutex_unlock (&gpiomutex);
    if (pwmtid != 0) {
        pthread_join (pwmtid, NULL);
        pwmtid = 0;
    }
}

// thread to modulate output duty
void *EZNix::threadwrap (void *zhis)
{
    ((EZNix *)zhis)->thread ();
    return NULL;
}

void EZNix::thread ()
{
    uint64_t startus = getnowus () / 1000000 * 1000000;
    while (true) {
        // 20mS per loop

        // turn output on for dutypct of that 20mS
        int dp = dutypct;
        if (dp > 0) {
            pthread_mutex_lock (&gpiomutex);
            if (gpiopage == NULL) break;
            gpout (_OUTEN, 0);
            pthread_mutex_unlock (&gpiomutex);
        }

        if ((0 < dp) && (dp < 100)) sleepus (startus + 200 * dp);

        // turn output off for remainder of that 20mS
        if (dp < 100) {
            pthread_mutex_lock (&gpiomutex);
            if (gpiopage == NULL) break;
            gpout (_OUTEN, 1);
            pthread_mutex_unlock (&gpiomutex);
        }

        startus += 20000;
        sleepus (startus);
    }

    // close() has been called, stop
    pthread_mutex_unlock (&gpiomutex);
}

void EZNix::sleepus (uint64_t when)
{
    uint64_t now = getnowus ();
    if (when > now) usleep (when - now);
}

uint64_t EZNix::getnowus ()
{
    struct timeval nowtv;
    if (gettimeofday (&nowtv, NULL) < 0) abort ();
    return nowtv.tv_sec * 1000000ULL + nowtv.tv_usec;
}

// shift the given 16-bit word out to the eznixie board
uint16_t EZNix::shiftout (uint16_t newword)
{
    uint16_t oldword = 0;
    pthread_mutex_lock (&gpiomutex);
    for (int i = 16; -- i >= 0;) {
        oldword += oldword + gpin (DSOUT);
        gpout (DSIN, (newword >> i) & 1);
        gpout (SHCP, 1);
        gpout (SHCP, 0);
    }
    gpout (STCP, 1);
    gpout (STCP, 0);
    pthread_mutex_unlock (&gpiomutex);
    return oldword;
}

// write the given gpio pin
void EZNix::gpout (int pin, int val)
{
    uint32_t mask = 1U << pin;
    gpiopage[val ? GPIO_SET0 : GPIO_CLR0] = mask;
}

// read the given gpio pin
int EZNix::gpin (int pin)
{
    return (gpiopage[GPIO_LEV0] >> pin) & 1;
}
