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

// access physical cpu boards
// assumes raspberry pi gpio connector is plugged into rasboard
// also accesses the abcd connector paddles on a pc or raspi

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <poll.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#include "gpiolib.h"
#include "myiowkit.h"
#include "rdcyc.h"

#define IGO 1

#define GPIO_FSEL0 (0)          // set output current / disable (readwrite)
#define GPIO_SET0 (0x1C/4)      // set gpio output bits (writeonly)
#define GPIO_CLR0 (0x28/4)      // clear gpio output bits (writeonly)
#define GPIO_LEV0 (0x34/4)      // read gpio bit levels (readonly)
#define GPIO_PUDMOD (0x94/4)    // pullup/pulldown enable (0=disable; 1=enable pulldown; 2=enable pullup)
#define GPIO_PUDCLK (0x98/4)    // which pins to change pullup/pulldown mode

PhysLib::PhysLib ()
{
    sigemptyset (&sigintmask);
    sigaddset (&sigintmask, SIGINT);
    sigaddset (&sigintmask, SIGTERM);

    this->paddlesopen = false;
    this->gpioudpfd = -1;
    this->gpioudptm = 0;
    this->memfd  = -1;
    this->memptr = NULL;
    this->gpiopage = NULL;
}

PhysLib::~PhysLib ()
{
    this->close ();
}

void PhysLib::open ()
{
    TimedLib::open ();

    writecount  = 0;
    writecycles = 0;
}

void PhysLib::close ()
{
    if (writecount > 0) {
        fprintf (stderr, "PhysLib::close: writecount=%llu avgcyclesperwrite=%3.1f\n", (LLU) writecount, ((double) writecycles / (double) writecount));
    }

    ::close (gpioudpfd);
    gpioudpfd = -1;

    if (gpiopage != NULL) {
        gpiopage[GPIO_FSEL0+0] = 0;
        gpiopage[GPIO_FSEL0+1] = 0;
        gpiopage[GPIO_FSEL0+2] = 0;
        gpiopage = NULL;
    }

    munmap (memptr, 4096);
    ::close (memfd);
    memptr = NULL;
    memfd  = -1;

    if (paddlesopen) {
        paddlesopen = false;
        for (int i = 0; i < NPADS; i ++) {
            delete iowhandles[i];
            iowhandles[i] = NULL;
        }
    }
}

