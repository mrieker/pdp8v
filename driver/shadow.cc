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

// keeps track of what internal cpu state should be, cycle by cycle
// and verifies all connector pins cycle by cycle

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "abcd.h"
#include "controls.h"
#include "disassemble.h"
#include "gpiolib.h"
#include "iodevs.h"
#include "memext.h"
#include "memory.h"
#include "miscdefs.h"
#include "pindefs.h"
#include "shadow.h"

#if UNIPROC
#define INCCYCLE cycle ++
#else
#define INCCYCLE __atomic_add_fetch (&cycle, 1, __ATOMIC_RELAXED)
#endif

Shadow::Shadow ()
{
    printinstr = false;
    printstate = false;
    printfile  = NULL;
    printname  = NULL;
}

// gpiolib = instance of PhysLib: verify physical cpu board
//                       PipeLib: verify simulated cpu board
void Shadow::open (GpioLib *gpiolib)
{
    this->gpiolib = gpiolib;
    acknown = false;
    irknown = false;
    lnknown = false;
    maknown = false;
    cycle = 0;
    instr = 0;
}

// force cpu to FETCH1 state
void Shadow::reset ()
{
    r.state = FETCH1;
    r.pc    = 0;
    r.reset = true;     // this FETCH1 cycle is from a reset

    INCCYCLE;

    if (printstate) {
        fprintf (printfile, "%c%8llu STARTING STATE RESET   L=%d AC=%04o PC=%04o IR=%04o MA=%04o\n",
            (r.tsaver ? '#' : ' '), (LLU) cycle, r.link, r.ac, r.pc, r.ir, r.ma);
        pthread_mutex_lock (&gpiolib->trismutex);
        gpiolib->numtrisoff = 0;
        gpiolib->ntotaltris = 0;
        pthread_mutex_unlock (&gpiolib->trismutex);
    }
}

// process script 'cpu' commands
SCRet *Shadow::scriptcmd (int argc, char const *const *argv)
{
    // get [ac/cycle/ir/link/ma/pc/state/tsaver/disas]
    if (strcmp (argv[0], "get") == 0) {
        if (argc == 1) {
            char const *dis;
            std::string dsm;
            if (this->r.state != FETCH1) {
                dis = iodisas (this->r.ir);
                if (dis == NULL) {
                    dsm = disassemble (this->r.ir, this->r.irpc);
                    dis = dsm.c_str ();
                }
            } else {
                dis = this->r.reset ? "RESET" : "";
            }
            return new SCRetStr ("ac=%05o;cycle=%llu;instr=%llu;ir=%05o;link=%o;ma=%05o;pc=%05o;state=%s;tsaver=%d;disas=%s",
                this->r.ac, (LLU) this->getcycles (), (LLU) this->getinstrs(), this->r.ir, this->r.link, this->r.ma, this->r.pc,
                statestr (this->r.state), this->r.tsaver, dis);
        }

        if (argc == 2) {
            if (strcmp (argv[1], "ac")     == 0) return new SCRetInt  (this->r.ac);
            if (strcmp (argv[1], "cycle")  == 0) return new SCRetLong (this->getcycles ());
            if (strcmp (argv[1], "instr")  == 0) return new SCRetLong (this->getinstrs ());
            if (strcmp (argv[1], "ir")     == 0) return new SCRetInt  (this->r.ir);
            if (strcmp (argv[1], "link")   == 0) return new SCRetInt  (this->r.link);
            if (strcmp (argv[1], "ma")     == 0) return new SCRetInt  (this->r.ma);
            if (strcmp (argv[1], "pc")     == 0) return new SCRetInt  (this->r.pc);
            if (strcmp (argv[1], "state")  == 0) return new SCRetStr  (statestr (this->r.state));
            if (strcmp (argv[1], "tsaver") == 0) return new SCRetInt  (this->r.tsaver);
            if (strcmp (argv[1], "disas")  == 0) {
                char const *dis;
                std::string dsm;
                if (this->r.state != FETCH1) {
                    dis = iodisas (this->r.ir);
                    if (dis == NULL) {
                        dsm = disassemble (this->r.ir, this->r.irpc);
                        dis = dsm.c_str ();
                    }
                } else {
                    dis = this->r.reset ? "RESET" : "";
                }
                return new SCRetStr (dis);
            }
        }

        return new SCRetErr ("cpu get [ac/cycle/instr/ir/link/ma/pc/state/tsaver/disas]");
    }

    if (strcmp (argv[0], "help") == 0) {
        puts ("");
        puts ("valid sub-commands:");
        puts ("");
        puts ("  get            - return all registers as a string");
        puts ("  get <register> - return specific register as a value");
        puts ("  history        - print out previous cycles");
        puts ("                   # at beginning of line indicates tubesaver cycle");
        puts ("");
        puts ("registers:");
        puts ("");
        puts ("  ac     - accumulator");
        puts ("  cycle  - cycle count");
        puts ("  instr  - instruction count");
        puts ("  ir     - instruction register (full 12 bits)");
        puts ("  link   - link bit");
        puts ("  ma     - memory address");
        puts ("  pc     - program counter");
        puts ("  state  - current state as a string (if halted, processor is at end of this state)");
        puts ("  tsaver - cycle was a result of tubesaver mode");
        puts ("  disas  - disassembly of instruction register");
        puts ("");
        puts ("this get command shows shadow state, ie, what the tubes *should* be");
        puts ("- use gpio get to see what the tubes actually are");
        puts ("");
        return NULL;
    }

    // print out previous cycles
    if (strcmp (argv[0], "history") == 0) {
        if (! ctl_ishalted ()) return new SCRetErr ("processor not halted");
        puts ("");
        this->history (stdout, "  ", 1);
        puts ("");
        return NULL;
    }

    return new SCRetErr ("unknown cpu command %s - valid: get help history", argv[0]);
}

