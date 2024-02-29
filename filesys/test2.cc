
// dd if=/dev/zero of=disk0.rk8 bs=4096 count=400
// export rk8disk_0=disk0.rk8
// valgrind ./test.x86_64
// od -to2 disk0.rk8 | more

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <time.h>
#include <unistd.h>
#include <vector>

#include "dumpfile.h"
#include "filesys.h"

#define ABORT() do { fprintf (stderr, "ABORT %s:%d\n", __FILE__, __LINE__); abort (); } while (0)
#define ASSERT(cond) do { if (__builtin_constant_p (cond)) { if (!(cond)) asm volatile ("assert failure"); } else { if (!(cond)) ABORT (); } } while (0)
#define CHKRC(rc) do { Word __rc = rc; if (__rc >= ER_MIN) { fprintf (stderr, "CHKRC %04o at %s:%d\n", __rc, __FILE__, __LINE__); abort (); } } while (0)

typedef unsigned int uint32_t;
typedef unsigned long long uint64_t;

struct MemDisk : Disk {
    MemDisk (Word nblocks);
    virtual ~MemDisk ();
    virtual Word rlb (Word lbn, Word nwords, Word *buffer);
    virtual Word wlb (Word lbn, Word nwords, Word const *buffer);

private:
    Word *array;
};

// simple random number generator
struct Rand {
    Rand ();
    bool bit ();
    uint32_t ru32 (uint32_t max);
    Word word (Word max);
private:
    uint64_t seed;
    void next ();
};

// shadow file - holds in-memory contents of file written to the disk
struct ShFile {
    Dirent dent;        // directory entry for the on-disk file
    uint32_t wcnt;      // array length
    uint32_t widx;      // write index
    Word *data;         // array of words in the file
    bool found;         // used by checker to make sure all files were found

    ShFile (Dirent dent, uint32_t wcnt);
    ~ShFile ();
    void write (Word *buf, Word len);
    void close ();
};

static char *sixtoasc (Word *six);
static void verifyfile (Vol *vol, ShFile *shfile, Rand *rgener);

int main ()
{
    setlinebuf (stdout);
    setlinebuf (stderr);

    // set up virtual disk pointing using memory (same size as an RK8)
    MemDisk disk (200 * 2 * 8);
    Vol volume (&disk);

    // initialize the volume
    Word label[6] = { (('T' & 077) << 6) | ('E' & 077), (('S' & 077) << 6) | ('T' & 077), (('V' & 077) << 6) | ('O' & 077), (('L' & 077) << 6), 0, 0 };
    Word rc = volume.initvol (label);
    CHKRC (rc);

    // mount the volume - opens the index file
    rc = volume.mount (label, false);
    CHKRC (rc);

    // open root directory
    File *rootdir;
    rc = volume.openfile (&rootdir, 2, 1);
    CHKRC (rc);

    std::vector<ShFile *> shadow;
    Rand rgener;
    Dirent credir;
    credir.fill (0, 0, "AAA@");
    while (true) {
        if ((volume.nfreeblks > 10) && rgener.bit ()) {

            // increment file name
            for (Word i = 2; i > 0;) {
                if ((++ credir.filnam[--i] & 077) <= 26) goto gotname;
                credir.filnam[i] += 0100 - 26;
                if ((credir.filnam[i] & 07700) <= (26 << 6)) goto gotname;
                credir.filnam[i] = 0101;
            }
            break;
        gotname:;

            uint32_t nwords = rgener.ru32 (volume.nfreeblks * 17 * (WPB / 16));
            char *asc = sixtoasc (credir.filnam);
            if ((asc[2] == 'A') && (asc[3] == 'A')) {
                printf ("creating %s %u words\n", asc, nwords);
            }

            // create file
            File *file;
            rc = volume.createfile (&file, &credir);
            CHKRC (rc);

            // write directory entry
            rc = rootdir->enter (&credir);
            CHKRC (rc);

            ShFile *shfile = new ShFile (credir, nwords);
            while (shadow.size () <= credir.filnum) shadow.push_back (NULL);
            shadow[credir.filnum] = shfile;

            Stream stream (file, false);
            while (nwords > 0) {
                Word buffer[4*WPB];
                Word nw = 4 * WPB;
                if (nw > nwords) nw = nwords;
                if (nw > 3) nw = rgener.word (nw - 2) + 1;
                for (Word i = 0; i < nw; i ++) {
                    buffer[i] = rgener.word (07777);
                }
                rc = stream.write (buffer, nw);
                //printf ("main*: rc=%u nw=%u\n", rc, nw);
                CHKRC (rc);
                ASSERT (rc <= nw);
                shfile->write (buffer, rc);
                if (rc < nw) {
                    ASSERT (volume.nfreeblks == 0);
                    break;
                }
                nwords -= rc;
            }
            rc = stream.close ();
            CHKRC (rc);
            ASSERT ((uint32_t) file->eofvbn * WPB + file->eofwrd == shfile->widx);
            //dumpfile (file, stdout);
            file->close ();

            verifyfile (&volume, shfile, &rgener);
        } else {

            // pick file at random to delete
            Word nfiles = 0;
            for (ShFile *shf : shadow) if (shf != NULL) nfiles ++;
            if (nfiles == 0) continue;
            nfiles = rgener.word (nfiles - 1);
            ShFile *shfile = NULL;
            for (ShFile *shf : shadow) {
                if (shf != NULL) {
                    shfile = shf;
                    if (nfiles == 0) break;
                    -- nfiles;
                }
            }
            ASSERT (shfile != NULL);

            char *asc = sixtoasc (shfile->dent.filnam);
            if ((asc[2] == 'A') && (asc[3] == 'A')) {
                printf ("deleting %s (%04o,%04o)\n", asc, shfile->dent.filnum, shfile->dent.filseq);
            }

            // open it
            File *file;
            rc = volume.openfile (&file, shfile->dent.filnum, shfile->dent.filseq);
            CHKRC (rc);

            // delete it
            file->markdl = true;
            rc = file->close ();
            CHKRC (rc);

            // remove from directory
            Dirent deldir = shfile->dent;
            rc = rootdir->remove (&deldir);
            CHKRC (rc);
            if ((deldir.filnum != shfile->dent.filnum) || (deldir.filseq != shfile->dent.filseq)) ABORT ();

            shadow[deldir.filnum] = NULL;
            delete shfile;
        }

        rc = volume.fsuck ();
        if (rc > 0) ABORT ();

        // make sure rootdirectory matches shadow
        for (ShFile *shfile : shadow) if (shfile != NULL) shfile->found = false;
        Stream rdread (rootdir, true);
        while (true) {
            Dirent rootent;
            rc = rdread.read ((Word *) &rootent, sizeof rootent / sizeof (Word));
            if (rc == 0) break;
            if (rc != sizeof rootent / sizeof (Word)) ABORT ();
            if (rootent.filnum == 0) continue;
            if (rootent.filnum == 1) continue;
            if (rootent.filnum == 2) continue;
            ShFile *shfile = shadow[rootent.filnum];
            ASSERT (shfile != NULL);
            if (memcmp (&rootent, &shfile->dent, sizeof rootent) != 0) ABORT ();
            if (shfile->found) ABORT ();
            shfile->found = true;
        }
        rdread.close ();

        // make sure all file content matches
        for (ShFile *shfile : shadow) {
            if (shfile == NULL) continue;
            if (! shfile->found) ABORT ();
            verifyfile (&volume, shfile, &rgener);
        }
    }

    // dismount the volume
    rootdir->close ();
    rootdir = NULL;
    rc = volume.umount ();
    CHKRC (rc);

    printf ("SUCCESS\n");

    return 0;
}