// returns all signals with active high orientation
uint32_t PhysLib::readgpio ()
{
    while (gpiopage == NULL) {
        if (gpioudpfd >= 0) return sendrecvudp (0);
        opengpio ();
    }

    uint32_t value = gpiopage[GPIO_LEV0];
    value ^= gpioreadflip;

#if 000
    printf ("PhysLib::readgpio*:  pins>%08X", value);
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
void PhysLib::writegpio (bool wdata, uint32_t value)
{
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
    while (gpiopage == NULL) {
        if (gpioudpfd >= 0) {
            ASSERT (value != 0);  // always have DENA or QENA
            sendrecvudp (value);
            return;
        }
        opengpio ();
    }

    // maybe update gpio pin direction
    uint32_t mask = wdata ? G_OUTS | G_LINK | G_DATA : G_OUTS;
    if ((gpiomask != mask) || (value & G_RESET)) {
        gpiomask = mask;
        ASSERT (G_OUTS == 0x3A000C);
        ASSERT ((G_LINK|G_DATA) == 0x001FFF0);
        if (wdata) {

            // about to write link,data bits
            gpiopage[GPIO_FSEL0+0] = IGO * 01111111100;     // enable data<05:00>,reset,clock outputs
            gpiopage[GPIO_FSEL0+1] = IGO * 01011111111;     // enable qena,ios,link,data<11:06> outputs
            gpioreadflip = (G_REVIS & G_INS) | G_REVOS;     // on read, flip any permanent input pins that need flipping
                                                            //          flip any permanent or temporary output pins that need flipping
        } else {

            // allow link,data bits to be read
            gpiopage[GPIO_FSEL0+0] = IGO * 00000001100;     // enable reset,clock outputs, data<05:00> are inputs
            gpiopage[GPIO_FSEL0+1] = IGO * 01010000000;     // enable qena,ioskip outputs, link,data<11:06> are inputs
            gpioreadflip = G_REVIS | (G_REVOS & G_OUTS);    // on read, flip any permanent or temporary input pins that need flipping
                                                            //          flip any permanent output pins that need flipping
        }
    }

    // update pins
    if (((value ^ gpiovalu) & mask) != 0) {
        gpiovalu = value;

#if 000
        printf ("PhysLib::writegpio*: pins<%08X", value);
        printf ("      ");
        printf ("        ");
        printf ("       ");
        printf ("       ");
        printf ("       ");
        printf (" QENA<%u",    (value / G_QENA)  & 1);
        printf (" DENA<%u",    (value / G_DENA)  & 1);
        printf (" IRQ<%u",     (value / G_IRQ)   & 1);
        printf (" IOS<%u",     (value / G_IOS)   & 1);
        if (wdata) {
            printf (" LINK<%u",    (value / G_LINK) & 1);
            printf (" DATA<%04o",  (value / G_DATA0) & 07777);
        } else {
            printf ("       ");
            printf ("          ");
        }
        printf (" RESET<%u",   (value / G_RESET) & 1);
        printf (" CLOCK<%u\n", (value / G_CLOCK) & 1);
#endif

        // flip some bits going outbound
        value ^= G_REVOS;

        // write gpio pins
        gpiopage[GPIO_CLR0] = ~ value & G_ALL;
        gpiopage[GPIO_SET0] =   value & G_ALL;

        writecount ++;

        // sometimes takes 1 or 2 retries to make sure signal gets out
        uint32_t retries = 0;
        while (true) {
            uint32_t readback = gpiopage[GPIO_LEV0];
            uint32_t diff = (readback ^ value) & mask;
            if (diff == 0) break;
            if (++ retries > 1000) {
                fprintf (stderr, "PhysLib::writegpio: wrote %08X mask %08X => %08X, readback %08X => %08X, diff %08X\n",
                            value, mask, value & mask, readback, readback & mask, diff);
                ABORT ();
            }
        }
        writecycles += retries + 1;
        if (lastretries < retries) {
            lastretries = retries;
            fprintf (stderr, "PhysLib::writegpio: retries %u\n", retries);
        }
    }
}

void PhysLib::opengpio ()
{
    char const *env = getenv ("physlib_gpioudpaddr");
    if ((env != NULL) && (env[0] != 0)) {
        opengpioudp (env);
        return;
    }

    struct flock flockit;

    // access gpio page in physical memory
    gpiopage = NULL;
    memfd = ::open ("/dev/gpiomem", O_RDWR | O_SYNC);
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

    memptr = mmap (NULL, 4096, PROT_READ | PROT_WRITE, MAP_SHARED, memfd, 0);
    if (memptr == MAP_FAILED) {
        fprintf (stderr, "PhysLib::open: error mmapping /dev/gpiomem: %m\n");
        ABORT ();
    }

    // save pointer to gpio register page
    gpiopage = (uint32_t volatile *) memptr;

    // https://www.raspberrypi.org/documentation/hardware/raspberrypi/gpio/gpio_pads_control.md
    // 111... means 2mA; 777... would be 14mA

    gpiomask = G_OUTS;                          // just drive output pins, not data pins, to start with
    gpioreadflip = G_REVIS | (G_REVOS & G_OUTS);  // these pins get flipped on read to show active high
    ASSERT (G_OUTS == 0x3A000C);
    gpiopage[GPIO_FSEL0+0] = IGO * 00000001100; // gpio<09:00> = data<05:00>, reset, clock, unused<1:0>
    gpiopage[GPIO_FSEL0+1] = IGO * 01010000000; // gpio<19:10> = qena, unused, ioskip, link, data<11:06>
    gpiopage[GPIO_FSEL0+2] = IGO * 00000000011; // gpio<27:20> = intak, write, read, dfrm, ioinst, jump, dena, intrq
    gpiovalu = 0;                               // make sure gpio pin state matches what we have cached
    gpiopage[GPIO_CLR0] = ~ G_REVOS & G_ALL;
    gpiopage[GPIO_SET0] =   G_REVOS & G_ALL;

    lastretries = 0;
}

// open udp connection to the server
void PhysLib::opengpioudp (char const *server)
{
    // set up server address and port number
    memset (&gpioudpsa, 0, sizeof gpioudpsa);
    gpioudpsa.sin_family = AF_INET;
    gpioudpsa.sin_port = htons (gpioudpport);
    if (!inet_aton (server, &gpioudpsa.sin_addr)) {
        struct hostent *he = gethostbyname (server);
        if (he == NULL) {
            fprintf (stderr, "PhysLib::opengpioudp: bad server address %s\n", server);
            ABORT ();
        }
        if ((he->h_addrtype != AF_INET) || (he->h_length != 4)) {
            fprintf (stderr, "PhysLib::opengpioudp: server %s not IP v4\n", server);
            ABORT ();
        }
        gpioudpsa.sin_addr = *(struct in_addr *)he->h_addr;
    }

    // set up send/recv socket and receive timeout
    gpioudpfd = socket (AF_INET, SOCK_DGRAM, 0);
    if (gpioudpfd < 0) ABORT ();

    struct sockaddr_in cliaddr;
    memset (&cliaddr, 0, sizeof cliaddr);
    cliaddr.sin_family = AF_INET;
    if (bind (gpioudpfd, (struct sockaddr *) &cliaddr, sizeof cliaddr) < 0) {
        fprintf (stderr, "PhysLib::opengpioudp: error binding: %m\n");
        ABORT ();
    }

    // set up sequence number gt anything previously used
    struct timespec nowts;
    if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) ABORT ();
    lastudpseq = nowts.tv_sec * 1000000000ULL + nowts.tv_nsec;

    char const *env = getenv ("physlib_gpioudpmsec");
    struct timeval rcvtotv;
    memset (&rcvtotv, 0, sizeof rcvtotv);
    gpioudptm = (env == NULL) ? 1000 : atoi (env);

    fprintf (stderr, "PhysLib::opengpioudp: using %s:%d timeout %ums\n",
        inet_ntoa (gpioudpsa.sin_addr), ntohs (gpioudpsa.sin_port), gpioudptm);
}

