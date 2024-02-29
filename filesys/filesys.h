#ifndef _FILESYS_H
#define _FILESYS_H

#define ER_IO  07777    // disk i/o error
#define ER_EOF 07776    // end-of-file
#define ER_FUL 07775    // volume full
#define ER_BFN 07774    // bad file number
#define ER_BFS 07773    // bad file sequence
#define ER_BHC 07772    // bad header checksum
#define ER_FHF 07771    // file header full
#define ER_BSY 07770    // busy
#define ER_NSF 07767    // no such file
#define ER_DUP 07766    // duplicate filename in directory
#define ER_RDO 07765    // write attempted on read-only device/volume/file
#define ER_IFF 07764    // index file full
#define ER_BBN 07763    // bad block number
#define ER_MIN 07760

#define RK8BLOCKS (200*2*8)  // number of blocks on RK8 disk

#define WPB 256                                 // words per block
#define PPH (WPB-16)                            // pointers per header
#define EPB ((WPB*sizeof(Word))/sizeof(Dirent)) // entries per block
#define HBW (sizeof (VHom) / sizeof (Word))     // home block words

#define DNLEN 6         // number of words for directory names
#define HBLEN 6         // number of words for home block label
#define HBLBN 1         // home block logical block number
#define FSVER 01234     // filesystem version

typedef unsigned short Word;

struct File;
struct Dirent;
struct Disk;
struct FHdr;
struct RK8Disk;
struct Stream;
struct Vol;

// volume structure:
//  boot block
//  home block / start of bitmap
//  additional bitmap block(s)
//  index file header block
//  next file header block

struct Dirent {
    Word filnum;
    Word filseq;
    Word filnam[DNLEN];

    void fill (Word filnum, Word filseq, char const *filnam);
};

struct FHdr {
    Dirent de;                  // filnum, filseq, filnam
                                // - prime: filnam = actual filename
                                // - exten: filnam[DNLEN-1] = incremented by extension number (1...)
    Word eofvbn;                // - prime: end of file block 0...
                                // - exten: prime header filnum
    Word eofwrd;                // - prime: end of file word 0..255
                                // - exten: prime header filseq
    Word allocd;                // - prime: number of blocks allocated for whole file
                                // - exten: number of blocks allocated in this extension
    Word chksum;                // checksum so it all adds up to 0
    Word modtim[4];             // modification time
    Word blocks[PPH];           // allocated logical blocks
};

struct VHom {
    Word fsver;                 // filesystem version (FSVER)
    Word nblocks;               // total number of blocks on volume
    Word label[HBLEN];          // volume label in 6-bit format
    Word chksum;                // so that all words total zero
};

struct Disk {
    Vol *vol;                   // associated volume struct
    Word nblocks;               // number of blocks on disk

    Disk ();
    virtual ~Disk ();

    virtual Word rlb (Word lbn, Word nwords, Word *buffer) = 0;
    virtual Word wlb (Word lbn, Word nwords, Word const *buffer) = 0;
};

struct File {
    Vol *vol;                   // volume the file is on

    Word markdl;                // marked for delete on close
    Word hdrlbn;                // header logical block number
    Word filnum;                // file number
    Word filseq;                // file sequence
    Word allocd;                // number of allocated blocks
    Word eofvbn;                // end of file virt block number
    Word eofwrd;                // end of file word
    Word modtim[4];             // modification time

    Word close ();              // call this instead of deleting the object
    Word vbntolbn (Word vbn);   // convert virtual block number to logical block number
    Word extend (Word newtot);  // extend the file if needed to be the given total
    Word updattrs ();           // write attributes to file header block

    // for use when this file is a directory
    Word enter (Dirent *dirent);
    Word lookup (Dirent *dirent);
    Word remove (Dirent *dirent);

private:
    friend struct Vol;

    File *nextfile;             // next file in this->vol->openfiles list
    Word refcnt;                // number of times Vol::createfile(), Vol::openfile() called

    File (Vol *vol);            // called by Vol::createfile(), Vol::openfile()
    ~File ();                   // called by this->close()
    Word create (Dirent *de);
    Word open (Word filnum, Word filseq, Word hdrlbn);
    Word delit ();
    Word rphb (FHdr *hdr);
    Word wphb (FHdr *hdr);
    Word rxhb (FHdr *hdr, Word xfilnum, Word xfilseq, Word *xhdrlbn_r);
    Word wxhb (FHdr *hdr);

    static int cmpdirent (Dirent const *d, Dirent const *e);
    static void calchdrchksum (FHdr *hdr);
};

struct RK8Disk : Disk {
    Word unit;                  // disk unit number
    bool rdonly;

    RK8Disk (Word unit, bool rdonly);
    virtual ~RK8Disk ();

    virtual Word rlb (Word lbn, Word nwords, Word *buffer);
    virtual Word wlb (Word lbn, Word nwords, Word const *buffer);

private:
    int fd;
    bool oback ();
};

struct Stream {
    Word curvbn;                // current position virtual block
    Word curwrd;                // current position word
    Word dirty;                 // eofvbn,eofwrd,modtim needs updating

    Stream (File *file, bool rdonly);
    ~Stream ();

    Word read (Word *buffer, Word nwords);
    Word write (Word *buffer, Word nwords);
    Word seek (Word vbn, Word wrd);
    Word close ();

private:
    File *file;                 // file that is open
    bool rdonly;                // read-only access
    Word curlbn;                // current position logical block
                                // - 0: unknown
};

struct Vol {
    Disk *disk;                 // disk the volume is on
    bool rdonly;                // volume mounted read-only
    Word nblocks;               // number of blocks in the volume
    Word f0vbn;                 // file 0 header vbn in index file
    Word nfreeblks;             // number of free blocks

    Vol (Disk *disk);
    ~Vol ();

    Word initvol (Word *label);
    Word mount (Word *label, bool rdonly);
    Word umount ();

    Word createfile (File **file_r, Dirent *de);
    Word openfile (File **file_r, Word filnum, Word filseq);

    int fsuck ();

private:
    friend struct File;

    File *openfiles;            // all open files
    File *indexf;               // index file
    Word lofreehdr;             // all headers below this are in use
    Word lofreeblk;             // all blocks below this are in use

    Word freeblks (Word nbl, Word *lbns);
    Word gethdrlbn (Word filnum);
    Word idxvbntolbn (Word vbn);
    Word allochdr (FHdr *hdr, Dirent *de);
    Word allocblk ();
};

#endif
