
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <time.h>
#include <unistd.h>

#include "filesys.h"

#define ABORT() do { fprintf (stderr, "ABORT %s:%d\n", __FILE__, __LINE__); abort (); } while (0)
#define ASSERT(cond) do { if (__builtin_constant_p (cond)) { if (!(cond)) asm volatile ("assert failure"); } else { if (!(cond)) ABORT (); } } while (0)

#define F0VBN(ndb) HBLBN + ((ndb + 11) / 12 + HBW + WPB - 1) / WPB - 1;

static int callerline;

struct Recurck {
    int outercallerline;

    Recurck (int line)
    {
        if (line <= callerline) {
            fprintf (stderr, "recurck: callerline=%d line=%d\n", callerline, line);
            abort ();
        }
        outercallerline = callerline;
        callerline = line;
    }

    ~Recurck ()
    {
        callerline = outercallerline;
    }
};

#define RECURCK Recurck recurck (__LINE__)

void getnow48 (Word *time48);

/////////////////////
////// LEVEL 8 //////
/////////////////////

// unmount volume
Word Vol::umount ()
{
    RECURCK;

    if (this->indexf != NULL) {

        // if files other than index open, cannot unmount
        if ((this->openfiles != this->indexf) || (this->indexf->nextfile != NULL) || (this->indexf->refcnt != 1)) return ER_BSY;

        // close index file
        this->indexf->close ();
        this->indexf = NULL;
    }

    ASSERT (this->openfiles == NULL);
    return 0;
}

/////////////////////
////// LEVEL 7 //////
/////////////////////

// close the file
// if marked for delete, delete it
// if last reference, unlink from volume and free it
// do not reference the object after calling this function
Word File::close ()
{
    RECURCK;

    Word rc = 0;
    ASSERT (this->vol != NULL);
    ASSERT (this->refcnt > 0);
    if (-- this->refcnt == 0) {

        // if marked for delete, delete it
        if (this->markdl) {
            rc = this->delit ();
        }

        // close by unlinking from volume
        File **lfile, *xfile;
        for (lfile = &this->vol->openfiles; (xfile = *lfile) != NULL; lfile = &xfile->nextfile) {
            if (xfile == this) {
                *lfile = this->nextfile;
                break;
            }
        }
        this->vol = NULL;

        // free off this object
        delete this;
    }
    return rc;
}

// given the opened file is a directory, enter the given entry
Word File::enter (Dirent *dirent)
{
    RECURCK;

    if (dirent->filnum == 0) return ER_BFN;
    if (dirent->filseq == 0) return ER_BFS;

    // look to see if there is a duplicate name entry for the file
    // also find an empty spot if there is one
    Word emptylbn = 0;
    Word emptyidx = 0;
    for (Word vbn = 0; vbn < this->eofvbn; vbn ++) {
        Word lbn = this->vbntolbn (vbn);
        if (lbn >= ER_MIN) return lbn;
        Dirent entries[EPB];
        Word rc = this->vol->disk->rlb (lbn, WPB, (Word *) entries);
        if (rc >= ER_MIN) return rc;
        ASSERT (rc == WPB);
        for (Word i = 0; i < EPB; i ++) {
            Dirent *de = &entries[i];
            if (de->filnum != 0) {
                int diff = File::cmpdirent (de, dirent);
                if (diff == 0) return ER_DUP;
            } else if (emptylbn == 0) {
                emptylbn = lbn;
                emptyidx = i;
            }
        }
    }

    // name doesn't exist already, see if an empty spot was found anywhere
    if (emptylbn != 0) {

        // re-read the block with the empty spot
        Dirent entries[EPB];
        Word rc = this->vol->disk->rlb (emptylbn, WPB, (Word *) entries);
        if (rc >= ER_MIN) return rc;

        // fill it in and write it back out
        ASSERT (entries[emptyidx].filnum == 0);
        entries[emptyidx] = *dirent;
        rc = this->vol->disk->wlb (emptylbn, WPB, (Word *) entries);
        if (rc >= ER_MIN) return rc;
    } else {

        // directory file is completely full, extend and write entry to new end block
        Word rc = this->extend (this->eofvbn + 1);
        if (rc >= ER_MIN) return rc;
        Word lbn = this->vbntolbn (this->eofvbn);
        if (lbn >= ER_MIN) return lbn;
        Dirent entries[EPB];
        memset (entries, 0, sizeof entries);
        entries[0] = *dirent;
        rc = this->vol->disk->wlb (lbn, WPB, (Word *) entries);
        if (rc >= ER_MIN) return rc;
        this->eofvbn ++;
    }

    getnow48 (this->modtim);
    return this->updattrs ();
}

// given the opened file is a directory, search it for the given name
Word File::lookup (Dirent *dirent)
{
    RECURCK;

    Dirent entries[EPB];
    for (Word vbn = 0; vbn < this->eofvbn; vbn ++) {
        Word lbn = this->vbntolbn (vbn);
        if (lbn >= ER_MIN) return lbn;
        Word rc = this->vol->disk->rlb (lbn, WPB, (Word *) entries);
        if (rc >= ER_MIN) return rc;
        ASSERT (rc == WPB);
        for (Word i = 0; i < EPB; i ++) {
            Dirent *de = &entries[i];
            if (de->filnum != 0) {
                int diff = File::cmpdirent (de, dirent);
                if (diff == 0) {
                    dirent->filnum = de->filnum;
                    dirent->filseq = de->filseq;
                    return 0;
                }
            }
        }
    }
    return ER_NSF;
}

// given the opened file is a directory, remove the given entry
Word File::remove (Dirent *dirent)
{
    RECURCK;

    Dirent entries[EPB];
    for (Word vbn = 0; vbn < this->eofvbn; vbn ++) {
        Word lbn = this->vbntolbn (vbn);
        if (lbn >= ER_MIN) return lbn;
        Word rc = this->vol->disk->rlb (lbn, WPB, (Word *) entries);
        if (rc >= ER_MIN) return rc;
        ASSERT (rc == WPB);
        for (Word i = 0; i < EPB; i ++) {
            Dirent *de = &entries[i];
            if (de->filnum != 0) {
                int diff = File::cmpdirent (de, dirent);
                if (diff == 0) {
                    if (de->filnum != dirent->filnum) return ER_BFN;
                    if (de->filseq != dirent->filseq) return ER_BFS;
                    memset (de, 0, sizeof *de);
                    rc = this->vol->disk->wlb (lbn, WPB, (Word *) entries);
                    if (rc >= ER_MIN) return rc;
                    getnow48 (this->modtim);
                    return this->updattrs ();
                }
            }
        }
    }
    return ER_NSF;
}

