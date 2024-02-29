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

// link a bunch of assembler output files to create runnable hex file
//  ./link.x86_64 -o <hexfile> <asmoutfile> ... > <mapfile>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pflags.h"

#define bootsymname "__boot"    // defined in bootpage.asm

typedef struct Objfile Objfile;
typedef struct PSect PSect;
typedef struct Symbol Symbol;

struct Objfile {            // input object file
    Objfile *next;          // next on objfiles list
    PSect *psects;          // psects in this object file
                            //  size = size of psect in this object file
                            //  base = offset within overall psect
    char const *fname;      // filename (archive or direct object file)
    long position;          // starting position within archive (or 0 for direct object file)
    char ifrefd;            // include file only if referenced
    char name[1];           // object filename
};

struct PSect {
    PSect *next;            // next in psects list
    PSect *allpsect;        // allpsects: NULL
                            // objpsects: points to entry in allpsects
    uint16_t base;          // allpsects: absolute start address of psect
                            // objpsects: start address within allpsect
    uint16_t size;          // size of the section
    uint16_t pflg;          // flags
    uint16_t alin;          // alignment in bytes (1, 2, 4, 8, ...)
    char name[1];           // null terminated psect name
};

struct Symbol {
    Symbol *next;           // next in symbols list
    PSect *objpsect;        // value relocation
    Objfile *defobjfile;    // defining object file
    uint16_t value;         // value
    char name[1];           // null terminated symbol name
};

static char error;
static char repeatpass2;
static FILE *outfile;
static int passno;          // 1: gathering psects & symbols; 2: generate output file
static Objfile *objfiles;   // as given on command line
static PSect *allpsects;    // combined psects
                            // - size is totals from all object files
                            // - base is absolute starting address of psect in output file
static Symbol *allsymbols;  // as gathered from all input files

static void dolinkerpass ();
static void processobjline (Objfile *objfile, char *objline);
static char *parsevaluestring (char *str, Objfile *objfile, uint16_t *value_r, PSect **objpsect_r);
static char *parsememrefstring (char *str, Objfile *objfile, uint16_t *value_r, uint16_t emitaddr);
static char *endofsymbol (char *sym);
static int sortpsect (void const *a, void const *b);
static int sortsymbyname (void const *a, void const *b);
static int sortsymbyaddr (void const *a, void const *b);
static uint16_t getsymval (Symbol const *symbol);