// read gpio value from the server
uint32_t PhysLib::sendrecvudp (uint32_t value)
{
    UDPMsg sendbuf;
    memset (&sendbuf, 0, sizeof sendbuf);
    sendbuf.seq = ++ lastudpseq;
    sendbuf.val = value;

    struct pollfd polls[] = { gpioudpfd, POLLIN, 0 };

    while (true) {
        int rc = sendto (gpioudpfd, &sendbuf, sizeof sendbuf, 0, (struct sockaddr *) &gpioudpsa, sizeof gpioudpsa);
        if (rc < (int) sizeof sendbuf) {
            if (rc < 0) {
                fprintf (stderr, "PhysLib::sendrecvudp: error writing to server: %m\n");
            } else {
                fprintf (stderr, "PhysLib::sendrecvudp: only wrote %d bytes of %d\n", rc, (int) sizeof sendbuf);
            }
            ABORT ();
        }

        while (true) {

            // wait up to gpioudptm for a response
            do rc = poll (polls, 1, gpioudptm);
            while ((rc < 0) && (errno == EINTR));
            if (rc < 0) {
                fprintf (stderr, "PhysLib::sendrecvudp: poll error: %m\n");
                ABORT ();
            }

            // if timed out, resend request
            if (rc == 0) break;

            // read in reply
            struct sockaddr_in recvadd;
            socklen_t recvlen = sizeof recvadd;
            UDPMsg recvbuf;
            rc = recvfrom (gpioudpfd, &recvbuf, sizeof recvbuf, 0, (struct sockaddr *) &recvadd, &recvlen);
            if (rc != sizeof recvbuf) {
                if (rc < 0) {
                    fprintf (stderr, "PhysLib::sendrecvudp: error reading from server: %m\n");
                } else {
                    fprintf (stderr, "PhysLib::sendrecvudp: only read %d bytes of %d\n", rc, (int) sizeof recvbuf);
                }
                ABORT ();
            }

            // if reply to this request, return the reply value
            if (recvbuf.seq == lastudpseq) {
                return recvbuf.val;
            }

            // it can't be a reply to a future request!
            if (recvbuf.seq > lastudpseq) {
                fprintf (stderr, "PhysLib::sendrecvudp: sent seq %llu rcvd seq %llu\n",
                    (unsigned long long) lastudpseq, (unsigned long long) recvbuf.seq);
                ABORT ();
            }

            // reply to an old request, ignore and read again
        }
    }
}

// -------------------------------------------------------------------------- //

// ask io-warrior-56 for state of all pins
static IOWKIT56_SPECIAL_REPORT const reqallpins56[] = { 255 };

// mapping of 32 abcd connector pins to io-warrior-56 input port pins
static uint8_t const inputpinmap56[] = {
        031,  // abcd pin 01 -> iow56 P3.1 -> retval bit 00
        055,  // abcd pin 02 -> iow56 P5.5 -> retval bit 01
        051,  // abcd pin 03 -> iow56 P5.1 -> retval bit 02
        013,  // abcd pin 04 -> iow56 P1.3 -> retval bit 03
        025,  // abcd pin 05 -> iow56 P2.5 -> retval bit 04
        021,  // abcd pin 06 -> iow56 P2.1 -> retval bit 05
        045,  // abcd pin 07 -> iow56 P4.5 -> retval bit 06
        041,  // abcd pin 08 -> iow56 P4.1 -> retval bit 07
        023,  // abcd pin 09 -> iow56 P2.3 -> retval bit 08
        006,  // abcd pin 10 -> iow56 P0.6 -> retval bit 09
        005,  // abcd pin 11 -> iow56 P0.5 -> retval bit 10
        001,  // abcd pin 12 -> iow56 P0.1 -> retval bit 11
        053,  // abcd pin 13 -> iow56 P5.3 -> retval bit 12
        057,  // abcd pin 14 -> iow56 P5.7 -> retval bit 13
        033,  // abcd pin 15 -> iow56 P3.3 -> retval bit 14
        037,  // abcd pin 16 -> iow56 P3.7 -> retval bit 15

        056,  // abcd pin 17 -> iow56 P5.6 -> retval bit 16
        052,  // abcd pin 18 -> iow56 P5.2 -> retval bit 17
        016,  // abcd pin 19 -> iow56 P1.6 -> retval bit 18
        012,  // abcd pin 20 -> iow56 P1.2 -> retval bit 19
        042,  // abcd pin 21 -> iow56 P4.2 -> retval bit 20
        036,  // abcd pin 22 -> iow56 P3.6 -> retval bit 21
        000,  // abcd pin 23 -> iow56 P0.0 -> retval bit 22
        024,  // abcd pin 24 -> iow56 P2.4 -> retval bit 23
        034,  // abcd pin 25 -> iow56 P3.4 -> retval bit 24
        040,  // abcd pin 26 -> iow56 P4.0 -> retval bit 25
        044,  // abcd pin 27 -> iow56 P4.4 -> retval bit 26
        020,  // abcd pin 28 -> iow56 P2.0 -> retval bit 27
        067,  // abcd pin 29 -> iow56 P6.7 -> retval bit 28
        014,  // abcd pin 30 -> iow56 P1.4 -> retval bit 29
        050,  // abcd pin 31 -> iow56 P5.0 -> retval bit 30
        030   // abcd pin 32 -> iow56 P3.0 -> retval bit 31
};

// these iow56 pins clock the 8-bit output latches
#define LOAD1 035  // P3.5  =>  LAT 05,06,07,08,04,03,02,01
#define LOAD2 043  // P4.3  =>  LAT 13,14,15,16,12,11,10,09
#define LOAD3 004  // P0.4  =>  LAT 24,23,22,21,17,18,19,20
#define LOAD4 054  // P5.4  =>  LAT 32,31,30,29,25,26,27,28
static uint8_t const loadpinmap56[] = { LOAD1, LOAD2, LOAD3, LOAD4 };

// these iow56 pins present data to the 8-bit output latches

