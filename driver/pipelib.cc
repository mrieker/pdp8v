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

// Tests the raspictl.cc program with the .mod modules using the simulation function of NetGen.java
// Slowest simulation that verifies module files

// Calling PipeLib::writegpio() cause 'force' commands to be written to NetGen
// These are read by the simulation function of NetGen.java to set the internal state of the pins.

// Reads of the GPIO pins cause 'examine' commands to be sent to NetGen.
// The simulation function of NetGen.java replies with the corresponding internal state of the pins.

// The module definitions loaded into NetGen.java must contain:
//   module proc ( various pins that would get connected to the raspberry pi )

// ./autoboardtest -pipelib -page -verbose all
// ./raspictl -printstate -randmem -pipelib

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gpiolib.h"
#include "makepipelibmod.h"
#include "pindefs.h"

PipeLib::PipeLib (char const *modname)
{
    libname = "pipelib";

    // make sure modname exists in ../modules/whole.mod so netgen will find it
    // also changes "rpi" to "rpi_testpads" so gpio pins will be found
    modname = makepipelibmod (modname);

    this->modname = modname;
    this->netgenpid = 0;
    this->sendseq = 0;
}

// open connections to the java program
void PipeLib::open ()
{
    char exedir[256], netgen[256+22], modfile[256+22];

    getexedir (exedir, sizeof exedir);

    char const *netgenenv = getenv ("pipelib_netgen");
    if (netgenenv == NULL) {
        strcpy (netgen, exedir);
        strcat (netgen, "../netgen/netgen.sh");
    } else {
        strncpy (netgen, netgenenv, sizeof netgen);
        netgen[sizeof netgen-1] = 0;
    }

    char const *modfileenv = getenv ("pipelib_modfile");
    if (modfileenv == NULL) {
        strcpy (modfile, exedir);
        strcat (modfile, "../modules/whole.mod");
    } else {
        strncpy (modfile, modfileenv, sizeof modfile);
        modfile[sizeof modfile-1] = 0;
    }

    fprintf (stderr, "PipeLib::open: netgen=%s\n", netgen);
    fprintf (stderr, "PipeLib::open: modfile=%s\n", modfile);

    // fork subprocess that runs simulator
    int frnetgenfds[2] = { -1, -1 };
    int tonetgenfds[2] = { -1, -1 };

    if (pipe (frnetgenfds) < 0) ABORT ();
    if (pipe (tonetgenfds) < 0) ABORT ();
    int subpid = fork ();
    if (subpid < 0) ABORT ();
    if (subpid == 0) {
        if (dup2 (tonetgenfds[0], 0) < 0) ABORT ();
        if (dup2 (frnetgenfds[1], 1) < 0) ABORT ();
        ::close (tonetgenfds[0]);
        ::close (tonetgenfds[1]);
        ::close (frnetgenfds[0]);
        ::close (frnetgenfds[1]);
        if (setpgid (0, 0) < 0) ABORT ();
        execl (netgen, netgen, modfile, "-sim", "-", NULL);
        fprintf (stderr, "PipeLib::open: error execing %s: %m\n", netgen);
        ABORT ();
    }
    netgenpid = subpid;
    ::close (tonetgenfds[0]);
    ::close (frnetgenfds[1]);

    // write simulator TCL commands to this pipe
    sendfile = fdopen (tonetgenfds[1], "w");
    if (sendfile == NULL) ABORT ();

    setlinebuf (sendfile);

    // read simulator output from this pipe
    recvfile = fdopen (frnetgenfds[0], "r");
    if (recvfile == NULL) ABORT ();

    // step sim this many times between clock transitions
    runcount = getenv ("pipelib_runcount");
    if (runcount == NULL) runcount = "150";
    fprintf (stderr, "PipeLib::open: runcount=%s\n", runcount);

    // tell simulator what module to simulate
    // ...it has the gpio and 32-pin connectors as inputs/outputs
    fprintf (stderr, "PipeLib::open: modname=%s\n", modname);
    fprintf (sendfile, "module %s\n", modname);
    //fprintf (sendfile, "monitor fetch1q/seq fetch2q/seq defer1q/seq defer2q/seq defer3q/seq exec1q/seq exec2q/seq exec3q/seq intak1q/seq irq/seq\n");
    //fprintf (sendfile, "monitor irq/seq acq _aluq maq _alua/alu _alub/alu\n");
}

