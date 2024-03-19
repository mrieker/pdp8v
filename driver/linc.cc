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

// emulate LINC instructions hopefully enough for the FOCAL-12 interpreter
// triggered by LINC (6141) i/o instruction when in PDP-8 mode
// stays in here until a PDP instruction is executed to return to PDP-8 mode

#include "extarith.h"
#include "iodevtty.h"
#include "linc.h"
#include "memext.h"
#include "memory.h"
#include "shadow.h"

Linc linc;

static IODevOps const iodevops[] = {
    { 06141, "LINC switch to Linc mode" },
};

Linc::Linc ()
{
    opscount = sizeof iodevops / sizeof iodevops[0];
    opsarray = iodevops;

    iodevname = "linc";
}

// reset to power-on state
void Linc::ioreset ()
{
    this->lincac       = 0;
    this->lincpc       = 0;
    this->specfuncs    = 0;
    this->disabjumprtn = false;
    this->link         = false;
    this->flow         = false;
}

// called in middle of EXEC2 when doing an I/O instruction
// returns in middle of DEFER2 doing a 'JMP @ 0' instruction
//  input:
//   opcode<15:12> = zeroes
//   opcode<11:00> = opcode being executed
//   input<15:13>  = zeroes
//   input<12>     = existing LINK
//   input<11:00>  = existing AC
//  output:
//   returns<15:12> = zeroes (send zeroes on G_LINK and G_IOS lines)
//   returns<11:00> = PC value to send to tubes on G_DATA lines
uint16_t Linc::ioinstr (uint16_t opcode, uint16_t input)
{
    switch (opcode) {

        // LINC - 6141 - interpret following instructions as LINC-12 instructions until PDP instruction
        // the tubes are in the middle of IOT2 state with the clock still high
        // the only tube state we need is:
        //  AC = input<11:00>
        //  L  = input<12>
        //  PC = lastreadaddr<11:00> + 1
        // we will have to stuff a JMP instruction down its throat on return to update the PC
        case 06141: {
            this->lincac  = input & 07777;
            this->lincpc  = (lastreadaddr + 1) & 001777;
            memext.dframe = memext.iframe = memext.iframeafterjump = lastreadaddr & 076000;
            this->link    = (input >> 12) & 1;
            this->execute ();

            // send updated AC,LINK back to tubes and drop clock
            gpio->writegpio (true, (this->lincac * G_DATA0) | (this->link ? G_LINK : 0));

            // let it soak in (setup time for the AC,LINK D-flipflope)
            gpio->halfcycle ();

            // tubes should be at very end of IOT2 now
            shadow.clock (gpio->readgpio ());

            // kerchunk them to beginning of FETCH1 by driving clock high
            // keep sending AC,LINK to be sure they get loaded into tube AC,LINK registers (hold time for the AC,LINK D-flipflope)
            gpio->writegpio (true, (this->lincac * G_DATA0) | (this->link ? G_LINK : 0) | G_CLOCK);

            // let AC,LINK soak into tubes and let FETCH1 state soak through tubes
            gpio->halfcycle ();

            // drop clock and start receiving tube PC pointing just past the original LINC instruction
            // give the tubes some time to put their PC on the bus, by waiting to the very end of FETCH1
            gpio->writegpio (false, 0);
            gpio->halfcycle ();
            shadow.clock (gpio->readgpio ());

            // kerchunk tubes into FETCH2 by driving clock high and let that soak through
            gpio->writegpio (false, G_CLOCK);
            gpio->halfcycle ();

            // give tubes a 'JMP @ 0' opcode and step them to middle of DEFER1
            gpio->writegpio (true, 05400 * G_DATA0);
            gpio->halfcycle ();
            shadow.clock (gpio->readgpio ());
            gpio->writegpio (true, 05400 * G_DATA0 | G_CLOCK);
            gpio->halfcycle ();

            // step tubes to middle of DEFER2 where they will be ready to accept the new PC
            gpio->writegpio (false, 0);
            gpio->halfcycle ();
            shadow.clock (gpio->readgpio ());
            gpio->writegpio (false, G_CLOCK);
            gpio->halfcycle ();

            // pass updated PC (points just past PDP instruction) back to caller who will send it to tubes
            // also clean up frame registers to hold PDP-style frame numbers
            uint16_t pdppc = (this->lincpc | memext.iframe) & 07777;
            memext.dframe &= 070000;
            memext.iframe &= 070000;
            memext.iframeafterjump &= 070000;
            memext.saveddframe &= 070000;
            memext.savediframe &= 070000;
            return pdppc;
        }
    }
    return UNSUPIO;
}