#define OUT1 022    // P2.2
#define OUT2 046    // P4.6
#define OUT3 002    // P0.2
#define OUT4 026    // P2.6
#define OUT5 047    // P4.7
#define OUT6 007    // P0.7
#define OUT7 027    // P2.7
#define OUT8 003    // P0.3

static uint8_t const outputpinmap56[] {

            // LOAD1
        OUT1,   // bit 00 -> LAT01
        OUT2,   // bit 01 -> LAT02
        OUT3,   // bit 02 -> LAT03
        OUT4,   // bit 03 -> LAT04
        OUT8,   // bit 04 -> LAT05
        OUT7,   // bin 05 -> LAT06
        OUT6,   // bin 06 -> LAT07
        OUT5,   // bin 07 -> LAT08

            // LOAD2
        OUT1,   // bit 08 -> LAT09
        OUT2,   // bit 09 -> LAT10
        OUT3,   // bit 10 -> LAT11
        OUT4,   // bit 11 -> LAT12
        OUT8,   // bit 12 -> LAT13
        OUT7,   // bit 13 -> LAT14
        OUT6,   // bit 14 -> LAT15
        OUT5,   // bit 15 -> LAT16

            // LOAD3
        OUT4,   // bin 16 -> LAT17
        OUT3,   // bin 17 -> LAT18
        OUT2,   // bin 18 -> LAT19
        OUT1,   // bin 19 -> LAT20
        OUT5,   // bin 20 -> LAT21
        OUT6,   // bin 21 -> LAT22
        OUT7,   // bin 22 -> LAT23
        OUT8,   // bin 23 -> LAT24

            // LOAD4
        OUT4,   // bin 24 -> LAT25
        OUT3,   // bin 25 -> LAT26
        OUT2,   // bin 26 -> LAT27
        OUT1,   // bin 27 -> LAT28
        OUT5,   // bin 28 -> LAT29
        OUT6,   // bin 29 -> LAT30
        OUT7,   // bin 30 -> LAT31
        OUT8    // bin 31 -> LAT32
};

// mask of all iow56 output pins = LOAD1..4 + OUT1..8
static uint64_t const vfyoutmask56 = (1ULL << LOAD1) | (1ULL << LOAD2) | (1ULL << LOAD3) | (1ULL << LOAD4) |
    (1ULL << OUT1) | (1ULL << OUT2) | (1ULL << OUT3) | (1ULL << OUT4) | (1ULL << OUT5) | (1ULL << OUT6) | (1ULL << OUT7) | (1ULL << OUT8);



// ask io-warrior-100 for state of all pins
static IOWKIT100_SPECIAL_REPORT const reqallpins100[] = { 255 };

// these iow100 pins get bits from 2N7000 input transistors
static uint8_t const inputpinmap100[32] = {
        0026,    // IN  01 => P2.6
        0024,    // IN  02 => P2.4
        0054,    // IN  03 => P5.4
        0065,    // IN  04 => P6.5
        0041,    // IN  05 => P4.1
        0043,    // IN  06 => P4.3
        0104,    // IN  07 => P8.4
        0056,    // IN  08 => P5.6
        0023,    // IN  09 => P2.3
        0025,    // IN  10 => P2.5
        0055,    // IN  11 => P5.5
        0031,    // IN  12 => P3.1
        0047,    // IN  13 => P4.7
        0051,    // IN  14 => P5.1
        0062,    // IN  15 => P6.2
        0011,    // IN  16 => P1.1

        0035,    // IN  17 => P3.5
        0071,    // IN  18 => P7.1
        0075,    // IN  19 => P7.5
        0107,    // IN  20 => P8.7
        0003,    // IN  21 => P0.3
        0007,    // IN  22 => P0.7
        0021,    // IN  23 => P2.1
        0002,    // IN  24 => P0.2
        0032,    // IN  25 => P3.2
        0020,    // IN  26 => P2.0
        0006,    // IN  27 => P0.6
        0036,    // IN  28 => P3.6
        0050,    // IN  29 => P5.0
        0076,    // IN  30 => P7.6
        0072,    // IN  31 => P7.2
        0012     // IN  32 => P1.2
};

// these iow100 pins send bits to the 2N3904 output transistors
static uint8_t const outputpinmap100[32] = {
        052,    // P5.2 => OUT 01
        061,    // P6.1 => OUT 02
        067,    // P6.7 => OUT 03
        063,    // P6.3 => OUT 04
        030,    // P3.0 => OUT 05
        016,    // P8.6 => OUT 06
        042,    // P4.2 => OUT 07
        040,    // P4.0 => OUT 08
        057,    // P5.7 => OUT 09
        015,    // P8.5 => OUT 10
        027,    // P2.7 => OUT 11
        013,    // P8.3 => OUT 12
        066,    // P6.6 => OUT 13
        064,    // P6.4 => OUT 14
        053,    // P5.3 => OUT 15
        060,    // P6.0 => OUT 16

        037,    // P3.7 => OUT 17
        033,    // P3.3 => OUT 18
        073,    // P7.3 => OUT 19
        077,    // P7.7 => OUT 20
        001,    // P0.1 => OUT 21
        000,    // P0.0 => OUT 22
        005,    // P0.5 => OUT 23
        045,    // P4.5 => OUT 24
        022,    // P2.2 => OUT 25
        034,    // P3.4 => OUT 26
        044,    // P4.4 => OUT 27
        004,    // P0.4 => OUT 28
        046,    // P4.6 => OUT 29
        010,    // P1.0 => OUT 30
        074,    // P7.4 => OUT 31
        070     // P7.0 => OUT 32
};