// read file from current position
// increments current position
Word Stream::read (Word *buffer, Word nwords)
{
    RECURCK;

    Word nread = 0;
    while (nwords > 0) {
        if (this->curvbn > this->file->eofvbn) break;
        if (this->curvbn == this->file->eofvbn) {
            if (this->file->eofwrd <= this->curwrd) break;
            Word n = this->file->eofwrd - this->curwrd;
            if (nwords > n) nwords = n;
        }
        Word lbn = this->curlbn;
        if (lbn == 0) {
            lbn = this->file->vbntolbn (this->curvbn);
            if (lbn >= ER_MIN) return lbn;
            this->curlbn = lbn;
        }
        Word nw = WPB - this->curwrd;
        if (nw > nwords) nw = nwords;
        if (this->curwrd == 0) {
            Word rc = file->vol->disk->rlb (lbn, nw, buffer);
            if (rc >= ER_MIN) return rc;
            ASSERT (rc == nw);
            buffer += nw;
        } else {
            Word temp[WPB];
            Word rc = file->vol->disk->rlb (lbn, nw + this->curwrd, temp);
            if (rc >= ER_MIN) return rc;
            ASSERT (rc == nw + this->curwrd);
            for (rc = 0; rc < nw; rc ++) {
                *(buffer ++) = temp[this->curwrd+rc];
            }
        }
        nread  += nw;
        nwords -= nw;
        this->curwrd += nw;
        if (this->curwrd >= WPB) {
            this->curvbn += this->curwrd / WPB;
            this->curwrd %= WPB;
            this->curlbn  = 0;
        }
    }
    return nread;
}

// write file from current position
// increments current position
// extends file as needed
Word Stream::write (Word *buffer, Word nwords)
{
    RECURCK;

    if (this->rdonly) return ER_RDO;

    Word nwrite = 0;
    while (nwords > 0) {
        Word lbn, nw, rc;

        this->dirty ++;

        // make sure there is a block to write to
        if (this->curvbn >= this->file->allocd) {
            rc = this->file->extend (this->curvbn + 1);
            if (rc >= ER_MIN) {
                if ((rc != ER_FUL) && (rc != ER_IFF)) return rc;
                nwords = 0;
                goto done;
            }
            this->dirty = 0;
        }

        // get logical block number to write to
        lbn = this->curlbn;
        if (lbn == 0) {
            lbn = this->file->vbntolbn (this->curvbn);
            if (lbn >= ER_MIN) return lbn;
            this->curlbn = lbn;
        }

        // compute number of words to write
        // not beyond end of block cuz that's all lbn is good for
        ASSERT (this->curwrd < WPB);
        nw = WPB - this->curwrd;
        if (nw > nwords) nw = nwords;

        // write some words out
        if (this->curwrd == 0) {
            Word rc = file->vol->disk->wlb (lbn, nw, buffer);
            if (rc >= ER_MIN) return rc;
            ASSERT (rc == nw);
            buffer += nw;
        } else {
            Word temp[WPB];
            Word rc = file->vol->disk->rlb (lbn, WPB, temp);
            if (rc >= ER_MIN) return rc;
            for (rc = 0; rc < nw; rc ++) {
                temp[this->curwrd+rc] = *(buffer ++);
            }
            rc = file->vol->disk->wlb (lbn, WPB, temp);
            if (rc >= ER_MIN) return rc;
            ASSERT (rc == WPB);
        }

        // increment past that number of words
        nwrite += nw;
        nwords -= nw;

        // increment current position
        this->curwrd += nw;
        if (this->curwrd >= WPB) {
            this->curvbn += this->curwrd / WPB;
            this->curwrd %= WPB;
            this->curlbn  = 0;
        }

        // update eof position to be at least where we finished writing
        if ((this->file->eofvbn < this->curvbn) || ((this->file->eofvbn == this->curvbn) && (this->file->eofwrd < this->curwrd))) {
            this->file->eofvbn = this->curvbn;
            this->file->eofwrd = this->curwrd;
            this->dirty ++;
        }

        // write header block if dirty enough
        if (this->dirty < 16) continue;
    done:;
        getnow48 (this->file->modtim);
        rc = this->file->updattrs ();
        if (rc >= ER_MIN) return rc;
        this->dirty = 0;
    }

    // return actual number of words written
    return nwrite;
}

/////////////////////
////// LEVEL 6 //////
/////////////////////

// internal routine to delete file
// - just set this->markdl and it gets deleted when closed
Word File::delit ()
{
    RECURCK;

    FHdr phdr;
    Word rc = this->rphb (&phdr);
    if (rc >= ER_MIN) return rc;

    // set filnum to zero and write header back out without checksumming
    // this frees the header
    if (this->vol->lofreehdr > phdr.de.filnum) this->vol->lofreehdr = phdr.de.filnum;
    phdr.de.filnum = 0;
    rc = this->vol->disk->wlb (this->hdrlbn, WPB, (Word *) &phdr);
    if (rc >= ER_MIN) return rc;

    // see if there are any extension headers
    if (this->allocd > PPH) {

        // see how many extension headers there are
        Word numexts = (this->allocd - 3) / (PPH - 2);
        phdr.allocd = PPH - 2 * numexts;

        // free off blocks in extension headers
        while (numexts > 0) {
            FHdr xhdr;
            Word xfilnum = phdr.blocks[PPH-2*numexts+0];
            Word xfilseq = phdr.blocks[PPH-2*numexts+1];
            Word xhdrlbn;
            rc = this->rxhb (&xhdr, xfilnum, xfilseq, &xhdrlbn);
            if (rc >= ER_MIN) return rc;
            if (this->vol->lofreehdr > xhdr.de.filnum) this->vol->lofreehdr = xhdr.de.filnum;
            xhdr.de.filnum = 0;
            rc = this->vol->disk->wlb (xhdrlbn, WPB, (Word *) &xhdr);
            if (rc >= ER_MIN) return rc;
            this->vol->freeblks (xhdr.allocd, xhdr.blocks);
            -- numexts;
        }
    }

    // free off blocks in primary header
    this->vol->freeblks (phdr.allocd, phdr.blocks);

    return 0;
}