static char *sixtoasc (Word *six)
{
    static char asc[DNLEN*2+1];

    for (Word i = 0; i < DNLEN; i ++) {
        asc[i*2+0] = ((six[i] >>  6) ^ 040) + 040;
        asc[i*2+1] = ((six[i] & 077) ^ 040) + 040;
    }
    for (Word i = DNLEN*2; i > 0;) {
        if (asc[--i] != '@') break;
        asc[i] = 0;
    }
    return asc;
}

static void verifyfile (Vol *vol, ShFile *shfile, Rand *rgener)
{
    File *file;
    Word rc = vol->openfile (&file, shfile->dent.filnum, shfile->dent.filseq);
    CHKRC (rc);
    ASSERT ((uint32_t) file->eofvbn * WPB + file->eofwrd == shfile->widx);
    Stream stream (file, true);
    uint32_t ridx = 0;
    while (true) {
        Word buffer[4*WPB];
        Word nr = rgener->word (4 * WPB - 1);
        rc = stream.read (buffer, nr);
        //printf ("verifyfile*: rc=%u nr=%u\n", rc, nr);
        CHKRC (rc);
        if (rc > nr) ABORT ();
        if (ridx + rc > shfile->widx) ABORT ();
        if (memcmp (buffer, shfile->data + ridx, rc * sizeof *buffer) != 0) ABORT ();
        ridx += rc;
        if (rc < nr) break;
    }
    if (ridx != shfile->widx) ABORT ();
    stream.close ();
    file->close ();
}

MemDisk::MemDisk (Word nblocks)
{
    this->nblocks = nblocks;
    this->array = new Word[nblocks*WPB];
}

MemDisk::~MemDisk ()
{
    delete this->array;
    this->array = NULL;
}

Word MemDisk::rlb (Word lbn, Word nwords, Word *buffer)
{
    uint32_t off = lbn * WPB;
    if (off + nwords > this->nblocks * WPB) return ER_IO;
    memcpy (buffer, &this->array[off], nwords * sizeof *buffer);
    return nwords;
}

Word MemDisk::wlb (Word lbn, Word nwords, Word const *buffer)
{
    uint32_t off = lbn * WPB;
    if (off + nwords > this->nblocks * WPB) return ER_IO;
    memcpy (&this->array[off], buffer, nwords * sizeof *buffer);
    return nwords;
}

ShFile::ShFile (Dirent dent, uint32_t wcnt)
{
    this->dent = dent;
    this->wcnt = wcnt;
    this->data = new Word[wcnt];
    this->widx = 0;
}

ShFile::~ShFile ()
{
    delete[] this->data;
    this->data = NULL;
}

void ShFile::write (Word *buf, Word len)
{
    ASSERT (this->widx + len <= this->wcnt);
    memcpy (this->data + this->widx, buf, len * sizeof *this->data);
    this->widx += len;
}

void ShFile::close ()
{
    ASSERT (this->widx == this->wcnt);
}

Rand::Rand ()
{
    seed = 0x123456789ABCDEF0ULL;
}

bool Rand::bit ()
{
    next ();
    return (seed & 1) != 0;
}

uint32_t Rand::ru32 (uint32_t max)
{
    next ();
    return seed % (max + 1);
}

Word Rand::word (Word max)
{
    next ();
    return seed % (max + 1);
}

void Rand::next ()
{
    // https://www.xilinx.com/support/documentation/application_notes/xapp052.pdf
    uint64_t xnor = ~ ((seed >> 63) ^ (seed >> 62) ^ (seed >> 60) ^ (seed >> 59));
    seed = (seed << 1) | (xnor & 1);
}
