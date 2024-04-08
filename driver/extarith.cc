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

// handle extended arithmetic instructions
// the tubes decode them as an I/O instruction
// ...so this module looks like an I/O device emulator

#include <stdio.h>
#include <string.h>

#include "dyndis.h"
#include "extarith.h"
#include "memory.h"
#include "shadow.h"

#define DEBUG(...) do { } while (0)
//#define DEBUG printf

#define sx12(v) ((((v) & 07777) ^ 04000) - 04000)

bool ExtArith::getgtflag ()
{
    return gtflag;
}

void ExtArith::setgtflag (bool gt)
{
    gtflag = modeb & gt;
}

ExtArith::ExtArith ()
{
    iodevname = "eae";
}

// reset to power-on state
void ExtArith::ioreset ()
{
    gtflag    = false;
    modeb     = false;
    multquot  = 0;
    stepcount = 0;
}

// process script 'iodev extarith' commands
SCRet *ExtArith::scriptcmd (int argc, char const *const *argv)
{
    // get [mq/gt/mode/sc]
    if (strcmp (argv[0], "get") == 0) {
        if (argc == 1) return new SCRetStr ("mq=%05o;gt=%o;mode=%c;sc=%03o", this->multquot, this->gtflag, (this->modeb ? 'B' : 'A'), this->stepcount);
        if (argc == 2) {
            if (strcmp (argv[1], "mq")   == 0) return new SCRetInt (this->multquot);
            if (strcmp (argv[1], "gt")   == 0) return new SCRetInt (this->gtflag ? 1 : 0);
            if (strcmp (argv[1], "mode") == 0) return new SCRetStr (this->modeb ? "B" : "A");
            if (strcmp (argv[1], "sc")   == 0) return new SCRetInt (this->stepcount);
        }
        return new SCRetErr ("iodev eae get [mq/gt/mode/sc]");
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
        puts ("  mq   - multiplicand / quotient");
        puts ("  gt   - greater than flag");
        puts ("  mode - mode A or B");
        puts ("  sc   - step count");
        puts ("");
        return NULL;
    }

    // set mq/gt/mode/sc <value>
    if (strcmp (argv[0], "set") == 0) {
        if (argc == 3) {
            char *p;
            int value = strtol (argv[2], &p, 0);
            if (strcmp (argv[1], "mq")   == 0) {
                if ((*p != 0) || (value < 0) || (value > 4095)) return new SCRetErr ("mq value %s not in range 0..4095", argv[2]);
                this->multquot = value;
                return NULL;
            }
            if (strcmp (argv[1], "gt")   == 0) {
                if ((*p != 0) || (value < 0) || (value > 1)) return new SCRetErr ("gt value %s not in range 0..1", argv[2]);
                this->gtflag = value != 0;
                return NULL;
            }
            if (strcmp (argv[1], "mode") == 0) {
                if ((argv[2][0] | ('A' ^ 'a')) == ('A' | 'a')) {
                    this->modeb = false;
                    return NULL;
                }
                if ((argv[2][0] | ('B' ^ 'b')) == ('B' | 'b')) {
                    this->modeb = true;
                    return NULL;
                }
                return new SCRetErr ("mode %s not A or B", argv[2]);
            }
            if (strcmp (argv[1], "sc")   == 0) {
                if ((*p != 0) || (value < 0) || (value > 31)) return new SCRetErr ("sc value %s not in range 0..31", argv[2]);
                this->stepcount = value;
                return NULL;
            }
        }
        return new SCRetErr ("iodev eae set mq/gt/mode/sc <value>");
    }

    return new SCRetErr ("unknown eae command %s - valid: get help set", argv[0]);
}

