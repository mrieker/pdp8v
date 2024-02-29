
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "filesys.h"

int main (int argc, char **argv)
{
    setlinebuf (stdout);
    setlinebuf (stderr);

    Word label[HBLEN], rc;
    memset (label, 0, sizeof label);

    Vol *vol = NULL;
    for (int i = 1; i < argc; i ++) {
        if (strcasecmp (argv[i], "-l") == 0) {
            if (++ i >= argc) goto usage;
            char *lbl = argv[i];
            Word c = 1;
            Word d = 1;
            for (Word i = 0; i < HBLEN; i ++) {
                c = (d == 0) ? 0 : *(lbl ++) & 0177;
                d = (c == 0) ? 0 : *(lbl ++) & 0177;
                if (c >= 0140) c -= 040;
                if (d >= 0140) d -= 040;
                label[i] = ((c & 077) << 6) | (d & 077);
            }
            continue;
        }
        if (argv[i][0] == '-') goto usage;
        if (vol != NULL) goto usage;
        setenv ("rk8disk_0", argv[i], true);
        RK8Disk disk (0, false);
        vol = new Vol (new RK8Disk (0, false));
    }
    if (vol == NULL) goto usage;

    rc = vol->initvol (label);
    if (rc >= ER_MIN) {
        fprintf (stderr, "error %04o initializing volume\n", rc);
        return 1;
    }
    return 0;

usage:;
    fprintf (stderr, "usage: initvol [-l <label>] <diskfile>\n");
    return 1;
}