int main (int argc, char **argv)
{
    char linebuf[256];
    char ifrefd;
    int i;
    PSect *allpsect, *objpsect;
    Symbol *bootsym, *symbol;

    Objfile **lobjfile, *objfile;
    char const *outname = NULL;

    lobjfile = &objfiles;

    ifrefd = 0;
    for (i = 0; ++ i < argc;) {
        if (strcasecmp (argv[i], "-ifrefd") == 0) {
            ifrefd = 1;
            continue;
        }
        if (strcasecmp (argv[i], "-o") == 0) {
            outname = argv[++i];
            continue;
        }
        if (argv[i][0] == '-') goto usage;
        FILE *ofile = fopen (argv[i], "r");
        if (ofile == NULL) {
            fprintf (stderr, "error opening %s: %m\n", argv[i]);
            error = 1;
            continue;
        }
        if ((fgets (linebuf, sizeof linebuf, ofile) != NULL) && (linebuf[0] == ':')) {
            while (1) {
                char *p = strchr (linebuf, '\n');
                if (p != NULL) *p = 0;
                objfile = malloc (strlen (argv[i]) + strlen (linebuf) + sizeof *objfile);
                objfile->fname = argv[i];
                objfile->psects = NULL;
                objfile->position = ftell (ofile);
                objfile->ifrefd = 1;
                sprintf (objfile->name, "%s%s", argv[i], linebuf);
                *lobjfile = objfile;
                lobjfile = &objfile->next;
                do if (fgets (linebuf, sizeof linebuf, ofile) == NULL) goto endarchive;
                while (linebuf[0] != ':');
            }
        endarchive:;
        } else {
            objfile = malloc (strlen (argv[i]) + sizeof *objfile);
            objfile->fname = argv[i];
            objfile->psects = NULL;
            objfile->position = 0;
            objfile->ifrefd = ifrefd;
            strcpy (objfile->name, argv[i]);
            *lobjfile = objfile;
            lobjfile = &objfile->next;
        }
        fclose (ofile);
    }
    *lobjfile = NULL;
    if (objfiles == NULL) goto usage;

    // pass 1: read psects and symbols into memory
    passno = 1;
    dolinkerpass ();

    // mark __boot as not ifrefd so it will always be loaded
    bootsym = NULL;
    for (symbol = allsymbols; symbol != NULL; symbol = symbol->next) {
        if (strcmp (symbol->name, bootsymname) == 0) {
            symbol->defobjfile->ifrefd = 0;
            bootsym = symbol;
            goto foundbootsym;
        }
    }
    fprintf (stderr, "no %s boot symbol found, cannot be started\n", bootsymname);
    error = 1;
foundbootsym:;

    // pass 2: clear ifrefd flag on objfiles that are referenced
    passno = 2;
    do {
        repeatpass2 = 0;
        dolinkerpass ();
    } while (repeatpass2);

    // assign addresses to psects
    for (lobjfile = &objfiles; (objfile = *lobjfile) != NULL;) {
        if (objfile->ifrefd) {
            // object module not referenced, get rid of it
            *lobjfile = objfile->next;
        } else {
            // referenced, go through all psects therein
            for (objpsect = objfile->psects; objpsect != NULL; objpsect = objpsect->next) {
                allpsect = objpsect->allpsect;
                if (allpsect->alin < objpsect->alin) allpsect->alin = objpsect->alin;
                if (allpsect->pflg & PF_OVR) {
                    objpsect->base = 0;
                    if (allpsect->size < objpsect->size) allpsect->size = objpsect->size;
                } else {
                    objpsect->base = (allpsect->size + objpsect->alin - 1) & - objpsect->alin;
                    allpsect->size = objpsect->base + objpsect->size;
                }
            }
            lobjfile = &objfile->next;
        }
    }

    // place psects in alphabetical order and print map to stdout
    // assign psects in alpabetical order
    int npsects = 0;
    for (allpsect = allpsects; allpsect != NULL; allpsect = allpsect->next) {
        npsects ++;
    }
    PSect **psectarray = malloc (npsects * sizeof *psectarray);
    npsects = 0;
    for (allpsect = allpsects; allpsect != NULL; allpsect = allpsect->next) {
        psectarray[npsects++] = allpsect;
    }
    qsort (psectarray, npsects, sizeof *psectarray, sortpsect);
    uint16_t base = 0;
    for (i = 0; i < npsects; i ++) {
        allpsect = psectarray[i];
        base = (base + allpsect->alin - 1) & - allpsect->alin;
        allpsect->base = base;
        printf ("\n  size %04o  base %05o  alin %05o  psect %s\n", allpsect->size, allpsect->base, allpsect->alin, allpsect->name);
        base += allpsect->size;
        for (objfile = objfiles; objfile != NULL; objfile = objfile->next) {
            for (objpsect = objfile->psects; objpsect != NULL; objpsect = objpsect->next) {
                if (objpsect->allpsect == allpsect) {
                    printf ("       %04o       %05o       %05o  %s\n", objpsect->size, allpsect->base + objpsect->base, objpsect->alin, objfile->name);
                }
            }
        }
    }

    // print symbols sorted by name in left column and address in right column
    printf ("\n");
    int nsymbols = 0;
    int wsymbols = 0;
    int wfilenms = 0;
    for (symbol = allsymbols; symbol != NULL; symbol = symbol->next) {
        if (! symbol->defobjfile->ifrefd) {
            nsymbols ++;
            int w = strlen (symbol->name);
            if (wsymbols < w) wsymbols = w;
            w = strlen (symbol->defobjfile->name);
            if (wfilenms < w) wfilenms = w;
        }
    }
    Symbol **symbyname = malloc (nsymbols * sizeof *symbyname);
    Symbol **symbyaddr = malloc (nsymbols * sizeof *symbyname);
    i = 0;
    for (symbol = allsymbols; symbol != NULL; symbol = symbol->next) {
        if (! symbol->defobjfile->ifrefd) {
            symbyname[i] = symbol;
            symbyaddr[i] = symbol;
            i ++;
        }
    }
    qsort (symbyname, nsymbols, sizeof *symbyname, sortsymbyname);
    qsort (symbyaddr, nsymbols, sizeof *symbyaddr, sortsymbyaddr);
    for (i = 0; i < nsymbols; i ++) {
        Symbol *sbn = symbyname[i];
        Symbol *sba = symbyaddr[i];
        printf ("  %05o  %-*s  %-*s    %05o  %-*s  %s\n",
                getsymval (sbn), wsymbols, sbn->name, wfilenms, sbn->defobjfile->name,
                getsymval (sba), wsymbols, sba->name, sba->defobjfile->name);
    }
    printf ("\n");

    // don't bother with output file if error
    if (error) return error;

    // create output file
    outfile = fopen (outname, "w");
    if (outfile == NULL) {
        fprintf (stderr, "error creating %s: %m\n", outname);
        return 1;
    }

    // pass 3: generate output file
    passno = 3;
    dolinkerpass ();
    if (bootsym != NULL) {
        // startaddress: without any data
        fprintf (outfile, "%05o:\n", getsymval (bootsym));
    }
    fclose (outfile);
    if (error) unlink (outname);

    return error;

usage:
    fprintf (stderr, "usage: link -o <outfile> <objfile> ...\n");
    return 1;
}