// extend the file to the given total number of blocks
Word File::extend (Word newtot)
{
    RECURCK;

    if (newtot <= this->allocd) return 0;

    // read prime header
    FHdr phdr;
    Word rc = this->rphb (&phdr);
    if (rc >= ER_MIN) return rc;

    getnow48 (phdr.modtim);

    // set up extension header block that is empty
    // ...in case we need it later
    FHdr xhdr;
    xhdr.de.filnum = 0;

    do {
        ASSERT (phdr.allocd == this->allocd);

        // allocate block before mucking with headers
        rc = this->vol->allocblk ();
        if (rc >= ER_MIN) goto errex;
        Word lbn = rc;

        // see if we have a small file, ie, all direct pointers in prime header, and room for at least one more
        if (this->allocd < PPH) {
            phdr.blocks[this->allocd] = lbn;
        } else {

            // see which extension header we will be accessing
            Word xindex = (this->allocd - PPH) / (PPH - 2);

            // if prime header is already full of extension headers, nothing more we can do
            rc = ER_FHF;
            if (xindex >= PPH / 2) goto errex;

            // see which word within the extension header we will be writing
            Word pindex = (this->allocd - PPH) % (PPH - 2);

            // see if will need a new extension header to hold a new block
            if (pindex == 0) {

                // flush any previous extension header
                if (xhdr.de.filnum != 0) {
                    memcpy (xhdr.modtim, phdr.modtim, sizeof xhdr.modtim);
                    rc = this->wxhb (&xhdr);
                    if (rc >= ER_MIN) goto errex2;
                }

                // allocate new extension header
                Dirent xde = phdr.de;
                xde.filnam[DNLEN-1] += xindex + 1;
                xde.filnam[DNLEN-1] &= 07777;
                rc = this->vol->allochdr (&xhdr, &xde);
                if (rc >= ER_MIN) goto errex2;

                // extension header eofvbn,wrd = primary filnum,seq for redundancy
                xhdr.eofvbn = this->filnum;
                xhdr.eofwrd = this->filseq;

                // copy last two pointers from prime header to extension header
                xhdr.blocks[0] = phdr.blocks[PPH-2-xindex*2];
                xhdr.blocks[1] = phdr.blocks[PPH-1-xindex*2];

                // follow that with newly allocated block
                xhdr.blocks[2] = lbn;

                xhdr.allocd = 3;

                // put extension header filnum,seq in the two pointers in prime header
                phdr.blocks[PPH-2-xindex*2] = xhdr.de.filnum;
                phdr.blocks[PPH-1-xindex*2] = xhdr.de.filseq;
            }

            // newly allocated blocks go in existing extension header
            // see which extension header the new block goes in
            else {
                Word xfilnum = phdr.blocks[PPH-2*xindex-2];
                Word xfilseq = phdr.blocks[PPH-2*xindex-1];

                // read existing extension header
                // flush any previous extension header
                if (xhdr.de.filnum != xfilnum) {
                    if (xhdr.de.filnum != 0) {
                        memcpy (xhdr.modtim, phdr.modtim, sizeof xhdr.modtim);
                        rc = this->wxhb (&xhdr);
                        if (rc >= ER_MIN) goto errex2;
                    }
                    rc = this->rxhb (&xhdr, xfilnum, xfilseq, NULL);
                    if (rc >= ER_MIN) goto errex;
                }

                // fill in new block pointer
                xhdr.blocks[pindex+2] = lbn;
                xhdr.allocd = pindex + 3;
            }
        }

        // block successfully added, increment total number allocated
        this->allocd = ++ phdr.allocd;
    } while (this->allocd < newtot);

    // flush extension header
    if (xhdr.de.filnum != 0) {
        memcpy (xhdr.modtim, phdr.modtim, sizeof xhdr.modtim);
        rc = this->wxhb (&xhdr);
        if (rc >= ER_MIN) goto errex2;
    }

    // write out primary header with new number of allocated blocks and updated modtim
    // might also have pointers to new extension headers
    return this->wphb (&phdr);

    // some error, flush out extension header and primary header in case of partial allocation
errex:;
    if (xhdr.de.filnum != 0) {
        memcpy (xhdr.modtim, phdr.modtim, sizeof xhdr.modtim);
        rc = this->wxhb (&xhdr);
        if (rc >= ER_MIN) goto errex2;
    }
errex2:;
    this->wphb (&phdr);
    return rc;
}

// convert virtual block number to logical block number
Word File::vbntolbn (Word vbn)
{
    RECURCK;

    if (vbn >= this->allocd) return ER_EOF;

    // read prime header
    FHdr hdr;
    Word rc = this->rphb (&hdr);
    if (rc >= ER_MIN) return rc;

    // see how many extension headers there are
    Word numexts = (this->allocd <= PPH) ? 0 : (this->allocd - 3) / (PPH - 2);

    // see if requested vbn is in prime header
    // if so, retrieve lbn from prime header
    if (vbn < PPH - 2 * numexts) {
        return hdr.blocks[vbn];
    }

    // see if vbn is in one of the pairs displaced by extension header pointers
    // if so, retrieve lbn from beginning of corresponding extension header
    if (vbn < PPH) {
        Word xfilnum = hdr.blocks[vbn&~1];
        Word xfilseq = hdr.blocks[vbn| 1];
        rc = this->rxhb (&hdr, xfilnum, xfilseq, NULL);
        if (rc >= ER_MIN) return rc;
        return hdr.blocks[vbn&1];
    }

    // see which extension header the vbn is in
    Word xindex = (vbn - PPH) / (PPH - 2);
    Word pindex = (vbn - PPH) % (PPH - 2);
    ASSERT (xindex < numexts);

    // read that extension header
    Word xfilnum = hdr.blocks[PPH-2*xindex-2];
    Word xfilseq = hdr.blocks[PPH-2*xindex-1];
    rc = this->rxhb (&hdr, xfilnum, xfilseq, NULL);
    if (rc >= ER_MIN) return rc;

    // retrieve lbn from the extension header
    // skip first two entries cuz they contain displaced pointers from prime header
    return hdr.blocks[2+pindex];
}

