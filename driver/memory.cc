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

#include <math.h>

#include "memext.h"
#include "memory.h"
#include "miscdefs.h"

MemPar mempar;          // memory parity
uint16_t *memarray;     // top 4 bits always zeroes

// read 48-bit double from memory
//  <sign> <11-bit-exponent-excess-1023> <36-bit-mantissa>
double readdoub (uint64_t i48)
{
    uint64_t normi = i48 & 07777777777777777ULL;
    if ((normi & 03777777777777777ULL) > 03777000000000000ULL) return NAN;
    int exp = (normi >> 36) & 03777;
    bool neg = (normi >> 47) & 1;
    normi &= 0777777777777ULL;
    if ((exp == 03777U) && (normi == 0)) return neg ? - INFINITY : INFINITY;
    if (exp > 0) normi |= 01000000000000ULL;
    double normf = ldexp (normi, exp - 01777 - 36);
    return neg ? - normf : normf;
}

// read 24-bit float from memory
//  <sign> <7-bit-exponent-excess-63> <16-bit-mantissa>
float readflt (uint32_t i24)
{
    uint32_t normi = i24 & 077777777U;
    if ((normi & 037777777ULL) > 037600000ULL) return NAN;
    int exp = (normi >> 16) & 0177;
    bool neg = (normi >> 23) & 1;
    normi &= 0177777ULL;
    if ((exp == 0177U) && (normi == 0)) return neg ? - INFINITY : INFINITY;
    if (exp > 0) normi |= 0200000ULL;
    float normf = ldexpf (normi, exp - 077 - 16);
    return neg ? - normf : normf;
}

// write 48-bit float to memory
//  <sign> <11-bit-exponent-excess-1023> <36-bit-mantissa>
uint64_t writedoub (double f64)
{
    if (isnan (f64)) return 07777777777777777ULL;
    bool neg = false;
    if (f64 < 0.0) {
        f64 = - f64;
        neg = true;
    }
    if (isinf (f64)) return neg ? 07777000000000000ULL : 03777000000000000ULL;

    int exp;
    double normf = frexp (f64, &exp);
    uint64_t normi = (uint64_t) round (ldexp (normf, 37));
    if (normi > 0) {
        exp += 01777;
        while ((normi >= 02000000000000ULL) || (exp < 0)) {
            normi /= 2;
            exp ++;
        }
        while ((normi < 01000000000000ULL) && (exp > 0)) {
            normi *= 2;
            -- exp;
        }
        if ((exp == 0) && (normi >= 01000000000000ULL)) {
            normi /= 2;
            exp = 1;
        }
        if (exp > 03777) {
            exp = 03777;
            normi = 0;
        }
        normi &= 0777777777777ULL;
        normi |= ((uint64_t) exp) << 36;
        if (neg) normi |= 04000000000000000ULL;
    }
    return normi;
}

// write 24-bit float to memory
//  <sign> <7-bit-exponent-excess-63> <16-bit-mantissa>
uint32_t writeflt (float f32)
{
    if (isnanf (f32)) return 077777777U;
    bool neg = false;
    if (f32 < 0.0) {
        f32 = - f32;
        neg = true;
    }
    if (isinff (f32)) return neg ? 077600000U : 037600000U;

    int exp;
    float normf = frexpf (f32, &exp);
    uint32_t normi = (uint32_t) roundf (ldexpf (normf, 17));
    if (normi > 0) {
        exp += 077;
        while ((normi >= 0400000U) || (exp < 0)) {
            normi /= 2;
            exp ++;
        }
        while ((normi < 0200000U) && (exp > 0)) {
            normi *= 2;
            -- exp;
        }
        if ((exp == 0) && (normi >= 0200000U)) {
            normi /= 2;
            exp = 1;
        }
        if (exp > 0177) {
            exp = 0177;
            normi = 0;
        }
        normi &= 0177777U;
        normi |= exp << 16;
        if (neg) normi |= 040000000U;
    }
    return normi;
}

// write 48-bit float to memory
void writememdoub (uint16_t addr, double val)
{
    writememquad (addr, writedoub (val));
}

// write 24-bit float to memory
void writememflt (uint16_t addr, float val)
{
    writememlong (addr, writeflt (val));
}

