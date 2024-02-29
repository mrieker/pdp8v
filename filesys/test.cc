
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

#include "dumpfile.h"
#include "filesys.h"

#define ABORT() do { fprintf (stderr, "ABORT %s:%d\n", __FILE__, __LINE__); abort (); } while (0)
#define ASSERT(cond) do { if (__builtin_constant_p (cond)) { if (!(cond)) asm volatile ("assert failure"); } else { if (!(cond)) ABORT (); } } while (0)

typedef unsigned char uint8_t;

static int writebootblock (Disk *disk, char const *label);
static int copyinfile (char const *fromfile, File *rootdir, char const *tofile);
static int copyoutfile (File *rootdir, char const *fromfile, char const *tofile);

int main ()
{
    setlinebuf (stdout);
    setlinebuf (stderr);

    // set up virtual disk pointing to the rk8disk_0 file
    RK8Disk disk (0, false);
    Vol volume (&disk);

    // initialize the volume
    Word label[6] = { (('T' & 077) << 6) | ('E' & 077), (('S' & 077) << 6) | ('T' & 077), (('V' & 077) << 6) | ('O' & 077), (('L' & 077) << 6), 0, 0 };
    Word rc = volume.initvol (label);
    printf ("initvol rc=%04o\n", rc);
    if (rc >= ER_MIN) return 1;

    // write bootblock
    rc = writebootblock (&disk, "TESTVOL");
    if (rc != 0) return rc;

    // mount the volume - opens the index file
    rc = volume.mount (label, false);
    printf ("mount rc=%04o\n", rc);
    if (rc >= ER_MIN) return 1;
    printf ("nfreeblks=%u\n", volume.nfreeblks);

    // open root directory
    File *rootdir;
    rc = volume.openfile (&rootdir, 2, 1);
    printf ("open root dir rc=%04o\n", rc);
    if (rc >= ER_MIN) return 1;

    // create a file
    Dirent ent;
    ent.fill (0, 0, "alpha.bet");
    File *file;
    rc = volume.createfile (&file, &ent);
    printf ("createfile rc=%04o\n", rc);
    if (rc >= ER_MIN) return 1;

    // write directory entry
    rc = rootdir->enter (&ent);
    printf ("enter rc=%04o\n", rc);
    if (rc >= ER_MIN) return 1;

    // write ABC...ZABC... to first block of file
    {
        Word buffer[WPB];
        for (int i = 0; i < WPB; i ++) {
            buffer[i] = 'A' + (i % 26);
        }
        Stream alphabet (file, false);
        rc = alphabet.write (buffer, WPB);
        printf ("write rc=%04o\n", rc);
        if (rc >= ER_MIN) return 1;
        rc = alphabet.close ();
        printf ("close rc=%04o\n", rc);
        if (rc >= ER_MIN) return 1;
    }
    file->close ();
    file = NULL;

    // copy source file to a file on the volume
    rc = copyinfile ("filesys.cc", rootdir, "filesys.cc");
    if (rc != 0) return rc;

    rc = copyinfile ("filesys.h", rootdir, "filesys.h");
    if (rc != 0) return rc;

    rc = copyinfile ("nullboot.lis", rootdir, "nullboot.lis");
    if (rc != 0) return rc;

    rc = copyinfile ("../driver/raspictl.cc", rootdir, "raspictl.cc");
    if (rc != 0) return rc;

    rc = copyinfile ("../driver/shadow.cc", rootdir, "shadow.cc");
    if (rc != 0) return rc;

    rc = copyinfile ("../goodpcbs/regboard.net", rootdir, "regboard.net");
    if (rc != 0) return rc;

    copyoutfile (rootdir, "filesys.cc",   "01.cc");
    copyoutfile (rootdir, "filesys.h",    "02.h");
    copyoutfile (rootdir, "nullboot.lis", "03.lis");
    copyoutfile (rootdir, "raspictl.cc",  "04.cc");
    copyoutfile (rootdir, "shadow.cc",    "05.cc");
    copyoutfile (rootdir, "regboard.net", "06.net");

    // dump all files in root directory
    for (Word vbn = 0; vbn < rootdir->eofvbn; vbn ++) {

        // read block from root directory
        Dirent entries[EPB];
        Word lbn = rootdir->vbntolbn (vbn);
        if (lbn >= ER_MIN) ABORT ();
        rc = rootdir->vol->disk->rlb (lbn, WPB, (Word *) entries);
        if (lbn >= ER_MIN) ABORT ();
        for (Word i = 0; i < EPB; i ++) {

            // get entry from root directory
            Dirent *de = &entries[i];
            if (de->filnum != 0) {

                // print out the filenumber, sequence, filename
                char namestr[13];
                for (int j = 0; j < 6; j ++) {
                    // 00 -> 100; 40 -> 040
                    namestr[j*2+0] = (((de->filnam[j] >> 6) & 077) ^ 040) + 040;
                    namestr[j*2+1] =  ((de->filnam[j]       & 077) ^ 040) + 040;
                }
                namestr[12] = 0;
                for (int j = 12; -- j >= 0;) {
                    if (namestr[j] != '@') break;
                    namestr[j] = 0;
                }
                printf ("%04o,%04o %s\n", de->filnum, de->filseq, namestr);

                // open the corresponding file
                rc = volume.openfile (&file, de->filnum, de->filseq);
                if (rc >= ER_MIN) {
                    printf ("  open rc=%04o\n", rc);
                } else {

                    // dump file contents
                    rc = dumpfile (file, stdout);
                    if (rc >= ER_MIN) printf ("  dump rc=%04o\n", rc);

                    // close file
                    file->close ();
                    file = NULL;
                }
            }
        }
    }

    // dismount the volume
    rootdir->close ();
    rootdir = NULL;
    printf ("nfreeblks=%u\n", volume.nfreeblks);
    rc = volume.fsuck ();
    printf ("fsuck=%u\n", rc);
    rc = volume.umount ();
    printf ("umount rc=%04o\n", rc);
    if (rc >= ER_MIN) return 1;

    return 0;
}

