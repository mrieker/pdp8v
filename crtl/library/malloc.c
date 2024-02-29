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

#include <assert.h>
#include <hode.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EXTENDSIZE 1024
#define MINSIZE 16
#define NGUARDS 0
#define OVERHEAD 8          // must include room for Block plus c-array size word (see __malloc2)
#define POISON 0xDEADU
#define VERIFY 1

#define PTR2BLK(p) ((Block *)((__size_t) (p) - (NGUARDS + 1) * OVERHEAD))
#define BLK2PTR(b) ((void *)((__size_t) (b) + (NGUARDS + 1) * OVERHEAD))
#define RNDSIZE(s) (((s) + (NGUARDS * 2 + 2) * OVERHEAD - 1) & - OVERHEAD)
#define VERIFYBLOCKS() do { if (VERIFY) verifyblocks (__LINE__); } while (0)

struct Block {
    Block *next;
    __size_t size;
};

extern char __endrw[];

static Block *endinitted;
static Block *freeblocks;

static void extendmemory (__size_t extendby);
static void freeablock (Block *block);
static void fillguards (Block *block);
static void verifyblocks (int line);
static void checkguards (Block *block);
static void verifyfailed (int line1, int line2);
static Block *roundedendrw ();

void *calloc (__size_t nmemb, __size_t size)
{
    __size_t totl = nmemb * size;
    assert (totl / nmemb == size);
    void *ptr = malloc (totl);
    memset (ptr, 0, totl);
    return ptr;
}

void *realloc (void *ptr, __size_t size)
{
    if (ptr == NULL) return malloc (size);

    VERIFYBLOCKS ();

    // round up size to our increment including block header
    __size_t rsiz = RNDSIZE (size);
    if (rsiz < MINSIZE) rsiz = MINSIZE;

    // point to block header and make sure it is marked allocated
    Block *block = PTR2BLK (ptr);
    assert (block->next == block);

    // if it is already big enough, just reuse the same block
    if (block->size >= rsiz) goto freeexcess;

    // see how much space is needed and point to block after this one
    __size_t needed = rsiz - block->size;
    Block *afterbl = (Block *) ((__size_t) block + block->size);
    assert (afterbl <= endinitted);

    // maybe extending memory will make the after block big enough
    if ((afterbl == endinitted) || ((afterbl->next == NULL) && (afterbl->size < needed))) {
        __size_t extendby = (__size_t) block + rsiz - (__size_t) endinitted;
        extendmemory (extendby);
        assert ((afterbl < endinitted) && (afterbl->next != afterbl) && (afterbl->size >= needed));
    }

    // maybe there is enough free memory following this block
    assert (afterbl < endinitted);
    if ((afterbl->next != afterbl) && (afterbl->size >= needed)) {

        // unlink afterbl from freeblocks list
        Block *nextbl, **prevbl;
        for (prevbl = &freeblocks; (nextbl = *prevbl) != afterbl; prevbl = &nextbl->next) {
            assert ((__size_t) nextbl > (__size_t) prevbl);
        }
        *prevbl = afterbl->next;

        // merge the sizes
        block->size += afterbl->size;
        assert (block->size >= rsiz);
        VERIFYBLOCKS ();
        goto freeexcess;
    }

    // next block is in use or not big enough, allocate new block, memcpy, then free old block
    void *newptr = malloc (size);
    memcpy (newptr, ptr, block->size - (NGUARDS * 2 + 1) * OVERHEAD);
    free (ptr);
    return newptr;

    // block already had enough room or was merged with a following block that had enough room
    // free off any excess then just return original pointer
freeexcess:
    __size_t extra = block->size - rsiz;
    if (extra >= MINSIZE) {
        block->size = rsiz;
        fillguards (block);
        block = (Block *) ((__size_t) block + rsiz);
        block->next = block;
        block->size = extra;
        fillguards (block);
        freeablock (block);
    }
    return ptr;
}

// used by compiler 'new'
void *__malloc2 (__size_t nmemb, __size_t size)
{
    __size_t totl = nmemb * size;
    assert (totl / nmemb == size);
    void *ptr = malloc (totl);
    ((__size_t *)ptr)[-1] = nmemb;  // must match CARROFFS in cc/mytypes.h
    return ptr;
}

