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

// process 'system call' I/O instructions
// this gives the PDP-8 access to many Linux functions

#include <errno.h>
#include <fcntl.h>
#include <math.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "controls.h"
#include "memext.h"
#include "memory.h"
#include "miscdefs.h"
#include "rdcyc.h"
#include "shadow.h"
#include "scncall.h"

#define SCN_EXIT 0
#define SCN_PRINTLN6 1
#define SCN_PRINTLN8 2
#define SCN_PRINTLN12 3
#define SCN_INTREQ 4
#define SCN_CLOSE 5
#define SCN_GETNOWNS 6
#define SCN_OPEN6 7
#define SCN_OPEN8 8
#define SCN_OPEN12 9
#define SCN_READ6 10
#define SCN_READ8 11
#define SCN_READ12 12
#define SCN_WRITE6 13
#define SCN_WRITE8 14
#define SCN_WRITE12 15
#define SCN_IRQATNS 16
#define SCN_INTACK 17
#define SCN_UNLINK6 18
#define SCN_UNLINK8 19
#define SCN_UNLINK12 20
#define SCN_GETCMDARG6 21
#define SCN_GETCMDARG8 22
#define SCN_GETCMDARG12 23
#define SCN_ADD_DDD 24
#define SCN_ADD_FFF 25
#define SCN_SUB_DDD 26
#define SCN_SUB_FFF 27
#define SCN_MUL_DDD 28
#define SCN_MUL_FFF 29
#define SCN_DIV_DDD 30
#define SCN_DIV_FFF 31
#define SCN_CMP_DD 32
#define SCN_CMP_FF 33
#define SCN_CVT_FP 34
#define SCN_UDIV 35
#define SCN_UMUL 36
#define SCN_PRINTINSTR 37
#define SCN_NEG_DD 38
#define SCN_NEG_FF 39
#define SCN_WATCHWRITE 40
#define SCN_GETENV6 41
#define SCN_GETENV8 42
#define SCN_GETENV12 43
#define SCN_SETTTYECHO 44
#define SCN_IRQATTS 45
#define SCN_READLN6 46
#define SCN_READLN8 47
#define SCN_READLN12 48

ScnCall scncall;
ScnCall2 scncall2;

static bool lineclockrun;
static pthread_cond_t lineclockcond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t lineclocklock = PTHREAD_MUTEX_INITIALIZER;
static struct timespec lineclockabs;
static uint16_t savederrno;

static void scnprintln (GetStrz *getstrz, uint16_t data, uint8_t *temp);
static uint16_t scnopen (GetStrz *getstrz, uint16_t data, uint8_t *temp);
static uint16_t scnread (PutBuf *putbuf, uint16_t data, uint8_t *temp);
static uint16_t scnreadln (PutBuf *putbuf, uint16_t data, uint8_t *temp);
static uint16_t scnwrite (GetBuf *getbuf, uint16_t data, uint8_t *temp);
static uint16_t scnunlink (GetStrz *getstrz, uint16_t data, uint8_t *temp);
static uint16_t scngetcmdarg (PutBuf *putbuf, uint16_t data);
static uint16_t scngetenv (GetStrz *getstrz, PutBuf *putbuf, uint16_t data, uint8_t *temp);
static void *lineclockthread (void *dummy);

// nothing to reset on power on
void ScnCall::ioreset ()
{ }

// io instruction used to make system call
uint16_t ScnCall::ioinstr (uint16_t opcode, uint16_t input)
{
    switch (opcode) {

        // read errno
        case SCN_IO+0: {
            input = savederrno & (IO_DATA | IO_LINK);
            break;
        }

        // write errno
        case SCN_IO+1: {
            savederrno = input;
            break;
        }

        // do syscall, set link and skip if error
        case SCN_IO+2: {
            input = docall (input);
            if (input & IO_LINK) input |= IO_SKIP;
            break;
        }

        // do syscall, set link and read errno and skip if error
        case SCN_IO+3: {
            input = docall (input);
            if (input & IO_LINK) input = (savederrno & IO_DATA) | IO_LINK | IO_SKIP;
            break;
        }

        // do syscall, set link if error
        case SCN_IO+4: {
            input = docall (input);
            break;
        }

        // do syscall, set link if error, skip if no error
        case SCN_IO+5: {
            input = docall (input);
            if (! (input & IO_LINK)) input |= IO_SKIP;
            break;
        }

        // do syscall, set link and read errno if error
        case SCN_IO+6: {
            input = docall (input);
            if (input & IO_LINK) input = (savederrno & IO_DATA) | IO_LINK;
            break;
        }

        // do syscall, set link and read errno if error, skip if no error
        case SCN_IO+7: {
            input = docall (input);
            if (input & IO_LINK) input = (savederrno & IO_DATA) | IO_LINK;
                                                    else input |= IO_SKIP;
            break;
        }

        // unknown opcode
        default: input = UNSUPIO;
    }

    return input;
}