// mask of all iow100 output pins
static uint64_t const vfyoutmask100 =
        (1ULL << 052) |    // OUT 01
        (1ULL << 061) |    // OUT 02
        (1ULL << 067) |    // OUT 03
        (1ULL << 063) |    // OUT 04
        (1ULL << 030) |    // OUT 05
        (1ULL << 016) |    // OUT 06
        (1ULL << 042) |    // OUT 07
        (1ULL << 040) |    // OUT 08
        (1ULL << 057) |    // OUT 09
        (1ULL << 015) |    // OUT 10
        (1ULL << 027) |    // OUT 11
        (1ULL << 013) |    // OUT 12
        (1ULL << 066) |    // OUT 13
        (1ULL << 064) |    // OUT 14
        (1ULL << 053) |    // OUT 15
        (1ULL << 060) |    // OUT 16

        (1ULL << 037) |    // OUT 17
        (1ULL << 033) |    // OUT 18
        (1ULL << 073) |    // OUT 19
        (1ULL << 077) |    // OUT 20
        (1ULL << 001) |    // OUT 21
        (1ULL << 000) |    // OUT 22
        (1ULL << 005) |    // OUT 23
        (1ULL << 045) |    // OUT 24
        (1ULL << 022) |    // OUT 25
        (1ULL << 034) |    // OUT 26
        (1ULL << 044) |    // OUT 27
        (1ULL << 004) |    // OUT 28
        (1ULL << 046) |    // OUT 29
        (1ULL << 010) |    // OUT 30
        (1ULL << 074) |    // OUT 31
        (1ULL << 070);     // OUT 32

// paddles aren't going to be used, make sure they are all floating
// but maybe they don't exist
void PhysLib::floatpaddles ()
{
    blocksigint ();
    IowKit::list (floateach, NULL);
    allowsigint ();
}
void PhysLib::floateach (void *ctx, char const *dn, int pipe, char const *sn, int pid, int rev)
{
    if (pipe == 0) {
        IowKit iow;

        iow.openbysn (sn);

        // initialize to open-drain all 32 pins
        switch (pid) {

            case IOWKIT_PRODUCT_ID_IOW56: {

                // set up to write zeroes to all the latches so the 2N7000s will be open-drained
                write56 (&iow, ~ vfyoutmask56);   // all LOADs = 0, all OUTs = 0, all other pins = 1

                // clock the LOADs to 1 to kerchunk zeroes into all the latches which will open-drain all the output 2N7000s
                write56 (&iow, ~ vfyoutmask56 | (1ULL << LOAD1) | (1ULL << LOAD2) | (1ULL << LOAD3) | (1ULL << LOAD4));
                break;
            }

            case IOWKIT_PRODUCT_ID_IOW100: {

                // write zeroes to open-drain all the output 2N7000s
                // write ones to all input pins to open drain the iow inputs
                write100 (&iow, ~ vfyoutmask100);
                break;
            }
        }
    }
}

// read abcd connector pins
//  output:
//    returns pins read from connectors
void PhysLib::readpads (uint32_t *pinss)
{
    openpaddles ();

    // iows go nuts if the io calls are interrupted
    blocksigint ();

    // tell each iowarrior to send its pins
    for (int pad = 0; pad < NPADS; pad ++) {
        IOWKIT_HANDLE iowh = iowhandles[pad];
        switch (iowh->getpid ()) {
            case IOWKIT_PRODUCT_ID_IOW56: {
                iowh->write (IOW_PIPE_SPECIAL_MODE, &reqallpins56, sizeof reqallpins56);
                break;
            }
            case IOWKIT_PRODUCT_ID_IOW100: {
                iowh->write (IOW_PIPE_SPECIAL_MODE, &reqallpins100, sizeof reqallpins100);
                break;
            }
            default: ABORT ();
        }
    }

    // read then unscramble pins from each iowarrior
    for (int pad = 0; pad < NPADS; pad ++) {
        IOWKIT_HANDLE iowh = iowhandles[pad];
        IOWKIT56_SPECIAL_REPORT rbk56pins;
        IOWKIT100_SPECIAL_REPORT rbk100pins;
        uint8_t const *map, *pins;
        switch (iowh->getpid ()) {
            case IOWKIT_PRODUCT_ID_IOW56: {
                memset (&rbk56pins, 0, sizeof rbk56pins);
                iowh->read (IOW_PIPE_SPECIAL_MODE, &rbk56pins, sizeof rbk56pins);
                if (rbk56pins.ReportID != 255) {
                    fprintf (stderr, "PhysLib::readpads: got report id %02X reading iow56 pins\n", rbk56pins.ReportID);
                    ABORT ();
                }
                map  = inputpinmap56;
                pins = rbk56pins.Bytes;
                break;
            }
            case IOWKIT_PRODUCT_ID_IOW100: {
                memset (&rbk100pins, 0, sizeof rbk100pins);
                iowh->read (IOW_PIPE_SPECIAL_MODE, &rbk100pins, sizeof rbk100pins);
                if (rbk100pins.ReportID != 255) {
                    fprintf (stderr, "PhysLib::readpads: got report id %02X reading iow100 pins\n", rbk100pins.ReportID);
                    ABORT ();
                }
                map  = inputpinmap100;
                pins = rbk100pins.Bytes;
                break;
            }
            default: ABORT ();
        }

        uint32_t val = 0;
        for (int j = 32; -- j >= 0;) {
            uint8_t p = map[j];
            val += val + ((pins[p/8] >> (p % 8)) & 1);
        }
        pinss[pad] = ~ val;
    }

    // signals ok now
    allowsigint ();
}