// write boot block to disk
//  input:
//   disk = disk to write to
//   label = volume label string
//  output:
//   boot block written
//   returns 0: success
//           1: failure
static int writebootblock (Disk *disk, char const *label)
{
    // open boot object code file
    FILE *nbf = fopen ("nullboot.obj", "r");
    if (nbf == NULL) {
        fprintf (stderr, "error opening nullboot.obj: %m\n");
        return 1;
    }

    // read boot object code into block buffer
    char nbb[160], *p;
    Word buffer[WPB], rc;
    Word volbl = 0;
    memset (buffer, 0, sizeof buffer);
    while (fgets (nbb, sizeof nbb, nbf) != NULL) {

        // check for ?volbl=<addr> line
        if (memcmp (nbb, "?volbl=", 7) == 0) {
            volbl = strtoul (nbb + 8, &p, 8);
            continue;
        }

        // parse <addr>:<data>... and store in block buffer
        // all addresses should fit in that array
        Word addr = strtoul (nbb, &p, 8);
        if (*p != '=') goto badobj;
        p ++;
        while (*p != '\n') {
            Word x = 0;
            for (int i = 0; i < 4; i ++) {
                char c = *(p ++);
                if ((c & ~ 7) != '0') goto badobj;
                x = x * 8 + (c & 7);
            }
            if (addr >= WPB) goto badobj;
            buffer[addr++] = x;
        }
    }
    fclose (nbf);
    if (volbl != 0) {
        while (*label != 0) {
            buffer[volbl++] = *(label ++);
        }
        buffer[volbl++] = '\r';
        buffer[volbl++] = '\n';
        buffer[volbl]   = 0;
    }
    rc = disk->wlb (0, WPB, buffer);
    printf ("boot write rc=%04o\n", rc);
    return rc >= ER_MIN;

badobj:
    printf ("bad obj line %s", nbb);
    fclose (nbf);
    return 1;
}