// execute LINC instructions until a PDP instruction is found
void Linc::execute ()
{
    while (true) {

        // fetch LINC opcode and execute
        uint16_t lastfetchaddr = memext.iframe | this->lincpc;
        uint16_t op = memarray[lastfetchaddr];
        if (shadow.printinstr) printf ("LINC PC=%04o L=%o AC=%04o IR=%04o IF=%04o DF=%04o\n", this->lincpc, this->link, this->lincac, op, memext.iframe, memext.dframe);
        this->lincpc = (this->lincpc + 1) & 01777;
        switch (op >> 10) {

            // 0000..1777
            case 0: {

                // 1000..1777
                if (op & 01000) {
                    this->execbeta (lastfetchaddr, op);
                }

                // 0000..0777
                else {
                    if (this->execalpha (lastfetchaddr, op)) return;
                }
                break;
            }

            // ADD y = 2000 + y
            // p 43 v 3-8
            case 1: {
                this->lincac = this->addition (this->lincac, memarray[memext.iframe|(op&01777)], false);
                break;
            }

            // STC y = 4000 + y
            // p 42 v 3-7
            case 2: {
                ASSERT (this->lincac <= 07777);
                memarray[memext.iframe|(op&01777)] = this->lincac;
                this->lincac = 0;
                break;
            }

            // JMP y = 6000 + y
            // p 50 v 3-15
            case 3: {
                uint16_t y = op & 01777;
                if (y != 0) {
                    memext.doingjump ();
                    if (! this->disabjumprtn) {
                        memarray[memext.iframe] = 06000 | this->lincpc;
                    }
                    this->disabjumprtn = false;
                }
                this->lincpc = y;
                break;
            }

            default: ABORT ();
        }

        // check for interrupt
        if (memext.intenabd () && getintreqmask ()) {
            // save and clear frames, disable interrupts
            memext.intack ();
            // save and set program counter
            memarray[040] = this->lincpc;
            this->lincpc  = 041;
        }
    }
}

