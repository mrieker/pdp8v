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

// process extended memory I/O opcodes
// page numbers in small-computer-handbook-1972.pdf

#include <string.h>

#include "extarith.h"
#include "memext.h"
#include "memory.h"
#include "shadow.h"

static IODevOps const iodevops[] = {
    { 06000, "SKON (CPU) skip if interrupt on" },
    { 06001, "ION (CPU) interrupt turn on" },
    { 06002, "IOF (CPU) interrupt turn off" },
    { 06003, "SRQ (CPU) skip if interrupt request" },
    { 06004, "GTF (CPU) get flags" },
    { 06005, "RTF (CPU) restore flags" },
    { 06006, "SGT (CPU) skip if greater than" },
    { 06007, "CAF (CPU) clear all flags" },
    { 06201, "CDF 000 (MEX) change to data field 0" },
    { 06211, "CDF 010 (MEX) change to data field 1" },
    { 06221, "CDF 020 (MEX) change to data field 2" },
    { 06231, "CDF 030 (MEX) change to data field 3" },
    { 06241, "CDF 040 (MEX) change to data field 4" },
    { 06251, "CDF 050 (MEX) change to data field 5" },
    { 06261, "CDF 060 (MEX) change to data field 6" },
    { 06271, "CDF 070 (MEX) change to data field 7" },
    { 06203, "CDIF 000 (MEX) change to d&i field 0" },
    { 06213, "CDIF 010 (MEX) change to d&i field 1" },
    { 06223, "CDIF 020 (MEX) change to d&i field 2" },
    { 06233, "CDIF 030 (MEX) change to d&i field 3" },
    { 06243, "CDIF 040 (MEX) change to d&i field 4" },
    { 06253, "CDIF 050 (MEX) change to d&i field 5" },
    { 06263, "CDIF 060 (MEX) change to d&i field 6" },
    { 06273, "CDIF 070 (MEX) change to d&i field 7" },
    { 06202, "CIF 000 (MEX) change to instr field 0" },
    { 06212, "CIF 010 (MEX) change to instr field 1" },
    { 06222, "CIF 020 (MEX) change to instr field 2" },
    { 06232, "CIF 030 (MEX) change to instr field 3" },
    { 06242, "CIF 040 (MEX) change to instr field 4" },
    { 06252, "CIF 050 (MEX) change to instr field 5" },
    { 06262, "CIF 060 (MEX) change to instr field 6" },
    { 06272, "CIF 070 (MEX) change to instr field 7" },
    { 06214, "RDF (MEX) read data field" },
    { 06224, "RIF (MEX) read instr field" },
    { 06234, "RIB (MEX) read interrupt buffer" },
    { 06244, "RMF (MEX) restore memory field" },
    { 06204, "CINT (MEX) clear user interrupt" },
    { 06254, "SINT (MEX) skip on user interrupt" },
    { 06264, "CUF (MEX) clear user flag" },
    { 06274, "SUF (MEX) set user flag" },
};

MemExt::MemExt ()
{
    opscount = sizeof iodevops / sizeof iodevops[0];
    opsarray = iodevops;

    iodevname = "memext";

    // according to DHMCA, these are not cleared by clear-all-flags (CAF 6007)
    this->savediframe = 0;                      // no instruction frame saved
    this->saveddframe = 0;                      // no data frame saved
    this->saveduserflag = false;                // no userflag saved
    this->userflagafterjump = false;            // stay in execmode after jump
    this->iframeafterjump = 0;                  // stay in iframe 0 after jump
}

void MemExt::setdfif (uint32_t frame)
{
    this->iframe = frame << 12;                 // use by GUI console to set frame
    this->iframeafterjump = frame << 12;
    this->dframe = frame << 12;
}

// reset to power-on state
void MemExt::ioreset ()
{
    this->iframe = 0;                           // start in iframe 0
    this->dframe = 0;                           // start in dframe 0
    this->intdisableduntiljump = false;         // don't override intenabled
    this->intenableafterfetch = false;          // don't enable interrups after next fetch
    this->intenabled = false;                   // interrupts disabled
    this->userflag = false;                     // start in execmode
    clrintreqmask (IRQ_USERINT);                // no usermode fault
}