uint16_t ScnCall::docall (uint16_t data)
{
    uint8_t temp[8193];                 // 8193 = whole frame of 6-bit chars + null terminator

    // accumulator holds the param block address
    data &= IO_DATA;                    // clear off link bit
    uint16_t scn = readmemword (data);  // word at that address has sys call number

    switch (scn) {

        // exit() system call
        case SCN_EXIT: {
            static char hrbuff[16];
            data = readmemword (data + 1);
            snprintf (hrbuff, sizeof hrbuff, "SCN_EXIT:%04o", data);
            if (! scriptmode) {
                fprintf (stderr, "raspictl: %s\n", hrbuff);
                exit (data);
            }
            bool sigint = ctl_lock ();
            haltreason  = hrbuff;
            haltflags  |= HF_HALTIT;
            ctl_unlock (sigint);
            break;
        }

        // print string with a newline
        //  .word   SCN_PRINTLN{6,8,12}
        //  .word   messageaddress
        case SCN_PRINTLN6:  { scnprintln (getstrz6,  data, temp); break; }   // - each character occupies  6 bits
        case SCN_PRINTLN8:  { scnprintln (getstrz8,  data, temp); break; }   // - each character occupies  8 bits
        case SCN_PRINTLN12: { scnprintln (getstrz12, data, temp); break; }   // - each character occupies 12 bits

        // force interrupt request
        //  .word   SCN_INTREQ
        //  .word   0: negate; 1: assert
        case SCN_INTREQ: {
            uint16_t code = readmemword (data + 1);
            fprintf (stderr, "raspictl: SCN_INTREQ:%s\n", ((code != 0) ? "true" : "false"));
            if (code != 0) setintreqmask (IRQ_SCNINTREQ);
                      else clrintreqmask (IRQ_SCNINTREQ);
            break;
        }

        // close() system call
        case SCN_CLOSE: {
            int fd = (readmemword (data + 1) ^ 2048) - 2048;
            if (fd <= 2) fd = -999;
            int rc = close (fd);
            data = rc & 017777;
            if (rc < 0) savederrno = errno;
            savedtermioss.erase (fd);
            break;
        }

        // get time via clock_gettime(CLOCK_REALTIME), ie, unix timestamp
        //  .word   SCN_GETNOW
        //  .word   addr of uint48_t that gets the current timestamp
        //              <47:12> = seconds since 1970-01-01 00:00 UTC'
        //              <11:00> = 1/4096ths second
        case SCN_GETNOWNS: {
            struct timespec nowts;
            if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) ABORT ();
            writememquad (readmemword (data + 1), (nowts.tv_sec * 4096ULL) + nowts.tv_nsec * 4096ULL / 1000000000);
            data = 0;
            break;
        }

        // open() system call
        //  .word   pathaddress
        //  .word   flags
        //  .word   mode
        case SCN_OPEN6:   { data = scnopen  (getstrz6,  data, temp); break; }
        case SCN_OPEN8:   { data = scnopen  (getstrz8,  data, temp); break; }
        case SCN_OPEN12:  { data = scnopen  (getstrz12, data, temp); break; }

        // read() system call
        //  .word   fd
        //  .word   buffaddress
        //  .word   8-bit-byte-count
        case SCN_READ6:   { data = scnread (putbuf6,   data, temp); break; }
        case SCN_READ8:   { data = scnread (putbuf8,   data, temp); break; }
        case SCN_READ12:  { data = scnread (putbuf12,  data, temp); break; }

        // read() system call
        //  .word   fd
        //  .word   buffaddress
        //  .word   8-bit-byte-count
        case SCN_READLN6:   { data = scnreadln (putbuf6,   data, temp); break; }
        case SCN_READLN8:   { data = scnreadln (putbuf8,   data, temp); break; }
        case SCN_READLN12:  { data = scnreadln (putbuf12,  data, temp); break; }

        // write() system call
        //  .word   fd
        //  .word   buffaddress
        //  .word   8-bit-byte-count
        case SCN_WRITE6:  { data = scnwrite (getbuf6,   data, temp); break; }
        case SCN_WRITE8:  { data = scnwrite (getbuf8,   data, temp); break; }
        case SCN_WRITE12: { data = scnwrite (getbuf12,  data, temp); break; }

        // request interrupt at given time
        //  .word   SCN_IRQATTS
        //  .word   addr of uint48_t when to interrupt or 0 to stop
        case SCN_IRQATTS: {
            uint16_t irqataddr = readmemword (data + 1);
            uint64_t irqatts = readmemquad (irqataddr);
            pthread_mutex_lock (&lineclocklock);
            clrintreqmask (IRQ_LINECLOCK);
            if (irqatts != 0) {
                lineclockabs.tv_sec  = irqatts / 4096;
                lineclockabs.tv_nsec = ((irqatts % 4096) * 1000000000ULL) / 4096;
                if (! lineclockrun) {
                    pthread_t pid;
                    int rc = createthread (&pid, lineclockthread, NULL);
                    if (rc != 0) ABORT ();
                    pthread_detach (pid);
                    lineclockrun = true;
                }
            } else {
                memset (&lineclockabs, 0, sizeof lineclockabs);
            }
            pthread_cond_broadcast (&lineclockcond);
            pthread_mutex_unlock (&lineclocklock);
            data = 0;
            break;
        }

        // get mask of interrupting devices
        case SCN_INTACK: {
            data = getintreqmask () & 07777;
            break;
        }

        // unlink() system call
        //  .word   SCN_UNLINK{6,8,12}
        //  .word   address-of-path
        case SCN_UNLINK6:  { data = scnunlink (getstrz6,  data, temp); break; }
        case SCN_UNLINK8:  { data = scnunlink (getstrz8,  data, temp); break; }
        case SCN_UNLINK12: { data = scnunlink (getstrz12, data, temp); break; }

        // get command line arguments following oct filename
        //  .word   SCN_GETCMDARG{6,8,12}
        //  .word   argindex
        //  .word   buffersize
        //  .word   bufferaddr or null
        // returns:
        //     -1 : beyond end
        //   else : length of item (might be gt buffersize)
        case SCN_GETCMDARG6:  { data = scngetcmdarg (putbuf6,  data); break; }
        case SCN_GETCMDARG8:  { data = scngetcmdarg (putbuf8,  data); break; }
        case SCN_GETCMDARG12: { data = scngetcmdarg (putbuf12, data); break; }

        // floatingpoint functions
        //  .word   uint16_t SCN_xxx_DDD
        //  .word   double *resultaddr
        //  .word   double *leftopaddr
        //  .word   double *riteopaddr
        case SCN_ADD_DDD:
        case SCN_SUB_DDD:
        case SCN_MUL_DDD:
        case SCN_DIV_DDD: {
            double leftop = readmemdoub (readmemword (data + 4));
            double riteop = readmemdoub (readmemword (data + 6));
            double result;
            switch (scn) {
                case SCN_ADD_DDD: result = leftop + riteop; break;
                case SCN_SUB_DDD: result = leftop - riteop; break;
                case SCN_MUL_DDD: result = leftop * riteop; break;
                case SCN_DIV_DDD: result = leftop / riteop; break;
                default: ABORT ();
            }
            writememdoub (readmemword (data + 2), result);
            break;
        }

        //  .word   uint16_t SCN_xxx_FFF
        //  .word   float *resultaddr
        //  .word   float *leftopaddr
        //  .word   float *riteopaddr
        case SCN_ADD_FFF:
        case SCN_SUB_FFF:
        case SCN_MUL_FFF:
        case SCN_DIV_FFF: {
            float leftop = readmemflt (readmemword (data + 4));
            float riteop = readmemflt (readmemword (data + 6));
            float result;
            switch (scn) {
                case SCN_ADD_FFF: result = leftop + riteop; break;
                case SCN_SUB_FFF: result = leftop - riteop; break;
                case SCN_MUL_FFF: result = leftop * riteop; break;
                case SCN_DIV_FFF: result = leftop / riteop; break;
                default: ABORT ();
            }
            writememflt (readmemword (data + 2), result);
            break;
        }

        //  .word   uint16_t SCN_CMP_DD
        //  .word   uint16_t comparecode
        //  .word   double *leftopaddr
        //  .word   double *riteopaddr
        case SCN_CMP_DD: {
            uint16_t code = readmemword (data + 2);
            double leftop = readmemdoub (readmemword (data + 4));
            double riteop = readmemdoub (readmemword (data + 6));
            int res = -1;
            switch (code >> 4) {
                case 0: res = leftop >= riteop; break;
                case 1: res = leftop >  riteop; break;
                case 2: res = leftop != riteop; break;
            }
            data = (res ^ ((code >> 3) & 1)) & 017777;
            break;
        }

        //  .word   uint16_t SCN_CMP_FF
        //  .word   uint16_t comparecode
        //  .word   float *leftopaddr
        //  .word   float *riteopaddr
        case SCN_CMP_FF: {
            uint16_t code = readmemword (data + 2);
            float leftop  = readmemflt (readmemword (data + 4));
            float riteop  = readmemflt (readmemword (data + 6));
            int res = -1;
            switch (code >> 4) {
                case 0: res = leftop >= riteop; break;
                case 1: res = leftop >  riteop; break;
                case 2: res = leftop != riteop; break;
            }
            data = (res ^ ((code >> 3) & 1)) & 017777;
            break;
        }

        //  .word   uint16_t SCN_CVT_PF
        //  .word   result       w W z : by value ; else : by ref
        //  .word   operand      w W   : by value ; else : by ref
        //  .word   operandtype  w W l L q Q f d
        //  .word   resulttype   w W l L q Q f d z
        case SCN_CVT_FP: {
            uint16_t r0 = readmemword (data + 1);
            uint16_t r1 = readmemword (data + 2);
            uint32_t r2 = readmemlong (data + 3);
            double value = 0.0;
            switch (r2 & 07777) {
                case 'b': value = (double)  (int8_t) r1; break;
                case 'B': value = (double)  (uint8_t) r1; break;
                case 'w': value = (double) (int16_t) r1; break;
                case 'W': value = (double) (uint16_t) r1; break;
                case 'l': value = (double) (int32_t) readmemlong (r1); break;
                case 'L': value = (double) (uint32_t) readmemlong (r1); break;
                case 'q': value = (double) (int64_t) readmemquad (r1); break;
                case 'Q': value = (double) (uint64_t) readmemquad (r1); break;
                case 'f': value = (double)            readmemflt  (r1); break;
                case 'd': value = (double)            readmemdoub (r1); break;
                default: fprintf (stderr, "raspictl: invalid SCN_CVT_FP from %c\n", r2 & 0xFF);
            }
            //fprintf (stderr, "raspictl SCN_CVT_FP*: %c %g %c\n", r2 & 0xFF, value, r2 >> 8);
            switch (r2 >> 12) {
                case 'b': r0 = (int16_t) (int8_t) value; break;
                case 'B': r0 = (uint16_t) (uint8_t) value; break;
                case 'w': r0 = (int16_t) value; break;
                case 'W': r0 = (uint16_t) value; break;
                case 'l': writememlong (r0, (int32_t) value); break;
                case 'L': writememlong (r0, (uint32_t) value); break;
                case 'q': writememquad (r0, (int64_t) value); break;
                case 'Q': writememquad (r0, (uint64_t) value); break;
                case 'f': writememflt  (r0, (float)    value); break;
                case 'd': writememdoub (r0, (double)   value); break;
                case 'z': r0 = (value != 0.0);
                default: fprintf (stderr, "raspictl: invalid SCN_CVT_FP to %c\n", r2 >> 8);
            }
            data = r0 & 07777;
            break;
        }

        //  .word   SCN_UDIV
        //  .word   dividend<15:00>  ->  quotient
        //  .word   dividend<31:16>  ->  remainder
        //  .word   divisor
        case SCN_UDIV: {
            uint32_t dividend = readmemlong (data + 2);
            uint32_t divisor  = readmemword (data + 6);
            data = 017777;
            if (divisor != 0) {
                uint32_t quotient  = dividend / divisor;
                uint32_t remainder = dividend % divisor;
                data = ((quotient > 0xFFFF) ? -2 : 0) & 017777;
                writememlong (data + 2, (quotient & 0xFFFF) | (remainder << 16));
            }
            break;
        }

        //  .word   SCN_UMUL
        //  .word   multiplicand<15:00>  ->  product<31:16>
        //  .word   multiplicand<31:16>  ->  product<31:16>
        //  .word   multiplier
        case SCN_UMUL: {
            uint32_t multiplicand = readmemlong (data + 2);
            uint32_t multiplier   = readmemword (data + 6);
            uint64_t product      = multiplicand * multiplier;
            writememlong (data + 2, product);
            data = (product >> 32) & 07777;
            break;
        }

        case SCN_PRINTINSTR: {
            shadow.printinstr = (readmemword (data + 1) & 1) != 0;
            data = 0;
            break;
        }

        //  .word   SCN_NEG_DD
        //  .word   result
        //  .word   operand
        case SCN_NEG_DD: {
            double op = readmemdoub (readmemword (data + 4));
            writememdoub (readmemword (data + 2), - op);
            data = 0;
            break;
        }

        case SCN_NEG_FF: {
            float op = readmemflt (readmemword (data + 4));
            writememflt (readmemword (data + 2), - op);
            data = 0;
            break;
        }

        case SCN_WATCHWRITE: {
            watchwrite = readmemword (data + 1) | ((readmemword (data + 2) & 7) << 12);
            data = 0;
            break;
        }

        //  .word   SCN_GETENV
        //  .word   bufaddr
        //  .word   bufsize
        //  .word   nameaddr
        // returns 0 if not found, or len incl null
        case SCN_GETENV6:  { data = scngetenv (getstrz6,  putbuf6,  data, temp); break; }
        case SCN_GETENV8:  { data = scngetenv (getstrz8,  putbuf8,  data, temp); break; }
        case SCN_GETENV12: { data = scngetenv (getstrz12, putbuf12, data, temp); break; }

        //  .word   SCN_SETTTYECHO
        //  .word   fd
        //  .word   0 : disable echo
        //          1 : enable echo
        case SCN_SETTTYECHO: {
            struct termios ttyattrs;
            int  fd = (int)(int16_t) readmemword (data + 1);
            bool echo = readmemword (data + 2) != 0;
            if (savedtermioss.count (fd) == 0) {
                if (tcgetattr (fd, &ttyattrs) < 0) {
                    savederrno = errno;
                    data = 017777;  // -1
                    break;
                }
                savedtermioss[fd] = ttyattrs;
            } else {
                ttyattrs = savedtermioss[fd];
            }
            if (! echo) {
                cfmakeraw (&ttyattrs);
                ttyattrs.c_oflag |= OPOST;      // still insert CR before LF on output
                if (fd == STDIN_FILENO) {
                    ttyattrs.c_lflag |= ISIG;   // still handle ^C, ^Z
                }
            }
            if (tcsetattr (fd, TCSANOW, &ttyattrs) < 0) {
                savederrno = errno;
                data = 017776;  // -2
                break;
            }
            data = 0;
            break;
        }

        default: {
            fprintf (stderr, "raspictl: unknown syscall %u\n", scn);
            savederrno = ENOSYS;
            data = 017777;
            break;
        }
    }

    return data;
}

