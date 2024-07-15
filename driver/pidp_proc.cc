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

// operate pidp panel via gpio connector or udp connection to raspi plugged into pidp panel

// if using remote pidp panel,
//  on remote raspi: ./pidpudpserver
//  on client:  export pidpudpaddr=<hostname of remote raspi>

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include "controls.h"
#include "gpiolib.h"
#include "miscdefs.h"
#include "pidp.h"

// gpio pins

#define COLMASK 0xFFF0

#define SWROW1  16
#define SWROW2  17
#define SWROW3  18

#define LEDROW1 20
#define LEDROW2 21
#define LEDROW3 22
#define LEDROW4 23
#define LEDROW5 24
#define LEDROW6 25
#define LEDROW7 26
#define LEDROW8 27

// directions

// - https://www.raspberrypi.org/documentation/hardware/raspberrypi/gpio/gpio_pads_control.md
//   111... means 2mA; 777... would be 14mA

#define IGO 1

// - when writing LEDs
//   all switch rows set to inputs (hi-Z)
//   led row select outputs are active high (just one at a time)
//   column outputs are active low

#define LEDSEL0 (IGO*01111110000)   // GPIO[09:00]
#define LEDSEL1 (IGO*00000111111)   // GPIO[19:10]
#define LEDSEL2 (IGO*00011111111)   // GPIO[29:20]

// - when reading switches
//   switch row select outputs are active low (just one at a time)
//   column inputs are active low

#define SWSEL0  (IGO*00000000000)   // GPIO[09:00]
#define SWSEL1  (IGO*00111000000)   // GPIO[19:10]
#define SWSEL2  (IGO*00011111111)   // GPIO[29:20]

#define GPIO_FSEL0 (0)          // set output current / disable (readwrite)
#define GPIO_SET0 (0x1C/4)      // set gpio output bits (writeonly)
#define GPIO_CLR0 (0x28/4)      // clear gpio output bits (writeonly)
#define GPIO_LEV0 (0x34/4)      // read gpio bit levels (readonly)
#define GPIO_PUDMOD (0x94/4)    // pullup/pulldown enable (0=disable; 1=enable pulldown; 2=enable pullup)
#define GPIO_PUDCLK (0x98/4)    // which pins to change pullup/pulldown mode

static GpioFile gpiofile;
static int gpioudpfd = -1;
static int gpioudptm;
static struct sockaddr_in gpioudpsa;
static uint32_t timeoutctr;
static uint32_t volatile *gpiopage;

static void writegpio (uint32_t mask, uint32_t data);