// clock to next state
// call at end of cycle just before raising clock
//  input:
//   sample = gpio pins
void Shadow::clock (uint32_t sample)
{
    // decode what we need from the sample
    uint16_t mq = (sample & G_DATA) / G_DATA0;  // memory data sent to CPU during last cycle (if G_QENA is set)
    bool irq = (sample & G_IRQ) != 0;           // interrupt request line sent to CPU during last cycle

    // at end of state ... do ... then go to state ...
    switch (r.state) {

        // getting PC from processor
        case FETCH1: {
            checkgpio (sample, G_DENA | G_READ | r.pc * G_DATA0);
            r.state = FETCH2;
            r.reset = false;
            break;
        }

        // sending opcode to processor
        case FETCH2: {
            r.ir    = mq;
            r.irpc  = r.pc;
            irknown = true;
            checkgpio (sample, G_QENA);
            r.ma = mq & 00177;                                // always get low 7 bits from memory
            if ((mq & 06000) == 06000) r.ma |= mq & 07600;    // non-memory instr, get top 5 bits as well
                  else if (mq & 00200) r.ma |= r.pc & 07600;  // memory pc page, get top 5 bits from pc
            maknown = true;
            if (printinstr | printstate) {
                char buf[4000];
                char *ptr = buf;
                uint16_t xpc = memext.iframe | r.pc;
                {
                    std::string disstr;
                    char const *dis = iodisas (r.ir);
                    if (dis == NULL) {
                        disstr = disassemble (r.ir, r.pc);
                        dis = disstr.c_str ();
                    }
                    ASSERT (ptr + 100 + strlen (dis) < buf + sizeof buf);
                    ptr += sprintf (ptr, "%c%8llu L=%d AC=%04o PC=%05o IR=%04o : %s",
                            (r.tsaver ? '#' : ' '), (LLU) cycle, r.link, r.ac, xpc, r.ir, dis);
                }
                if ((r.ir >> 9) < 6) {
                    uint16_t xma = memext.iframe | r.ma;
                    ASSERT (xma < MEMSIZE);
                    if (r.ir & 00400) {
                        uint16_t def = memarray[xma];
                        ptr += sprintf (ptr, " =%05o/%04o", xma, def);
                        if ((xma & 07770) == 00010) {
                            def  = (def + 1) & 07777;
                            ptr += sprintf (ptr, ">%04o", def);
                        }
                        xma = ((r.ir & 04000) ? memext.iframe : memext.dframe) | def;
                        ASSERT (xma < MEMSIZE);
                    }
                    ptr += sprintf (ptr, " =%05o", xma);
                    if ((r.ir >> 9) <= 2) ptr += sprintf (ptr, "/%04o", memarray[xma]);               // AND,TAD,ISZ
                    if ((r.ir >> 9) == 2) ptr += sprintf (ptr, ">%04o", (memarray[xma] + 1) & 07777); // ISZ
                    if ((r.ir >> 9) == 3) ptr += sprintf (ptr, "<%04o", r.ac);                        // DCA
                }
                ASSERT (ptr < buf + sizeof buf - 1);
                strcpy (ptr, "\n");
                fputs (buf, printfile);
            }
            r.pc = (r.pc + 1) & 07777;
            if ((r.ir < 06000) && (r.ir & 00400)) {
                r.state = DEFER1;
            } else {
                r.state = firstexecstate ();
            }
            #if UNIPROC
                instr ++;
            #else
                __atomic_add_fetch (&instr, 1, __ATOMIC_RELAXED);
            #endif
            break;
        }

        // getting MA from processor
        case DEFER1: {
            bool autoidx = (r.ma & 07770) == 010;
            checkgpio (sample, G_DENA | (autoidx ? G_WRITE : 0) | G_READ | r.ma * G_DATA0);
            r.state = DEFER2;
            break;
        }

        // sending unincremented pointer to processor
        case DEFER2: {
            checkgpio (sample, G_QENA);
            bool autoidx = (r.ma & 07770) == 010;
            r.ma = mq;
            if (autoidx) {
                r.state = DEFER3;
            } else {
                r.state = firstexecstate ();
            }
            break;
        }

        // receiving incremented pointer from processor
        case DEFER3: {
            uint16_t aluq = (r.ma + 1) & 07777;
            checkgpio (sample, G_DENA | aluq * G_DATA0);
            r.ma = aluq;
            r.state = firstexecstate ();
            break;
        }

        // receiving MA from processor for a memory read cycle
        case ARITH1: {
            checkgpio (sample, G_DENA | ((r.ir & 00400) ? G_DFRM : 0) | G_READ | r.ma * G_DATA0);
            r.state = ((r.ir & 01000) && (r.ac != 0)) ? TAD2 : AND2;
            break;
        }

        // sending operand to processor
        // it performs arithmetic & updates accumulator
        case AND2: {
            checkgpio (sample, G_QENA);
            r.ac = mq & ((r.ir & 01000) ? 07777 : r.ac);
            if (mq == 0) acknown = true;
            r.state = endOfInst (irq);
            break;
        }

        // sending operand to processor
        // it saves operand value in MA
        case TAD2: {
            checkgpio (sample, G_QENA);
            r.ma = mq;
            r.state = TAD3;
            break;
        }

        // it performs arithmetic & updates link and accumulator
        case TAD3: {
            uint16_t newac = r.ac + r.ma;
            checkgpio (sample, G_DENA | (newac & 07777) * G_DATA0);
            r.ac    = newac & 07777;
            r.link ^= newac > 07777;
            r.state = endOfInst (irq);
            break;
        }

        // receiving MA from processor
        case ISZ1: {
            checkgpio (sample, G_DENA | ((r.ir & 00400) ? G_DFRM : 0) | G_WRITE | G_READ | r.ma * G_DATA0);
            r.state = ISZ2;
            break;
        }

        // sending unincremented operand to processor
        case ISZ2: {
            checkgpio (sample, G_QENA);
            r.ma = mq;
            r.state = ISZ3;
            break;
        }

        // receiving incremented operand from processor
        case ISZ3: {
            uint32_t aluq = (r.ma + 1) & 07777;
            checkgpio (sample, G_DENA | aluq * G_DATA0);
            if (aluq == 0) r.pc = (r.pc + 1) & 07777;
            r.state = endOfInst (irq);
            break;
        }

        // receiving MA from processor
        case DCA1: {
            checkgpio (sample, G_DENA | ((r.ir & 00400) ? G_DFRM : 0) | G_WRITE | r.ma * G_DATA0);
            r.state = DCA2;
            break;
        }

        // receiving AC from processor and clearing AC
        case DCA2: {
            checkgpio (sample, G_DENA | r.ac * G_DATA0);
            r.ac = 0;
            acknown = true;
            r.state = endOfInst (irq);
            break;
        }

        // neither sending nor receiving but should see MA coming from ALU
        // if no interrupt request, should be reading and acting like fetch1
        case JMP1: {
            checkgpio (sample, G_DENA | r.ma * G_DATA0 | G_JUMP | (irq ? 0 : G_READ));
            r.pc = r.ma;
            r.state = irq ? INTAK1 : FETCH2;
            break;
        }

        // receiving MA from processor for a write cycle
        case JMS1: {
            checkgpio (sample, G_DENA | G_WRITE | r.ma * G_DATA0 | G_JUMP);
            r.state = JMS2;
            break;
        }

        // receive PC from processor
        case JMS2: {
            checkgpio (sample, G_DENA | r.pc * G_DATA0);
            r.state = JMS3;
            break;
        }

        // neither sending nor receiving but should see MA+1 coming from ALU
        // if no interrupt request, should be reading and acting like fetch1
        case JMS3: {
            uint16_t aluq = (r.ma + 1) & 07777;
            checkgpio (sample, G_DENA | aluq * G_DATA0 | (irq ? 0 : G_READ));
            r.pc = aluq;
            r.state = irq ? INTAK1 : FETCH2;
            break;
        }

        // receiving LINK,AC from processor
        // could be real IOT, group 2 with OSR or HLT, or group 3
        case IOT1: {
            checkgpio (sample, G_DENA | G_IOIN | (r.link ? G_LINK : 0) | r.ac * G_DATA0);
            r.state = IOT2;
            break;
        }

        // sending updated IOS,LINK,AC to processor
        case IOT2: {
            checkgpio (sample, G_QENA);
            r.ac = mq;
            r.link = (sample & G_LINK) != 0;
            if (sample & G_IOS) r.pc = (r.pc + 1) & 07777;
            acknown = true;
            lnknown = true;
            r.state = endOfInst (irq);
            break;
        }

        // neither sending nor receiving but should see unrotated result coming from ALU
        case GRPA1: {
            uint16_t aluq;
            uint16_t rotq = computegrpa (&aluq);
            checkgpio (sample, G_DENA | (aluq * G_DATA0));
            r.ac    = rotq & 07777;
            r.link  = (rotq >> 12) & 1;
            if (r.ir & 00200) acknown = true;     // CLA
            if (r.ir & 00100) lnknown = true;     // CLL
            r.state = endOfInst (irq);
            break;
        }

        // should see possibly incremented PC coming from ALU
        // if no interrupt request, should see a read request to fetch next instruction
        case GRPB1: {
            uint16_t newpc = doesgrpbskip () ? (r.pc + 1) & 07777 : r.pc;
            checkgpio (sample, G_DENA | newpc * G_DATA0 | (irq ? 0 : G_READ));
            r.pc = newpc;
            if (r.ir & 00200) {
                r.ac = 0;
                acknown = true;
            }
            r.state = irq ? INTAK1 : FETCH2;
            break;
        }

        // receiving 0 from processor saying to write PC to location 0
        // - interrupts were disabled by raspictl main() because of the G_IAK
        // - this cycle is just like the JMS1 of a 'JMS 0'
        case INTAK1: {
            r.ir = 04000;
            checkgpio (sample, G_DENA | G_IAK | G_WRITE | 0);
            if (printinstr) {
                uint16_t xpc = memext.iframe | r.pc;
                fprintf (printfile, "%c%8llu L=%d AC=%04o PC=%05o           INTERRUPT\n",
                        (r.tsaver ? '#' : ' '), (LLU) cycle, r.link, r.ac, xpc);
            }
            r.ma = 0;
            r.state = JMS2;
            break;
        }

        default: ABORT ();
    }

    INCCYCLE;

    if (printstate) {
        pthread_mutex_lock (&gpiolib->trismutex);
        uint64_t ntt = gpiolib->ntotaltris;
        uint64_t nto = ntt - gpiolib->numtrisoff;
        gpiolib->numtrisoff = 0;
        gpiolib->ntotaltris = 0;
        pthread_mutex_unlock (&gpiolib->trismutex);
        char line[96];
        int rc = snprintf (line, sizeof line,
            "%c%8llu STARTING STATE %-6s  L=%o AC=%04o PC=%04o IR=%04o MA=%04o",
            (r.tsaver ? '#' : ' '), (LLU) cycle, statestr (r.state), r.link, r.ac, r.pc, r.ir, r.ma);
        ASSERT (rc > 0);
        if (ntt > 0) {
            rc += snprintf (line + rc, sizeof line - rc, "  trison=%llu/%llu=%5.2f%%",
                (LLU) nto / 2, (LLU) ntt / 2, nto * 100.0 / ntt);
        }
        puts (line);
    }
}