// write abcd connector pins by loading the 4 8-bit latches (actually DFFs)
//  input:
//    masks = which pins are output pins (others are inputs or not connected)
//    pinss = values for the pins listed in mask (others are don't care)
void PhysLib::writepads (uint32_t const *masks, uint32_t const *pinss)
{
    openpaddles ();

    // iows go nuts if the io calls are interrupted
    blocksigint ();

    // write output pins of the iow100s
    for (int pad = 0; pad < NPADS; pad ++) {
        if (iowhandles[pad]->getpid () != IOWKIT_PRODUCT_ID_IOW100) continue;

        uint32_t mask = masks[pad];
        uint32_t pins = pinss[pad];

        // set up to write ones to all iow100 pins so they are open-drained
        uint64_t updallpins = -1ULL;

        // for any abcd pin not in the mask, clear the iow100 out pin so the 2N3904 will be open-collectored
        // for any abcd pin in the mask that is set, also clear the iow100 out pin to open-collector the 2N3904
        // for any abcd pin in the mask that is clear, leave the out pin set so the 2N3904 grounds that output
        for (int j = 0; j < 32; j ++) {
            if (((~ mask | pins) >> j) & 1) {
                uint8_t p = outputpinmap100[j];
                updallpins &= ~ (1ULL << p);
            }
        }

        IOWKIT_HANDLE iowh = iowhandles[pad];
        write100 (iowh, updallpins);
    }

    // write each of the 4 8-bit latches of the iow56s
    for (int lat = 0; lat < 4; lat ++) {
        int loadp = loadpinmap56[lat];

        // for this 8-bit latch, compute iow56 pins for all 4 paddles with clock low
        bool havea56 = false;
        uint64_t updallpinss[NPADS];
        for (int pad = 0; pad < NPADS; pad ++) {
            if (iowhandles[pad]->getpid () != IOWKIT_PRODUCT_ID_IOW56) continue;
            havea56 = true;

            uint32_t mask = masks[pad];
            uint32_t pins = pinss[pad];

            // at this point, we can assume all iow56 LOAD output pins are high

            // set up to write ones to all iow56 pins so they are open-drained
            // except set this LOAD<lat> output pin to zero
            uint64_t updallpins = ~ (1ULL << loadp);

            // for any abcd pin not in the mask, clear the iow56 OUT<n> pin so the 2N7000 will be open-drained
            // for any abcd pin in the mask that is set, also clear the iow56 OUT<n> pin to open-drain the 2N7000
            // for any abcd pin in the mask that is clear, leave the OUT<n> pin set so the 2N7000 grounds that output
            int j0 = lat * 8;
            int j8 = j0 + 8;
            for (int j = j0; j < j8; j ++) {
                if (((~ mask | pins) >> j) & 1) {
                    uint8_t p = outputpinmap56[j];
                    updallpins &= ~ (1ULL << p);
                }
            }

            updallpinss[pad] = updallpins;
        }
        if (! havea56) break;

        // first write out all 4 paddles with clock low then with clock high
        for (int clk = 0; clk < 2; clk ++) {

            for (int pad = 0; pad < NPADS; pad ++) {
                if (iowhandles[pad]->getpid () != IOWKIT_PRODUCT_ID_IOW56) continue;

                uint64_t updallpins = updallpinss[pad];
                IOWKIT_HANDLE iowh = iowhandles[pad];

                // send write command to IOW-56 paddle with LOAD<lat> low first time, high second time
                write56 (iowh, updallpins);
            }

            if (clk != 0) break;

            // have to wait for it to soak thru iow56s, they're s-l-o-w
            delayiow ();

            // same write command but with LOAD<lat> high to kerchunk the 8 bits into latch
            for (int pad = 0; pad < NPADS; pad ++) {
                updallpinss[pad] |= 1ULL << loadp;
            }
        }
    }

    // signals ok now
    allowsigint ();

    // now verify the 32-pin abcd connector output pins match what we wrote out
    // do a couple retries to give signals time to flow through the slow iowarriors
    // usually verifies first time through
    for (int retry = -3;; retry ++) {
        uint32_t verifies[NPADS];
        readpads (verifies);
        bool bad = false;
        for (int pad = 0; pad < NPADS; pad ++) {
            uint32_t verify = verifies[pad];
            uint32_t pins   = pinss[pad];
            uint32_t mask   = masks[pad];
            if (((verify ^ pins) & mask) != 0) {
                bad = true;
                if (retry >= 0) fprintf (stderr, "PhysLib::writepads: verify %c error, wrote %08X readback %08X\n", 'A' + pad, pins & mask, verify & mask);
            }
        }
        if (! bad) break;
        if (retry >= 0) ABORT ();
    }
}