// create a file
//  input:
//   de->filnam = file name
//  output:
//   returns >= ER_MIN: error
//                else: *file_r = filled in
//                      de->filnum,seq = filled in
Word Vol::createfile (File **file_r, Dirent *de)
{
    RECURCK;

    *file_r = NULL;

    if (this->rdonly) return ER_RDO;

    // create file object and create file
    File *file = new File (this);
    Word rc = file->create (de);
    if (rc >= ER_MIN) return rc;

    // link to list of files opened on this volume
    file->nextfile = this->openfiles;
    this->openfiles = file;

    // return file block to caller and a success status
    ASSERT (file->refcnt == 0);
    file->refcnt ++;
    *file_r = file;
    return 0;
}

/////////////////////
////// LEVEL 5 //////
/////////////////////

// create and open file on the volume
// internally called by Vol::createfile()
// - so this can be linked to volume's open file list
//  input:
//   de->filnam = name to put in file header block
//  output:
//   returns >= ER_MIN: error code
//                else: de->filnum,seq = filled in
//                      this filled in
Word File::create (Dirent *de)
{
    RECURCK;

    FHdr hdr;
    Word rc = this->vol->allochdr (&hdr, de);
    if (rc >= ER_MIN) return rc;
    this->filnum = hdr.de.filnum;
    this->filseq = hdr.de.filseq;
    this->hdrlbn = rc;
    this->allocd = hdr.allocd;
    this->eofvbn = hdr.eofvbn;
    this->eofwrd = hdr.eofwrd;
    return 0;
}

// read extension header block for the open file
Word File::rxhb (FHdr *hdr, Word xfilnum, Word xfilseq, Word *xhdrlbn_r)
{
    RECURCK;

    // get extension header logical block number in the index file
    Word lbn = this->vol->gethdrlbn (xfilnum);
    if (lbn >= ER_MIN) return lbn;
    if (xhdrlbn_r != NULL) *xhdrlbn_r = lbn;

    // read extension header block from index file
    ASSERT (sizeof *hdr == WPB * sizeof (Word));
    Word rc = this->vol->disk->rlb (lbn, WPB, (Word *) hdr);
    if (rc >= ER_MIN) return rc;
    ASSERT (rc == WPB);

    // make sure the checksum is good
    Word ck = 0;
    for (Word i = 0; i < WPB; i ++) ck += ((Word *) hdr)[i];
    if (ck & 07777) return ER_BHC;

    // make sure filnum,seq match
    if (hdr->de.filnum != xfilnum) return ER_BFN;
    if (hdr->de.filseq != xfilseq) return ER_BFS;

    // extension header eofvbn,wrd should be primary header filnum,seq
    if (hdr->eofvbn != this->filnum) return ER_BFN;
    if (hdr->eofwrd != this->filseq) return ER_BFS;

    // success
    return 0;
}

// write extension header block for the open file
Word File::wxhb (FHdr *hdr)
{
    // extension headers have eofvbn,wrd set to primary header filnum,seq
    if (hdr->eofvbn != this->filnum) return ER_BFN;
    if (hdr->eofwrd != this->filseq) return ER_BFS;

    // update the checksum
    ASSERT (sizeof *hdr == WPB * sizeof (Word));
    File::calchdrchksum (hdr);

    // get extension header logical block number
    Word xhlbn = this->vol->gethdrlbn (hdr->de.filnum);
    if (xhlbn >= ER_MIN) return xhlbn;

    // write extension header to disk
    Word rc = this->vol->disk->wlb (xhlbn, WPB, (Word *) hdr);
    if (rc >= ER_MIN) return rc;
    ASSERT (rc == WPB);
    return 0;
}

// close the open file
Stream::~Stream ()
{
    RECURCK;

    this->close ();
    ASSERT (this->dirty == 0);
}

// open a file
Word Vol::openfile (File **file_r, Word filnum, Word filseq)
{
    RECURCK;

    *file_r = NULL;

    // see if file already open on volume
    // if so, return same file object
    for (File *file = this->openfiles; file != NULL; file = file->nextfile) {
        if (file->filnum == filnum) {
            if (file->filseq != filseq) return ER_BFS;
            file->refcnt ++;
            *file_r = file;
            return 0;
        }
    }

    // not open, get index header block location on disk
    Word hdrlbn = this->gethdrlbn (filnum);
    if (hdrlbn >= ER_MIN) return hdrlbn;

    // create file object and open it with that header block
    File *file = new File (this);
    Word rc = file->open (filnum, filseq, hdrlbn);
    if (rc >= ER_MIN) {
        delete file;
        return rc;
    }

    // link to list of files opened on this volume
    file->nextfile = this->openfiles;
    this->openfiles = file;

    // return file block to caller and a success status
    file->refcnt ++;
    *file_r = file;
    return 0;
}

/////////////////////
////// LEVEL 4 //////
/////////////////////

Word Stream::close ()
{
    RECURCK;

    Word rc = 0;
    if (this->dirty > 0) {
        ASSERT (! this->rdonly);
        this->dirty = 0;
        getnow48 (this->file->modtim);
        rc = this->file->updattrs ();
    }
    this->file = NULL;
    return rc;
}

