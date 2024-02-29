
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "filesys.h"

static bool check (int *error, bool p, char const *fmt, ...)
{
    if (! p) {
        fputs ("fsuck: ", stderr);
        va_list ap;
        va_start (ap, fmt);
        vfprintf (stderr, fmt, ap);
        va_end (ap);
        ++ *error;
    }
    return p;
}

int Vol::fsuck ()
{
    int error = 0;

    // read bitmap (also gets home block at beginning)
    Word nbmwds = (this->nblocks + 11) / 12;
    Word bitmap[HBW+nbmwds];
    Word rc = this->disk->rlb (HBLBN, HBW + nbmwds, bitmap);
    bool havebm = check (&error, rc == HBW + nbmwds, "error %04o reading bitmap\n", rc);

    // set up array to keep track of what file uses a block
    Word inuse[this->nblocks];
    memset (inuse, 0, sizeof inuse);

    // check index file eof mark
    check (&error, this->indexf->eofwrd == 0, "non-zero indexf eofwrd %04o\n", this->indexf->eofwrd);
    check (&error, this->indexf->eofvbn <= this->indexf->allocd, "indexf eofvbn %04o gt allocd %04o\n", this->indexf->eofvbn, this->indexf->allocd);

    // loop through all file headers
    for (Word idxvbn = this->f0vbn; ++ idxvbn < this->indexf->eofvbn;) {

        // get file header lbn
        Word filnum = idxvbn - this->f0vbn;
        Word idxlbn = this->idxvbntolbn (idxvbn);
        if (! check (&error, idxlbn < ER_MIN, "error %04o getting file %04o header lbn\n", filnum)) break;

        // read file header block
        FHdr hdr;
        rc = this->disk->rlb (idxlbn, WPB, (Word *) &hdr);
        if (! check (&error, rc == WPB, "error %04o reading file %04o header at %04o\n", rc, filnum, idxlbn)) continue;

        // ignore if free
        if (hdr.de.filnum == 0) continue;

        // validate checksum
        Word ck = 0;
        for (Word i = 0; i < WPB; i ++) ck += ((Word *) &hdr)[i];
        ck &= 07777;
        if (! check (&error, ck == 0, "checsum error %04o header file %04o\n", ck, filnum)) continue;

        // validate number and sequence
        if (! check (&error, hdr.de.filnum == filnum, "header filnum %04o mismatch file %04o\n", hdr.de.filnum, filnum)) continue;
        if (! check (&error, hdr.de.filseq != 0, "header filseq zero file %04o\n", filnum)) continue;

        // compute how many extensions it should have
        Word numexts = 0;                               // number of extension headers
        Word directs = hdr.allocd;                      // number of direct pointers in primary header
        if (hdr.allocd > PPH) {                         // must have more pointers than would fit in primary header

            //     0   <= hdr.allocd <=   PPH    ->  0         -3   <= hdr.allocd-3 <=   PPH-3  ->  0
            //   PPH+1 <= hdr.allocd <= 2*PPH-2  ->  1        PPH-2 <= hdr.allocd-3 <= 2*PPH-5  ->  1
            // 2*PPH-1 <= hdr.allocd <= 3*PPH-4  ->  2      2*PPH-4 <= hdr.allocd-3 <= 3*PPH-7  ->  2
            // 3*PPH-3 <= hdr.allocd <= 4*PPH-6  ->  3      3*PPH-6 <= hdr.allocd-3 <= 4*PPH-9  ->  3
            numexts = (hdr.allocd - 3) / (PPH - 2);
            if (! check (&error, numexts <= PPH / 2, "numexts %u too big file %04o\n", numexts, filnum)) continue;
            directs = PPH - 2 * numexts;
        }

        // validate any blocks directly allocated by this header
        for (Word i = 0; i < directs; i ++) {
            Word lbn = hdr.blocks[i];
            if (! check (&error, lbn < this->nblocks, "lbn %04o too big file %04o\n", lbn, filnum)) continue;
            Word bmi = lbn / 12;
            Word bmb = lbn % 12;
            if (havebm) check (&error, (bitmap[HBW+bmi] >> bmb) & 1, "lbn %04o in use by file %04o free in bitmap\n", lbn, filnum);
            check (&error, inuse[lbn] == 0, "lbn %04o in use by both file %04o and %04o\n", lbn, inuse[lbn], filnum);
            inuse[lbn] = filnum;
        }

        // validate any extension headers pointed to by this header
        for (Word i = 0; i < numexts; i ++) {

            // get extension header info from end of pointer area
            Word xfilnum = hdr.blocks[PPH-i*2-2];
            Word xfilseq = hdr.blocks[PPH-i*2-1];

            // get extension header lbn
            Word xlbn = this->gethdrlbn (xfilnum);
            if (! check (&error, xlbn < ER_MIN, "error %04o getting ext hdr %04o lbn file %04o\n", xlbn, xfilnum, filnum)) continue;

            // read extension header from index file
            FHdr xhdr;
            rc = this->disk->rlb (xlbn, WPB, (Word *) &xhdr);
            if (! check (&error, rc == WPB, "error %04o reading ext hdr %04o file %04o\n", rc, xfilnum, filnum)) continue;

            // check ext header filnum,seq,nam
            if (! check (&error, xhdr.de.filnum == xfilnum, "ext hdr %04o filnum mismatch %04o file %04o\n", xhdr.de.filnum, xfilnum, filnum)) continue;
            if (! check (&error, xhdr.de.filseq == xfilseq, "ext hdr %04o filseq mismatch %04o file %04o\n", xhdr.de.filseq, xfilseq, filnum)) continue;
            bool namok = true;
            for (Word j = 0; j < DNLEN - 1; j ++) {
                namok &= (xhdr.de.filnam[j] == hdr.de.filnam[j]);
            }
            namok &= (xhdr.de.filnam[DNLEN-1] == ((hdr.de.filnam[DNLEN-1] + i + 1) & 07777));
            check (&error, namok, "name mismatch ext hdr %04o file %04o\n", xfilnum, filnum);

            // it should have between 3 and PPH lbn mappings
            // two for the ones it displaced in prime header and at least one more to justify itself
            Word allocdsb = PPH;
            if (i == numexts - 1) {

                // numexts = 1:
                //  hdr.allocd =   PPH+1  ->  allocdsb = 3
                //  hdr.allocd = 2*PPH-2  ->  allocdsb = PPH
                // numexts = 2:
                //  hdr.allocd = 2*PPH-1  ->  allocdsb = 3
                //  hdr.allocd = 3*PPH-4  ->  allocdsb = PPH
                // numexts = 3:
                //  hdr.allocd = 3*PPH-3  ->  allocdsb = 3
                //  hdr.allocd = 4*PPH-6  ->  allocdsb = PPH
                allocdsb = (hdr.allocd - 3) % (PPH - 2) + 3;
            }
            check (&error, xhdr.allocd == allocdsb, "ext hdr %04o allocd %04o sb %04o file %04o\n", xfilnum, xhdr.allocd, allocdsb, filnum);

            // its eofvbn,wrd should point to prime header
            check (&error, xhdr.eofvbn == hdr.de.filnum, "eofvbn mismatch ext hdr %04o file %04o\n", xfilnum, filnum);
            check (&error, xhdr.eofwrd == hdr.de.filseq, "eofblk mismatch ext hdr %04o file %04o\n", xfilnum, filnum);

            // checksum and blocks get checked when extension header is seen in main loop
        }
    }

    // make sure every block marked as allocated in the bitmap is pointed to by some file header
    if (havebm) {
        for (Word lbn = 0; lbn < this->nblocks; lbn ++) {
            Word bmi = lbn / 12;
            Word bmb = lbn % 12;
            if ((bitmap[HBW+bmi] >> bmb) & 1) {
                check (&error, inuse[lbn] != 0, "lbn %04o marked allocd in bitmap not used by any file\n", lbn);
            }
        }
    }
    return error;
}