// process an EAE instruction
//  input:
//   opcode = EAE opcode
//   input<15:13> = zeroes
//           <12> = link
//        <11:00> = accumulator
//  output:
//   returns<13> = skip
//          <12> = link
//       <11:00> = accumulator
uint16_t ExtArith::ioinstr (uint16_t opcode, uint16_t input)
{
    DEBUG ("ExtArith::ioinstr*: %llu  pc=%05o ir=%04o  l.ac=%o.%04o mq=%04o gt=%d  [%c]",
        shadow.getcycles (), lastreadaddr, opcode, input >> 12, input & 07777, multquot, gtflag, modeb ? 'B' : 'A');
    nextreadaddr = (lastreadaddr & 070000) | ((lastreadaddr + 1) & 007777);

    // step 1
    if (opcode & 00200) input &= ~ IO_DATA;     // 0200 CLA - clear accumulator

    // step 2
    if ((opcode & 07577) == 07431) {            // 7431 (MQL NMI) SWAB - switch to mode B
        modeb = true;
        return input;
    }

    switch (opcode & 00120) {

        // 7421 & 0120: p137 / v3-11
        case 00020: {                           // 0020 MQL - load multipler quotient from accumulator
            multquot = input & IO_DATA;
            input &= ~ IO_DATA;
            break;
        }

        // 7501 & 0120: p137 / v3-11
        case 00100: {                           // 0100 MQA - or multiplier quotient into accumulator
            input |= multquot;
            break;
        }

        // 7521 & 0120: p137 / v3-11
        case 00120: {                           // 0120 SWP - swap accumulator and multiplier quotient
            uint16_t tmp = multquot;
            multquot = input & IO_DATA;
            input    = (input & IO_LINK) | tmp;
            break;
        }
    }

    // step 3
    uint16_t output = modeb ? eae_modeb (opcode, input) : eae_modea (opcode, input);
    DEBUG ("  =>  l.ac=%o.%04o mq=%04o gt=%d  [%c]\n", output >> 12, output & 07777, multquot, gtflag, modeb ? 'B' : 'A');
    return output;
}

// EAE - p225
//   1   1   1   1  cla mqa sca mql <..instr..>  1
//  11  10   9 . 8   7   6 . 5   4   3 . 2   1   0
uint16_t ExtArith::eae_modea (uint16_t opcode, uint16_t input)
{
    switch ((opcode >> 1) & 027) {                  // step '3' bits

        case 020: {
            input |= stepcount;                     // 7441 SCA - or step count into accumulator
            break;
        }

        // SCL - step counter load from memory
        // p227 v7-7
        case 021: {
            input |= stepcount;                     // 0040 SCA - or step count into accumulator
        }
        case 001: {                                 // 7403 SCL
            if (dyndisena) dyndisread (nextreadaddr);
            stepcount = ~ memarray[nextreadaddr] & 037;
            input |= IO_SKIP;
            break;
        }

        // MUY - multiply
        // p227 v7-7
        case 022: {
            input |= stepcount;                     // 0040 SCA - or step count into accumulator
        }
        case 002: {                                 // 7405 MUY
            if (dyndisena) dyndisread (nextreadaddr);
            uint16_t multiplier = memarray[nextreadaddr];
            input = multiply (input, multiplier);
            break;
        }

        // DVI - divide
        // p227 v7-7
        case 023: {
            break;                                  // 7447 SWBA - redundant switch to mode A
        }
        case 003: {                                 // 7407 DVI
            if (dyndisena) dyndisread (nextreadaddr);
            uint16_t divisor  = memarray[nextreadaddr];
            input = divide (input, divisor);
            break;
        }

        // NMI - normalize (p 229 / v 7-9)
        // p227 v7-7
        case 024: {
            input |= stepcount;                     // 0040 SCA - or step count into accumulator
        }
        case 004: {                                 // 7411 NMI
            input = normalize (input);
            break;
        }

        // SHL - shift left
        // p228 v7-8
        case 025: {
            input |= stepcount;                     // 0040 SCA - or step count into accumulator
        }
        case 005: {                                 // 7413 SHL
            if (dyndisena) dyndisread (nextreadaddr);
            stepcount = (memarray[nextreadaddr] & 037) + 1;
            DEBUG ("  SHL %02o", stepcount);
            uint32_t acmq = (((uint32_t) input & 017777) << 12) | multquot;
            do acmq = (acmq << 1) & 0177777777;
            while (-- stepcount > 0);
            multquot = acmq & 07777;
            input = ((acmq >> 12) & 017777) | IO_SKIP;
            break;
        }

        // ASR - arithmetic shift right
        // p228 v7-8
        case 026: {
            input |= stepcount;                     // 0040 SCA - or step count into accumulator
        }
        case 006: {                                 // 7415 ASR
            if (dyndisena) dyndisread (nextreadaddr);
            stepcount = (memarray[nextreadaddr] & 037) + 1;
            DEBUG ("  ASR %02o", stepcount);
            uint32_t acmq = (((uint32_t) input & 07777) << 12) | (((uint32_t) input & 04000) << 13) | multquot;
            do acmq = (acmq >> 1) | (acmq & 0100000000);
            while (-- stepcount > 0);
            input = (acmq >> 12) | IO_SKIP;
            multquot = acmq & 07777;
            break;
        }

        // LSR - logical shift right
        // p228 v7-8
        case 027: {
            input |= stepcount;                     // 0040 SCA - or step count into accumulator
        }
        case 007: {                                 // 7417 LSR
            if (dyndisena) dyndisread (nextreadaddr);
            stepcount = (memarray[nextreadaddr] & 037) + 1;
            DEBUG ("  LSR %02o", stepcount);
            uint32_t acmq = (((uint32_t) input & 07777) << 12) | multquot;
            do acmq = acmq >> 1;
            while (-- stepcount > 0);
            input = (acmq >> 12) | IO_SKIP;
            multquot = acmq & 07777;
            break;
        }
    }
    return input;
}