// allocate an index file block for a file header
//  input:
//   de->filnam = name written to header
//  output:
//   returns >= ER_MIN: error
//                else: header logical block number
//                      *hdr = filled in and zeroed out
//                      de->filnum,seq = filled in
Word Vol::allochdr (FHdr *hdr, Dirent *de)
{
    RECURCK;

    Word filnum, hdrlbn, hdrvbn, rc;

    for (bool retry = false;; retry = true) {

        for (filnum = this->lofreehdr;; filnum ++) {
            if (filnum == 0) continue;

            // get corresponding header virtual block number in index file
            hdrvbn = filnum + this->f0vbn;

            // maybe extend index file to contain that block
            hdrlbn = 0;
            if (hdrvbn >= this->indexf->allocd) {

                // we shouldn't be leaving any gaps
                hdrvbn = this->indexf->allocd;
                filnum = hdrvbn - this->f0vbn;

                // do not use this->indexf->extend() as we might be being
                // called from extend() to allocate an extension header

                // no extension headers for index file
                if (hdrvbn >= PPH) break;

                // allocate block before mucking with headers
                rc = this->allocblk ();
                if (rc >= ER_MIN) return rc;
                hdrlbn = rc;

                // update index file header to include new block
                // don't update eof marker yet cuz block might be a mess
                FHdr hdr;
                rc = this->indexf->rphb (&hdr);
                if (rc >= ER_MIN) return rc;
                ASSERT (hdr.allocd == this->indexf->allocd);
                hdr.blocks[hdrvbn] = hdrlbn;
                this->indexf->allocd = ++ hdr.allocd;
                rc = this->indexf->wphb (&hdr);
                if (rc >= ER_MIN) return rc;
            } else {

                // already allocated, get corresponding logical block number
                hdrlbn = this->idxvbntolbn (hdrvbn);
                if (hdrlbn >= ER_MIN) return hdrlbn;
            }

            // if beyond eof mark, use next header block as sequence 1
            ASSERT (this->indexf->eofwrd == 0);
            if (hdrvbn >= this->indexf->eofvbn) {
                de->filseq = 1;
                goto gothdr;
            }

            // within eof mark, see if header block already in use
            // if not in use, use it with incremented sequence number
            // skip over seq 07777 so as to put off re-using sequence numers as long as possible
            rc = this->disk->rlb (hdrlbn, WPB, (Word *) hdr);
            if (rc >= ER_MIN) return rc;
            if ((hdr->de.filnum == 0) && (hdr->de.filseq < 07777)) {
                de->filseq = hdr->de.filseq + 1;
                goto gothdr;
            }
        }

        // end if index file with no available headers
        // first time we skip over any free ones with seq 07777
        // so fail if this is our second time through
        if (retry) return ER_IFF;

        // reset all the free seq 07777s to sequence 0
        // also find the earliest one
        this->lofreehdr = 0;
        for (filnum = 1;; filnum ++) {
            hdrvbn = filnum + this->f0vbn;
            if (hdrvbn >= this->indexf->allocd) break;
            hdrlbn = this->idxvbntolbn (hdrvbn);
            if (hdrlbn >= ER_MIN) return hdrlbn;
            rc = this->disk->rlb (hdrlbn, WPB, (Word *) hdr);
            if (rc >= ER_MIN) return rc;
            if (hdr->de.filnum == 0) {
                if (this->lofreehdr == 0) this->lofreehdr = filnum;
                if (hdr->de.filseq == 07777) {
                    hdr->de.filseq = 0;
                    rc = this->disk->wlb (hdrlbn, WPB, (Word *) hdr);
                    if (rc >= ER_MIN) return rc;
                }
            }
        }

        // if we didn't find any, then index file is full
        if (this->lofreehdr == 0) return ER_IFF;
    }

    // this is the lowest free header, so next time start looking right after it
gothdr:;
    this->lofreehdr = filnum + 1;

    // fill in file header as an empty file
    de->filnum = filnum;
    hdr->de = *de;
    hdr->allocd = 0;
    hdr->eofvbn = 0;
    hdr->eofwrd = 0;
    getnow48 (hdr->modtim);
    memset (hdr->blocks, 0, sizeof hdr->blocks);
    File::calchdrchksum (hdr);
    rc = this->disk->wlb (hdrlbn, WPB, (Word *) hdr);
    if (rc >= ER_MIN) return rc;

    // if beyond eof mark, bump index file eof mark
    if (hdrvbn >= this->indexf->eofvbn) {
        ASSERT (this->indexf->eofvbn == hdrvbn);
        this->indexf->eofvbn = hdrvbn + 1;
        Word rc = this->indexf->updattrs ();
        if (rc >= ER_MIN) return rc;
    }

    ASSERT (hdrlbn < ER_MIN);
    return hdrlbn;
}

// get file header's logical block number in the index file
Word Vol::gethdrlbn (Word filnum)
{
    RECURCK;

    if (filnum == 0) return ER_BFN;
    Word vbn = filnum + this->f0vbn;
    if (vbn >= this->indexf->eofvbn) return ER_BFN;

    // do not use vbntolbn() to get index file logical block number as we
    // might be being called from vbntolbn() and we don't want recursion

    return this->idxvbntolbn (vbn);
}

// mount volume
Word Vol::mount (Word *label, bool rdonly)
{
    RECURCK;

    // make sure it is connected to a disk
    if (this->disk == NULL) return ER_BSY;

    // make sure volume not already mounted
    if (this->indexf != NULL) return ER_BSY;

    // maybe only allow reading
    this->rdonly = rdonly;

    // read home block
    VHom home;
    Word rc = this->disk->rlb (HBLBN, HBW, (Word *) &home);
    if (rc >= ER_MIN) return rc;                // error reading block
    Word ck = 0;
    for (Word i = 0; i < HBW; i ++) {
        ck += ((Word *) &home)[i];
    }
    if (ck & 07777) return ER_BHC;              // checksum is bad
    if (home.fsver != FSVER) return ER_BHC;     // version is bad
    if (label != NULL) {
        if (label[0] != 0) {                    // make sure non-zero label matched
            for (Word i = 0; i < HBLEN; i ++) {
                if (label[i] != home.label[i]) return ER_NSF;
            }
        } else {
            for (Word i = 0; i < HBLEN; i ++) { // zero label passed, return actual label
                label[i] = home.label[i];
            }
        }
    }
    this->nblocks = home.nblocks;
    this->f0vbn   = F0VBN (this->nblocks);
    this->lofreehdr = 1;

    // count number of free blocks and find lowest one
    this->nfreeblks = this->nblocks;
    this->lofreeblk = 07777;
    Word idx = HBW;
    Word blk[WPB];
    Word havelbn = 0;
    Word lbn;
    for (lbn = 0; lbn < this->nblocks; lbn += 12) {

        // read bitmap block if we don't already have it
        Word needlbn = idx / WPB + HBLBN;
        if (havelbn != needlbn) {
            havelbn = needlbn;
            rc = this->disk->rlb (havelbn, WPB, blk);
            if (rc >= ER_MIN) return rc;
        }

        // get bitmap word that has a set bit for every allocated block
        Word bmwrd = blk[idx%WPB];

        // if this is the first word that has at least one clear bit, save it as lowest free block word
        // the last bitmap word might have zero bits padding it so will trigger this which is ok
        if ((this->lofreeblk == 07777) && (bmwrd != 07777)) this->lofreeblk = idx - HBW;

        // decrement number of free blocks for each allocated block
        // the last bitmap word might have zero bits padding it so won't be a factor
        while (bmwrd != 0) {
            this->nfreeblks -= bmwrd & 1;
            bmwrd /= 2;
        }

        // check out next word in the bitmap
        idx ++;
    }

    // open index file with a fixed header lbn
    File *idxf = new File (this);
    rc = idxf->open (1, 1, this->f0vbn + 1);
    if (rc >= ER_MIN) {
        delete idxf;
        return rc;
    }

    // link to list of open files on this volume
    ASSERT (this->openfiles == NULL);
    idxf->nextfile = NULL;
    this->openfiles = idxf;

    // save index file pointer, and this marks the volume as open
    idxf->refcnt ++;
    this->indexf = idxf;
    return 0;
}