// write 48-bit integer to memory
void writememquad (uint16_t addr, uint64_t val)
{
    writememword (addr,     val);
    writememword (addr + 1, val >> 12);
    writememword (addr + 2, val >> 24);
    writememword (addr + 3, val >> 36);
}

// write 24-bit integer to memory
void writememlong (uint16_t addr, uint32_t val)
{
    writememword (addr,     val);
    writememword (addr + 1, val >> 12);
}

// write 12-bit integer to memory
void writememword (uint16_t addr, uint16_t val)
{
    addr = memext.dframe | (addr & 4095);
    ASSERT (addr < MEMSIZE);
    memarray[addr] = val & 4095;
}

// read 48-bit double from memory
double readmemdoub (uint16_t addr)
{
    return readdoub (readmemquad (addr));
}

// read 24-bit float from memory
float readmemflt (uint16_t addr)
{
    return readflt (readmemlong (addr));
}

// read 48-bit integer from memory
uint64_t readmemquad (uint16_t addr)
{
    uint16_t word0 = readmemword (addr);
    uint32_t word1 = readmemword (addr + 1);
    uint64_t word2 = readmemword (addr + 2);
    uint64_t word3 = readmemword (addr + 3);
    return (word3 << 36) | (word2 << 24) | (word1 << 12) | word0;
}

// read 24-bit integer from memory
uint32_t readmemlong (uint16_t addr)
{
    uint16_t word0 = readmemword (addr);
    uint32_t word1 = readmemword (addr + 1);
    return (word1 << 12) | word0;
}

// read 12-bit integer from memory
uint16_t readmemword (uint16_t addr)
{
    addr = memext.dframe | (addr & 4095);
    ASSERT (addr < MEMSIZE);
    uint16_t word = memarray[addr];
    ASSERT (word < 4096);
    return word;
}

// write 6-bit character string from buff
// packs 2 characters into each word
//  input:
//   size = number of characters to convert
//   addr = address in dframe
//   buff = 'size' byte buffer
//  output:
//   *addr = filled in
static uint16_t bytetosix (uint8_t b)
{
    b &= 0177;
    if (b <  0040) return b;
    if (b >= 0140) b -= 0040;
    return b - 0040;
}
void putbuf6 (uint16_t size, uint16_t addr, uint8_t const *buff)
{
    while (size >= 2) {
        uint16_t word = (bytetosix (buff[0]) << 6) | bytetosix (buff[1]);
        writememword (addr ++, word);
        buff += 2;
        size -= 2;
    }
    if (size > 0) {
        uint16_t word = bytetosix (buff[0]) << 6;
        writememword (addr, word);
    }
}

// write 8-bit character string from buff
// packs 3 characters into 2 words
//  input:
//   size = number of characters to convert
//   addr = address in dframe
//   buff = 'size' byte buffer
//  output:
//   *addr = filled in
void putbuf8 (uint16_t size, uint16_t addr, uint8_t const *buff)
{
    while (size >= 3) {
        writememword (addr ++, (buff[0] << 4) | (buff[1] >> 4));
        writememword (addr ++, ((buff[1] & 15) << 8) | buff[2]);
        buff += 3;
        size -= 3;
    }
    switch (size) {
        case 1: {
            writememword (addr ++, buff[0] << 4);
            break;
        }
        case 2: {
            writememword (addr ++, (buff[0] << 4) | (buff[1] >> 4));
            writememword (addr ++, (buff[1] & 15) << 8);
            break;
        }
    }
}

// write 12-bit character string from buff
// fills in zeroes for top 4 bits of each word
//  input:
//   size = number of characters to convert
//   addr = address in dframe
//   buff = 'size' byte buffer
//  output:
//   *addr = filled in
void putbuf12 (uint16_t size, uint16_t addr, uint8_t const *buff)
{
    while (size > 0) {
        uint16_t word = *(buff ++);
        writememword (addr ++, word);
        -- size;
    }
}

// read 6-bit character string into buff
// splits each word into 2 characters
//  input:
//   size = number of characters to convert
//   addr = address in dframe
//   buff = 'size' byte buffer
//  output:
//   *buff = filled in
static uint8_t sixtobyte (uint16_t s)
{
    return s + 0040;
}
void getbuf6 (uint16_t size, uint16_t addr, uint8_t *buff)
{
    while (size >= 2) {
        uint16_t word = readmemword (addr ++);
        *(buff ++) = sixtobyte (word >>  6);
        *(buff ++) = sixtobyte (word & 077);
        size -= 2;
    }
    if (size > 0) {
        uint16_t word = readmemword (addr);
        *buff = sixtobyte (word >> 6);
    }
}

