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

// used by physlib when envar physlib_gpioudp is set
// runs on raspberry pi plugged into gpio connector on processor to serve up gpio pins
// set physlib_gpioudp = this ip address then run program on other system

//  ./gpioudpserver.armv6l [-verbose]

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "gpiolib.h"

int main (int argc, char **argv)
{
    bool verbose = false;

    for (int i = 1; i < argc; i ++) {
        if (strcasecmp (argv[i], "-verbose") == 0) {
            verbose = true;
            continue;
        }
        fprintf (stderr, "unknown argument %s\n", argv[i]);
        return 1;
    }

    setlinebuf (stdout);

    PhysLib *physlib = new PhysLib ();

    struct sockaddr_in cliaddr;
    memset (&cliaddr, 0, sizeof cliaddr);
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons (PhysLib::gpioudpport);

    int udpfd = socket (AF_INET, SOCK_DGRAM, 0);
    if (udpfd < 0) ABORT ();

    if (bind (udpfd, (struct sockaddr *) &cliaddr, sizeof cliaddr) < 0) {
        fprintf (stderr, "error binding to %d: %m\n", PhysLib::gpioudpport);
        ABORT ();
    }
    printf ("listening on %d\n", PhysLib::gpioudpport);

    uint64_t lastseq = 0;
    while (true) {
        PhysLib::UDPMsg buffer;
        socklen_t addrlen = sizeof cliaddr;
        int rc = recvfrom (udpfd, &buffer, sizeof buffer, 0, (struct sockaddr *) &cliaddr, &addrlen);
        if (rc != (int) sizeof buffer) {
            if (rc < 0) {
                fprintf (stderr, "error receiving packet: %m\n");
            } else {
                fprintf (stderr, "only received %d bytes of %d\n", rc, (int) sizeof buffer);
            }
            ABORT ();
        }
        if (lastseq > buffer.seq) {
            if (verbose) printf ("old %016llX:  %08X\n", (unsigned long long) buffer.seq, buffer.val);
        } else {
            if (verbose) {
                char const *age;
                switch (buffer.seq - lastseq) {
                    case 0:  age = "dup"; break;
                    case 1:  age = "   "; break;
                    default: age = "gap"; break;
                }
                printf ("%s %016llX:  ", age, (unsigned long long) buffer.seq);
            }
            lastseq = buffer.seq;
            if (buffer.val == 0) {
                buffer.val = physlib->readgpio ();
                if (verbose) printf ("    gpio => %08X", buffer.val);
            } else {
                if (verbose) printf ("%08X => gpio    ", buffer.val);
                physlib->writegpio ((buffer.val & G_QENA) != 0, buffer.val);
            }
            if (verbose) {
                printf ("  %s  %s  %s  %s  %s  %s  %s  %s  %s  %s  %s  <%04o>  %s  %s\n",
                    (buffer.val & G_IAK)   ? "IAK  " : "     ",
                    (buffer.val & G_WRITE) ? "WRITE" : "     ",
                    (buffer.val & G_READ)  ? "READ " : "     ",
                    (buffer.val & G_DFRM)  ? "DFRM " : "     ",
                    (buffer.val & G_IOIN)  ? "IOIN " : "     ",
                    (buffer.val & G_JUMP)  ? "JUMP " : "     ",
                    (buffer.val & G_DENA)  ? "DENA " : "     ",
                    (buffer.val & G_IRQ)   ? "IRQ  " : "     ",
                    (buffer.val & G_QENA)  ? "QENA " : "     ",
                    (buffer.val & G_IOS)   ? "IOS  " : "     ",
                    (buffer.val & G_LINK)  ? "LINK " : "     ",
                    (buffer.val & G_DATA) / G_DATA0,
                    (buffer.val & G_RESET) ? "RESET" : "     ",
                    (buffer.val & G_CLOCK) ? "CLOCK" : "     ");
            }
            rc = sendto (udpfd, &buffer, sizeof buffer, 0, (struct sockaddr *) &cliaddr, addrlen);
            if (rc != (int) sizeof buffer) {
                if (rc < 0) {
                    fprintf (stderr, "error transmitting: %m\n");
                } else {
                    fprintf (stderr, "only transmitted %d bytes of %d\n", rc, (int) sizeof buffer);
                }
            }
        }
    }
}
