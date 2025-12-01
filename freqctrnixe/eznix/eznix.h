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

#ifndef _EZNIX_H
#define _EZNIX_H

#include <pthread.h>
#include <stdint.h>

struct EZNix {
    int dutypct;

    EZNix ();
    virtual ~EZNix ();
    void open (int dutypct);
    void close ();

    uint16_t shiftout (uint16_t newword);
    static uint64_t getnowus ();
    static void sleepus (uint64_t when);

private:
    int memfd;
    void *memptr;
    uint32_t volatile *gpiopage;
    pthread_t pwmtid;
    pthread_mutex_t gpiomutex;

    static void *threadwrap (void *zhis);
    void thread ();
    void gpout (int pin, int val);
    int gpin (int pin);
};

#endif
