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

// access zynq implementation

#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
#include <unistd.h>

#include "gpiolib.h"
#include "rdcyc.h"

ZynqLib::ZynqLib ()
    : ZynqLib ("aclalumapcrpiseq")
{ }

ZynqLib::ZynqLib (char const *modnames)
{
    libname = "zynqlib";

    this->memfd  = -1;
    this->memptr = NULL;

    gpiopage = NULL;

    boardena =
        ((strstr (modnames, "acl") != NULL) ? 001 : 0) |
        ((strstr (modnames, "alu") != NULL) ? 002 : 0) |
        ((strstr (modnames, "ma")  != NULL) ? 004 : 0) |
        ((strstr (modnames, "pc")  != NULL) ? 010 : 0) |
        ((strstr (modnames, "rpi") != NULL) ? 020 : 0) |
        ((strstr (modnames, "seq") != NULL) ? 040 : 0);
}

ZynqLib::~ZynqLib ()
{
    this->close ();
}

void ZynqLib::open ()
{
    TimedLib::open ();

    writecount = 0;
}

void ZynqLib::close ()
{
    fprintf (stderr, "ZynqLib::close: writecount=%llu\n", (LLU) writecount);

    pthread_mutex_lock (&trismutex);
    gpiopage = NULL;
    pthread_mutex_unlock (&trismutex);
    munmap (memptr, 4096);
    ::close (memfd);
    memptr = NULL;
    memfd  = -1;
}

void ZynqLib::halfcycle (bool aluadd)
{
    // let the values written to gpio port soak into gates
    TimedLib::halfcycle (aluadd);

    // now read the stats as to how many gates are one = triodes are off
    pthread_mutex_lock (&trismutex);
    if (gpiopage != NULL) {
        uint32_t nttnto = gpiopage[5];
        numtrisoff += nttnto & 0xFFFFU;
        ntotaltris += nttnto >> 16;
    }
    pthread_mutex_unlock (&trismutex);
}

// returns all signals with active high orientation
uint32_t ZynqLib::readgpio ()
{
    if (gpiopage == NULL) {
        opengpio ();
    }

    uint32_t value = gpiopage[2];
    value ^= gpioreadflip;

#if 000
    printf ("ZynqLib::readgpio*:  pins>%08X", value);
    printf (" IAK>%u",     (value / G_IAK)   & 1);
    printf (" WRITE>%u",   (value / G_WRITE) & 1);
    printf (" READ>%u",    (value / G_READ)  & 1);
    printf (" DFRM>%u",    (value / G_DFRM)  & 1);
    printf (" IOIN>%u",    (value / G_IOIN)  & 1);
    printf (" JUMP>%u",    (value / G_JUMP)  & 1);
    printf (" DENA>%u",    (value / G_DENA)  & 1);
    printf (" IRQ>%u",     (value / G_IRQ)   & 1);
    printf (" IOS>%u",     (value / G_IOS)   & 1);
    printf (" QENA>%u",    (value / G_QENA)  & 1);
    printf (" LINK>%u",    (value / G_LINK) & 1);
    printf (" DATA>%04o",  (value / G_DATA0) & 07777);
    printf (" RESET>%u",   (value / G_RESET) & 1);
    printf (" CLOCK>%u\n", (value / G_CLOCK) & 1);
#endif

    return value;
}

//  wdata = false: reading G_LINK,DATA pins; true: writing G_LINK,DATA pins
//  value = value to write, all active high orientation, excluding G_DENA and G_QENA
void ZynqLib::writegpio (bool wdata, uint32_t value)
{
    gpiovalu = value;

    // insert G_DENA or G_QENA as appropriate for mask
    if (wdata) {

        // caller is writing data to the GPIO pins and wants it to go to MQ
        // clearing G_DENA blocks MD from interfering with GPIO pins
        // setting G_QENA enables MQ going to processor
        value &= ~ G_DENA;
        value |=   G_QENA;
    } else {

        // caller isn't writing and may want to read the GPIO data pins
        // setting G_DENA enables data from MD to get to GPIO data pins
        // clearing G_QENA blocks MQ from loading GPIO data pins
        value |=   G_DENA;
        value &= ~ G_QENA;
    }

    // make sure we have access to gpio pins
    if (gpiopage == NULL) {
        opengpio ();
    }

    // maybe update gpio pin direction
    uint32_t mask = wdata ? G_OUTS | G_LINK | G_DATA : G_OUTS;
    if (wdata) {
        gpioreadflip = (G_REVIS & G_INS) | G_REVOS;     // on read, flip any permanent input pins that need flipping
                                                        //          flip any permanent or temporary output pins that need flipping
    } else {
        gpioreadflip = G_REVIS | (G_REVOS & G_OUTS);    // on read, flip any permanent or temporary input pins that need flipping
                                                        //          flip any permanent output pins that need flipping
    }

    // flip some bits going outbound
    value ^= G_REVOS;

    // send out the value
    gpiopage[1] = value;
    writecount ++;

    // verify that it got the value
    uint32_t readback = gpiopage[2];
    uint32_t diff = (readback ^ value) & mask;
    if (diff != 0) {
        fprintf (stderr, "ZynqLib::writegpio: wrote %08X mask %08X => %08X, readback %08X => %08X, diff %08X\n",
                    value, mask, value & mask, readback, readback & mask, diff);
        ABORT ();
    }
}