// betas
// op = 1000..1777
void Linc::execbeta (uint16_t lastfetchaddr, uint16_t op)
{
    // compute operand address
    // p 39 v 3-4
    uint16_t ea;
    if (op & 020) {
        // I != 0; B != 0
        //  auto-increment B then use B as the address
        if (op & 017) {
            uint16_t *p = &memarray[memext.iframe|(op&017)];
            ea = *p;
            *p = ea = (ea & 06000) | ((ea + 1) & 01777);
        }
        // I != 0; B == 0
        //  value is word following the instruction (immediate)
        else {
            ea = this->lincpc;
            this->lincpc = (this->lincpc + 1) & 01777;
        }
    } else {
        // I == 0; B != 0
        if (op & 017) {
            ea = memarray[memext.iframe|(op&017)];
        }
        // I == 0; B == 0
        else {
            ea = memarray[memext.iframe|this->lincpc];
            this->lincpc = (this->lincpc + 1) & 01777;
        }
    }
    ASSERT (ea <= 07777);
    bool eatop = ea >> 11;
    ea = ((ea & 02000) ? memext.dframe : memext.iframe) | (ea & 01777);
    ASSERT (ea <= MEMSIZE);

    switch (op >> 5) {

        // LDA I B = 1000 + 20I + B
        // p 43 v 3-8
        case 020: {
            this->lincac = memarray[ea];
            ASSERT (this->lincac <= 07777);
            break;
        }

        // STA I B = 1040 + 20I + B
        // p 43 v 3-8
        case 021: {
            memarray[ea] = this->lincac;
            break;
        }

        // ADA I B = 1100 + 20I + B
        // p 44 v 3-9
        case 022: {
            this->lincac = this->addition (this->lincac, memarray[ea], false);
            break;
        }

        // ADM I B = 1140 + 20I + B
        // p 44 v 3-9
        case 023: {
            memarray[ea] = this->lincac = this->addition (this->lincac, memarray[ea], false);
            break;
        }

        // LAM I B = 1200 + 20I + B
        // p 44 v 3-9
        case 024: {
            this->lincac = this->addition (this->lincac, memarray[ea], true);
            break;
        }

        // MUL I B = 1240 + 20I + B
        // p 45 v 3-10
        case 025: {
            uint32_t a = this->lincac;              // get 1s comp operands
            uint32_t b = memarray[ea];
            uint32_t s = a ^ b;                     // sign goes in link
            this->link = (s >> 11) & 1;
            if (a & 04000) a ^= 07777;              // get abs values
            if (b & 04000) b ^= 07777;
            uint32_t p = a * b;                     // perform unsigned multiply
            extarith.multquot = (p & 03777) << 1;   // return low-order product abs value
            if (eatop) p >>= 11;                    // maybe get top 11 bits of result
            p &= 03777;                             // discard any overflow
            if (this->link) p ^= 07777;             // apply sign to product
            this->lincac = p;                       // return signed product
            break;
        }

        // LDH I B = 1300 + 20I + B
        // p 49 v 3-14
        case 026: {
            this->lincac = memarray[ea];
            if (eatop) this->lincac &= 077;
                   else this->lincac >>= 6;
            break;
        }

        // STH I B = 1340 + 20I + B
        // p 49 v 3-14
        case 027: {
            if (eatop) {
                memarray[ea] = (memarray[ea] & 07700) | (this->lincac & 077);
            } else {
                memarray[ea] = ((this->lincac & 077) << 6) | (memarray[ea] & 077);
            }
            break;
        }

        // SHD I B = 1400 + 20I + B
        // p 49 v 3-14
        case 030: {
            uint16_t y = memarray[ea];
            if (eatop) y &= 077;
                   else y >>= 6;
            if (y != (this->lincac & 077)) this->lincpc = (this->lincpc + 1) & 01777;
            break;
        }

        // SAE I B = 1440 + 20I + B
        // p 47 v 3-12
        case 031: {
            if (this->lincac == memarray[ea]) this->lincpc = (this->lincpc + 1) & 01777;
            break;
        }

        // SRO I B = 1500 + 20I + B
        // p 48 v 3-13
        case 032: {
            uint16_t y = memarray[ea];
            if (! (y & 1)) this->lincpc = (this->lincpc + 1) & 01777;
            memarray[ea] = ((y & 1) << 11) | (y >> 1);
            break;
        }

        // BCL I B = 1540 + 20I + B
        // p 46 v 3-11
        case 033: {
            this->lincac &= ~ memarray[ea];
            break;
        }

        // BSE I B = 1600 + 20I + B
        // p 46 v 3-11
        case 034: {
            this->lincac |= memarray[ea];
            break;
        }

        // BCO I B = 1640 + 20I + B
        // p 47 v 3-12
        case 035: {
            this->lincac ^= memarray[ea];
            break;
        }

        // 1000..1777 not covered above
        default: {
            this->unsupinst (lastfetchaddr, op);
            break;
        }
    }
}