/////////////////////
////// LEVEL 3 //////
/////////////////////

// open file on the volume given its header logical block number
// internally called by Vol::openfile()
// - so this can be linked to volume's open file list
//  input:
//   filnum,seq = file to open
//   hlbn = file header lbn in index file
//  output:
//   returns >= ER_MIN: error code
//                else: this filled in
Word File::open (Word filnum, Word filseq, Word hlbn)
{
    RECURCK;

    this->filnum = filnum;
    this->filseq = filseq;
    this->hdrlbn = hlbn;
    FHdr hdr;
    Word rc = this->rphb (&hdr);
    if (rc >= ER_MIN) return rc;
    this->allocd = hdr.allocd;
    this->eofvbn = hdr.eofvbn;
    this->eofwrd = hdr.eofwrd;
    this->modtim[0] = hdr.modtim[0];
    this->modtim[1] = hdr.modtim[1];
    this->modtim[2] = hdr.modtim[2];
    this->modtim[3] = hdr.modtim[3];
    return 0;
}

// update file header attributes
Word File::updattrs ()
{
    RECURCK;

    FHdr hdr;
    Word rc = this->rphb (&hdr);
    if (rc >= ER_MIN) return rc;
    ASSERT (hdr.allocd == this->allocd);
    hdr.eofvbn = this->eofvbn;
    hdr.eofwrd = this->eofwrd;
    return this->wphb (&hdr);
}

// translate index file virtual block number to logical block number
// don't use indexf->vbntolbn() so it can't recurse
Word Vol::idxvbntolbn (Word vbn)
{
    RECURCK;

    // do not use vbntolbn() to get index file logical block number as we
    // might be being called from rxhb() and we don't want recursion

    // make sure not accessing more than what is allocated
    if (vbn >= this->indexf->allocd) return ER_EOF;

    // read index file primary (and only) header
    FHdr hdr;
    Word rc = this->indexf->rphb (&hdr);
    if (rc >= ER_MIN) return rc;

    // make sure our cached allocd matches on-disk value
    ASSERT (this->indexf->allocd == hdr.allocd);

    // we don't handle extension headers, so make sure
    // index file doesn't have extension headers
    ASSERT (hdr.allocd <= PPH);

    // get requested extension header's lbn
    return hdr.blocks[vbn];
}

/////////////////////
////// LEVEL 2 //////
/////////////////////

// read primary header block for the open file
Word File::rphb (FHdr *hdr)
{
    // read header block from index file
    ASSERT (sizeof *hdr == WPB * sizeof (Word));
    Word rc = this->vol->disk->rlb (this->hdrlbn, WPB, (Word *) hdr);
    if (rc >= ER_MIN) return rc;
    ASSERT (rc == WPB);

    // make sure the checksum is good
    Word ck = 0;
    for (Word i = 0; i < WPB; i ++) ck += ((Word *) hdr)[i];
    if (ck & 07777) return ER_BHC;

    // make sure filnum,seq match
    if (hdr->de.filnum != this->filnum) return ER_BFN;
    if (hdr->de.filseq != this->filseq) return ER_BFS;

    // success
    return 0;
}

// write primary header block for the open file
Word File::wphb (FHdr *hdr)
{
    // make sure header being written has correct filnum,seq
    if (hdr->de.filnum != this->filnum) return ER_BFN;
    if (hdr->de.filseq != this->filseq) return ER_BFS;

    // update the checksum
    ASSERT (sizeof *hdr == WPB * sizeof (Word));
    File::calchdrchksum (hdr);

    // write to disk
    Word rc = this->vol->disk->wlb (this->hdrlbn, WPB, (Word *) hdr);
    if (rc >= ER_MIN) return rc;
    ASSERT (rc == WPB);
    return 0;
}

// allocate a block
Word Vol::allocblk ()
{
    RECURCK;

    Word mask  = 1;
    Word bmb[WPB];
    Word bmw, rc;
    Word lbn   = this->lofreeblk * 12;
    Word indx  = HBW + this->lofreeblk;
    Word bmlbn = 0;

    // search for a bitmap word containing at least one zero bit
    while (lbn + 12 <= this->nblocks) {

        // make sure we have bitmap block contents
        if (bmlbn != HBLBN + indx / WPB) {
            bmlbn = HBLBN + indx / WPB;
            rc = this->disk->rlb (bmlbn, WPB, bmb);
            if (rc >= ER_MIN) return rc;
        }

        // check the word of the bitmap for zero bit(s)
        bmw = bmb[indx%WPB];
        ASSERT (bmw <= 07777);
        if (bmw < 07777) goto gotone;

        // all ones, try next word
        indx ++;
        lbn += 12;
    }

    // very last word of bitmap has fewer than 12 bits valid
    if (lbn < this->nblocks) {

        // make sure we have bitmap block contents
        if (bmlbn != HBLBN + indx / WPB) {
            bmlbn = HBLBN + indx / WPB;
            rc = this->disk->rlb (bmlbn, WPB, bmb);
            if (rc >= ER_MIN) return rc;
        }

        // check the word of the bitmap for zero bit(s)
        // only look at bits corresponding to valid lbns
        bmw = bmb[indx%WPB];
        ASSERT (bmw <= 07777);
        do {
            if (! (bmw & mask)) goto gotbit;
            mask *= 2;
        } while (++ lbn < this->nblocks);
    }
    return ER_FUL;

    // some bit in the word is zero, find the lowest one
gotone:;
    while (bmw & mask) {
        mask *= 2;
        lbn ++;
    }

    // 'mask' bit is clear, set it and write bitmap block out
gotbit:;
    this->lofreeblk = indx - HBW;
    bmb[indx%WPB] = bmw | mask;
    rc = this->disk->wlb (bmlbn, WPB, bmb);
    if (rc >= ER_MIN) return rc;
    this->nfreeblks --;
    return lbn;
}