// called on exit to restore terminal characteristics
void ScnCall::exithandler ()
{
    // maybe turn stdin echo back on
    if (savedtermioss.count (0) != 0) {
        tcsetattr (0, TCSANOW, &savedtermioss[0]);
    }
}

// do println() system call
//  input:
//   getstrz = getstrz{6,8,12}
//   data = address of println parameter block
//   temp = temp buffer for string argument
static void scnprintln (GetStrz *getstrz, uint16_t data, uint8_t *temp)
{
    uint16_t addr = readmemword (data + 1);
    getstrz6 (addr, temp);
    fprintf (stderr, "raspictl: SCN_PRINTLN:%s\n", temp);
}

// do open() system call
//  input:
//   getstrz = getstrz{6,8,12}
//   data = address of open parameter block
//   temp = temp buffer for path argument
static uint16_t scnopen (GetStrz *getstrz, uint16_t data, uint8_t *temp)
{
    getstrz (readmemword (data + 1), temp);
    uint16_t flags = readmemword (data + 2);
    uint16_t mode = readmemword (data + 3);
    int rc = open ((char const *) temp, flags, mode);
    if (rc > 2047) {
        close (rc);
        errno = EMFILE;
        rc = -1;
    }
    data = rc & 017777;
    if (rc < 0) savederrno = errno;
    return data;
}