// send LEDs to PiDP panel, read switches from PiDP panel
bool pidp_proc (PiDPMsg *pidpmsg)
{
    if ((gpiopage == NULL) && (gpioudpfd < 0)) {
        char const *server = getenv ("pidpudpaddr");
        if (server != NULL) {

            // set up server address and port number
            gpioudpsa.sin_family = AF_INET;
            gpioudpsa.sin_port = htons (pidpudpport);
            if (!inet_aton (server, &gpioudpsa.sin_addr)) {
                struct hostent *he = gethostbyname (server);
                if (he == NULL) {
                    fprintf (stderr, "pidp_proc: bad server address %s\n", server);
                    ABORT ();
                }
                if ((he->h_addrtype != AF_INET) || (he->h_length != 4)) {
                    fprintf (stderr, "pidp_proc: server %s not IP v4\n", server);
                    ABORT ();
                }
                gpioudpsa.sin_addr = *(struct in_addr *)he->h_addr;
            }

            // create udp socket
            gpioudpfd = socket (AF_INET, SOCK_DGRAM, 0);
            if (gpioudpfd < 0) ABORT ();

            // bind to ephemeral client port
            struct sockaddr_in cliaddr;
            memset (&cliaddr, 0, sizeof cliaddr);
            cliaddr.sin_family = AF_INET;
            if (bind (gpioudpfd, (struct sockaddr *) &cliaddr, sizeof cliaddr) < 0) {
                fprintf (stderr, "pidp_proc: error binding: %m\n");
                ABORT ();
            }

            // get timeout
            char const *env = getenv ("pidpudpmsec");
            gpioudptm = (env == NULL) ? 50 : atoi (env);

            fprintf (stderr, "pidp_proc: using %s:%d timeout %ums\n",
                inet_ntoa (gpioudpsa.sin_addr), ntohs (gpioudpsa.sin_port), gpioudptm);
        } else {

            // not using UDP, access gpio pins directly
            gpiopage = (uint32_t volatile *) gpiofile.open ("/dev/gpiomem");
        }
    }

    if (gpiopage != NULL) {

        // set pins for outputting to LEDs
        gpiopage[GPIO_FSEL0+0] = LEDSEL0;
        gpiopage[GPIO_FSEL0+1] = LEDSEL1;
        gpiopage[GPIO_FSEL0+2] = LEDSEL2;

        // output LED rows
        for (int i = 0; i < 8; i ++) {
            writegpio ((0xFF << LEDROW1) | COLMASK, (1 << (LEDROW1 + i)) | (pidpmsg->ledrows[i] ^ COLMASK));
            usleep (200);
        }

        // set pins for reading switches
        gpiopage[GPIO_FSEL0+0] = SWSEL0;
        gpiopage[GPIO_FSEL0+1] = SWSEL1;
        gpiopage[GPIO_FSEL0+2] = SWSEL2;

        // read switch rows
        for (int i = 0; i < 3; i ++) {
            writegpio ((0xFF << LEDROW1) | (7 << SWROW1), (7 << SWROW1) & ~ (1 << (SWROW1 + i)));
            usleep (10);
            pidpmsg->swrows[i] = gpiopage[GPIO_LEV0];
        }
    } else {
        struct pollfd polls[] = { gpioudpfd, POLLIN, 0 };

        int rc = sendto (gpioudpfd, pidpmsg, sizeof *pidpmsg, 0, (struct sockaddr *) &gpioudpsa, sizeof gpioudpsa);
        if (rc < (int) sizeof *pidpmsg) {
            if (rc < 0) {
                fprintf (stderr, "pidp_proc: error writing to server: %m\n");
            } else {
                fprintf (stderr, "pidp_proc: only wrote %d bytes of %d\n", rc, (int) sizeof *pidpmsg);
            }
            ABORT ();
        }

        while (true) {

            // wait up to gpioudptm for a response
            do rc = poll (polls, 1, gpioudptm);
            while ((rc < 0) && (errno == EINTR));
            if (rc < 0) {
                fprintf (stderr, "pidp_proc: poll error: %m\n");
                ABORT ();
            }

            // if timed out, return failure status so we get called back with up-to-date LEDs
            if (rc == 0) {
                if (++ timeoutctr == 5) {
                    fprintf (stderr, "pidp_proc: repeated timeouts\n");
                }
                return false;
            }
            if (timeoutctr >= 5) {
                fprintf (stderr, "pidp_proc: recovered\n");
            }
            timeoutctr = 0;

            // read in reply
            struct sockaddr_in recvadd;
            socklen_t recvlen = sizeof recvadd;
            PiDPMsg recvbuf;
            rc = recvfrom (gpioudpfd, &recvbuf, sizeof recvbuf, 0, (struct sockaddr *) &recvadd, &recvlen);
            if (rc != sizeof recvbuf) {
                if (rc < 0) {
                    fprintf (stderr, "pidp_proc: error reading from server: %m\n");
                } else {
                    fprintf (stderr, "pidp_proc: only read %d bytes of %d\n", rc, (int) sizeof recvbuf);
                }
                ABORT ();
            }

            // if reply to this request, return the reply value
            if (recvbuf.seq == pidpmsg->seq) {
                memcpy (pidpmsg->swrows, &recvbuf.swrows, sizeof pidpmsg->swrows);
                break;
            }

            // it can't be a reply to a future request!
            if (recvbuf.seq > pidpmsg->seq) {
                fprintf (stderr, "pidp_proc: sent seq %llu rcvd seq %llu\n",
                    (unsigned long long) pidpmsg->seq, (unsigned long long) recvbuf.seq);
                ABORT ();
            }

            // reply to an old request, ignore and read again
        }
    }

    return true;
}

// write value to GPIO pins
static void writegpio (uint32_t mask, uint32_t data)
{
    gpiopage[GPIO_SET0] = data;
    gpiopage[GPIO_CLR0] = data ^ mask;
}