void PipeLib::close ()
{
    if (netgenpid > 0) {
        kill (netgenpid, SIGKILL);
        netgenpid = 0;
    }
    if (sendfile != NULL) {
        fclose (sendfile);
        sendfile = NULL;
    }
    if (recvfile != NULL) {
        fclose (recvfile);
        recvfile = NULL;
    }
}

// delay half a clock cycle
void PipeLib::halfcycle ()
{
    // tell sim how much to run
    fprintf (sendfile, "run %s\n", runcount);

    // force simulation to run before we return
    // - dumps any monitor command output before returning
    ////????////examine ("[examine _JUMP]");  //// undefined for module "seq", ie, when just using paddles
}

// read raspi gpio pins
uint32_t PipeLib::readgpio ()
{
    // get what is being sent from processor to raspi as a string of gpio bits
    ASSERT (G_IAK   == (1 << 27));
    ASSERT (G_WRITE == (1 << 26));
    ASSERT (G_READ  == (1 << 25));
    ASSERT (G_DFRM  == (1 << 24));
    ASSERT (G_IOIN  == (1 << 23));
    ASSERT (G_JUMP  == (1 << 22));
    ASSERT (G_LINK  == (1 << 16));
    ASSERT (G_DATA  == (4095 << 4));
    uint32_t rawgpio = examine (
        "[examine _INTAK]"      //    [27] processor is acknowledging interrupt
        "[examine _MWRITE]"     //    [26] processor is requesting to write memory
        "[examine _MREAD]"      //    [25] processor is requesting to read memory
        "[examine _DFRM]"       //    [24] processor is accessing data frame memory
        "[examine IOINST]"      //    [23] processor is executing an I/O instruction
        "[examine _JUMP]"       //    [22] processor is executing a JMP/JMS instruction
        "00000"
        "[examine _MDL]"        //    [16] state of the LINK register
        "[examine  _MD]"        // [15:04] state of the ALU output
        "0000");
    rawgpio ^= G_IAK | G_WRITE | G_READ | G_DFRM | G_JUMP | G_LINK | G_DATA; // convert active low to active high
    rawgpio |= lastwritegpio & G_OUTS;                              // these always come out of raspi
    if (lastwritegpio & G_QENA) {                                   // QENA means data,link going from raspi to processor
        rawgpio &= ~ (G_LINK | G_DATA);
        rawgpio |= lastwritegpio & (G_LINK | G_DATA);
    }
    return rawgpio;
}

// write raspi gpio pins
void PipeLib::writegpio (bool wdata, uint32_t valu)
{
    lastwritegpio = valu;
    if (wdata) {
        lastwritegpio &= ~ G_DENA;
        lastwritegpio |=   G_QENA;
        fprintf (sendfile, "force MQ %u ; force MQL %u", (valu & G_DATA) / G_DATA0, (valu & G_LINK) != 0);
    } else {
        lastwritegpio &= ~ G_QENA;
        lastwritegpio |=   G_DENA;
        fprintf (sendfile, "force MQ 0bXXXXXXXXXXXX ; force MQL 0bX");
    }
    fprintf (sendfile, " ; force CLOK2 %u ; force INTRQ %u ; force IOSKP %u ; force RESET %u\n",
        (valu & G_CLOCK) != 0, (valu & G_IRQ) != 0, (valu & G_IOS) != 0, (valu & G_RESET) != 0);
}

int PipeLib::examine (char const *varname, uint32_t *value)
{
    char linebuf[4096], *p, *q;
    uint32_t recvseq;

    fprintf (sendfile, "echo @@@ %u [examine -nothrow %s]\n", ++ sendseq, varname);

    while (true) {
        if (fgets (linebuf, sizeof linebuf, recvfile) == NULL) {
            if (errno == 0) errno = EPIPE;
            fprintf (stderr, "PipeLib::examine: error reading reply: %m\n");
            ABORT ();
        }
        p = strstr (linebuf, "@@@ ");
        if (p != NULL) break;
        fputs (linebuf, stdout);
    }

    *p = 0;
    fputs (linebuf, stdout);
    p += 4;

    recvseq = strtoul (p, &q, 10);
    if ((recvseq != sendseq) || (*q != ' ')) {
        fprintf (stderr, "PipeLib::examine: sendseq=%u linebuf=@@@ %s", sendseq, p);
        ABORT ();
    }
    ////if (strchr (q, 'x') != NULL) {
    ////    fprintf (stderr, "PipeLib::examine: %s = %s", varname, q);
    ////}

    int width = 0;
    uint32_t pinmask = 0;
    while (true) {
        switch (*(++ q)) {
            case '\n': {
                *value = pinmask;
                return width;
            }
            case '0':  pinmask += pinmask;     break;
            case 'x':
            case '1':  pinmask += pinmask + 1; break;
            default: {
                fprintf (stderr, "PipeLib::examine: bad linebuf=@@@ %s", p);
                ABORT ();
            }
        }
        width ++;
    }
}