// do read() system call
//  input:
//   putbuf = putbuf{6,8,12}
//   data = address of read parameter block
//   temp = temp buffer
static uint16_t scnread (PutBuf *putbuf, uint16_t data, uint8_t *temp)
{
    int fd = (readmemword (data + 1) ^ 2048) - 2048;
    uint16_t adr = readmemword (data + 2);
    uint16_t len = readmemword (data + 3);
    int rc = read (fd, temp, len);
    data = rc & 017777;
    if (rc < 0) savederrno = errno;
       else putbuf (rc, adr, temp);
    return data;
}

// do read() system call up to end of line
//  input:
//   putbuf = putbuf{6,8,12}
//   data = address of read parameter block
//   temp = temp buffer
static uint16_t scnreadln (PutBuf *putbuf, uint16_t data, uint8_t *temp)
{
    int fd = (readmemword (data + 1) ^ 2048) - 2048;
    uint16_t adr = readmemword (data + 2);
    uint16_t len = readmemword (data + 3);
    int rc = 0;
    uint16_t i;
    for (i = 0; i < len;) {
        rc = read (fd, temp + i, 1);
        if (rc <= 0) break;
        if (temp[i++] == '\n') break;
    }
    if (rc >= 0) rc = i;
    data = rc & 017777;
    if (rc < 0) savederrno = errno;
       else putbuf (rc, adr, temp);
    return data;
}

