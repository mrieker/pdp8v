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

typedef __uint16_t uint16_t;

typedef struct Opcode Opcode;
typedef struct PSect PSect;
typedef struct Stack Stack;
typedef struct Symbol Symbol;
typedef struct Token Token;

struct PSect {
    PSect *next;            // next in psects list
    uint16_t p2al;          // log2 power of alignment
    uint16_t base;          // section base address
    uint16_t offs;          // offset for next object code generation
    uint16_t size;          // total size of the section
    char name[1];           // null terminated psect name
};

extern void *malloc (uint16_t size);

int main (int argc, char **argv)
{
    char const *asmname, *hexname;
    PSect *psect, *textpsect;
    uint16_t base, mask;

    if (argc != 3) {
        return 1;
    }

    asmname = argv[1];

    textpsect = (PSect *) malloc (5 + sizeof *textpsect);

    return 0;
}
