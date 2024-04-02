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

// takes an instruction trace from raspictl -printinstr and prints correspondimg lines from listing file
// trace [-pc pclo-pchi] ... listfile < tracefile > enhancedtracefile

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct Line {
    struct Line *next;
    int address;
    char data[1];
} Line;

typedef struct PCRange {
    struct PCRange *next;
    int pclo, pchi;
} PCRange;

int main (int argc, char **argv)
{
    char const *listname;
    char linebuf[1000], *p;
    FILE *listfile;
    int address, i, j;
    Line *line, *lines, **lline;
    Line *lineindex[32768];
    PCRange *pcrange, *pcranges;

    setlinebuf (stdout);

    listname = NULL;
    pcranges = NULL;
    for (i = 1; i < argc; i ++) {
        if (strcasecmp (argv[i], "-pc") == 0) {
            if (++ i >= argc) goto usage;
            pcrange = malloc (sizeof *pcrange);
            pcrange->next = pcranges;
            pcrange->pclo = strtoul (argv[i], &p, 8);
            if (*p != '-') goto usage;
            pcrange->pchi = strtoul (p + 1, &p, 8);
            if (*p != 0) goto usage;
            pcranges = pcrange;
            continue;
        }
        if (argv[i][0] == '-') goto usage;
        if (listname != NULL) goto usage;
        listname = argv[i];
    }
    if (listname == NULL) goto usage;

    memset (lineindex, 0, sizeof lineindex);

    listfile = fopen (listname, "r");
    if (listfile == NULL) {
        fprintf (stderr, "error opening %s: %m\n", listname);
        return 1;
    }

    lline = &lines;
    while (fgets (linebuf, sizeof linebuf, listfile) != NULL) {
        if ((linebuf[0] >= '0') && (linebuf[0] <= '7') && (linebuf[7] >= '0') && (linebuf[7] <= '7')) {
            address = strtol (linebuf, &p, 8);
            if ((address >= 0) && (address <= 32767)) {
                line = malloc (strlen (linebuf) + sizeof *line);
                line->address = address;
                strcpy (line->data, linebuf);
                *lline = line;
                lline = &line->next;
                lineindex[address] = line;
            }
        }
    }
    fclose (listfile);
    *lline = NULL;

    while (fgets (linebuf, sizeof linebuf, stdin) != NULL) {
        p = strstr (linebuf, "PC=");
        if (p != NULL) {
            address = strtol (p + 3, &p, 8);
            if (pcranges != NULL) {
                for (pcrange = pcranges; pcrange != NULL; pcrange = pcrange->next) {
                    if ((address >= pcrange->pclo) && (address <= pcrange->pchi)) goto inrange;
                }
                goto outrange;
            inrange:;
            }
            line = lineindex[address];
            if (line != NULL) {
                for (j = 0;; j ++) {
                    if (line->data[16+j] != ' ') break;
                }
                i = strlen (linebuf);
                while ((i > 0) && (linebuf[i-1] < ' ')) -- i;
                if (i > 78 + j) i = 78 + j;
                memset (linebuf + i, ' ', 78 + j - i);
                linebuf[78+j] = 0;
                printf ("%s  %s", linebuf, line->data + 16 + j);
                continue;
            }
        }
        fputs (linebuf, stdout);
    outrange:;
    }
    return 0;

usage:
    fprintf (stderr, "usage: trace [-pc pclo-pchi] ... listfile < tracefile > enhancedtracefile\n");
    return 1;
}
