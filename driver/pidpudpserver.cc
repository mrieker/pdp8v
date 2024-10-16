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

// runs on raspberry pi plugged into pidp8 panel to serve up leds and switches
// used by raspictl -pidp whenever envar pidpudpaddr is set

// on raspi plugged into pidp8 panel:
//  ./pidpudpserver [-verbose]

// on some other computer:
//  export pidpudpaddr=<ip address of the pidpudpserver raspi>
//  ./raspictl ... -pidp ...

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define ABORT() do { fprintf (stderr, "abort() %s:%d\n", __FILE__, __LINE__); abort (); } while (0)

#include "pidp.h"

int main (int argc, char **argv)
{
    bool verbose = false;

    for (int i = 1; i < argc; i ++) {
        if (strcasecmp (argv[i], "-verbose") == 0) {
            verbose = true;
            continue;
        }
        fprintf (stderr, "pidpudpserver: unknown argument %s\n", argv[i]);
        return 1;
    }

    setlinebuf (stdout);

    struct sockaddr_in cliaddr;
    memset (&cliaddr, 0, sizeof cliaddr);
    cliaddr.sin_family = AF_INET;
    cliaddr.sin_port = htons (pidpudpport);

    int udpfd = socket (AF_INET, SOCK_DGRAM, 0);
    if (udpfd < 0) ABORT ();

    if (bind (udpfd, (struct sockaddr *) &cliaddr, sizeof cliaddr) < 0) {
        fprintf (stderr, "pidpudpserver: error binding to %d: %m\n", pidpudpport);
        ABORT ();
    }
    printf ("listening on %d\n", pidpudpport);

    uint64_t lastseq = 0;
    while (true) {
        PiDPMsg pidpmsg;
        socklen_t addrlen = sizeof cliaddr;
        int rc = recvfrom (udpfd, &pidpmsg, sizeof pidpmsg, 0, (struct sockaddr *) &cliaddr, &addrlen);
        if (rc != (int) sizeof pidpmsg) {
            if (rc < 0) {
                fprintf (stderr, "pidpudpserver: error receiving packet: %m\n");
            } else {
                fprintf (stderr, "pidpudpserver: only received %d bytes of %d\n", rc, (int) sizeof pidpmsg);
            }
            ABORT ();
        }
        if (lastseq > pidpmsg.seq) {
            if (verbose) printf ("old %016llX\n", (unsigned long long) pidpmsg.seq);
        } else {
            if (verbose) {
                char const *age;
                switch (pidpmsg.seq - lastseq) {
                    case 0:  age = "dup"; break;
                    case 1:  age = "   "; break;
                    default: age = "gap"; break;
                }
                printf ("%s %016llX\n", age, (unsigned long long) pidpmsg.seq);
            }
            lastseq = pidpmsg.seq;
            if (pidp_proc (&pidpmsg)) {
                rc = sendto (udpfd, &pidpmsg, sizeof pidpmsg, 0, (struct sockaddr *) &cliaddr, addrlen);
                if (rc != (int) sizeof pidpmsg) {
                    if (rc < 0) {
                        fprintf (stderr, "pidpudpserver: error transmitting: %m\n");
                    } else {
                        fprintf (stderr, "pidpudpserver: only transmitted %d bytes of %d\n", rc, (int) sizeof pidpmsg);
                    }
                }
            }
        }
    }
}