// read 32 input pins
uint32_t PhysLib::read32 (int pad)
{
    if ((pad < 0) && (pad >= NPADS)) ABORT ();

    openpaddles ();

    IOWKIT_HANDLE iowh = iowhandles[pad];

    // iows go nuts if the io calls are interrupted
    blocksigint ();

    // read then unscramble pins
    uint32_t pins = 0;
    switch (iowh->getpid ()) {
        case IOWKIT_PRODUCT_ID_IOW56: {

            // read 56 pins from iow56
            uint64_t allpins = read56 (iowh);

            // gather up the abcd bits from the iow56 pins
            for (int j = 32; -- j >= 0;) {
                uint8_t p = inputpinmap56[j];
                pins += pins + ((allpins >> p) & 1);
            }
            break;
        }

        case IOWKIT_PRODUCT_ID_IOW100: {

            // read 100 pins from iow100
            uint64_t allpins = read100 (iowh);

            // gather up the abcd bits from the iow100 pins
            for (int j = 32; -- j >= 0;) {
                uint8_t p = inputpinmap100[j];
                if (p >= 0100) p -= 0070;
                pins += pins + ((allpins >> p) & 1);
            }
            break;
        }

        default: ABORT ();
    }

    // signals ok now
    allowsigint ();

    // they go through 2N7000s which invert the value
    return ~ pins;
}

// write a single paddle
void PhysLib::write32 (int pad, uint32_t mask, uint32_t pins)
{
    if ((pad < 0) || (pad >= NPADS)) ABORT ();

    openpaddles ();

    // iows go nuts if the io calls are interrupted
    blocksigint ();

    IOWKIT_HANDLE iowh = iowhandles[pad];

    switch (iowh->getpid ()) {
        case IOWKIT_PRODUCT_ID_IOW100: {
            // read current IOW100 pins, fill in 1s for the IOW100 input pins to keep them open-drained
            uint64_t updallpins = read100 (iowh) | ~ vfyoutmask100;
            // loop through the passed-in bits
            for (int j = 0; j < 32; j ++) {
                // if mask not set, leave bit as is
                if ((~ mask >> j) & 1) continue;
                // set IOW100 bit to complement of passed-in pin cuz it goes through a 2N3904
                uint64_t p = 1ULL << outputpinmap100[j];
                updallpins &= ~ p;
                if ((~ pins >> j) & 1) {
                    updallpins |= p;
                }
            }
            write100 (iowh, updallpins);
            delayiow ();
            break;
        }

        default: ABORT ();
    }

    // signals ok now
    allowsigint ();

    // make sure latch written, made it to 32-pin connector
    uint32_t verify = read32 (pad);
    if ((verify ^ pins) & mask) {
        fprintf (stderr, "PhysLib::write32: verify error on paddle %c, wrote %08X mask %08X readback %08X, diff %08X\n",
            'A' + pad, pins, mask, verify, (verify ^ pins) & mask);
        ABORT ();
    }
}

// open io-warrior-56/100 paddles plugged into a,b,c,d connectors
// set all the outputs to 1s to open the open drain outputs so we can read the pins
void PhysLib::openpaddles ()
{
    if (! paddlesopen) {
        paddlesopen = true;

        memset (iowhandles, 0, sizeof iowhandles);
        blocksigint ();
        for (int i = 0; i < NPADS; i ++) {
            char env[12];
            sprintf (env, "iowsn_%c", 'a' + i);
            char const *esn = getenv (env);
            if (esn == NULL) {
                fprintf (stderr, "PhysLib::openpaddles: missing envar %s\n", env);
                ABORT ();
            }
            IOWKIT_HANDLE iowh = new IowKit ();
            if (! iowh->openbysn (esn)) {
                fprintf (stderr, "PhysLib::openpaddles: missing device %s sn %s\n", env, esn);
                ABORT ();
            }
            iowhandles[i] = iowh;

            // get type so we know how to access it
            int prodid = iowh->getpid ();

            // initialize to open-drain all 32 pins
            switch (prodid) {

                case IOWKIT_PRODUCT_ID_IOW56: {

                    // set up to write zeroes to all the latches so the 2N7000s will be open-drained
                    write56 (iowh, ~ vfyoutmask56);   // all LOADs = 0, all OUTs = 0, all other pins = 1

                    // clock the LOADs to 1 to kerchunk zeroes into all the latches which will open-drain all the output 2N7000s
                    write56 (iowh, ~ vfyoutmask56 | (1ULL << LOAD1) | (1ULL << LOAD2) | (1ULL << LOAD3) | (1ULL << LOAD4));
                    break;
                }

                case IOWKIT_PRODUCT_ID_IOW100: {

                    // write zeroes to open-drain all the output 2N7000s
                    // write ones to all input pins to open drain the iow inputs
                    write100 (iowh, ~ vfyoutmask100);
                    break;
                }

                default: {
                    fprintf (stderr, "PhysLib::openpaddles: io-warrior %s sn %s is not an io-warrior-56 or io-warrior-100 but is product-id 0x%X\n", env, esn, prodid);
                    ABORT ();
                }
            }
        }
        allowsigint ();
    }
}