// alphas
// op range 0000..0777
bool Linc::execalpha (uint16_t lastfetchaddr, uint16_t op)
{
    uint16_t *p = &memarray[memext.iframe|(op&017)];
    switch (op >> 5) {

        // SET I A = 0040 + 20I + A
        // p 49 v 3-14
        case 001: {
            uint16_t y = memarray[memext.iframe|this->lincpc];
            this->lincpc = (this->lincpc + 1) & 01777;
            if (! (op & 020)) y = memarray[((y&02000)?memext.dframe:memext.iframe)|(y&01777)];
            *p = y;
            break;
        }

        // XSK I A = 0200 + 20I + A
        // p 50 v 3-15
        case 004: {
            if (op & 020) {
                *p = (*p & 06000) | ((*p + 1) & 01777);
            }
            if ((*p & 01777) == 01777) {
                this->lincpc = (this->lincpc + 1) & 01777;
            }
            break;
        }

        // ROL I N = 0240 + 20I + N
        // p 51 v 3-16
        case 005: {
            uint16_t sc = op & 017;
            if (op & 020) {
                if (sc >= 13) sc -= 13;
                if (this->link) this->lincac |= 010000;
                this->lincac  = (this->lincac << sc) | (this->lincac >> (13 - sc));
                this->link    = (this->lincac >> 12) & 1;
                this->lincac &= 07777;
            } else {
                if (sc >= 12) sc -= 12;
                this->lincac  = (this->lincac << sc) | (this->lincac >> (12 - sc));
                this->lincac &= 07777;
            }
            break;
        }

        // ROR I N = 0300 + 20I + N
        // p 52 v 3-17
        case 006: {
            uint16_t sc = op & 017;
            if (op & 020) {
                if (this->link) this->lincac |= 010000;
                if (sc >= 13) {
                    extarith.multquot = this->lincac >> 1;
                    sc -= 13;
                }
                extarith.multquot = ((this->lincac << (13 - sc)) | (extarith.multquot >> sc)) & 007777;
                this->lincac  = ((this->lincac << (13 - sc)) | (this->lincac    >> sc)) & 017777;
                this->link    = (this->lincac >> 12) & 1;
                this->lincac &= 07777;
            } else {
                if (sc >= 12) {
                    extarith.multquot = this->lincac;
                    sc -= 12;
                }
                extarith.multquot = ((this->lincac << (12 - sc)) | (extarith.multquot >> sc)) & 07777;
                this->lincac = ((this->lincac << (12 - sc)) | (this->lincac    >> sc)) & 07777;
            }
            break;
        }

        // SCR I N = 0340 + 20I + N
        // p 53 v 3-18
        case 007: {
            uint16_t sc = op & 017;
            if (sc >= 12) {
                extarith.multquot = this->lincac;
                this->lincac = (this->lincac & 04000) ? 07777 : 00000;
                if (op & 020) this->link = this->lincac & 1;
                sc -= 12;
            }
            if (sc != 0) {
                if (op & 020) this->link = (this->lincac >> (sc - 1)) & 1;
                extarith.multquot = ((this->lincac << (12 - sc)) | (extarith.multquot >> sc)) & 07777;
                this->lincac = (((((int16_t) this->lincac) ^ 04000) - 04000) >> sc) & 07777;
            }
            break;
        }

        // LIF = 0600 + N
        // p 59 v 3-24
        case 014: {
            memext.iframeafterjump = (op & 037) << 10;
            memext.saveddframe = memext.dframe;
            memext.savediframe = memext.iframe;
            memext.intdisableduntiljump = true;
            break;
        }

        // LDF = 0640 + N
        // p 60 v 3-25
        case 015: {
            memext.dframe = (op & 037) << 10;
            break;
        }

        // 0000..0777 not covered above
        default: {
            if (this->execgamma (lastfetchaddr, op)) return true;
            break;
        }
    }

    return false;
}

// gammas
// op range 0000..0777 not handled by execalpha()
bool Linc::execgamma (uint16_t lastfetchaddr, uint16_t op)
{
    bool skip = (op & 020) != 0;
    switch (op) {

        // APO I
        // p 54 v 3-19
        case 0451:
        case 0471: {
            skip ^= (this->lincac & 04000) == 0;
            break;
        }

        // AZE I
        // p 54 v 3-19
        case 0450:
        case 0470: {
            skip ^= (this->lincac == 00000) || (this->lincac == 07777);
            break;
        }

        // LZE I
        // p 54 v 3-19
        case 0452:
        case 0472: {
            skip ^= ! this->link;
            break;
        }

        // QLZ I
        // p 54 v 3-19
        case 0455:
        case 0475: {
            skip ^= (extarith.multquot & 00001) == 0;
            break;
        }

        // FLO I
        // p 55 v 3-20
        case 0454:
        case 0474: {
            skip ^= this->flow;
            break;
        }

        // SKP I
        // p 55 v 3-20
        case 0456:
        case 0476: {
            skip ^= true;
            break;
        }

        // SNS I N
        case 0440 ... 0445:
        case 0460 ... 0465: {
            uint16_t swreg = readswitches ("senseregister");
            skip ^= (swreg >> (op & 7)) & 1;
            break;
        }

        // KST
        // p 56 v 3-21
        case 0435:
        case 0415: {
            skip ^= iodevtty.keystruck ();
            break;
        }

        // HLT
        // p 56 v 3-21
        case 0000: {
            haltinstr ("Linc::execute: LINC HALT PC=%05o L=%o AC=%04o\n", lastfetchaddr, this->link, this->lincac);
            break;
        }

        // CLR
        // p 56 v 3-21
        case 0011: {
            this->lincac = 0;
            this->link = false;
            extarith.multquot = 0;
            return false;
        }

        // COM
        // p 56 v 3-21
        case 0017: {
            this->lincac ^= 07777;
            return false;
        }

        // NOP
        // p 56 v 3-21
        case 0016: {
            return false;
        }

        // QAC
        // p 56 v 3-21
        case 0005: {
            this->lincac = extarith.multquot >> 1;
            return false;
        }

        // LSW
        // p 57 v 3-22
        case 0517: {
            this->lincac = readswitches ("leftswitches") & 07777;
            return false;
        }

        // RSW
        // p 57 v 3-22
        case 0516: {
            this->lincac = readswitches ("rightswitches") & 07777;
            return false;
        }

        // PDP
        // p 57 v 3-22
        case 0002: {
            return true;
        }

        // IOB
        // p 58 v 3-23
        case 0500: {
            this->execiob (lastfetchaddr, op);
            return false;
        }

        // DJR
        // p 63 v 3-28
        case 0006: {
            this->disabjumprtn = true;
            return false;
        }

        // ESF
        // p 64 v 3-30
        case 0004: {
            this->specfuncs = this->lincac & 01740;
            if (this->lincac & 00020) ::ioreset ();
            iodevtty.updintreq ();
            return false;
        }

        // SFA
        // p 64 v 3-30
        case 0024: {
            this->lincac = this->specfuncs;
            return false;
        }

        default: {
            this->unsupinst (lastfetchaddr, op);
            return false;
        }
    }
    if (skip) this->lincpc = (this->lincpc + 1) & 01777;
    return false;
}