// release any allocated blocks by clearing bitmap bits
Word Vol::freeblks (Word nbl, Word *lbns)
{
    RECURCK;

    Word havebmlbn = 0;
    Word bmb[WPB];
    Word rc = 0;
    while (nbl > 0) {
        Word lbn = *(lbns ++);                  // block being freed
        Word idx = lbn / 12;                    // word within whole bitmap
        if (this->lofreeblk > idx) this->lofreeblk = idx;
        idx += HBW;                             // ... + home block words
        Word needbmlbn = idx / WPB + HBLBN;
        if (havebmlbn != needbmlbn) {
            if (havebmlbn != 0) {
                rc = this->disk->wlb (havebmlbn, WPB, bmb);
                if (rc >= ER_MIN) return rc;
            }
            havebmlbn = needbmlbn;
            rc = this->disk->rlb (havebmlbn, WPB, bmb);
            if (rc >= ER_MIN) return rc;
        }
        bmb[idx%WPB] &= ~(1 << (lbn % 12));
        this->nfreeblks ++;
        -- nbl;
    }
    if (havebmlbn != 0) {
        rc = this->disk->wlb (havebmlbn, WPB, bmb);
    }
    return rc;
}

// initialize volume (does not mount it)
Word Vol::initvol (Word *label)
{
    RECURCK;

    // make sure it is connected to a disk
    if (this->disk == NULL) return ER_BSY;

    // make sure volume not mounted
    if (this->indexf != NULL) return ER_BSY;

    // set up home block
    VHom home;
    memset (&home, 0, sizeof home);
    home.fsver   = FSVER;
    home.nblocks = disk->nblocks;
    if (home.nblocks > ER_MIN) home.nblocks = ER_MIN;
    for (Word i = 0; i < HBLEN; i ++) {
        home.label[i] = label[i];
    }
    Word ck = 0;
    for (Word i = 0; i < HBW; i ++) {
        ck -= ((Word *) &home)[i];
    }
    home.chksum = ck & 07777;

    this->nblocks = home.nblocks;
    this->f0vbn   = F0VBN (this->nblocks);

    // preallocate some blocks at beginning of disk
    // enough for two file headers and root directory block
    Word prealloc = (this->f0vbn + 2 + 1) / 12 + 1;
    Word prealbct = prealloc * 12;

    // zero out the pre-allocated blocks at beginning of disk
    Word blk[WPB];
    memset (blk, 0, sizeof blk);
    for (Word i = 0; i < prealbct; i ++) {
        Word rc = this->disk->wlb (i, WPB, blk);
        if (rc >= ER_MIN) return rc;
    }

    // write home block + initial bitmap with prealloc bits
    memcpy (blk, &home, sizeof home);
    for (Word i = 0; i < prealloc; i ++) {
        ASSERT (i + HBW < WPB);
        blk[i+HBW] = 07777;
    }
    Word rc = this->disk->wlb (HBLBN, WPB, blk);
    if (rc >= ER_MIN) return rc;

    // set up entries for our 2 fixed files
    Dirent entries[2];
    entries[0].fill (1, 1, "indexf.sys");
    entries[1].fill (2, 1, "000000.dir");

    // make index file containing pre-allocated blocks except last one
    // fixed filenumber (1,1)
    FHdr hdr;
    memset (&hdr, 0, sizeof hdr);
    hdr.de = entries[0];
    hdr.allocd = prealbct - 1;
    hdr.eofvbn = prealbct - 1;
    ASSERT (hdr.allocd - 1 < (Word) (PPH));
    for (Word i = 0; i < hdr.allocd; i ++) {
        hdr.blocks[i] = i;
    }
    getnow48 (hdr.modtim);
    File::calchdrchksum (&hdr);
    rc = this->disk->wlb (this->f0vbn + 1, WPB, (Word *) &hdr);
    if (rc >= ER_MIN) return rc;

    // make root directory file containing the last pre-allocated block
    // fixed filenumber (2,2)
    memset (&hdr, 0, sizeof hdr);
    hdr.de = entries[1];
    hdr.allocd = 1;
    hdr.eofvbn = 1;
    hdr.blocks[0] = prealbct - 1;
    getnow48 (hdr.modtim);
    File::calchdrchksum (&hdr);
    rc = this->disk->wlb (this->f0vbn + 2, WPB, (Word *) &hdr);
    if (rc >= ER_MIN) return rc;

    // write directory entries for those two files
    rc = this->disk->wlb (hdr.blocks[0], sizeof entries / sizeof (Word), (Word *) entries);
    if (rc >= ER_MIN) return rc;

    return 0;
}

/////////////////////
////// LEVEL 1 //////
/////////////////////

Word RK8Disk::rlb (Word lbn, Word nwords, Word *buffer)
{
    if (lbn + (nwords + WPB - 1) / WPB > this->nblocks) return ER_BBN;
    if ((this->fd < 0) && ! this->oback ()) return ER_IO;
    int bc = nwords * sizeof *buffer;
    int rc = pread (this->fd, buffer, bc, lbn * WPB * sizeof *buffer);
    if (rc != bc) {
        if (rc < 0) {
            fprintf (stderr, "RK8Disk::rlb: error reading %d bytes at %u on disk %u: %m\n", bc, lbn, this->unit);
        } else {
            fprintf (stderr, "RK8Disk::rlb: only read %d of %d bytes at %u on disk %u\n", rc, bc, lbn, this->unit);
        }
        return ER_IO;
    }
    return nwords;
}