uint16_t ExtArith::eae_modeb (uint16_t opcode, uint16_t input)
{
    switch ((opcode >> 1) & 027) {                  // step '3' bits

        // ACS - accumulator to step count
        // p229 v7-9
        case 001: {                                 // 7403 SCL
            stepcount = input & 037;
            input &= IO_LINK;
            break;
        }

        // MUY - multiply
        // p229 v7-9
        case 002: {                                 // 7405 MUY
            uint16_t multiplier = getdeferredaddr ();
            multiplier = readmemword (multiplier);
            input = multiply (input, multiplier);
            break;
        }

        // DVI - divide
        // p229 v7-9
        case 003: {                                 // 7407 DVI
            uint16_t divisor = getdeferredaddr ();
            divisor = readmemword (divisor);
            input = divide (input, divisor);
            break;
        }

        // NMI - normalize
        // p229 v7-9
        case 004: {                                 // 7411 NMI
            input = normalize (input);
            break;
        }

        // SHL - shift left
        // p230 v7-10
        case 005: {                                 // 7413 SHL
            if (dyndisena) dyndisread (nextreadaddr);
            stepcount = memarray[nextreadaddr] & 037;
            DEBUG ("  SHL %02o", stepcount);
            uint32_t acmq = (((uint32_t) input & 017777) << 12) | multquot;
            while (stepcount > 0) {
                acmq = (acmq << 1) & 0177777777;
                -- stepcount;
            }
            stepcount = 037;
            multquot = acmq & 07777;
            input = ((acmq >> 12) & 017777) | IO_SKIP;
            break;
        }

        // ASR - arithmetic shift right
        // p230 v7-10
        case 006: {                                 // 7415 ASR
            if (dyndisena) dyndisread (nextreadaddr);
            stepcount = memarray[nextreadaddr] & 037;
            DEBUG ("  ASR %02o", stepcount);
            uint32_t acmq = (((uint32_t) input & 07777) << 12) | (((uint32_t) input & 04000) << 13) | multquot;
            while (stepcount > 0) {
                gtflag = acmq & 1;
                acmq = (acmq >> 1) | (acmq & 0100000000);
                -- stepcount;
            }
            stepcount = 037;
            multquot = acmq & 07777;
            input = (acmq >> 12) | IO_SKIP;
            break;
        }

        // LSR - logical shift right
        // p230 v7-10
        case 007: {                                 // 7417 LSR
            if (dyndisena) dyndisread (nextreadaddr);
            stepcount = memarray[nextreadaddr] & 037;
            DEBUG ("  LSR %02o", stepcount);
            uint32_t acmq = (((uint32_t) input & 07777) << 12) | multquot;
            while (stepcount > 0) {
                gtflag = acmq & 1;
                acmq = acmq >> 1;
                -- stepcount;
            }
            stepcount = 037;
            multquot = acmq & 07777;
            input = (acmq >> 12) | IO_SKIP;
            break;
        }

        // SCA - step counter OR with accumulator
        // p230 v7-10
        case 020: {                                 // 7441 SCA
            input |= stepcount & 037;
            break;
        }

        // DAD - double precision add (p231)
        case 021: {                                 // 7443 DAD
            uint16_t memaddr = getdeferredaddr ();
            uint32_t acmq = readmemlong (memaddr);
            acmq += (((uint32_t) input & IO_DATA) << 12) | multquot;
            multquot = acmq & 07777;
            input = (acmq >> 12) | IO_SKIP;
            break;
        }

        // DST - double precision store
        case 022: {                                 // 7445 DST
            uint16_t memaddr = getdeferredaddr ();
            writememlong (memaddr, (((uint32_t) input & IO_DATA) << 12) | multquot);
            input |= IO_SKIP;
            break;
        }

        // SWBA - switch mode from B to A
        // p226 v7-6
        case 023: {                                 // 7447 SWBA
            gtflag = false;
            modeb  = false;
            break;
        }

        // DPSZ - double precision skip if zero
        // p232 v7-12
        case 024: {                                 // 7451 DPSZ
            if (((input & 07777) == 0) && (multquot == 0)) input |= IO_SKIP;
            break;
        }

        // DPIC - double precision increment
        // p232 v7-12
        case 025: {                                 // 7453 DPIC
            // doc says it must be accompanied by SWP 7520
            // so MQ has high order bits; AC has low order bits
            uint32_t acmq = (((uint32_t) multquot << 12) | (input & IO_DATA)) + 1;
            multquot = acmq & 07777;                // but put low order back in MQ
            input = acmq >> 12;                     // ... and high order in LINK/AC
            break;
        }

        // DCM - double precision complement (negate)
        // p232 v7-12
        case 026: {                                 // 7455 DCM
            // doc says it must be accompanied by SWP 7520
            // so MQ has high order bits; AC has low order bits
            uint32_t acmq = ((((uint32_t) multquot << 12) | (input & IO_DATA)) ^ 077777777) + 1;
            multquot = acmq & 07777;                // but put low order back in MQ
            input = acmq >> 12;                     // ... and high order in LINK/AC
            break;
        }

        // SAM - subtract AC from MQ, put in AC
        // p230 v7-10
        case 027: {                                 // 7457 SAM
            gtflag = sx12 (multquot) >= sx12 (input);
            input = ((multquot - (input & 07777)) & 017777) ^ 010000;
            break;
        }
    }
    return input;
}