// IOB - execute following word as a PDP-8 i/o instruction
// p 58 v 3-23
void Linc::execiob (uint16_t lastfetchaddr, uint16_t op)
{
    uint16_t iot = 06000 | (memarray[memext.iframe|this->lincpc] & 00777);
    this->lincpc = (this->lincpc + 1) & 01777;

    switch (iot) {

        // some opcodes we handle internally

        // don't recurse on LINC instruction
        case 06141: break;

        // RDF - read data field register into AC
        //   lincdf<14:10> => AC<05:01>
        // p 61 v 3-26
        case 06214: {
            this->lincac |= memext.dframe >> 9;
            break;
        }

        // RIF - read instruction field register into AC
        //   lincif<14:10> => AC<05:01>
        // p 60 v 3-25
        case 06224: {
            this->lincac |= memext.iframe >> 9;
            break;
        }

        // RIB - read interrupt buffer into AC
        // p 62 v 3-27
        case 06234: {
            this->lincac |= memext.saveddframe & 06000;     // AC<11:10> = DF<11:10>
            this->lincac |= memext.savediframe >>  7;       // AC<07:03> = IF<14:10>
            this->lincac |= memext.saveddframe >> 12;       // AC<02:00> = DF<14:12>
            break;
        }

        // RMF - restore memory fields
        // p 62 v 3-27
        case 06244: {
            memext.iframeafterjump = memext.savediframe;    // inst frame restored at next JMP
            memext.dframe = memext.saveddframe;             // data frame restored immediately
            break;
        }

        // all others go to PDP-8 i/o processing
        default: {
            uint16_t linkac = (this->link ? IO_LINK : 0) | this->lincac;
            uint16_t skiplinkac = ::ioinstr (iot, linkac);
            this->lincac = skiplinkac & IO_DATA;
            this->link   = (skiplinkac & IO_LINK) != 0;
            if (skiplinkac & IO_SKIP) this->lincpc = (this->lincpc + 1) & 01777;
            break;
        }
    }
}

// unsupported linc instruction code encountered
void Linc::unsupinst (uint16_t lastfetchaddr, uint16_t op)
{
    if (this->specfuncs & 01000) {
        // save and clear frames, disable interrupts
        memext.intack ();
        // save and set program counter
        memarray[0140] = this->lincpc;
        this->lincpc   = 0141;
    } else if (! quiet) {
        fprintf (stderr, "Linc::execute: unsupported linc opcode %04o at %05o\n", op, lastfetchaddr);
    }
}

// perform addition
//  input:
//   a = one 12-bit operand
//   b = other 12-bit operand
//   lame = true: LAM instruction (2s comp with link)
//         false: 1s comp without link
//  output:
//   returns 12-bit sum
//   updates flow (signed overflow flag)
uint16_t Linc::addition (uint16_t a, uint16_t b, bool lame)
{
    uint16_t c = a + b + (lame & this->link);
    if (lame) this->link = c >> 12;
                 else c += c >> 12;
    this->flow = (((a & b & ~ c) | (~ a & ~ b & c)) >> 11) & 1;
    return c & 07777;
}