// write all 56 pins of the iowarrior-56
void PhysLib::write56 (IOWKIT_HANDLE iowh, uint64_t allpins)
{
#if 000
    int pad;
    for (pad = 0; pad < NPADS; pad ++) if (iowhandles[pad] == iowh) break;
    uint8_t latchbyte =
        ((allpins >> (OUT1 - 0)) & 0x01) |
        ((allpins >> (OUT2 - 1)) & 0x02) |
        ((allpins >> (OUT3 - 2)) & 0x04) |
        ((allpins >> (OUT4 - 3)) & 0x08) |
        ((allpins >> (OUT5 - 4)) & 0x10) |
        ((allpins >> (OUT6 - 5)) & 0x20) |
        ((allpins >> (OUT7 - 6)) & 0x40) |
        ((allpins << (7 - OUT8)) & 0x80);
    fprintf (stderr, "PhysLib::write56*: %c LOAD1=%d LOAD2=%d LOAD3=%d LOAD4=%d  OUT<8..1>=%02X\n",
        'A' + pad, (int)(allpins >> LOAD1) & 1, (int)(allpins >> LOAD2) & 1,
        (int)(allpins >> LOAD3) & 1, (int)(allpins >> LOAD4) & 1, latchbyte);
#endif

    // only allow load and out pins to be zero
    // all input pins must be 1s to be able to read
    if ((allpins & ~ vfyoutmask56) != ~ vfyoutmask56) ABORT ();

    // split up into bytes for iow56
    IOWKIT56_IO_REPORT writepins;
    memset (&writepins, 0, sizeof writepins);
    for (int i = 0; i < 7; i ++) {
        writepins.Bytes[i] = allpins;
        allpins >>= 8;
    }

    // send to iow56 for writing
    iowh->write (IOW_PIPE_IO_PINS, &writepins, sizeof writepins);
}

// read all 56 pins of the iowarrior-56
uint64_t PhysLib::read56 (IOWKIT_HANDLE iowh)
{
    // tell iow56 to send back what it has on the pins
    iowh->write (IOW_PIPE_SPECIAL_MODE, &reqallpins56, sizeof reqallpins56);

    // get state of all the iow56 pins
    IOWKIT56_SPECIAL_REPORT rbkallpins;
    memset (&rbkallpins, 0, sizeof rbkallpins);
    iowh->read (IOW_PIPE_SPECIAL_MODE, &rbkallpins, sizeof rbkallpins);
    if (rbkallpins.ReportID != 255) {
        fprintf (stderr, "PhysLib::read56: got report id %02X reading iow56 pins\n", rbkallpins.ReportID);
        ABORT ();
    }

    uint64_t allpins = 0;
    for (int i = 7; -- i >= 0;) {
        allpins = (allpins << 8) | (rbkallpins.Bytes[i] & 0xFFU);
    }
    return allpins;
}

// write all 100 pins of the iowarrior-100
void PhysLib::write100 (IOWKIT_HANDLE iowh, uint64_t allpins)
{
    // only allow out pins to be zero
    // all input pins must be 1s to be able to read
    if ((allpins & ~ vfyoutmask100) != ~ vfyoutmask100) ABORT ();

    // split up into bytes for iow100
    IOWKIT100_IO_REPORT writepins;
    memset (&writepins, 0, sizeof writepins);
    for (int i = 0; i < 8; i ++) {
        writepins.Bytes[i] = allpins;
        allpins >>= 8;
    }

    // what caller thinks are pins P1.3..P1.7 are really P8.3..P8.7
    writepins.Bytes[8] = writepins.Bytes[1] | 0x07;
    writepins.Bytes[1] = writepins.Bytes[1] | 0xF8;

    // open-drain all P9..P11 pins
    writepins.Bytes[ 9] = 0xFF;
    writepins.Bytes[10] = 0xFF;
    writepins.Bytes[11] = 0xFF;

    // send to iow100 for writing
    iowh->write (IOW_PIPE_IO_PINS, &writepins, sizeof writepins);
}

// read all 100 pins of the iowarrior-100
uint64_t PhysLib::read100 (IOWKIT_HANDLE iowh)
{
    // tell iow100 to send back what it has on the pins
    iowh->write (IOW_PIPE_SPECIAL_MODE, &reqallpins100, sizeof reqallpins100);

    // get state of all the iow100 pins
    IOWKIT100_SPECIAL_REPORT rbkallpins;
    memset (&rbkallpins, 0, sizeof rbkallpins);
    iowh->read (IOW_PIPE_SPECIAL_MODE, &rbkallpins, sizeof rbkallpins);
    if (rbkallpins.ReportID != 255) {
        fprintf (stderr, "PhysLib::read100: got report id %02X reading iow100 pins\n", rbkallpins.ReportID);
        ABORT ();
    }

    // what caller thinks are pins P1.3..P1.7 are really P8.3..P8.7
    rbkallpins.Bytes[1] = (rbkallpins.Bytes[1] & 0x07) | (rbkallpins.Bytes[8] & 0xF8);

    uint64_t allpins = 0;
    for (int i = 8; -- i >= 0;) {
        allpins = (allpins << 8) | (rbkallpins.Bytes[i] & 0xFFU);
    }
    return allpins;
}

// delay for 3ms
void PhysLib::delayiow ()
{
    struct timespec delayabs;
    if (clock_gettime (CLOCK_MONOTONIC, &delayabs) < 0) ABORT ();
    delayabs.tv_nsec += 3000000;
    if (delayabs.tv_nsec >= 1000000000) {
        delayabs.tv_nsec -= 1000000000;
        delayabs.tv_sec ++;
    }
    int rc;
    do rc = clock_nanosleep (CLOCK_MONOTONIC, TIMER_ABSTIME, &delayabs, NULL);
    while ((rc < 0) && (errno == EINTR));
    if (rc < 0) ABORT ();
}

// the iowarrior chips get really bent out of shape if the access functions are aborted
// so block signals while they are running
void PhysLib::blocksigint ()
{
    if (sigprocmask (SIG_BLOCK, &sigintmask, NULL) < 0) ABORT ();
}

void PhysLib::allowsigint ()
{
    if (sigprocmask (SIG_UNBLOCK, &sigintmask, NULL) < 0) ABORT ();
}
