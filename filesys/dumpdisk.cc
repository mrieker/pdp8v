
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

    Word rc;

    File *file = NULL;
    Vol *vol = NULL;
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

    dumpfile (file, stdout);
    return 0;

usage:;
    fprintf (stderr, "usage: dumpdisk [-12 (default) | -8 | -6] <diskfile> [ <subdir> ... ] <filename>\n");
    return 1;
}