// do write() system call
//  input:
//   getbuf = getbuf{6,8,12}
//   data = address of write parameter block
//   temp = temp buffer
static uint16_t scnwrite (GetBuf *getbuf, uint16_t data, uint8_t *temp)
{
    int fd = (readmemword (data + 1) ^ 2048) - 2048;
    uint16_t adr = readmemword (data + 2);
    uint16_t len = readmemword (data + 3);
    getbuf (len, adr, temp);
    int rc = read (fd, temp, len);
    data = rc & 017777;
    if (rc < 0) savederrno = errno;
    return data;
}

// do unlink() system call
//  input:
//   getstrz = getstrz{6,8,12}
//   data = address of open parameter block
//   temp = temp buffer for path argument
static uint16_t scnunlink (GetStrz *getstrz, uint16_t data, uint8_t *temp)
{
    getstrz (readmemword (data + 1), temp);
    int rc = unlink ((char const *) temp);
    data = rc & 017777;
    if (rc < 0) savederrno = errno;
    return data;
}

// get command-line argument
//  .word   SCN_GETCMDARG{6,8,12}
//  .word   argindex
//  .word   buffersize
//  .word   bufferaddr or 0
// returns:
//     -1 : beyond end
//   else : length of item (might be gt buffersize)
static uint16_t scngetcmdarg (PutBuf *putbuf, uint16_t data)
{
    uint16_t i = readmemword (data + 1);
    uint16_t retdata = 017777;
    if (i < cmdargc) {
        char const *cmdptr = cmdargv[i];
        uint16_t cmdlen = strlen (cmdptr);
        uint16_t buflen = readmemword (data + 2);
        if (buflen > cmdlen + 1) buflen = cmdlen + 1;
        uint16_t bufadr = readmemword (data + 3);
        if (bufadr != 0) {
            putbuf (buflen, bufadr, (uint8_t const *) cmdptr);
        }
        retdata = (cmdlen > 07777) ? 07777 : cmdlen;
    }
    return retdata;
}