// process script 'iodev memext' commands
SCRet *MemExt::scriptcmd (int argc, char const *const *argv)
{
    // get [df/iduj/ie/ieaf/if/ifaj/ireq/sdf/sif/suf/uf/ufaj]
    if (strcmp (argv[0], "get") == 0) {
        std::string ireqs;
        if ((argc < 2) || (strcmp (argv[1], "ireq") == 0)) {
            uint16_t irqm = getintreqmask ();
            char const *irqx = "";
            if (irqm & IRQ_SCNINTREQ) { irqm -= IRQ_SCNINTREQ; ireqs.append (irqx); ireqs.append ("SCN"); irqx = "+"; }
            if (irqm & IRQ_LINECLOCK) { irqm -= IRQ_LINECLOCK; ireqs.append (irqx); ireqs.append ("LNC"); irqx = "+"; }
            if (irqm & IRQ_RANDMEM)   { irqm -= IRQ_RANDMEM;   ireqs.append (irqx); ireqs.append ("RAN"); irqx = "+"; }
            if (irqm & IRQ_USERINT)   { irqm -= IRQ_USERINT;   ireqs.append (irqx); ireqs.append ("USR"); irqx = "+"; }
            if (irqm & IRQ_TTYKBPR)   { irqm -= IRQ_TTYKBPR;   ireqs.append (irqx); ireqs.append ("TTY"); irqx = "+"; }
            if (irqm & IRQ_PTAPE)     { irqm -= IRQ_PTAPE;     ireqs.append (irqx); ireqs.append ("PT");  irqx = "+"; }
            if (irqm & IRQ_RTC)       { irqm -= IRQ_RTC;       ireqs.append (irqx); ireqs.append ("RTC"); irqx = "+"; }
            if (irqm & IRQ_RK8)       { irqm -= IRQ_RK8;       ireqs.append (irqx); ireqs.append ("RK8"); irqx = "+"; }
            if (irqm != 0) { char buf[12]; snprintf (buf, sizeof buf, "%04X", irqm); ireqs.append (irqx); ireqs.append (buf); }
        }

        if (argc == 1) return new SCRetStr ("df=%o;iduj=%o;ie=%o;ieaf=%o;if=%o;ifaj=%o;ireq=%s;sdf=%o;sif=%o;suf=%o;uf=%o;ufaj=%o",
            this->dframe >> 12, this->intdisableduntiljump, this->intenabled, this->intenableafterfetch, this->iframe >> 12,
            this->iframeafterjump >> 12, ireqs.c_str (), this->saveddframe >> 12, this->savediframe >> 12, this->saveduserflag,
            this->userflag, this->userflagafterjump);

        if (argc == 2) {
            if (strcmp (argv[1], "df")   == 0) return new SCRetInt (this->dframe >> 12);
            if (strcmp (argv[1], "iduj") == 0) return new SCRetInt (this->intdisableduntiljump);
            if (strcmp (argv[1], "ie")   == 0) return new SCRetInt (this->intenabled);
            if (strcmp (argv[1], "ieaf") == 0) return new SCRetInt (this->intenableafterfetch);
            if (strcmp (argv[1], "if")   == 0) return new SCRetInt (this->iframe >> 12);
            if (strcmp (argv[1], "ifaj") == 0) return new SCRetInt (this->iframeafterjump >> 12);
            if (strcmp (argv[1], "ireq") == 0) return new SCRetStr (ireqs.c_str ());
            if (strcmp (argv[1], "sdf")  == 0) return new SCRetInt (this->saveddframe >> 12);
            if (strcmp (argv[1], "sif")  == 0) return new SCRetInt (this->savediframe >> 12);
            if (strcmp (argv[1], "suf")  == 0) return new SCRetInt (this->saveduserflag);
            if (strcmp (argv[1], "uf")   == 0) return new SCRetInt (this->userflag);
            if (strcmp (argv[1], "ufaj") == 0) return new SCRetInt (this->userflagafterjump);
        }

        return new SCRetErr ("iodev memext get [df/iduj/ie/ieaf/if/ifaj/ireq/sdf/sif/suf/uf/ufaj]");
    }

    if (strcmp (argv[0], "help") == 0) {
        puts ("");
        puts ("valid sub-commands:");
        puts ("");
        puts ("  get                    - return all registers as a string");
        puts ("  get <register>         - return specific register as a value");
        puts ("  set <register> <value> - set register to value");
        puts ("");
        puts ("registers:");
        puts ("");
        puts ("  df   - data frane");
        puts ("  iduj - interrupts disabled until jump");
        puts ("  ie   - interrupts enabled");
        puts ("  ieaf - interrupts enabled after fetch");
        puts ("  if   - instruction frame");
        puts ("  ifaj - instruction frame after jump");
        puts ("  ireq - interrupt requests (read-only)");
        puts ("  sdf  - saved data frame");
        puts ("  sif  - saved instruction frame");
        puts ("  suf  - saved usermode flag");
        puts ("  uf   - usermode flag");
        puts ("  ufaj - usermode flag after jump");
        puts ("");
        return NULL;
    }

    // set df/iduj/ie/ieaf/if/ifaj/sdf/sif/suf/uf/ufaj <value>
    if (strcmp (argv[0], "set") == 0) {
        if (argc == 3) {
            char *p;
            int value = strtol (argv[2], &p, 0);
            if (strcmp (argv[1], "df")   == 0) {
                if ((*p != 0) || (value < 0) || (value > 7)) return new SCRetErr ("df value %s not in range 0..7", argv[2]);
                this->dframe = value << 12;
                return NULL;
            }
            if (strcmp (argv[1], "iduj") == 0) {
                if ((*p != 0) || (value < 0) || (value > 1)) return new SCRetErr ("iduj value %s not in range 0..1", argv[2]);
                this->intdisableduntiljump = value;
                return NULL;
            }
            if (strcmp (argv[1], "ie")   == 0) {
                if ((*p != 0) || (value < 0) || (value > 1)) return new SCRetErr ("ie value %s not in range 0..1", argv[2]);
                this->intenabled = value;
                return NULL;
            }
            if (strcmp (argv[1], "ieaf") == 0) {
                if ((*p != 0) || (value < 0) || (value > 1)) return new SCRetErr ("ieaf value %s not in range 0..1", argv[2]);
                this->intenableafterfetch = value;
                return NULL;
            }
            if (strcmp (argv[1], "if")   == 0) {
                if ((*p != 0) || (value < 0) || (value > 7)) return new SCRetErr ("if value %s not in range 0..7", argv[2]);
                this->iframe = value << 12;
                return NULL;
            }
            if (strcmp (argv[1], "ifaj") == 0) {
                if ((*p != 0) || (value < 0) || (value > 7)) return new SCRetErr ("ifaj value %s not in range 0..7", argv[2]);
                this->iframeafterjump = value << 12;
                return NULL;
            }
            if (strcmp (argv[1], "sdf")  == 0) {
                if ((*p != 0) || (value < 0) || (value > 7)) return new SCRetErr ("sdf value %s not in range 0..7", argv[2]);
                this->saveddframe = value << 12;
                return NULL;
            }
            if (strcmp (argv[1], "sif")  == 0) {
                if ((*p != 0) || (value < 0) || (value > 7)) return new SCRetErr ("sif value %s not in range 0..7", argv[2]);
                this->savediframe = value << 12;
                return NULL;
            }
            if (strcmp (argv[1], "suf")  == 0) {
                if ((*p != 0) || (value < 0) || (value > 1)) return new SCRetErr ("suf value %s not in range 0..1", argv[2]);
                this->saveduserflag = value;
                return NULL;
            }
            if (strcmp (argv[1], "uf")   == 0) {
                if ((*p != 0) || (value < 0) || (value > 1)) return new SCRetErr ("uf value %s not in range 0..1", argv[2]);
                this->userflag = value;
                return NULL;
            }
            if (strcmp (argv[1], "ufaj") == 0) {
                if ((*p != 0) || (value < 0) || (value > 1)) return new SCRetErr ("ufaj value %s not in range 0..1", argv[2]);
                this->userflagafterjump = value;
                return NULL;
            }
        }

        return new SCRetErr ("iodev memext set df/iduj/ie/ieaf/if/ifaj/sdf/sif/suf/uf/ufaj <value>");
    }

    return new SCRetErr ("unknown memext command %s - valid: get set", argv[0]);
}

