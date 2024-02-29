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

#include "linkloader.h"
#include "memory.h"

static int fouroctchars (char const *str);

int linkloader (char const *loadname, FILE *loadfile)
{
    char loadline[144], *p;
    int startaddr = 0;
    while (fgets (loadline, sizeof loadline, loadfile) != NULL) {
        uint32_t addr = strtoul (loadline, &p, 8);
        if (*(p ++) != ':') goto badload;
        bool hasdata = false;
        while (*p != '\n') {
            int data = fouroctchars (p);
            if ((data < 0) || (addr >= MEMSIZE)) goto badload;
            memarray[addr++] = data;
            p += 4;
            hasdata = true;
        }
        if (! hasdata) startaddr = addr;
    }
    return startaddr;
badload:;
    fprintf (stderr, "raspictl: bad loadfile line %s", loadline);
    return -1;
}

static int fouroctchars (char const *str)
{
    int data = 0;
    for (int i = 0; i < 4; i ++) {
        char c = str[i];
        if ((c < '0') || (c > '7')) return -1;
        data = data * 8 + c - '0';
    }
    return data;
}