//  .word   SCN_GETENV
//  .word   bufaddr
//  .word   bufsize
//  .word   nameaddr
// returns 0 if not found, or len incl null
static uint16_t scngetenv (GetStrz *getstrz, PutBuf *putbuf, uint16_t data, uint8_t *temp)
{
    getstrz (readmemword (data + 3), temp);
    char *envstr = getenv ((char const *) temp);
    if (envstr == NULL) return 0;
    uint16_t envlen = strlen (envstr) + 1;
    if (envlen > 07777) envlen = 07777;
    uint16_t bufsize = readmemword (data + 2);
    if (bufsize > 0) {
        if (envlen > bufsize) envlen = bufsize;
        uint16_t addr = readmemword (data + 1);
        putbuf (envlen, addr, (uint8_t const *) envstr);
    }
    return envlen;
}

// runs in background to post interrupt request whenever lineclockabs time is reached
// exits when woken with lineclockabs.tv == 0
static void *lineclockthread (void *dummy)
{
    pthread_mutex_lock (&lineclocklock);
    while (lineclockabs.tv_sec != 0) {

        // if already requsting an interrupt,
        //  block until SCN_LINECLOCK is called
        // else,
        //  block until the given time is reached or SCN_LINECLOCK is called
        int rc = (getintreqmask () & IRQ_LINECLOCK) ?
                pthread_cond_wait (&lineclockcond, &lineclocklock) :
                pthread_cond_timedwait (&lineclockcond, &lineclocklock, &lineclockabs);

        // if reached the requested time, post an interrupt request
        if (rc != 0) {
            if (rc != ETIMEDOUT) ABORT ();
            setintreqmask (IRQ_LINECLOCK);
        }
    }

    // SCN_LINECLOCK was passed a zero, so exit
    lineclockrun = false;
    pthread_mutex_unlock (&lineclocklock);
    return NULL;
}

