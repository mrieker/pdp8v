
// page numbers in small-computer-handbook-1972.pdf

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