// compute new value resulting from a Group 1 instruction
//  input:
//   ir = Group 1 instruction
//   ac = accumulator contents
//   link = link state
//  output:
//   returns<12> = new link state
//          <11:00> = new accumulator contents
//   *aluq = unrotated result
uint16_t Shadow::computegrpa (uint16_t *aluq)
{
    uint16_t newac = r.ac;
    bool newlink = r.link;

    // group 1 (p 131 / v 3-5)
    if (r.ir & 00200) newac = 0;              // CLA
    if (r.ir & 00100) newlink = false;        // CLL
    if (r.ir & 00040) newac = newac ^ 07777;  // CMA
    if (r.ir & 00020) newlink = ! newlink;    // CML
    if (r.ir & 00001) {                       // IAC
        newac = (newac + 1) & 07777;
        if (newac == 0) newlink = ! newlink;
    }
    *aluq = newac;
    switch ((r.ir >> 1) & 7) {
        case 0: break;                      // NOP
        case 1: {                           // BSW
            newac = ((newac & 077) << 6) | (newac >> 6);
            break;
        }
        case 2: {                           // RAL
            newac = (newac << 1) | (newlink ? 1 : 0);
            newlink = (newac & 010000) != 0;
            newac &= 07777;
            break;
        }
        case 3: {                           // RTL
            newac = (newac << 2) | (newlink ? 2 : 0) | (newac >> 11);
            newlink = (newac & 010000) != 0;
            newac &= 07777;
            break;
        }
        case 4: {                           // RAR
            uint16_t oldac = newac;
            newac = (newac >> 1) | (newlink ? 04000 : 0);
            newlink = (oldac & 1) != 0;
            break;
        }
        case 5: {                           // RTR
            newac = (newac >> 2) | (newlink ? 02000 : 0) | ((newac & 3) << 11);
            newlink = (newac & 010000) != 0;
            newac &= 07777;
            break;
        }
        default: {                          // 6,7: undefined
            if (! quiet && ! r.tsaver) fprintf (stderr, "Shadow::computegrpa: undefined rotate opcode %04o at %05o\n", r.ir, lastreadaddr);
            newac = 07777;                  // hardware has open circuit
            newlink = 1;
            break;
        }
    }

    return (newlink ? 010000 : 0) | newac;
}

