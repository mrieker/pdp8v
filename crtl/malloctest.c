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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define BLOCKSIZE(b) (*(__size_t *)((__size_t)b - 14));

int main (int argc, char **argv)
{
    void *alloclist = NULL;
    __size_t totalalloc = 0;
    __size_t numblocks = 0;

    printf ("starting\n");

    while (1) {
        __uint16_t nowns = (__uint16_t) getnowns ();
        __size_t size = nowns & 2047;
        if ((nowns & 256) && (totalalloc < 40*1024 - size)) {
            printf ("malloc size=%u\n", size);
            void *newalloc = malloc (size);
            *(void **)newalloc = alloclist;
            alloclist = newalloc;
            totalalloc += BLOCKSIZE (newalloc);
            numblocks ++;
            printf ("numblocks=%u totalalloc=%u\n", numblocks, totalalloc);
        } else if (alloclist != NULL) {
            void *freeblock = alloclist;
            totalalloc -= BLOCKSIZE (freeblock);
            alloclist = *(void **)freeblock;
            free (freeblock);
            -- numblocks;
            printf ("numblocks=%u totalalloc=%u\n", numblocks, totalalloc);
        }
    }
}