void *malloc (__size_t size)
{
    VERIFYBLOCKS ();

    // round up size to our increment including block header
    size = RNDSIZE (size);
    if (size < MINSIZE) size = MINSIZE;

    while (1) {

        // look for first fit in existing free list
        Block *lastfree = NULL;
        Block *block, **lblock;
        for (lblock = &freeblocks; (block = *lblock) != NULL; lblock = &block->next) {
            assert (block > lastfree);
            if (block->size >= size) {
                *lblock = block->next;
                __size_t extrasize = block->size - size;
                if (extrasize >= MINSIZE) {
                    Block *xblock = (Block *) ((__size_t) block + size);
                    xblock->next = block->next;
                    xblock->size = extrasize;
                    fillguards (xblock);
                    *lblock = xblock;
                    block->size = size;
                    fillguards (block);
                }
                block->next = block;
                VERIFYBLOCKS ();
                return BLK2PTR (block);
            }
            lastfree = block;
        }

        // nothing big enough, get amount to extend by
        __size_t extendby = size;
        if (lastfree != NULL) {
            assert (lastfree != NULL);
            assert (lastfree->size < size);
            extendby -= lastfree->size;
        }
        extendmemory (extendby);
    }
}

__size_t allocsz (void *ptr)
{
    if (ptr == NULL) return 0;
    Block *block = PTR2BLK (ptr);
    assert ((block >= roundedendrw ()) && (block < endinitted));
    assert (block->size % OVERHEAD == 0);
    assert (block->size >= MINSIZE);
    assert (block->size <= (__size_t) endinitted - (__size_t) block);
    assert (block->next == block);
    return block->size - (NGUARDS * 2 + 1) * OVERHEAD;
}

void free (void *ptr)
{
    if (ptr != NULL) {
        Block *block = PTR2BLK (ptr);
        freeablock (block);
    }
}

__size_t freememscan (__size_t *totbytes_r, __size_t *smallest_r, __size_t *largest_r)
{
    __size_t totbytes = 0;
    __size_t smallest = (__size_t) -1;
    __size_t largest  = 0;
    __size_t nblocks  = 0;
    Block *lastblock  = NULL;
    for (Block *block = freeblocks; block != NULL; block = block->next) {
        assert (block > lastblock);
        nblocks ++;
        totbytes += block->size;
        if (smallest > block->size) smallest = block->size;
        if (largest  < block->size) largest  = block->size;
        lastblock = block;
    }
    *totbytes_r = totbytes;
    *smallest_r = smallest;
    *largest_r  = largest;
    return nblocks;
}

static void extendmemory (__size_t extendby)
{
    if (extendby >= 0xFFFF - EXTENDSIZE) goto oom;

    extendby = (extendby + EXTENDSIZE - 1) & - EXTENDSIZE;

    if (endinitted == NULL) {
        __size_t endofused = roundedendrw ();
        endinitted = roundedendrw ();
        if (endofused & (EXTENDSIZE - 1)) {
            extendby += EXTENDSIZE - (endofused & (EXTENDSIZE - 1));
        }
    }

    //ass_putstr ("malloc*: endinitted="); ass_putptr (endinitted); ass_putstr (" extendby="); ass_putuinthex (extendby); ass_putchr ('\n');

    // make the extension look like an allocated block
    Block *block = endinitted;
    block->next  = block;
    block->size  = extendby;
    fillguards (block);

    // update new end of memory in use
    endinitted = (void *) ((__size_t) endinitted + extendby);

    // make sure we haven't run out of memory
    // - &extendby = approximate stack pointer
    if (((__size_t) endinitted < extendby) || ((__size_t) endinitted > (__size_t) &extendby - EXTENDSIZE)) goto oom;

    // crash if stack pointer goes below that
    setstklim ((__size_t) endinitted);

    // free off the extension
    freeablock (block);
    return;

oom:
    ass_putstr ("\nmalloc: OUT OF MEMORY; extendby="); ass_putuinthex (extendby); ass_putchr ('\n');
    __size_t totbytes, smallest, largest;
    __size_t nblocks = freememscan (&totbytes, &smallest, &largest);
    ass_putstr ("  nblocks="); ass_putint (nblocks); ass_putstr (" totbytes="); ass_putint (totbytes); ass_putstr (" smallest="); ass_putint (smallest); ass_putstr (" largest="); ass_putint (largest); ass_putchr ('\n');
    exit (255);
}

