
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

static Word lastchar = 0177777;
static void outch (Word ch)
{
    if (lastchar != 0177777) putchar ((char) lastchar);
    lastchar = ch & 0377;
}

int main (int argc, char **argv)
{
    setlinebuf (stdout);
    setlinebuf (stderr);

    Stream *stream;
    Word buf[WPB], pad, rc;

    File *file = NULL;
    Vol *vol = NULL;
    Word charsize = 12;
    for (int i = 1; i < argc; i ++) {
        if (strcmp (argv[i], "-6") == 0) {
            charsize = 6;
            continue;
        }
        if (strcmp (argv[i], "-8") == 0) {
            charsize = 8;
            continue;
        }
        if (strcmp (argv[i], "-12") == 0) {
            charsize = 12;
            continue;
        }
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

    pad = file->eofwrd & 04000;     // file is padded with a nul on the end
    file->eofwrd &= 03777;
    stream = new Stream (file, true);
    while ((rc = stream->read (buf, WPB)) != 0) {
        switch (charsize) {
            case 12: {
                for (Word i = 0; i < rc; i ++) {
                    putchar (buf[i]);
                }
                break;
            }
            case 8: {
                for (Word i = 0; i + 2 <= rc;) {
                    Word a = buf[i++];
                    Word b = buf[i++];
                    Word c = ((a >> 4) & 0360) | ((b >> 8) & 0017);
                    outch (a);
                    outch (b);
                    outch (c);
                }
                if (rc & 1) {   // should only happen at eof
                    outch (buf[rc-1]);
                }
                break;
            }
            case 6: {
                for (Word i = 0; i < rc; i ++) {
                    Word w = buf[i];
                    outch ((((w >> 6) & 077) ^ 040) + 040);
                    outch ((( w       & 077) ^ 040) + 040);
                }
                break;
            }
            default: ABORT ();
        }
    }

    if (! pad) outch (0);
    return 0;

usage:;
    fprintf (stderr, "usage: readdisk [-12 (default) | -8 | -6] <diskfile> [ <subdir> ... ] <filename>\n");
    return 1;
}