// compute if a Group 2 instruction would skip
//  input:
//   ir = Group 2 instruction
//   ac = accumulator contents
//   link = link state
//  output:
//   returns true iff it would skip
bool Shadow::doesgrpbskip ()
{
    // group 2 (p 134 / v 3-8)

    if (! acknown && (r.ir & 0140)) {
        fprintf (stderr, "Shadow::doesgrpbskip: %05o sma sza with ac unknown\n", memext.iframe | r.irpc);
        ABORT ();
    }
    if (! lnknown && (r.ir & 0020)) {
        fprintf (stderr, "Shadow::doesgrpbskip: %05o snl with link unknown\n", memext.iframe | r.irpc);
        ABORT ();
    }

    bool skip = false;
    if ((r.ir & 0100) && (r.ac & 04000)) skip = true;     // SMA
    if ((r.ir & 0040) && (r.ac ==    0)) skip = true;     // SZA
    if ((r.ir & 0020) &&         r.link) skip = true;     // SNL
    if  (r.ir & 0010)                    skip = ! skip;   // reverse
    return skip;
}

// check that the gpio pins at the end of current state are what we expect them to be
//   always verify all the input pins coming from the processor into the raspberry pi
//   always verify all the output pins (signals generated by the raspberry pi) except for G_IOS and G_IRQ
//     G_IOS only matters in IOT2 and that state checks it
//     G_IOS is a don't care for all other states
//     G_IRQ is checked indirectly by endOfInst() when it counts
//   if G_DENA is set (meaning raspberry pi is reading the MD and MDL bits from processor), verify those as well
//   if G_DENA is clear (meaning raspberry pi is sending data out on MQ and MQL),
//     don't verify those as they are data coming from the raspberry pi (they contain memory or i/o data coming from raspi)
void Shadow::checkgpio (uint32_t sample, uint32_t expect)
{
    bool gpiobad = false;
    bool padsbad = false;

    // save registers in case of error or for 'cpu history' command
    saveregs[cycle%NSAVEREGS] = r;

    // check all the input and output pins except the G_IOS pin
    // G_IOS only counts at end of IOT2 and gets checked there
    // G_IRQ gets checked indirectly via endOfInst()
    uint32_t mask = (G_OUTS | G_INS) & ~ G_IOS & ~ G_IRQ;

    // check the G_DATA and G_LINK lines if we are receiving them from the processor
    if (expect & G_DENA) {
        mask |= G_DATA | G_LINK;
        if (r.link) expect |= G_LINK;
             else expect &= ~ G_LINK;
    }

    // if link state unknown (ie, just after reset), don't verify it
    if (! lnknown) mask &= ~ G_LINK;

    // check and abort if error
    uint32_t diff = (sample ^ expect) & mask;
    if (diff != 0) {
        fprintf (stderr, "Shadow::checkgpio: end of %s  sample %08X  mask %08X  expect %08X  diff %08X\n", statestr (r.state), sample, mask, expect, diff);
        fprintf (stderr, "Shadow::checkgpio:   sample %s\n", GpioLib::decogpio (sample).c_str ());
        fprintf (stderr, "Shadow::checkgpio:   expect %s\n", GpioLib::decogpio (expect).c_str ());
        gpiobad = true;
    }

    // maybe there are paddles to check
    if (paddles) {
        ABCD abcdmask, abcdvals;

        uint16_t gdata = (sample & G_DATA) / G_DATA0;

        abcdmask.pcq = 07777;           abcdvals.pcq = r.pc;
        abcdmask.fetch1q = true;        abcdvals.fetch1q = (r.state == FETCH1);
        abcdmask.fetch2q = true;        abcdvals.fetch2q = (r.state == FETCH2);
        abcdmask.defer1q = true;        abcdvals.defer1q = (r.state == DEFER1);
        abcdmask.defer2q = true;        abcdvals.defer2q = (r.state == DEFER2);
        abcdmask.defer3q = true;        abcdvals.defer3q = (r.state == DEFER3);
        abcdmask.exec1q  = true;        abcdvals.exec1q  = ((r.state & 15) == (ISZ1 & 15));
        abcdmask.exec2q  = true;        abcdvals.exec2q  = ((r.state & 15) == (ISZ2 & 15));
        abcdmask.exec3q  = true;        abcdvals.exec3q  = ((r.state & 15) == (ISZ3 & 15));
        abcdmask.intak1q = true;        abcdvals.intak1q = (r.state == INTAK1);
        abcdmask.ioskp   = true;        abcdvals.ioskp   = (sample & G_IOS)   != 0;
        abcdmask._jump   = true;        abcdvals._jump   = (sample & G_JUMP)  == 0;
        abcdmask.clok2   = true;        abcdvals.clok2   = (sample & G_CLOCK) != 0;
        abcdmask._dfrm   = true;        abcdvals._dfrm   = (sample & G_DFRM)  == 0;
        abcdmask._intak  = true;        abcdvals._intak  = (sample & G_IAK)   == 0;
        abcdmask.intrq   = true;        abcdvals.intrq   = (sample & G_IRQ)   != 0;
        abcdmask.ioinst  = true;        abcdvals.ioinst  = (sample & G_IOIN)  != 0;
        abcdmask._mread  = true;        abcdvals._mread  = (sample & G_READ)  == 0;
        abcdmask._mwrite = true;        abcdvals._mwrite = (sample & G_WRITE) == 0;
        abcdmask.reset   = true;        abcdvals.reset   = (sample & G_RESET) != 0;

        abcdmask._ac_sc   = true;       abcdvals._ac_sc   = ! ((r.state == DCA2) || ((r.state == GRPB1) && (r.ir & 00200)));
        abcdmask._ac_aluq = true;       abcdvals._ac_aluq = ! ((r.state == AND2) || (r.state == TAD3) || (r.state == IOT2) || (r.state == GRPA1) || ! abcdvals._ac_sc);
        abcdmask._alu_add = true;       abcdvals._alu_add = ! ((r.state == DEFER3) || (r.state == JMS3) || (r.state == ISZ3) || (r.state == TAD3) || (r.state == GRPB1));
        abcdmask._alu_and = true;       abcdvals._alu_and = ! (abcdvals._alu_add & (r.state != GRPA1));
        abcdmask.inc_axb  = true;       abcdvals.inc_axb  = (r.state == GRPA1) && (r.ir & 1);

        abcdmask._alua_m1 = true;       abcdvals._alua_m1 = ! ((r.state == DCA2) || (r.state == IOT1) || ((r.state == GRPA1) && (r.ir & 00040)));
        abcdmask._alua_ma = true;       abcdvals._alua_ma = ! ((r.state == DEFER1) || (r.state == DEFER3) || (r.state == ARITH1) || (r.state == ISZ1) || (r.state == DCA1) || (r.state == JMS1) || (r.state == JMP1) || (r.state == TAD3) || (r.state == JMS3) || (r.state == ISZ3));
        abcdmask.alua_mq0600 = true;    abcdvals.alua_mq0600 = (r.state == FETCH2) || (r.state == DEFER2) || (r.state == AND2) || (r.state == TAD2) || (r.state == ISZ2) || (r.state == IOT2);
        abcdmask.alua_mq1107 = true;    abcdvals.alua_mq1107 = ((r.state == FETCH2) && ((r.ir & 06000) == 06000)) || (r.state == DEFER2) || (r.state == AND2) || (r.state == TAD2) || (r.state == ISZ2) || (r.state == IOT2);
        abcdmask.alua_pc0600 = true;    abcdvals.alua_pc0600 = (r.state == FETCH1) || (r.state == JMS2) || (r.state == GRPB1);
        abcdmask.alua_pc1107 = true;    abcdvals.alua_pc1107 = (r.state == FETCH1) || ((r.state == FETCH2) && ((r.ir & 06000) != 06000) && (gdata & 00200)) || (r.state == JMS2) || (r.state == GRPB1);

        abcdmask._alub_m1 = true;       abcdvals._alub_m1 = ! ((r.state == FETCH1) || (r.state == FETCH2) || (r.state == DEFER1) || (r.state == DEFER2) || (r.state == JMP1) || (r.state == JMS1) || (r.state == JMS2) || (r.state == ARITH1) || (r.state == TAD2) || (r.state == DCA1) || (r.state == ISZ1) || (r.state == ISZ2) || (r.state == IOT2) || ((r.state == AND2) && (r.ir & 01000)));
        abcdmask._alub_ac = true;       abcdvals._alub_ac = ! ((r.state == AND2) || (r.state == TAD3) || (r.state == DCA2) || (r.state == IOT1) || ((r.state == GRPA1) & ! (r.ir & 00200)));

        abcdmask.iot2q    = true;       abcdvals.iot2q    = (r.state == IOT2);
        abcdmask.tad3q    = true;       abcdvals.tad3q    = (r.state == TAD3);
        abcdmask._grpa1q  = true;       abcdvals._grpa1q  = (r.state != GRPA1);

        abcdmask._ln_wrt  = true;       abcdvals._ln_wrt  = ! ((r.state == TAD3) || (r.state == GRPA1) || (r.state == IOT2));
        abcdmask._ma_aluq = true;       abcdvals._ma_aluq = ! ((r.state == FETCH2) || (r.state == DEFER2) || (r.state == DEFER3) || (r.state == TAD2) || (r.state == ISZ2) || (r.state == INTAK1));
        abcdmask._pc_aluq = true;       abcdvals._pc_aluq = ! ((r.state == JMP1) || (r.state == JMS3) || (r.state == GRPB1));

        if (acknown) {
            abcdmask.acq = 07777;           abcdvals.acq = r.ac;
            abcdmask.acqzero = true;        abcdvals.acqzero = (r.ac == 0);
        }
        if (irknown) {
            abcdmask.irq = 07000;           abcdvals.irq = r.ir;
        }
        if (lnknown) {
            abcdmask.lnq = true;            abcdvals.lnq = r.link;
            abcdmask._lnq = true;           abcdvals._lnq = ! r.link;
        }
        if (maknown) {
            abcdmask.maq = 07777;           abcdvals.maq = r.ma;
            abcdmask._maq = 07777;          abcdvals._maq = r.ma ^ 07777;
        }

        if (maknown) {
            abcdmask.grpb_skip = true;      abcdvals.grpb_skip = false;
            if (r.ma & 00100) {   // ac neg
                abcdmask.grpb_skip &= acknown;  abcdvals.grpb_skip |= (r.ac & 04000) != 0;
            }
            if (r.ma & 00040) {   // ac zero
                abcdmask.grpb_skip &= acknown;  abcdvals.grpb_skip |= (r.ac == 0);
            }
            if (r.ma & 00020) {   // link set
                abcdmask.grpb_skip &= lnknown;  abcdvals.grpb_skip |= r.link;
            }
            if (r.ma & 00010) {   // reverse sense
                                                abcdvals.grpb_skip = ! abcdvals.grpb_skip;
            }
        }

        if (r.state != GRPB1) {
            abcdmask.alub_1 = true;         abcdvals.alub_1 = (r.state == DEFER3) || (r.state == JMS3) || (r.state == ISZ3);
        } else if (abcdmask.grpb_skip) {
            abcdmask.alub_1 = true;         abcdvals.alub_1 = abcdvals.grpb_skip;
        }

        if ((r.state == FETCH2) || (r.state == DEFER2) || (r.state == AND2) || (r.state == TAD2) || (r.state == ISZ2) || (r.state == IOT2)) {
            // sample has what we are actually sending to tubes over gpio connector
            // verify that the signals make it to the paddles
            ASSERT (sample & G_QENA);
            abcdmask.mq = 07777;            abcdvals.mq = gdata;
            abcdmask.mql = true;            abcdvals.mql = (sample & G_LINK) != 0;
        }

        if ((maknown | abcdvals._alua_ma) & (acknown | abcdvals._alub_ac)) {
            uint16_t alua = (abcdvals._alua_m1 ? 0 : 07777) | (abcdvals._alua_ma ? 0 : r.ma) | (abcdvals.alua_mq0600 ? gdata & 00177 : 0) | (abcdvals.alua_mq1107 ? gdata & 07600 : 0) | (abcdvals.alua_pc0600 ? r.pc & 00177 : 0) | (abcdvals.alua_pc1107 ? r.pc & 07600 : 0);
            uint16_t alub = (abcdvals._alub_m1 ? 0 : 07777) | (abcdvals._alub_ac ? 0 : r.ac) | (abcdvals.alub_1 ? 1 : 0);

            uint16_t sum = (alua + alub) + (abcdvals.inc_axb ? 1 : 0);
            uint16_t inc = (alua ^ alub) + (abcdvals.inc_axb ? 1 : 0);
            bool cin_add_12 = sum > 07777;
            bool cin_inc_12 = inc > 07777;

            uint16_t aluq = (abcdvals._alu_and ? 0 : alua & alub) | (abcdvals._alu_add ? 0 : sum & 07777) | (abcdvals._grpa1q ? 0 : inc & 07777);

            abcdmask._aluq = 07777;         abcdvals._aluq = aluq ^ 07777;
            abcdmask._alucout = true;       abcdvals._alucout = ! ((! abcdvals._alu_add & cin_add_12) | (! abcdvals._grpa1q & cin_inc_12));

            if (lnknown) {
                bool oldlink = (r.state != GRPA1) ? r.link :
                    (((r.ir & 00120) == 00000) ?    r.link :
                    (((r.ir & 00120) == 00020) ?  ! r.link :
                    (((r.ir & 00120) == 00120))));
                abcdmask._newlink = true;       abcdvals._newlink = oldlink ^ abcdvals._alucout;
            }
        }

        if ((r.state != ISZ3) || abcdmask._alucout) {
            abcdmask._pc_inc = true;        abcdvals._pc_inc  = ! ((r.state == FETCH2) || ((r.state == ISZ3) && ! abcdvals._alucout) || ((r.state == IOT2) && abcdvals.ioskp));
        }

        abcdmask.encode ();             abcdvals.encode ();

        uint32_t diffs[NPADS], pinss[NPADS];
        gpiolib->readpads (pinss);
        for (int j = 0; j < NPADS; j ++) {
            diffs[j] = (pinss[j] ^ abcdvals.cons[j]) & abcdmask.cons[j];
            padsbad |= diffs[j] != 0;
        }
        if (padsbad) {
            fprintf (stderr, "Shadow::checkgpio: paddle mismatch, cycle %llu\n", (LLU) cycle);
            for (int j = 0; j < NPADS; j ++) {
                if (diffs[j] != 0) {
                    PinDefs const *xpindefs = pindefss[j];
                    for (int p = 1; p <= 32; p ++) {
                        uint32_t m = 1U << (p - 1);
                        if (diffs[j] & m) {
                            for (int i = 0; xpindefs[i].varname != NULL; i ++) {
                                if (xpindefs[i].pinmask & m) {
                                    fprintf (stderr, "Shadow::checkgpio:  PIN %c<%02d>: %12s is %s should be %s\n",
                                        j + 'A', p, xpindefs[i].varname,
                                                ((pinss[j] & m) ? "HIGH" : "LOW "),
                                        ((abcdvals.cons[j] & m) ? "HIGH" : "LOW "));
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    if (gpiobad || padsbad) {
        this->history (stderr, "Shadow::checkgpio: ", 0);
        StateMismatchException sme;
        throw sme;
    }
}

// print cycle history
//  input:
//   out = file to write history to
//   pfx = prefix for lines
//   offset = 0: saveregs[cycle%NSAVEREGS] has current cycle
//            1: saveregs[cycle%NSAVEREGS] has an old cycle
void Shadow::history (FILE *out, char const *pfx, unsigned offset)
{
    char const *distc = NULL;
    std::string distr;
    uint16_t disir = 0xFFFFU;
    uint16_t dispc = 0xFFFFU;

    for (unsigned i = offset; (i < NSAVEREGS + offset) && (i < cycle); i ++) {
        uint64_t c = cycle - i;
        Regs const *p = &saveregs[c%NSAVEREGS];
        if (p->state == FETCH1) {
            distc = p->reset ? "RESET" : "";
            disir = 0xFFFFU;
            dispc = 0xFFFFU;
        } else if ((disir != p->ir) || (dispc != p->irpc)) {
            disir = p->ir;
            dispc = p->irpc;
            distc = iodisas (p->ir);
            if (distc == NULL) {
                distr = disassemble (p->ir, p->irpc);
                distc = distr.c_str ();
            }
        }
        fprintf (out, "%s%c%8llu  %-6s  L.AC=%o.%04o IR=%04o MA=%04o PC=%04o  %s\n",
            pfx, (p->tsaver ? '#' : ' '), (LLU) c, statestr (p->state), p->link, p->ac, p->ir, p->ma, p->pc, distc);
    }
}

Shadow::State Shadow::getprevstate ()
{
    return saveregs[(cycle-1)%NSAVEREGS].state;
}

char const *Shadow::StateMismatchException::what ()
{
    return "shadow detected state mismatch on GPIO and/or paddles";
}

Shadow::State Shadow::firstexecstate ()
{
    switch (r.ir >> 9) {
        case 0:
        case 1: return ARITH1;
        case 2: return ISZ1;
        case 3: return DCA1;
        case 4: return JMS1;
        case 5: return JMP1;
        case 6: return IOT1;
        case 7: return (r.ir & 0400) ? ((r.ir & 0007) ? IOT1 : GRPB1) : GRPA1;
        default: ABORT ();
    }
}

// return whether this cycle is using the alu adder
bool Shadow::aluadd ()
{
    return (r.state == DEFER3) ||
        (r.state == GRPA1) ||
        (r.state == GRPB1) ||
        (((uint32_t)r.state & 15) == ((uint32_t)TAD3 & 15));
}

uint64_t Shadow::getcycles ()
{
#if UNIPROC
    return cycle;
#else
    return __atomic_load_n (&cycle, __ATOMIC_RELAXED);
#endif
}

uint64_t Shadow::getinstrs ()
{
#if UNIPROC
    return instr;
#else
    return __atomic_load_n (&instr, __ATOMIC_RELAXED);
#endif
}

// end of instruction's last cycle, transition to either fetch or start interrupt
Shadow::State Shadow::endOfInst (bool irq)
{
    return irq ? INTAK1 : FETCH1;
}

char const *Shadow::boolstr (bool b)
{
    return b ? "true" : "false";
}

char const *Shadow::statestr (State s)
{
    switch (s) {
        case FETCH1: return "FETCH1";
        case FETCH2: return "FETCH2";
        case DEFER1: return "DEFER1";
        case DEFER2: return "DEFER2";
        case DEFER3: return "DEFER3";
        case ARITH1: return "ARITH1";
        case AND2:   return "AND2";
        case TAD2:   return "TAD2";
        case TAD3:   return "TAD3";
        case ISZ1:   return "ISZ1";
        case ISZ2:   return "ISZ2";
        case ISZ3:   return "ISZ3";
        case DCA1:   return "DCA1";
        case DCA2:   return "DCA2";
        case JMP1:   return "JMP1";
        case JMS1:   return "JMS1";
        case JMS2:   return "JMS2";
        case JMS3:   return "JMS3";
        case IOT1:   return "IOT1";
        case IOT2:   return "IOT2";
        case GRPA1:  return "GRPA1";
        case GRPB1:  return "GRPB1";
        case INTAK1: return "INTAK1";
        case DMA:    return "DMA";
    }
    return "bad state enum";
}