static IODevOps const iodevops[] = {
    { 06310, "PIOFF (SYS) turn printinstr off" },
    { 06311, "PION (SYS) turn printinstr on" },
    { 06312, "HCF (SYS) dump everything and exit" },
};

ScnCall2::ScnCall2 ()
{
    opscount = sizeof iodevops / sizeof iodevops[0];
    opsarray = iodevops;
    iodevname = "scncall2";
}

void ScnCall2::ioreset ()
{ }

uint16_t ScnCall2::ioinstr (uint16_t opcode, uint16_t input)
{
    switch (opcode) {

        // PIOFF - turn printinstr off
        case 06310: {
            shadow.printinstr = false;
            break;
        }

        // PION - turn printinstr on
        case 06311: {
            shadow.printinstr = true;
            break;
        }

        // HCF - dump everything and exit
        case 06312: {
            uint32_t now = time (NULL);
            char hcfname[16];
            sprintf (hcfname, "%u.hcf", now);
            printf ("SysCall::ioinstr: HCF L=%o AC=%04o PC=%05o => %s\n", input / IO_LINK, input & IO_DATA, lastreadaddr, hcfname);
            FILE *hcffile = fopen (hcfname, "w");
            if (hcffile == NULL) {
                fprintf (stderr, "SysCall::ioinstr: error creating %s: %m\n", hcfname);
            } else {
                for (uint32_t i = 0; i < MEMSIZE; i += 16) {
                    for (uint32_t j = 0; j < 16; j ++) {
                        if (memarray[i+j] != 0) goto nonz;
                    }
                    continue;
                nonz:;
                    fprintf (hcffile, "  %05o:", i);
                    for (uint32_t j = 0; j < 16; j ++) {
                        fprintf (hcffile, " %04o", memarray[i+j]);
                    }
                    fprintf (hcffile, "  <");
                    for (uint32_t j = 0; j < 16; j ++) {
                        char c = memarray[i+j] & 0177;
                        if ((c < ' ') || (c > 0176)) c = '.';
                        fprintf (hcffile, "%c", c);
                    }
                    fprintf (hcffile, ">\n");
                }
            }
            haltordie ("HCFINSTR");
        }
    }
    return input;
}