Word RK8Disk::wlb (Word lbn, Word nwords, Word const *buffer)
{
    if (lbn + (nwords + WPB - 1) / WPB > this->nblocks) return ER_BBN;
    if (this->rdonly) return ER_RDO;
    if ((this->fd < 0) && ! this->oback ()) return ER_IO;
    for (Word i = 0; i < nwords; i ++) {
        ASSERT (buffer[i] <= 07777);
    }
    int bc = nwords * sizeof *buffer;
    int rc = pwrite (this->fd, buffer, bc, lbn * WPB * sizeof *buffer);
    if (rc != bc) {
        if (rc < 0) {
            fprintf (stderr, "RK8Disk::wlb: error writing %d bytes at %u on disk %u: %m\n", bc, lbn, this->unit);
        } else {
            fprintf (stderr, "RK8Disk::wlb: only wrote %d of %d bytes at %u on disk %u\n", rc, bc, lbn, this->unit);
        }
        return ER_IO;
    }
    return nwords;
}

/////////////////////
////// LEVEL 0 //////
/////////////////////

void Dirent::fill (Word filnum, Word filseq, char const *filnam)
{
    this->filnum = filnum;
    this->filseq = filseq;
    Word c = 1;
    Word d = 1;
    memset (this->filnam, 0, sizeof this->filnam);
    for (Word i = 0; i < DNLEN; i ++) {
        c = (d == 0) ? 0 : *(filnam ++) & 0177;
        d = (c == 0) ? 0 : *(filnam ++) & 0177;
        if (c >= 0140) c -= 040;
        if (d >= 0140) d -= 040;
        this->filnam[i] = ((c & 077) << 6) | (d & 077);
    }
}

Disk::Disk ()
{
    this->vol = NULL;
    this->nblocks = 0;
}

Disk::~Disk ()
{
    ASSERT (this->vol == NULL);
}

// construct file object for a file on the given volume
// internally called by Vol::openfile() to link File object to volume's list of open file
// - do not call it externally so the volume's open file list is maintained
File::File (Vol *vol)
{
    ASSERT (vol != NULL);
    this->vol       = vol;
    this->markdl    = 0;
    this->hdrlbn    = 0;
    this->filnum    = 0;
    this->filseq    = 0;
    this->allocd    = 0;
    this->eofvbn    = 0;
    this->eofwrd    = 0;
    this->modtim[0] = 0;
    this->modtim[1] = 0;
    this->modtim[2] = 0;
    this->modtim[3] = 0;
    this->nextfile  = this;
    this->refcnt    = 0;
}

// private destructor
// called by this->close() to destruct when refcount goes zero
File::~File ()
{
    ASSERT (this->refcnt == 0);
}

// calculate header block checksum
void File::calchdrchksum (FHdr *hdr)
{
    ASSERT (sizeof *hdr == WPB * sizeof (Word));
    hdr->chksum = 0;
    Word ck = 0;
    for (Word i = 0; i < WPB; i ++) ck -= ((Word *) hdr)[i];
    hdr->chksum = ck & 07777;
}

// compare names in directory entries
int File::cmpdirent (Dirent const *d, Dirent const *e)
{
    int diff = 0;
    for (Word i = 0; i < DNLEN; i ++) {
        diff = d->filnam[i] - e->filnam[i];
        if (diff != 0) break;
    }
    return diff;
}

RK8Disk::RK8Disk (Word unit, bool rdonly) : Disk ()
{
    this->nblocks = RK8BLOCKS;
    this->unit    = unit;
    this->rdonly  = rdonly;
    this->fd      = -1;
}

RK8Disk::~RK8Disk ()
{
    close (this->fd);
    this->fd = -1;
}

// internal method to open backing disk file
bool RK8Disk::oback ()
{
    char rk8nam[16];
    snprintf (rk8nam, sizeof rk8nam, "rk8disk_%u", this->unit);
    char const *rk8env = getenv (rk8nam);
    if (rk8env == NULL) {
        fprintf (stderr, "RK8Disk::oback: envar %s not defined\n", rk8nam);
        return false;
    }
    this->fd = open (rk8env, this->rdonly ? O_RDONLY : O_RDWR);
    if (this->fd < 0) {
        fprintf (stderr, "RK8Disk::oback: error opening %s: %m\n", rk8env);
        return false;
    }
    if (flock (this->fd, (this->rdonly ? LOCK_SH : LOCK_EX) | LOCK_NB) < 0) {
        fprintf (stderr, "RK8Disk::oback: error locking %s: %m\n", rk8env);
        goto failed;
    }
    if (! this->rdonly && (ftruncate (this->fd, RK8BLOCKS * WPB * sizeof (Word)) < 0)) {
        fprintf (stderr, "RK8Disk::oback: error extending %s: %m\n", rk8env);
        goto failed;
    }
    return true;

failed:;
    close (this->fd);
    this->fd = -1;
    return false;
}

// open the supplied file, allowing reads and writes
// sets current position to beginning of file
Stream::Stream (File *file, bool rdonly)
{
    ASSERT (file != NULL);
    this->file   = file;
    this->rdonly = rdonly;
    this->dirty  = 0;
    this->curvbn = 0;
    this->curwrd = 0;
    this->curlbn = 0;
}

// construct volume object
// can have only one per disk
// prevent any other from creating one of these
Vol::Vol (Disk *disk)
{
    ASSERT (disk != NULL);
    this->disk      = NULL;
    this->indexf    = NULL;
    this->openfiles = NULL;
    if (disk->vol == NULL) {
        disk->vol  = this;
        this->disk = disk;
    }
}

// destruct volume object
// has to be unmounted first
Vol::~Vol ()
{
    ASSERT (this->indexf == NULL);
    ASSERT (this->openfiles == NULL);
    if (this->disk != NULL) {
        ASSERT (this->disk->vol == this);
        this->disk->vol = NULL;
        this->disk = NULL;
    }
}

// get current time
//  [0..2] = seconds since 1970-01-01 00:00:00 UTC' (big endian)
//     [3] = 4096ths of a second
void getnow48 (Word *time48)
{
    struct timespec nowts;
    if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) ABORT ();
    time48[0] = (((__syscall_ulong_t) nowts.tv_sec) >> 24) & 07777;
    time48[1] = (nowts.tv_sec >> 12) & 07777;
    time48[2] =  nowts.tv_sec & 07777;
    time48[3] = nowts.tv_nsec * 4096ULL / 1000000000;
    ASSERT (time48[3] <= 07777);
}
