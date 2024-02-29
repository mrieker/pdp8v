
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

int main (int argc, char **argv)
{
    setlinebuf (stdout);
    setlinebuf (stderr);

    File *file = NULL;
    Stream *stream;
    Vol *vol = NULL;
    Word rc;
    for (int i = 1; i < argc; i ++) {
        if (argv[i][0] == '-') goto usage;
        if (file == NULL) {
            setenv ("rk8disk_0", argv[i], true);
            RK8Disk disk (0, true);
            vol = new Vol (new RK8Disk (0, true));
            rc = vol->mount (NULL, true);
            if (rc >= ER_MIN) {
                fprintf (stderr, "error %04o mounting disk\n", rc);
                return 1;
            }
            printf ("free blocks: %u\n", vol->nfreeblks);
            rc = vol->openfile (&file, 2, 1);
            if (rc >= ER_MIN) {
                fprintf (stderr, "error %04o opening root directory\n", rc);
                return 1;
            }
            continue;
        }
        Dirent de;
        de.fill (0, 0, argv[i]);
        rc = file->lookup (&de);
        if (rc >= ER_MIN) {
            fprintf (stderr, "error %04o looking up %s\n", rc, argv[i]);
            return 1;
        }
        file->close ();
        rc = vol->openfile (&file, de.filnum, de.filseq);
        if (rc >= ER_MIN) {
            fprintf (stderr, "error %04o opening %s\n", rc, argv[i]);
            return 1;
        }
    }
    if (file == NULL) goto usage;

    // dump all filenames in directory
    stream = new Stream (file, true);
    while (true) {

        // read block from directory
        Dirent de;
        rc = stream->read ((Word *) &de, sizeof de / sizeof (Word));
        if (rc >= ER_MIN) {
            fprintf (stderr, "error %04o reading directory\n", rc);
            return 1;
        }
        if (rc == 0) break;
        ASSERT (rc == sizeof de / sizeof (Word));

        // get entry from directory
        if (de.filnum != 0) {

            // print out the filenumber, sequence, filename
            char namestr[13];
            for (int j = 0; j < 6; j ++) {
                // 00 -> 100; 40 -> 040
                namestr[j*2+0] = (((de.filnam[j] >> 6) & 077) ^ 040) + 040;
                namestr[j*2+1] =  ((de.filnam[j]       & 077) ^ 040) + 040;
            }
            namestr[12] = 0;
            for (int j = 12; -- j >= 0;) {
                if (namestr[j] != '@') break;
                namestr[j] = 0;
            }
            printf ("%04o,%04o  %-12s", de.filnum, de.filseq, namestr);

            // open the corresponding file to get its attributes
            File *file;
            rc = vol->openfile (&file, de.filnum, de.filseq);
            if (rc >= ER_MIN) {
                printf ("  error %04o\n", rc);
            } else {

                // print out its attributes (modtime, eof position, allocated blocks)
                time_t modtimsec = (((time_t) file->modtim[0]) << 24) | (((time_t) file->modtim[1]) << 12) | file->modtim[2];
                struct tm modtimtm = *gmtime (&modtimsec);
                char pad = ' ';
                if (file->eofwrd & 04000) {
                    pad = '+';
                    file->eofwrd -= 04001;
                }
                printf ("  %04d-%02u-%02u@%02u:%02u:%02u.%04u  %7u%c/%u\n",
                    modtimtm.tm_year + 1900, modtimtm.tm_mon + 1, modtimtm.tm_mday, modtimtm.tm_hour, modtimtm.tm_min, modtimtm.tm_sec,
                    file->modtim[3] * 10000 / 4096, file->eofvbn * WPB + file->eofwrd, pad, file->allocd);

                // close file
                file->close ();
                file = NULL;
            }
        }
    }
    return 0;

usage:;
    fprintf (stderr, "usage: listdisk <diskfile> [ <subdir> ... ]\n");
    return 1;
}