// called in INTAK1 when acknowledging interrupt request
void MemExt::intack ()
{
    this->savediframe = this->iframe;           // save iframe, dframe, userflag
    this->saveddframe = this->dframe;
    this->saveduserflag = this->userflag;
    this->intenabled = false;                   // block further interrupts
    this->intenableafterfetch = false;
    this->iframe = 0;                           // reset to frame 0
    this->iframeafterjump = 0;
    this->dframe = 0;
    this->userflag = false;                     // reset to execmode
    this->userflagafterjump = false;
}

// called in middle of state following EXEC1 for JMP/JMS execution.
// for JMS, this is when it is writing the return address, so it needs the new iframe.
// for JMP, this is when it is reading the target instruction, so it needs the new iframe.
void MemExt::doingjump ()
{
    // maybe set new iframe for next instruction
    this->iframe = this->iframeafterjump;

    // maybe set usermode for next instruction
    this->userflag = this->userflagafterjump;

    // don't block this->intenabled any more
    // this is way too late for DHMCA but good
    // enough for all practical purposes, and
    // being redundant should be ok if the hack
    // in doingread() is there, the flag just
    // gets cleared twice in a single jump instr
    this->intdisableduntiljump = false;
}

// called whenever memory is being read, could be opcode or data
// all we need is to be called when an opcode is being read so
// we can enable interrupts if last instruction was an ION
void MemExt::doingread (uint16_t data)
{
    // the effect of an 'ION' instruction can take effect
    // now that we have started fetching the instruction after the ION,
    // presumably a 'JMPI 0' to return from where the interrupt took place
    this->intenabled = this->intenableafterfetch;

    // HACK FOR maindec DHMCA - we must get interrupts enabled before JMP1 state ends
    //                          this gets them enabled in middle of FETCH2
    // if sending a JMP/JMS back to processor and it has blocked interrupts, allow them
    if ((shadow.r.state == Shadow::FETCH2) && ((data & 06000) == 04000)) {
        this->intdisableduntiljump = false;     // don't block this->intenabled any more
    }
}

