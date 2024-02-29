//    Copyright (C) Mike Rieker, Beverly, MA USA
//    www.outerworldapps.com
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; version 2 of the License.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    EXPECT it to FAIL when someone's HeALTh or PROpeRTy is at RISk.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
//    http://www.gnu.org/licenses/gpl-2.0.html

// lookup PC= values from raspictl -printinstr in the map file
//  ./raspictl -printinstr <hexfile> | ./applymap <mapfile>

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct PSect PSect;

struct PSect {
    PSect *next;            // next in psects list
    uint16_t base;          // base address of psect in file
    uint16_t size;          // size of the section
    char *name;             // psect name
    char file[1];           // file name
};

int main (int argc, char **argv)
{
    char c, mapline[4096], *p, *psectname, *q, *words[4096];
    FILE *mapfile;
    int i, nwords, skipsp;
    PSect *psect, *psects;
    uint16_t pcrel, pcval;

    setlinebuf (stdout);

    mapfile = fopen (argv[1], "r");
    if (mapfile == NULL) {
        fprintf (stderr, "error opening %s: %m\n", argv[1]);
        return 1;
    }
    psectname = NULL;
    psects = NULL;
    while (fgets (mapline, sizeof mapline, mapfile) != NULL) {

        // split mapline into words
        nwords = 0;
        skipsp = 1;
        for (i = 0; mapline[i] != 0; i ++) {
            c = mapline[i];
            if (c <= ' ') {
                skipsp = 1;
                mapline[i] = 0;
            } else if (skipsp) {
                skipsp = 0;
                words[nwords++] = &mapline[i];
            }
        }

        // empty line resets the psect name
        if (nwords == 0) psectname = NULL;

        // size ... base ... alin ... psect ... line starts a psect section
        if ((nwords == 8) && (strcmp (words[0], "size") == 0) && (strcmp (words[2], "base") == 0) && (strcmp (words[4], "alin") == 0) && (strcmp (words[6], "psect") == 0)) {
            psectname = strdup (words[7]);
        }

        // <hex> <hex> <hex> <filename> gives a filename in a psect
        if ((psectname != NULL) && (nwords == 4)) {
            psect = malloc (strlen (words[3]) + sizeof *psect);
            psect->next = psects;
            psect->size = strtoul (words[0], NULL, 16);
            psect->base = strtoul (words[1], NULL, 16);
            psect->name = psectname;
            strcpy (psect->file, words[3]);
            psects = psect;
            printf ("psect %s, file %s, base %04X, size %04X\n", psect->name, psect->file, psect->base, psect->size);
        }
    }
    fclose (mapfile);

    while (fgets (mapline, sizeof mapline, stdin) != NULL) {
        q = strstr (mapline, "R0=");
        p = strstr (mapline, "PC=");
        if ((q != NULL) && (p != NULL)) {
            pcval = strtoul (p + 3, NULL, 16);
            for (psect = psects; psect != NULL; psect = psect->next) {
                pcrel = pcval - psect->base;
                if (pcrel < psect->size) {
                    p = strchr (q, '\n');
                    if (p != NULL) *p = 0;
                    printf ("%-112s  %04X + %s / %s\n", mapline, pcrel, psect->name, psect->file);
                    goto nextline;
                }
            }
        }
        fputs (mapline, stdout);
    nextline:;
    }

    return 0;
}