// read 8-bit character string into buff
// splits 2 words into 3 characters
//  input:
//   size = number of characters to convert
//   addr = address in dframe
//   buff = 'size' byte buffer
//  output:
//   *buff = filled in
void getbuf8 (uint16_t size, uint16_t addr, uint8_t *buff)
{
    while (size > 0) {
        uint16_t word0 = readmemword (addr ++);
        uint16_t word1 = (size > 1) ? readmemword (addr ++) : 0;
        *(buff ++) = word0 >> 4;
        if (size < 2) break;
        *(buff ++) = (word0 << 4) | (word1 >> 8);
        if (size < 3) break;
        *(buff ++) = word1;
        size -= 3;
    }
}

// read 12-bit character string into buff
// discards top 4 bits of each word
//  input:
//   size = number of characters to convert
//   addr = address in dframe
//   buff = 'size' byte buffer
//  output:
//   *buff = filled in
void getbuf12 (uint16_t size, uint16_t addr, uint8_t *buff)
{
    while (size > 0) {
        *(buff ++) = readmemword (addr ++);
        -- size;
    }
}

// read space/null-terminated 6-bit string into strbuf
// splits each word into 2 characters
//  input:
//   addr = address in dframe of space/null-terminated string
//   buff = 8193 byte buffer
//  output:
//   *buff = filled in (null terminated)
#define TERM6 ' '   // zero in 6-bit code
void getstrz6 (uint16_t addr, uint8_t *buff)
{
    ASSERT (addr < 4096);
    do {
        uint16_t word = readmemword (addr);
        *buff = sixtobyte (word >> 6);
        if (*buff == TERM6) break;
        buff ++;
        *buff = sixtobyte (word & 077);
        if (*buff == TERM6) break;
        buff ++;
    } while (++ addr < 4096);
    *buff = 0;
}

// read null-terminated 8-bit string into buff
// splits 2 words into 3 characters
//  input:
//   addr = address in dframe of null-terminated string
//   buff = 6145 byte buffer
//  output:
//   *buff = filled in
void getstrz8 (uint16_t addr, uint8_t *buff)
{
    ASSERT (addr < 4096);
    do {
        uint16_t word0 = readmemword (addr);
        *buff = word0 >> 4;
        if (*(buff ++) == 0) return;
        if (++ addr >= 4096) break;
        uint16_t word1 = readmemword (addr);
        *buff = (word0 << 4) | (word1 >> 8);
        if (*(buff ++) == 0) return;
        *buff = word1;
        if (*(buff ++) == 0) return;
    } while (++ addr < 4096);
    *buff = 0;
}

// read null-terminated 12-bit string into buff
// discards top 4 bits of each word
//  input:
//   addr = address in dframe of null-terminated string
//   buff = 4097 byte buffer
//  output:
//   *buff = filled in
void getstrz12 (uint16_t addr, uint8_t *buff)
{
    ASSERT (addr < 4096);
    do {
        *buff = readmemword (addr);
        if (*(buff ++) == 0) return;
    } while (++ addr < 4096);
    *buff = 0;
}

// memory parity (p 242 v 7-22)

void MemPar::ioreset ()
{ }

uint16_t MemPar::ioinstr (uint16_t opcode, uint16_t input)
{
    switch (opcode) {

        // disable parity error interrupt
        case 06100: {
            break;
        }

        // skip on no memory parity error
        case 06101: {
            input |= IO_SKIP;
            break;
        }

        // skip on power low
        case 06102: {
            break;
        }

        // enable parity error interrupt
        case 06103: {
            break;
        }

        // clear memory parity error flag
        case 06104: {
            break;
        }

        // check for even parity during next instruction's exec states
        case 06106: {
            break;
        }

        // skip on no memory parity error and clear flag
        case 06105: {
            input |= IO_SKIP;
            break;
        }

        // skip if memory parity option is present
        case 06107: {
            break;
        }

        default: input = UNSUPIO;
    }
    return input;
}