void ZynqLib::opengpio ()
{
    struct flock flockit;

    // access gpio page in physical memory
    // /proc/zynqgpio is a simple mapping of the single gpio page of zynq.vhd
    // it is created by loading the km-zynqgpio/zynqgpio.ko kernel module
    gpiopage = NULL;
    memfd = ::open ("/proc/zynqgpio", O_RDWR);
    if (memfd < 0) {
        fprintf (stderr, "ZynqLib::open: error opening /proc/zynqgpio: %m\n");
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
            fprintf (stderr, "ZynqLib::open: error locking /proc/zynqgpio: locked by pid %d\n", (int) flockit.l_pid);
            ABORT ();
        }
        fprintf (stderr, "ZynqLib::open: error locking /proc/zynqgpio: %m\n");
        ABORT ();
    }

    memptr = mmap (NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0);
    if (memptr == MAP_FAILED) {
        fprintf (stderr, "ZynqLib::open: error mmapping /proc/zynqgpio: %m\n");
        ABORT ();
    }

    // save pointer to gpio register page
    gpiopage = (uint32_t volatile *) memptr;
    fprintf (stderr, "ZynqLib::open: zynq fpga version %08X\n", gpiopage[3]);

    gpiopage[4] = boardena;
    uint32_t berb = gpiopage[4];
    std::string best;
    if (berb & 001) best.append ("acl ");
    if (berb & 002) best.append ("alu ");
    if (berb & 004) best.append ("ma ");
    if (berb & 010) best.append ("pc ");
    if (berb & 020) best.append ("rpi ");
    if (berb & 040) best.append ("seq ");
    fprintf (stderr, "ZynqLib::open: boardena %02o=%s\n", berb, best.c_str ());
    if (berb != boardena) {
        fprintf (stderr, "ZynqLib::open: boardena %02o read back as %02o\n", boardena, berb);
        ABORT ();
    }

    gpioreadflip = G_REVIS | (G_REVOS & G_OUTS);    // these pins get flipped on read to show active high
}

// we always have paddles
// physlib open-collectors everything so do equivalent here
bool ZynqLib::haspads ()
{
    if (gpiopage == NULL) {
        opengpio ();
    }
    gpiopage[12] = 0xFFFFFFFFU;
    gpiopage[13] = 0xFFFFFFFFU;
    gpiopage[14] = 0xFFFFFFFFU;
    gpiopage[15] = 0xFFFFFFFFU;
    return true;
}

// read abcd connector pins
//  output:
//    returns pins read from connectors
void ZynqLib::readpads (uint32_t *pinss)
{
    if (gpiopage == NULL) {
        opengpio ();
    }
    pinss[0] = gpiopage[ 8];
    pinss[1] = gpiopage[ 9];
    pinss[2] = gpiopage[10];
    pinss[3] = gpiopage[11];
}

// write abcd connector pins
//  input:
//    masks = which pins are output pins (others are inputs or not connected)
//    pinss = values for the pins listed in mask (others are don't care)
void ZynqLib::writepads (uint32_t const *masks, uint32_t const *pinss)
{
    if (gpiopage == NULL) {
        opengpio ();
    }
    gpiopage[12] = pinss[0] | ~ masks[0];
    gpiopage[13] = pinss[1] | ~ masks[1];
    gpiopage[14] = pinss[2] | ~ masks[2];
    gpiopage[15] = pinss[3] | ~ masks[3];
}