static void freeablock (Block *block)
{
    Block **lblock, *xblock, *yblock;

    assert ((block >= roundedendrw ()) && (block < endinitted));
    assert (block->size % OVERHEAD == 0);
    assert (block->size >= MINSIZE);
    assert (block->size <= (__size_t) endinitted - (__size_t) block);
    assert (block->next == block);

    VERIFYBLOCKS ();

    if (NGUARDS > 0) {
        __size_t *end = (__size_t *) ((__size_t) block + block->size);
        do *(-- end) = POISON;
        while ((__size_t) end > (__size_t) block + OVERHEAD);
    }

    // find insertion point by ascending block address
    xblock = NULL;
    for (lblock = &freeblocks; (yblock = *lblock) != NULL; lblock = &yblock->next) {

        // stop if next one comes after one being freed
        if (yblock > block) break;
        xblock = yblock;

        // next one should always be at ascending address
        assert ((yblock->next == NULL) || (yblock->next > yblock));
    }

    // xblock .. block .. yblock

    // either merge with preceding block or link just after it
    if ((xblock != NULL) && ((__size_t) xblock + xblock->size == (__size_t) block)) {
        xblock->size += block->size;
        block = xblock;
    } else {
        *lblock = block;
        block->next = yblock;
    }

    // see if it merges with block after this
    if ((__size_t) block + block->size == (__size_t) yblock) {
        block->next  = yblock->next;
        block->size += yblock->size;
    }

    VERIFYBLOCKS ();
}

static void fillguards (Block *block)
{
    __size_t *beg = (__size_t *)((__size_t) block + OVERHEAD);
    __size_t *end = (__size_t *)((__size_t) block + block->size);
    for (int i = 0; i < NGUARDS * OVERHEAD / sizeof (__size_t); i ++) {
        *(beg ++) = POISON;
        *(-- end) = POISON;
    }
}

// verify block list intact
//  starts at __endrw, ends at endinitted
//  freeblocks list is always ascending
//  gaps in freeblocks list is exactly filled by allocated blocks
static void verifyblocks (int line)
{
    if (endinitted == NULL) return;

    Block *nextfree  = freeblocks;
    Block *thisblock = roundedendrw ();

    while (thisblock < endinitted) {
        checkguards (thisblock);

        // see if this block should be free
        if (thisblock == nextfree) {

            // barf if it is marked allocated
            if (thisblock->next == thisblock) {
                verifyfailed (line, __LINE__);
            }

            // it should point to this free block which should be higher or NULL
            nextfree = thisblock->next;
            if ((nextfree != NULL) && (nextfree <= thisblock)) {
                verifyfailed (line, __LINE__);
            }
        } else {

            // this block should be allocated, barf if marked free
            if (thisblock->next != thisblock) {
                verifyfailed (line, __LINE__);
            }
        }

        // point to next block, size bytes after this block
        Block *nb = (Block *) ((__size_t) thisblock + thisblock->size);
        if (nb <= thisblock) {
            verifyfailed (line, __LINE__);
        }
        thisblock = nb;
    }

    // we shouldn't run off past the end
    if (thisblock > endinitted) {
        verifyfailed (line, __LINE__);
    }

    // we should have found all the free blocks
    if (nextfree != NULL) {
        verifyfailed (line, __LINE__);
    }
}

static void checkguards (Block *block)
{
    assert (block->size % OVERHEAD == 0);
    __size_t *beg = (__size_t *)((__size_t) block + OVERHEAD);
    __size_t *end = (__size_t *)((__size_t) block + block->size);
    for (int i = 0; i < NGUARDS * OVERHEAD / sizeof (__size_t); i ++) {
        assert (*(beg ++) == POISON);
        assert (*(-- end) == POISON);
    }
}

static void verifyfailed (int line1, int line2)
{
    ass_putstr ("malloc verifyfailed*: ");
    ass_putstr (__FILE__);
    ass_putchr (':');
    ass_putint (line1);
    ass_putchr (':');
    ass_putint (line2);
    ass_putchr ('\n');
    exit (255);
}

static Block *roundedendrw ()
{
    __size_t xb = (__size_t) __endrw;
    return (Block *) ((xb + OVERHEAD - 1) & - OVERHEAD);
}