static void dolinkerpass ()
{
    char oline[256];
    FILE *ofile;
    Objfile *objfile;

    for (objfile = objfiles; objfile != NULL; objfile = objfile->next) {
        if ((passno == 1) || ! objfile->ifrefd) {
            ofile = fopen (objfile->fname, "r");
            if (ofile == NULL) {
                fprintf (stderr, "error opening %s: %m\n", objfile->fname);
                exit (1);
            }
            if (fseek (ofile, objfile->position, SEEK_SET) < 0) abort ();
            while (fgets (oline, sizeof oline, ofile) != NULL) {
                if (oline[0] == ':') break;
                processobjline (objfile, oline);
            }
            fclose (ofile);
        }
    }
}

static void processobjline (Objfile *objfile, char *objline)
{
    switch (objline[0]) {

        // '>' psectname '=' psectsize ';pflg=' flags ';p2al=' alignment
        case '>': {
            if (passno == 1) {

                // parse object line
                PSect *allpsect, *objpsect;
                char *psectname = objline + 1;
                char *p = endofsymbol (psectname);
                assert (*p == '=');
                *(p ++) = 0;
                uint16_t psectsize = strtoul (p, &p, 8);
                int p2align = 0;
                int pflags  = 0;
                while (1) {
                    if (memcmp (p, ";pflg=", 6) == 0) {
                        pflags  = (int) strtoul (p + 6, &p, 0);
                        continue;
                    }
                    if (memcmp (p, ";p2al=", 6) == 0) {
                        p2align = (int) strtoul (p + 6, &p, 0);
                        continue;
                    }
                    assert (*p < ' ');
                    break;
                }

                // find psect in list of all psects
                for (allpsect = allpsects; allpsect != NULL; allpsect = allpsect->next) {
                    if (strcmp (allpsect->name, psectname) == 0) break;
                }

                // find psect in object file's list of psects
                for (objpsect = objfile->psects; objpsect != NULL; objpsect = objpsect->next) {
                    if (strcmp (objpsect->name, psectname) == 0) break;
                }

                // add psect to list of all psects if not there already
                if (allpsect == NULL) {
                    allpsect = malloc (strlen (psectname) + sizeof *allpsect);
                    allpsect->next = allpsects;
                    allpsect->base = 0;
                    allpsect->size = 0;
                    allpsect->alin = p2align;
                    allpsect->pflg = pflags;
                    allpsect->allpsect = NULL;
                    strcpy (allpsect->name, psectname);
                    allpsects = allpsect;
                }

                // add psect to list of object file's psects
                assert (objpsect == NULL);
                objpsect = malloc (strlen (psectname) + sizeof *objpsect);
                objpsect->next = objfile->psects;
                objpsect->size = psectsize;
                objpsect->alin = 1U << p2align;
                objpsect->pflg = pflags;
                objpsect->allpsect = allpsect;
                strcpy (objpsect->name, psectname);
                objfile->psects = objpsect;
                allpsect->pflg |= objpsect->pflg;
            }
            break;
        }

        // '?' symbolname '=' value [ '+>' psectname ]
        case '?': {
            if (passno == 1) {
                char *symbolname = objline + 1;
                char *p = endofsymbol (symbolname);
                assert (*p == '=');
                *(p ++) = 0;
                uint16_t symvalue;
                PSect *sympsect;
                parsevaluestring (p, objfile, &symvalue, &sympsect);
                Symbol *symbol;
                for (symbol = allsymbols; symbol != NULL; symbol = symbol->next) {
                    if (strcmp (symbol->name, symbolname) == 0) {
                        if ((symbol->value != symvalue) || (symbol->objpsect->allpsect != sympsect->allpsect)) {
                            fprintf (stderr, "multiple definition of '%s' in '%s' and '%s'\n",
                                    symbolname, objfile->name, symbol->defobjfile->name);
                            error = 1;
                        }
                        goto symdefnd;
                    }
                }
                symbol = malloc (strlen (symbolname) + sizeof *symbol);
                symbol->next = allsymbols;
                symbol->value = symvalue;
                symbol->objpsect = sympsect;
                symbol->defobjfile = objfile;
                strcpy (symbol->name, symbolname);
                allsymbols = symbol;
            symdefnd:;
            }
            break;
        }

        // address = data
        case '0' ... '7': {
            if ((passno == 2) || (passno == 3)) {
                uint16_t address, value;
                PSect *objpsect;
                char *p = parsevaluestring (objline, objfile, &address, &objpsect);
                assert (*p == '=');
                if (objpsect != NULL) address += objpsect->base + objpsect->allpsect->base;
                if (passno == 3) fprintf (outfile, "%05o:", address);
                if (*(++ p) == '(') {
                    p = parsevaluestring (++ p, objfile, &value, &objpsect);
                    if (*p != '`') {
                        assert (*p == ')');
                        if (passno == 3) {
                            if (objpsect != NULL) value += objpsect->base + objpsect->allpsect->base;
                            fprintf (outfile, "%04o\n", value & 07777);
                        }
                    } else if (passno == 3) {
                        assert (objpsect == NULL);
                        p = parsememrefstring (p, objfile, &value, address);
                        assert (*p == ')');
                        fprintf (outfile, "%04o\n", value & 07777);
                    }
                } else {
                    if (passno == 3) {
                        fprintf (outfile, "%s", p);
                    }
                }
            }
            break;
        }
    }
}

