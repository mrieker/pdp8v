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

// keep track of what accesses what memory for dynamic disassembly purpose

#include <pthread.h>
#include <stdio.h>
#include <string.h>

#include "disassemble.h"
#include "dyndis.h"
#include "iodevs.h"
#include "memory.h"
#include "shadow.h"

struct Access {
    Access *next;
    bool write;
    Shadow::State state;
    uint16_t pc;
};

bool dyndisena;
uint16_t dyndispc;

static Access *accesses[MEMSIZE];
static pthread_mutex_t accesslock = PTHREAD_MUTEX_INITIALIZER;

static void addaccess (uint16_t addr, bool write, Shadow::State state, uint16_t pc);

// clear all dynamic disassembly entries
void dyndisclear ()
{
    pthread_mutex_lock (&accesslock);
    for (uint16_t addr = 0; addr < MEMSIZE; addr ++) {
        for (Access *access; (access = accesses[addr]) != NULL;) {
            accesses[addr] = access->next;
            delete access;
        }
    }
    pthread_mutex_unlock (&accesslock);
}

// cpu is reading memory
//  input:
//   addr = 15-bit address being read from
//   processor in middle of state doing read
void dyndisread (uint16_t addr)
{
    if (shadow.r.state == Shadow::FETCH2) {
        dyndispc = addr;
    }
    addaccess (addr, false, shadow.r.state, dyndispc);
}

// cpu is writing memory
//  input:
//   addr = 15-bit address being written to
//   processor in middle of state after state doing write
//   memory location has been updated
void dyndiswrite (uint16_t addr)
{
    addaccess (addr, true, shadow.getprevstate (), dyndispc);
}

// iodev is directly accessing memory
//  input:
//   addr = starting address
//   size = number of words starting at addr
//   write = true: locations being written
//          false: locations being read
//   iopc = 15-bit pc where the i/o instruction was issued
void dyndisdma (uint16_t addr, uint16_t size, bool write, uint16_t iopc)
{
    if (dyndisena) {
        while (size != 0) {
            addaccess (addr, write, Shadow::DMA, iopc);
            addr = (addr & 070000) | ((addr + 1) & 007777);
            -- size;
        }
    }
}

// dump the disassembly to the given file
//  input:
//   dumpfile = file to dump the disassembly to
void dyndisdump (FILE *dumpfile)
{
    pthread_mutex_lock (&accesslock);

    for (uint16_t addr = 0; addr < MEMSIZE; addr ++) {
        bool accessed = false;
        bool fetched = false;
        for (Access *access = accesses[addr]; access != NULL; access = access->next) {
            accessed = true;
            if (access->state == Shadow::FETCH2) fetched = true;
        }
        if (accessed) {
            uint16_t data = memarray[addr];
            char const *d = "";
            std::string dstr;
            if (fetched) {
                d = iodisas (data);
                if (d == NULL) {
                    dstr = disassemble (data, addr);
                    d = dstr.c_str ();
                }
            }
            fprintf (dumpfile, "%05o:  %04o  %s\n", addr, data, d);

            for (Access *access = accesses[addr]; access != NULL; access = access->next) {
                if (access->state != Shadow::FETCH2) {
                    fprintf (dumpfile, "    %-6s  %c  %05o\n", Shadow::statestr (access->state), (access->write ? 'w' : 'r'), access->pc);
                }
            }
        }
    }

    pthread_mutex_unlock (&accesslock);
}

// add access type to memory address if not already there
//  input:
//   addr = 15-bit address being accessed
//   write = it is a write access
//   state = cpu state being accessed from
//   pc = 15-bit pc of instruction causing the access
static void addaccess (uint16_t addr, bool write, Shadow::State state, uint16_t pc)
{
    ASSERT (addr < MEMSIZE);
    pthread_mutex_lock (&accesslock);
    for (Access *access = accesses[addr]; access != NULL; access = access->next) {
        if ((access->write == write) && (access->state == state) && (access->pc == pc)) goto ret;
    }
    {
        Access *access = new Access ();
        access->next   = accesses[addr];
        access->write  = write;
        access->state  = state;
        access->pc     = pc;
        accesses[addr] = access;
    }
ret:;
    pthread_mutex_unlock (&accesslock);
}