// copy an external file to the disk
//  input:
//   fromfile = external file name
//   rootdir = directory to enter the file in
//   tofile = on-disk file name
//  output:
//   copies file to disk
//   returns 0: success
//        else: failure
static int copyinfile (char const *fromfile, File *rootdir, char const *tofile)
{
    printf ("copyinfile: %s -> %s\n", fromfile, tofile);
    int fd = open (fromfile, O_RDONLY);
    if (fd < 0) {
        fprintf (stderr, "error opening %s: %m\n", fromfile);
        return 1;
    }

    // create disk file
    Dirent ent;
    ent.fill (0, 0, tofile);
    File *file;
    Word rc = rootdir->vol->createfile (&file, &ent);
    printf ("createfile rc=%04o\n", rc);
    if (rc >= ER_MIN) return 1;
    printf ("copyinfile:   (%04o,%04o)\n", file->filnum, file->filseq);

    // write directory entry
    rc = rootdir->enter (&ent);
    printf ("enter rc=%04o\n", rc);
    if (rc >= ER_MIN) return 1;

    // copy blocks
    Stream source (file, false);
    while (true) {

        // read bytes from the external file
        uint8_t bytes[WPB*3/2];
        Word buffer[WPB];
        int rc2 = read (fd, bytes, sizeof bytes);
        if (rc2 <= 0) break;

        // pack the 8-bit chars to 12-bit words pdp-8 style
        int i;
        int j = 0;
        for (i = 0; i + 3 <= rc2; i += 3) {
            uint8_t a = bytes[i+0];
            uint8_t b = bytes[i+1];
            uint8_t c = bytes[i+2];
            buffer[j++] = a | ((c << 4) & 07400);
            buffer[j++] = b | ((c << 8) & 07400);
        }
        while (i < rc2) {
            buffer[j++] = bytes[i++];
        }

        // write to disk file
        rc = source.write (buffer, j);
        if (rc >= ER_MIN) {
            printf ("write rc=%04o\n", rc);
            return 1;
        }
    }

    // close it all up
    close (fd);
    rc = source.close ();
    printf ("close rc=%04o\n", rc);
    if (rc >= ER_MIN) return 1;
    file->close ();
    return 0;
}

// copy a file from the disk to an external file
//  input:
//   rootdir = directory to lookup the file in
//   fromfile = external file name
//   tofile = on-disk file name
//  output:
//   copies file to disk
//   returns 0: success
//        else: failure
static int copyoutfile (File *rootdir, char const *fromfile, char const *tofile)
{
    printf ("copyoutfile: %s -> %s\n", fromfile, tofile);

    // lookup disk file
    Dirent ent;
    ent.fill (0, 0, fromfile);
    Word rc = rootdir->lookup (&ent);
    printf ("lookup rc=%04o\n", rc);
    if (rc >= ER_MIN) return 1;

    // open disk file
    File *file;
    rc = rootdir->vol->openfile (&file, ent.filnum, ent.filseq);
    printf ("open rc=%04o\n", rc);
    if (rc >= ER_MIN) return 1;

    // create external file
    int fd = open (tofile, O_CREAT | O_WRONLY, 0666);
    if (fd < 0) {
        fprintf (stderr, "error opening %s: %m\n", tofile);
        return 1;
    }

    // copy blocks
    Stream source (file, true);
    while (true) {

        // read words from disk file
        Word buffer[WPB];
        rc = source.read (buffer, WPB);
        if (rc >= ER_MIN) {
            printf ("read rc=%04o\n", rc);
            return 1;
        }
        if (rc == 0) break;

        // convert 12-bit words to 8-bit chars
        uint8_t bytes[WPB*3/2];
        int i;
        int j = 0;
        for (i = 0; i + 2 <= rc; i += 2) {
            bytes[j++] = buffer[i+0];
            bytes[j++] = buffer[i+1];
            bytes[j++] = ((buffer[i+0] >> 4) & 0xF0) | ((buffer[i+1] >> 8) & 0x0F);
        }
        if (i < rc) {
            bytes[j++] = buffer[i];
        }
        if ((j > 0) && (bytes[j-1] == 0)) -- j;

        // write to external file
        int rc2 = write (fd, bytes, j);
        if (rc2 < 0) {
            fprintf (stderr, "error writing %s: %m\n", tofile);
            return 1;
        }
        if (rc2 < j) {
            fprintf (stderr, "only wrote %d of %d bytes to %s\n", rc, j, tofile);
            return 1;
        }
    }

    // close it all up
    close (fd);
    rc = source.close ();
    printf ("close rc=%04o\n", rc);
    if (rc >= ER_MIN) return 1;
    file->close ();
    return 0;
}