// parse a value string
//  input:
//   str = points to value string to parse
//   objfile = object file being processed
//  output:
//   returns pointer to terminating character
//   *value_r = offset value given in value string
//   *objpsect_r = psect given in value string
//  syntax: octvalue[+>psectname][+?symbolname]
static char *parsevaluestring (char *str, Objfile *objfile, uint16_t *value_r, PSect **objpsect_r)
{
    *objpsect_r = NULL;
    *value_r = (uint16_t) strtoul (str, &str, 8);

    while (*str == '+') {
        str ++;
        switch (*str) {
            case '>': {
                char *p = endofsymbol (++ str);
                int len = p - str;
                PSect *objpsect;
                for (objpsect = objfile->psects; objpsect != NULL; objpsect = objpsect->next) {
                    if ((memcmp (objpsect->name, str, len) == 0) && (objpsect->name[len] == 0)) {
                        *objpsect_r = objpsect;
                        goto psectfound;
                    }
                }
                fprintf (stderr, "psect '%.*s' used for value in '%s' not defined\n", len, str, objfile->name);
                error = 1;
            psectfound:;
                str = p;
                break;
            }

            case '?': {
                char *p = endofsymbol (++ str);
                int len = p - str;
                Symbol *symbol;
                for (symbol = allsymbols; symbol != NULL; symbol = symbol->next) {
                    if ((memcmp (symbol->name, str, len) == 0) && (symbol->name[len] == 0)) {
                        *value_r += getsymval (symbol);
                        if (symbol->defobjfile->ifrefd) {
                            symbol->defobjfile->ifrefd = 0;
                            repeatpass2 = 1;
                            printf ("symbol %s referenced by %s causes %s to be included\n",
                                    symbol->name, objfile->name, symbol->defobjfile->name);
                        }
                        goto symbolfound;
                    }
                }
                fprintf (stderr, "symbol '%.*s' used for value in '%s' not defined\n", len, str, objfile->name);
                error = 1;
            symbolfound:;
                str = p;
                break;
            }

            default: abort ();
        }
    }
    return str;
}