// read value of the given simulator variable
//  input:
//   varname = [examine varname][examine varname]...
//  output:
//   returns binary value
uint32_t PipeLib::examine (char const *varname)
{
    char linebuf[4096], *p, *q;
    uint32_t recvseq;

    fprintf (sendfile, "echo @@@ %u %s\n", ++ sendseq, varname);

    while (true) {
        if (fgets (linebuf, sizeof linebuf, recvfile) == NULL) {
            if (errno == 0) errno = EPIPE;
            fprintf (stderr, "PipeLib::examine: error reading reply: %m\n");
            ABORT ();
        }
        p = strstr (linebuf, "@@@ ");
        if (p != NULL) break;
        fputs (linebuf, stdout);
    }

    *p = 0;
    fputs (linebuf, stdout);
    p += 4;

    recvseq = strtoul (p, &q, 10);
    if ((recvseq != sendseq) || (*q != ' ')) {
        fprintf (stderr, "PipeLib::examine: sendseq=%u linebuf=@@@ %s", sendseq, p);
        ABORT ();
    }
    ////if (strchr (q, 'x') != NULL) {
    ////    fprintf (stderr, "PipeLib::examine: %s = %s", varname, q);
    ////}

    uint32_t pinmask = 0;
    while (true) {
        switch (*(++ q)) {
            case '\n': return pinmask;
            case '0':  pinmask += pinmask;     break;
            case 'x':
            case '1':  pinmask += pinmask + 1; break;
            default: {
                fprintf (stderr, "PipeLib::examine: bad linebuf=@@@ %s", p);
                ABORT ();
            }
        }
    }
}

// we have paddles
bool PipeLib::haspads ()
{
    return true;
}

// read A,B,C,D connectors
//  output:
//   returns pins for each connector
void PipeLib::readpads (uint32_t *pinss)
{
    static std::string *pexamines[NPADS];

    for (int pad = 0; pad < NPADS; pad ++) {

        // get table and exam string corresponding to connector
        PinDefs const *pindefs = pindefss[pad];
        std::string **pexamine = &pexamines[pad];

        // if we don't already have it, build the examine command string
        // returns the 32 bit value from the connector all at once
        std::string *xexamine = *pexamine;
        if (xexamine == NULL) {
            xexamine = new std::string ();
            for (uint32_t mask = 0x80000000U; mask != 0; mask /= 2) {
                for (PinDefs const *p = pindefs; p->pinmask != 0; p ++) {
                    if (p->pinmask == mask) {
                        xexamine->append ("[examine {");
                        xexamine->append (p->varname);
                        xexamine->append ("}]");
                        goto gotpin;
                    }
                }
                xexamine->append ("0");
            gotpin:;
            }
            *pexamine = xexamine;
        }

        // send examine command to netgen and read reply
        pinss[pad] = this->examine (xexamine->c_str ());
    }
}

// write A,B,C,D connectors
//  input:
//   masks = which pins of the connector to write
//   pinss = value to write to the pins
//  output:
//   pins written
void PipeLib::writepads (uint32_t const *masks, uint32_t const *pinss)
{
    for (int pad = 0; pad < NPADS; pad ++) {
        uint32_t mask = masks[pad];
        uint32_t pins = pinss[pad];
        char chr = 'a' + pad;
        char sep = ' ';
        for (int i = 0; i < 32; i ++) {
            if ((mask >> i) & 1) {
                fprintf (sendfile, "%cforce I%u/%ccon %u", sep, i + 1, chr, (pins >> i) & 1);
                sep = ';';
            }
        }
        if (sep != ' ') fputc ('\n', sendfile);
    }
}
