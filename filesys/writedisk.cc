
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filesys.h"

#define ABORT() do { fprintf (stderr, "ABORT %s:%d\n", __FILE__, __LINE__); abort (); } while (0)
#define ASSERT(cond) do { if (__builtin_constant_p (cond)) { if (!(cond)) asm volatile ("assert failure"); } else { if (!(cond)) ABORT (); } } while (0)

int main (int argc, char **argv)
{
    setlinebuf (stdout);
    setlinebuf (stderr);

    int ch;
    Stream *stream;
    Word buf[WPB], pad, rc;

    char const *filnm = NULL;
    Dirent de;
    de.fill (0, 0, "");
    File *file = NULL;
    File *newfile;
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
            RK8Disk disk (0, false);
            vol = new Vol (new RK8Disk (0, false));
            rc = vol->mount (NULL, false);
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

        if (de.filnam[0] != 0) {
            rc = file->lookup (&de);
            if (rc < ER_MIN) {
                rc = vol->openfile (&newfile, de.filnum, de.filseq);
                if (rc >= ER_MIN) {
                    fprintf (stderr, "error %04o opening directory %s\n", rc, filnm);
                    return 1;
                }
            }
            else if (rc == ER_NSF) {
                rc = vol->createfile (&newfile, &de);
                if (rc >= ER_MIN) {
                    fprintf (stderr, "error %04o creating directory %s\n", rc, filnm);
                    return 1;
                }
                rc = file->enter (&de);
                if (rc >= ER_MIN) {
                    fprintf (stderr, "error %04o entering directory %s\n", rc, filnm);
                    return 1;
                }
            }
            else {
                fprintf (stderr, "error %04o looking up %s\n", rc, filnm);
                return 1;
            }
            file->close ();
            file = newfile;
        }

        filnm = argv[i];
        de.fill (0, 0, filnm);
        if (de.filnam[0] == 0) goto usage;
    }
    if (de.filnam[0] == 0) goto usage;
    rc = vol->createfile (&newfile, &de);
    if (rc >= ER_MIN) {
        fprintf (stderr, "error %04o creating file %s\n", rc, filnm);
        return 1;
    }
    rc = file->enter (&de);
    if (rc >= ER_MIN) {
        fprintf (stderr, "error %04o entering file %s\n", rc, filnm);
        newfile->markdl = true;
        newfile->close ();
        return 1;
    }
    file->close ();
    file = newfile;

    stream = new Stream (file, false);
    rc  = 0;
    pad = 0;
    while ((ch = getchar ()) >= 0) {
        ASSERT (ch <= 0377);
        switch (charsize) {
            case 12: {
                buf[rc++] = ch;
                break;
            }
            case 8: {
                if (pad) {
                    buf[rc-2] |= (ch << 4) & 07400;
                    buf[rc-1] |= (ch << 8) & 07400;
                    pad = 0;
                }
                else if (rc & 1) {
                    buf[rc++] = ch;
                    pad = 04000;
                }
                else {
                    buf[rc++] = ch;
                }
                break;
            }
            case 6: {
                if (ch >= 0140) ch -= 0040;
                if (pad) {
                    buf[rc-1] |= ch & 077;
                    pad = 0;
                } else {
                    buf[rc++] = (ch << 6) & 07700;
                    pad = 04000;
                }
                break;
            }
            default: ABORT ();
        }
        if ((rc | pad) == WPB) {
            rc = stream->write (buf, WPB);
            if (rc != WPB) {
                fprintf (stderr, "error %04o writing %s\n", rc, filnm);
                return 1;
            }
            rc = 0;
        }
    }

    if (rc > 0) {
        Word rc2 = stream->write (buf, rc);
        if (rc2 != rc) {
            fprintf (stderr, "error %04o writing %s\n", rc2, filnm);
            return 1;
        }
    }
    file->eofwrd |= pad;
    stream->dirty ++;
    rc = stream->close ();
    if (rc < ER_MIN) rc = file->close ();
    if (rc >= ER_MIN) {
        fprintf (stderr, "error %04o closing %s\n", rc, filnm);
        return 1;
    }

    rc = vol->umount ();
    if (rc >= ER_MIN) {
        fprintf (stderr, "error %04o unmounting vokume\n", rc);
        return 1;
    }
    return 0;

usage:;
    fprintf (stderr, "usage: writedisk [-12 (default) | -8 | -6] <diskfile> [ <subdir> ... ] <filename>\n");
    return 1;
}