// called to sense if processor interrupts are enabled
bool MemExt::intenabd ()
{
    return this->intenabled &&                  // must be enabled with such as an ION (6001) instruction
            ! this->intdisableduntiljump;       // and must not be blocked with such as an RTF (6005) instruction
}

// called to see if I/O instruction (including OSR and/or HLT) is allowed
// requests user error interrupt if not
bool MemExt::candoio ()
{
    if (! this->userflag) return true;
    setintreqmask (IRQ_USERINT);
    return false;
}

// called in middle of EXEC2 when doing an I/O instruction
//  input:
//   opcode<15:12> = zeroes
//   opcode<11:00> = opcode being executed
//   input<15:13>  = zeroes
//   input<12>     = existing LINK
//   input<11:00>  = existing AC
//  output:
//   returns<13> = 0: normal PC to execute next instruction
//                 1: increment PC to skip next instruction
//   returns<12> = new LINK value
//   returns<11:00> = new AC value
uint16_t MemExt::ioinstr (uint16_t opcode, uint16_t input)
{
    switch (opcode) {

        // skip if interrupt on (p 140 / v 3-14)
        // ...and turn interrupt off
        case 06000: {
            if (this->intenabled) input |= IO_SKIP;
            this->intenableafterfetch = this->intenabled = false;
            return input;
        }

        // interrupt turn on (p 140 / v 3-14)
        case 06001: {
            this->intenableafterfetch = true;
            return input;
        }

        // interrupt turn off (p 140 / v 3-14)
        case 06002: {
            this->intenableafterfetch = this->intenabled = false;
            return input;
        }

        // skip if interrupt request (p 140 / v 3-14)
        case 06003: {
            if (getintreqmask () != 0) input |= IO_SKIP;
            return input;
        }

        // get flags (p 140 / v 3-14)
        //      (also p 236 / v 7-16)
        //     <11> = LINK
        //     <10> = eae GT flag
        //     <09> = some interrupt request pending
        //     <08> = interrupts disabled until next jump
        //     <07> = interrupts enabled
        //     <06> = was in user mode at last interrupt
        //  <05:03> = iframe in effect at last interrupt
        //  <02:00> = dframe in effect at last interrupt
        case 06004: {
            return                     (input & IO_LINK) |
                         ((input & IO_LINK) ? 04000 : 0) |
                     (extarith.getgtflag () ? 02000 : 0) |
                          (getintreqmask () ? 01000 : 0) |
        //??    (this->intdisableduntiljump ? 00400 : 0) |  // 'NO INT' - must be 0 to pass DHMCA (memory-extension-time-share.pdf 2.10.1 p 2-11)
                          (this->intenabled ? 00200 : 0) |
                       (this->saveduserflag ? 00100 : 0) |
                               (this->savediframe >>  9) |
                               (this->saveddframe >> 12);
        }

        // restore flags
        //  p 236 / v 7-16
        case 06005: {
            input = (input & IO_DATA) | ((input & 04000) ? IO_LINK : 0);    // restore link
            extarith.setgtflag ((input & 02000) != 0);                      // restore eae gt
            this->intdisableduntiljump = true;                              // always disable ints until next jump
            this->intenableafterfetch = true;                               // always enable interrupts at next jump
            this->userflagafterjump = (input & 00100) != 0;                 // maybe restore user mode at next jump
            this->iframeafterjump = (input & 070) << 9;                     // restore iframe at next jump
            this->dframe = (input & 7) << 12;                               // restore dframe right now
            return input;
        }

        // skip if greater than
        case 06006: {
            if (extarith.getgtflag ()) input |= IO_SKIP;
            return input;
        }

        // clear all flags
        case 06007: {
            ::ioreset ();
            return 0;
        }

        // change to data field
        case 06201:
        case 06211:
        case 06221:
        case 06231:
        case 06241:
        case 06251:
        case 06261:
        case 06271: {
            this->dframe = ((opcode >> 3) & 7) << 12;
            return input;
        }

        // change both data & instr fields
        case 06203:
        case 06213:
        case 06223:
        case 06233:
        case 06243:
        case 06253:
        case 06263:
        case 06273: {
            this->dframe = ((opcode >> 3) & 7) << 12;
            // fallthrough
        }

        // change to instr field
        case 06202:
        case 06212:
        case 06222:
        case 06232:
        case 06242:
        case 06252:
        case 06262:
        case 06272: {
            this->iframeafterjump = ((opcode >> 3) & 7) << 12;
            this->intdisableduntiljump = true;
            return input;
        }

        // read data field
        case 06214: {
            return input | (this->dframe >> 9);
        }

        // read instr field (p 237 / v 7-17)
        case 06224: {
            return input | (this->iframe >> 9);
        }

        // read interrupt buffer (p 237 / v 7-17)
        case 06234: {
            return                         input |      // link,ac<11:07> preserved
               (this->saveduserflag ? 00100 : 0) |      //    ac<06> |= saved userflag
                       (this->savediframe >>  9) |      // ac<05:03> |= saved iframe
                       (this->saveddframe >> 12);       // ac<02:00> |= saved dframe
        }

        // restore memory field (p 237 / v 7-17)
        // restores iframe, dframe, usermode saved on entry to interrupt service
        // iframe, usermode effect delayed until next jump
        case 06244: {
            this->dframe = this->saveddframe;
            this->iframeafterjump = this->savediframe;
            this->userflagafterjump = this->saveduserflag;
            return input;
        }

        // clear user interrupt
        case 06204: {
            clrintreqmask (IRQ_USERINT);
            return input;
        }

        // skip on user interrupt
        case 06254: {
            if (getintreqmask () & IRQ_USERINT) input |= IO_SKIP;
            return input;
        }

        // clear user flag (p 240 / v 7-20)
        case 06264: {
            this->userflag = this->userflagafterjump = false;
            return input;
        }

        // set user flag (p 241 / v 7-21)
        case 06274: {
            this->userflagafterjump = true;
            this->intdisableduntiljump = true;
            return input;
        }
    }
    return UNSUPIO;
}