// parse memory-reference instruction relocation string
//  input:
//   str = points to string starting with '`'
//   objfile = object file being processed
//   *value_r = base opcode
//   emitaddr = address the opcode is at
//  output:
//   returns pointer to terminating character
//   *value_r = base opcode plus memory reference
//  syntax: baseopcode[`>psectname][`?symbolname][`offset]
static char *parsememrefstring (char *str, Objfile *objfile, uint16_t *value_r, uint16_t emitaddr)
{
    uint16_t valtemp = 0;

    while (*str == '`') {
        str ++;
        switch (*str) {
            case '>': {
                char *p = endofsymbol (++ str);
                int len = p - str;
                PSect *objpsect;
                for (objpsect = objfile->psects; objpsect != NULL; objpsect = objpsect->next) {
                    if ((memcmp (objpsect->name, str, len) == 0) && (objpsect->name[len] == 0)) {
                        valtemp += objpsect->base + objpsect->allpsect->base;
                        goto psectfound;
                    }
                }
                fprintf (stderr, "psect '%.*s' used for value in '%s' not defined\n", len, str, objfile->name);
                error = 1;
            psectfound:;
                str = p;
                break;
            }

            case '?': {
                char *p = endofsymbol (++ str);
                int len = p - str;
                Symbol *symbol;
                for (symbol = allsymbols; symbol != NULL; symbol = symbol->next) {
                    if ((memcmp (symbol->name, str, len) == 0) && (symbol->name[len] == 0)) {
                        valtemp += getsymval (symbol);
                        if (symbol->defobjfile->ifrefd) {
                            symbol->defobjfile->ifrefd = 0;
                            repeatpass2 = 1;
                            printf ("symbol %s referenced by %s causes %s to be included\n",
                                    symbol->name, objfile->name, symbol->defobjfile->name);
                        }
                        goto symbolfound;
                    }
                }
                fprintf (stderr, "symbol '%.*s' used for value in '%s' not defined\n", len, str, objfile->name);
                error = 1;
            symbolfound:;
                str = p;
                break;
            }

            case '0'...'7': {
                valtemp += strtoul (str, &str, 8);
                break;
            }

            default: abort ();
        }
    }

    // parsing complete, merge value into opcode if possible
    assert ((*value_r & 0377) == 0);
    if (valtemp < 0200) {
        *value_r |= valtemp;
    } else if (((valtemp ^ emitaddr) & -0200) == 0) {
        *value_r |= (valtemp & 0177) | 0200;
    } else {
        fprintf (stderr, "mem ref opcode %04o at %05o cannot reach address %05o\n", *value_r, emitaddr, valtemp);
        error = 1;
    }

    return str;
}

static char *endofsymbol (char *sym)
{
    char c = *sym;
    if ((c >= '0') && (c <= '9')) return sym;
    while (1) {
        if ((c == ':') && (sym[1] == ':')) { ++ sym; goto ok; }
        if ((c >= 'a') && (c <= 'z')) goto ok;
        if ((c >= 'A') && (c <= 'Z')) goto ok;
        if ((c >= '0') && (c <= '9')) goto ok;
        if ((c != '_') && (c != '$') && (c != '.')) return sym;
    ok:;
        c = *(++ sym);
    }
}

static int sortpsect (void const *a, void const *b)
{
    PSect const *s = *(PSect **) a;
    PSect const *t = *(PSect **) b;
    return strcmp (s->name, t->name);
}

static int sortsymbyname (void const *a, void const *b)
{
    Symbol const *s = *(Symbol **) a;
    Symbol const *t = *(Symbol **) b;
    return strcmp (s->name, t->name);
}

static int sortsymbyaddr (void const *a, void const *b)
{
    Symbol const *s = *(Symbol **) a;
    Symbol const *t = *(Symbol **) b;
    uint16_t v = getsymval (s);
    uint16_t w = getsymval (t);
    if (v < w) return -1;
    if (v > w) return  1;
    return strcmp (s->name, t->name);
}

static uint16_t getsymval (Symbol const *symbol)
{
    uint16_t v = symbol->value;
    if (symbol->objpsect != NULL) v += symbol->objpsect->base + symbol->objpsect->allpsect->base;
    return v;
}