// the word following the group 3 instruction is the address of a pointer to the value
// get the pointer from that address
// if the pointer is in an autoincrement location, increment the pointer first then retrieve
uint16_t ExtArith::getdeferredaddr ()
{
    if ((nextreadaddr & 07770) == 00010) {
        if (dyndisena) dyndiswrite (nextreadaddr);
        memarray[nextreadaddr] = (memarray[nextreadaddr] + 1) & 07777;
    }
    if (dyndisena) dyndisread (nextreadaddr);
    return memarray[nextreadaddr];
}

uint16_t ExtArith::multiply (uint16_t input, uint16_t multiplier)
{
    DEBUG ("  MUL %04o", multiplier);
    uint32_t product = multiplier * multquot + (input & IO_DATA);
    multquot = product & 07777;
    return (product >> 12) | IO_SKIP;
}

uint16_t ExtArith::divide (uint16_t input, uint16_t divisor)
{
    DEBUG ("  DIV %04o", divisor);
    uint32_t dividend = (((uint32_t) input & IO_DATA) << 12) | multquot;
    if ((dividend >> 12) < divisor) {
        multquot = dividend / divisor;
        input    = IO_SKIP | (dividend % divisor);
    } else if (dividend == ((uint32_t) divisor << 12)) {
        // weird values to pass D0MB test
        multquot = 1;
        input    = IO_SKIP | IO_LINK | divisor;
    } else {
        // weird values to pass D0MB test
        multquot = (multquot * 2 + 1) & 07777;
        input   |= IO_SKIP | IO_LINK;
    }
    return input;
}

uint16_t ExtArith::normalize (uint16_t input)
{
    stepcount = 0;

    // acmq<24> = link
    //  <23:12> = accum
    //  <11:00> = multquot
    uint32_t acmq = (((uint32_t) input & 017777) << 12) | multquot;

    // shift left until either:
    //   ACMQ is zero
    //   ACMQ is 6000 0000
    //   top two bits of ACMQ are different
    while ((acmq & 077777777) != 0) {
        if ((acmq & 077777777) == 060000000) break;
        if ((acmq & 060000000) == 040000000) break;
        if ((acmq & 060000000) == 020000000) break;
        stepcount ++;
        acmq = (acmq << 1) & 0177777777;
    }

    if (modeb && ((acmq & 077777777) == 040000000)) acmq &= 0100000000;

    multquot = acmq & 07777;
    return (acmq >> 12) & 017777;
}
