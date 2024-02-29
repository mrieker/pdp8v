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

// ./assemble [-pal] asmfile objfile > lisfile

#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "pflags.h"

#define TT_ENDLINE 0
#define TT_INTEGER 1
#define TT_SYMBOL 2
#define TT_DELIM 3
#define TT_STRING 4
#define TT_REGISTER 5

#define ST_NORMAL 0
#define ST_PMEMOP 1
#define ST_LDIR 2
#define ST_LBETA 3
#define ST_LALPHAR 4
#define ST_LALPHAN 5
#define ST_LALPHAF 6
#define ST_LSKIP 7
#define ST_LMISC 8

typedef struct PSect PSect;
typedef struct Stack Stack;
typedef struct StrCtx StrCtx;
typedef struct Symbol Symbol;
typedef struct Symref Symref;
typedef struct Token Token;

struct PSect {
    PSect *next;            // next in psects list
    uint16_t pflg;          // flags
    uint16_t p2al;          // log2 power of alignment
    uint16_t offs;          // offset for next object code generation
    uint16_t size;          // total size of the section
    char name[1];           // null terminated psect name
};

struct Stack {
    Stack *next;            // next stack entry
    PSect *psect;           // value relocation
    Symbol *undsym;         // undefined symbol
    Token *opctok;          // operator token
    uint32_t value;         // value
    char preced;            // precedence
};

struct StrCtx {
    char *p;
    uint16_t bpc;
    uint16_t state;
    uint16_t word0;
    uint16_t word1;
};

struct Symbol {
    Symbol *next;           // next in symbols list
    PSect *psect;           // value relocation
    int (*namcmp) (char const *a, char const *b);
    Symref *refs;           // references
    int nrefs;              // number of elements in refs
    uint16_t value;         // value
    char defind;            // set if defined
    char global;            // set if global
    char hidden;            // set to hide from symbol table listing
    char symtyp;            // symbol type ST_*
    char name[1];           // null terminated symbol name
};

struct Symref {
    Symref *next;           // next in symbol->refs list
    char const *name;       // referencing filename
    int line;               // referencing linenumber
    char type;              // reference type (' ', ':', '=')
};

struct Token {
    Token *next;            // next token in line
    char const *srcname;    // source file name
    int srcline;            // one-based line in source file
    int srcchar;            // zero-based character in source line
    uint32_t intval;        // integer value
    char toktype;           // TT_ token type
    char strval[1];         // string value (null terminated)
};

static char const *const pmemops[] = { "AND",  "TAD",  "ISZ",  "DCA",  "JMS",  "JMP"  };
static char const *const pindops[] = { "ANDI", "TADI", "ISZI", "DCAI", "JMSI", "JMPI" };

static struct { char symtyp; uint16_t opc; char mne[4]; } const lincops[] = {
    { ST_LMISC,   00000, "HLT" },
    { ST_LMISC,   00002, "PDP" },
    { ST_LMISC,   00004, "ESF" },
    { ST_LMISC,   00005, "QAC" },
    { ST_LMISC,   00006, "DJR" },
    { ST_LMISC,   00011, "CLR" },
    { ST_LMISC,   00017, "COM" },
    { ST_LMISC,   00016, "NOP" },
    { ST_LMISC,   00024, "SFA" },
    { ST_LALPHAR, 00040, "SET" },
    { ST_LALPHAR, 00140, "DIS" },
    { ST_LALPHAR, 00200, "XSK" },
    { ST_LALPHAN, 00240, "ROL" },
    { ST_LALPHAN, 00300, "ROR" },
    { ST_LALPHAN, 00340, "SCR" },
    { ST_LSKIP,   00415, "KST" },
    { ST_LSKIP,   00451, "APO" },
    { ST_LSKIP,   00450, "AZE" },
    { ST_LSKIP,   00452, "LZE" },
    { ST_LSKIP,   00453, "IBZ" },
    { ST_LSKIP,   00455, "QLZ" },
    { ST_LSKIP,   00456, "SKP" },
    { ST_LMISC,   00500, "IOB" },
    { ST_LMISC,   00516, "RSW" },
    { ST_LMISC,   00517, "LSW" },
    { ST_LALPHAF, 00600, "LIF" },
    { ST_LALPHAF, 00640, "LDF" },
    { ST_LBETA,   01000, "LDA" },
    { ST_LBETA,   01040, "STA" },
    { ST_LBETA,   01100, "ADA" },
    { ST_LBETA,   01140, "ADM" },
    { ST_LBETA,   01200, "LAM" },
    { ST_LBETA,   01240, "MUL" },
    { ST_LBETA,   01300, "LDH" },
    { ST_LBETA,   01340, "STH" },
    { ST_LBETA,   01400, "SHD" },
    { ST_LBETA,   01440, "SAE" },
    { ST_LBETA,   01500, "SRO" },
    { ST_LBETA,   01540, "BCL" },
    { ST_LBETA,   01600, "BSE" },
    { ST_LBETA,   01640, "BCO" },
    { ST_LBETA,   01740, "DSC" },
    { ST_LDIR,    02000, "ADD" },
    { ST_LDIR,    04000, "STC" },
    { ST_LDIR,    06000, "JMP" }
};

static char const precedence[] = "*/% +-  &   ^   |   ";

static char comch, error, lblch, lincmode;
static char lisaddr[6], lisdata[9];
static FILE *octfile;
static int (*symstrcmp) (char const *a, char const *b);
static int emitidx, passno;
static int ifblock, iflevel;
static int palmode;
static int stdoutdifferr;
static PSect *psects;
static Symbol *dotsymbol, *lincsyms, *pdpsyms, *symbols;
static uint16_t emitbuf[32];

static Symbol *defopsym (Symbol *lastsym, char symtyp, uint16_t opc, char const *mne);
static void doasmpass (char const *asmname);
static Token *tokenize (char const *srcname, int srcline, char const *asmline);
static void processline (Token *token, int lineno, char const *asmline);
static uint16_t decodeopcode (Token *token, int absonly);
static void processasciidirective (Token *token, int nulonend);
static Symbol *definesymbol (Token *token, char const *name, PSect *psect, uint16_t value);
static Token *decodexpr (Token *token, PSect **psect_r, uint32_t *value_r, Symbol **undsym_r);
static Token *decodeval (Token *token, PSect **psect_r, uint32_t *value_r, Symbol **undsym_r);
static Stack *dostackop (Stack *stackb);
static void emitchar (StrCtx *strctx, uint8_t ch);
static void emitlong (uint32_t w);
static void emitword (uint16_t w);
static void emitwordrel (uint16_t w, PSect *psect, Symbol *undsym);
static void emitwordmem (uint16_t w, PSect *psect, uint32_t value, Symbol *undsym);
static void emitflush ();
static void setdotsymval (uint16_t val);
static char *filloct (int len, char *buf, uint32_t val);
static void errortok (Token *tok, char const *fmt, ...);
static void errorloc (char const *srcname, int srcline, int srcchar, char const *fmt, ...);
static uint16_t decomicro (Token *token);
static char const *microconflict (uint64_t already, uint64_t adding);
static void addsymref (Symbol *symbol, Token *token, char type);
static void symboltable ();
static int sortsyms (void const *v1, void const *v2);

int main (int argc, char **argv)
{
    char const *asmname, *octname;
    char stderrlink[64], stdoutlink[64];
    int i;
    PSect *psect;
    Symbol *symbol;

    setlinebuf (stdout);
    setlinebuf (stderr);

    comch = ';';
    lblch = ':';
    asmname = NULL;
    octname = NULL;
    symstrcmp = strcmp;
    for (i = 0; ++ i < argc;) {
        if (strcasecmp (argv[i], "-pal") == 0) {
            comch = '/';
            lblch = ',';
            palmode = 1;
            symstrcmp = strcasecmp;
            continue;
        }
        if (argv[i][0] == '-') goto usage;
        if (asmname == NULL) {
            asmname = argv[i];
            continue;
        }
        if (octname == NULL) {
            octname = argv[i];
            continue;
        }
        goto usage;
    }
    if (octname == NULL) goto usage;

    for (i = 0; i < sizeof pmemops / sizeof pmemops[0]; i ++) {
        pdpsyms = defopsym (pdpsyms, ST_PMEMOP, i * 01000, pmemops[i]);
        if (! palmode) pdpsyms = defopsym (pdpsyms, ST_PMEMOP, i * 01000 + 00400, pindops[i]);
    }

    for (i = 0; i < sizeof lincops / sizeof lincops[0]; i ++) {
        lincsyms = defopsym (lincsyms, lincops[i].symtyp, lincops[i].opc, lincops[i].mne);
    }

    // see if stdout/stderr different
    i = readlink ("/proc/self/fd/1", stdoutlink, sizeof stdoutlink - 1);
    if (i >= 0) {
        stdoutlink[i] = 0;
        i = readlink ("/proc/self/fd/2", stderrlink, sizeof stderrlink - 1);
        if (i >= 0) {
            stderrlink[i] = 0;
            stdoutdifferr = strcmp (stdoutlink, stderrlink);
        }
    }

    dotsymbol = malloc (1 + sizeof *dotsymbol);
    dotsymbol->next   = pdpsyms;
    dotsymbol->psect  = NULL;
    dotsymbol->namcmp = strcmp;
    dotsymbol->refs   = NULL;
    dotsymbol->nrefs  = 0;
    dotsymbol->value  = 0;          // invariant: dotsymbol->value == dotsymbol->psect->offs
    dotsymbol->defind = 1;
    dotsymbol->global = 0;
    dotsymbol->hidden = 1;
    dotsymbol->symtyp = ST_NORMAL;
    strcpy (dotsymbol->name, ".");
    symbols = dotsymbol;

    passno = 1;
    doasmpass (asmname);

    lincmode = 0;
    dotsymbol->next  = pdpsyms;
    dotsymbol->psect = NULL;
    setdotsymval (0);
    passno = 2;

    octfile = fopen (octname, "w");
    if (octfile == NULL) {
        fprintf (stderr, "error creating %s: %m\n", octname);
        return 1;
    }
    for (psect = psects; psect != NULL; psect = psect->next) {
        fprintf (octfile, ">%s=%04o;pflg=%u;p2al=%u\n", psect->name, psect->size, psect->pflg, psect->p2al);
    }
    for (symbol = symbols; symbol != NULL; symbol = symbol->next) {
        if (symbol->global && symbol->defind) {
            fprintf (octfile, "?%s=%04o", symbol->name, symbol->value);
            if (symbol->psect != NULL) fprintf (octfile, "+>%s", symbol->psect->name);
            fprintf (octfile, "\n");
        }
    }

    for (psect = psects; psect != NULL; psect = psect->next) {
        psect->offs = 0;
    }

    doasmpass (asmname);
    fclose (octfile);
    if (error) unlink (octname);

    symboltable ();

    return error;

usage:;
    fprintf (stderr, "usage: assemble <asmfile> <octfile>\n");
    return 1;
}

static Symbol *defopsym (Symbol *lastsym, char symtyp, uint16_t opc, char const *mne)
{
    Symbol *symbol = malloc (strlen (mne) + sizeof *symbol);
    symbol->next   = lastsym;
    symbol->psect  = NULL;
    symbol->namcmp = strcasecmp;
    symbol->refs   = NULL;
    symbol->nrefs  = 0;
    symbol->value  = opc;
    symbol->defind = 1;
    symbol->global = 0;
    symbol->hidden = 1;
    symbol->symtyp = symtyp;
    strcpy (symbol->name, mne);
    return symbol;
}

static void doasmpass (char const *asmname)
{
    char asmline[256];
    FILE *asmfile;
    int lineno;
    Token *token, *tokens;

    asmfile = fopen (asmname, "r");
    if (asmfile == NULL) {
        fprintf (stderr, "error opening %s: %m\n", asmname);
        return;
    }

    lineno = 0;
    while (fgets (asmline, sizeof asmline, asmfile) != NULL) {
        int linelen = strlen (asmline);
        if ((linelen == 0) || (asmline[linelen-1] != '\n')) {
            if (linelen > sizeof asmline - 5) {
                linelen = sizeof asmline - 5;
            }
            strcpy (asmline + linelen, "???\n");
            int c;
            while ((c = fgetc (asmfile)) >= 0) {
                if (c == '\n') break;
            }
        }
        ++ lineno;
        memset (lisaddr, ' ', 5);
        memset (lisdata, ' ', 8);
        tokens = tokenize (asmname, lineno, asmline);
        processline (tokens, lineno, asmline);
        if (passno == 1) {
            while ((token = tokens) != NULL) {
                tokens = token->next;
                free (token);
            }
        }
        if (passno == 2) {
            printf ("%s  %s  %6d:%s", lisaddr, lisdata, lineno, asmline);
        }
    }

    fclose (asmfile);
    emitflush ();
}

static Token *tokenize (char const *srcname, int srcline, char const *asmline)
{
    char const *q;
    char c, d, *p;
    int i, j;
    Token **ltoken, *token, *tokens;
    uint32_t v;

    q = asmline;
    ltoken = &tokens;
    while ((c = *q) != 0) {

        // skip whitespace
        if (c <= ' ') {
            ++ q;
            continue;
        }

        // stop if comment
        if (c == comch) break;

        // numeric constant
        if ((c >= '0') && (c <= '9')) {
            v = strtoul (q, &p, 0);
            if (palmode) {
                if (*p == '.') p ++;
                else {
                    v = strtoul (q, &p, 8);
                    if ((*p == '8') || (*p == '9')) {
                        errorloc (srcname, srcline, p - asmline, "use . at end of decimal constant");
                        v = strtoul (q, &p, 0);
                    }
                }
            }
            i = (char const *) p - q;
            token = malloc (i + sizeof *token);
            token->srcname = srcname;
            token->srcline = srcline;
            token->srcchar = q - asmline;
            token->toktype = TT_INTEGER;
            token->intval = v;
            memcpy (token->strval, q, i);
            token->strval[i] = 0;
            q = p;
            *ltoken = token;
            ltoken = &token->next;
            continue;
        }

        // symbol
        if (((c >= 'a') && (c <= 'z')) || ((c >= 'A') && (c <= 'Z')) || (c == '_') || (c == '$') || (c == '.')) {
            for (i = 0;; i ++) {
                c = q[i];
                if ((c >= 'a') && (c <= 'z')) continue;
                if ((c >= 'A') && (c <= 'Z')) continue;
                if ((c >= '0') && (c <= '9')) continue;
                if (c == '_') continue;
                if (c == '$') continue;
                if (c == '.') continue;
                if ((c == ':') && (q[i+1] == ':')) {
                    i ++;
                    continue;
                }
                break;
            }
            token = malloc (i + sizeof *token);
            token->srcname = srcname;
            token->srcline = srcline;
            token->srcchar = q - asmline;
            token->toktype = TT_SYMBOL;
            token->intval = i;
            memcpy (token->strval, q, i);
            token->strval[i] = 0;
            q += i;
            *ltoken = token;
            ltoken = &token->next;
            continue;
        }

        // delimiter
        if (strchr ("@+-&|^~*/%=():#,", c) != NULL) {
            token = malloc (1 + sizeof *token);
            token->srcname = srcname;
            token->srcline = srcline;
            token->srcchar = q - asmline;
            token->toktype = TT_DELIM;
            token->intval = c & 0377;
            token->strval[0] = c;
            token->strval[1] = 0;
            q ++;
            *ltoken = token;
            ltoken = &token->next;
            continue;
        }

        // quoted string
        //  ": null terminated string
        //  ': integer constant
        if ((c == '"') || (c == '\'')) {

            // count string length (might go over a little)
            d = c;
            for (i = 0;;) {
                c = q[++i];
                if (c == 0) break;
                if (c == d) break;
                if ((c == '\\') && (q[++i] == 0)) break;
            }

            // allocate token to take whole string
            token = malloc (i + 1 + sizeof *token);
            token->srcname = srcname;
            token->srcline = srcline;
            token->srcchar = q - asmline;
            token->toktype = (d == '"') ? TT_STRING : TT_INTEGER;

            // fill in string characters
            for (i = 0;;) {
                c = *(++ q);
                if (c == 0) break;
                if (c == d) {
                    ++ q;
                    break;
                }
                if (c == '\\') {
                    c = *(++ q);
                    switch (c) {
                        case 'b': c = '\b'; break;
                        case 'n': c = '\n'; break;
                        case 'r': c = '\r'; break;
                        case 't': c = '\t'; break;
                        case 'z': c =    0; break;
                        case '0' ... '7': {
                            c -= '0';
                            j = 1;
                            do {
                                d = *(++ q);
                                if ((d < '0') || (d > '7')) {
                                    -- q;
                                    break;
                                }
                                c = (c << 3) + d - '0';
                            } while (++ j < 3);
                            break;
                        }
                    }
                }
                token->strval[i++] = c;
            }
            token->strval[i] = 0;

            // intval for TT_INTEGER is first character, null extended
            // intval for TT_STRING is string length
            if (token->toktype == TT_STRING) {
                token->intval = i;
            } else {
                if (i > 1) errorloc (srcname, srcline, q - asmline, "string too long for integer constant");
                token->strval[i+1] = 0;
                token->intval = token->strval[0] & 0377;
            }

            // link on end of tokens
            *ltoken = token;
            ltoken = &token->next;
            continue;
        }

        // who knows
        errorloc (srcname, srcline, q - asmline, "unknown char %c", c);
        break;
    }

    token = malloc (sizeof *token);
    token->srcname = srcname;
    token->srcline = srcline;
    token->srcchar = q - asmline;
    token->next = NULL;
    token->toktype = TT_ENDLINE;
    token->intval = 0;
    token->strval[0] = 0;
    *ltoken = token;

    return tokens;
}

static void processline (Token *token, int lineno, char const *asmline)
{
    char const *mne;
    char memop, *p;
    PSect *exprpsect, **lpsect, *psect;
    Symbol *exprundsym, *symbol;
    Token *tok;
    uint16_t word;
    uint32_t exprvalue;

    // process all <symbol>: at beginning of line
    while ((token->toktype == TT_SYMBOL) && (token->next->toktype == TT_DELIM) && (token->next->intval == lblch)) {
        if (ifblock > 0) return;
        symbol = definesymbol (token, token->strval, dotsymbol->psect, dotsymbol->value);
        addsymref (symbol, token, lblch);
        filloct (5, lisaddr, dotsymbol->value);
        token = token->next->next;
    }

    // skip otherwise blank lines
    if (token->toktype == TT_ENDLINE) return;

    // check for <symbol> = <expression>
    if ((token->toktype == TT_SYMBOL) && (token->next->toktype == TT_DELIM) && (token->next->intval == '=')) {
        if (ifblock > 0) return;
        memop = 0;
        if (token->next->next->toktype == TT_SYMBOL) {
            // can also have <symbol> = .memop <expression>
            if (strcasecmp (token->next->next->strval, ".memop") == 0) {
                tok = decodexpr (token->next->next->next, &exprpsect, &exprvalue, &exprundsym);
                if (tok->toktype != TT_ENDLINE) errortok (tok, "extra at end of expression %s", tok->strval);
                else if ((exprpsect != NULL) || (exprundsym != NULL)) errortok (tok, "cannot define opcode symbol off relocatable or undefined symbol");
                else if (exprvalue & ~07000) errortok (tok, "memory opcode 0%04o not a multiple of 01000", exprvalue);
                exprvalue &= 07000;
                memop = 1;
                goto defsym;
            }
            // can also have <symbol> = <instruction>
            exprpsect = NULL;
            exprvalue = decodeopcode (token->next->next, 1);
            if (! (exprvalue & 0x8000)) goto defsym;
        }
        tok = decodexpr (token->next->next, &exprpsect, &exprvalue, &exprundsym);
        if (exprundsym != NULL) errortok (tok, "cannot define one symbol off another undefined symbol %s", exprundsym->name);
        if (tok->toktype != TT_ENDLINE) errortok (tok, "extra at end of expression %s", tok->strval);
    defsym:;
        symbol = definesymbol (token, token->strval, exprpsect, (uint16_t) exprvalue);
        symbol->symtyp = memop ? ST_PMEMOP : ST_NORMAL;
        addsymref (symbol, token, '=');
        filloct (4, lisdata, exprvalue);
        return;
    }

    // palmode can have * <expression> meaning . = <expression>
    if (palmode && (token->toktype == TT_DELIM) && (token->intval == '*')) {
        if (ifblock > 0) return;
        tok = decodexpr (token->next, &exprpsect, &exprvalue, &exprundsym);
        if (exprundsym != NULL) errortok (tok, "cannot define location off undefined symbol %s", exprundsym->name);
        definesymbol (token, ".", exprpsect, (uint16_t) exprvalue);
        filloct (4, lisdata, exprvalue);
        if (tok->toktype != TT_ENDLINE) errortok (tok, "extra at end of expression %s", tok->strval);
        return;
    }

    // should have an opcode or directive
    if (token->toktype != TT_SYMBOL) {
        if (! palmode) {
            errortok (token, "expecting directive/opcode at %s", token->strval);
            return;
        }

        if (ifblock > 0) return;

        // palmode allows implicit .word directive, ie, an expression
        token = decodexpr (token, &exprpsect, &exprvalue, &exprundsym);
        if (token->toktype != TT_ENDLINE) {
            errortok (token, "expecting end-of-line");
        }
        word = exprvalue;
        filloct (5, lisaddr, dotsymbol->value);
        filloct (4, lisdata, word);
        emitwordrel (word, exprpsect, exprundsym);
        goto done2;
    }

    mne = token->strval;

    // if blocked by an .if, just look for .ifs, .elses, and .endifs
    if (ifblock > 0) {
        if (strcasecmp (mne, ".else") == 0) {
            if (iflevel == ifblock) ifblock = 0;
        }
        if (strcasecmp (mne, ".endif") == 0) {
            if (-- iflevel < ifblock) ifblock = 0;
        }
        if (strcasecmp (mne, ".if") == 0) {
            ++ iflevel;
        }
        return;
    }

    // check out directives
    if ((mne[0] == '.') && ((mne[1] | 0x20) >= 'a') && ((mne[1] | 0x20) <= 'z')) {

        if (strcasecmp (mne, ".align") == 0) {
            emitflush ();
            token = decodexpr (token->next, &exprpsect, &exprvalue, &exprundsym);
            if (token->toktype != TT_ENDLINE) {
                errortok (token, "expecting end of line");
            }
            if ((exprpsect != NULL) || (exprundsym != NULL)) errortok (token, "alignment must be absolute");
            if ((exprvalue == 0) || ((exprvalue & - exprvalue) != exprvalue)) {
                errortok (token, "alignment 0%o must be a power of 2", exprvalue);
                exprvalue = 1;
            }
            uint16_t p2 = 0;
            while (!(exprvalue & 1)) {
                p2 ++;
                exprvalue >>= 1;
            }
            if ((dotsymbol->psect != NULL) && (dotsymbol->psect->p2al < p2)) {
                dotsymbol->psect->p2al = p2;
            }
            exprvalue = (1 << p2) - 1;
            emitflush ();
            setdotsymval ((uint16_t) ((dotsymbol->value + exprvalue) & ~ exprvalue));
            return;
        }

        if (strcasecmp (mne, ".ascii") == 0) {
            processasciidirective (token, 0);
            return;
        }

        if (strcasecmp (mne, ".asciz") == 0) {
            processasciidirective (token, 1);
            return;
        }

        if (strcasecmp (mne, ".blkw") == 0) {
            emitflush ();
            filloct (5, lisaddr, dotsymbol->value);
            token = token->next;
            token = decodexpr (token, &exprpsect, &exprvalue, &exprundsym);
            if (token->toktype != TT_ENDLINE) {
                errortok (token, "expecting end of line");
            }
            if ((exprpsect != NULL) || (exprundsym != NULL)) errortok (token, "block sizes must be absolute");
            filloct (4, lisdata, exprvalue);
            emitflush ();
            setdotsymval ((uint16_t) (dotsymbol->value + exprvalue));
            return;
        }

        if (strcasecmp (mne, ".else") == 0) {
            if (iflevel <= 0) {
                errortok (token, ".else without corresponding .if");
            }
            ifblock = iflevel;
            return;
        }

        if (strcasecmp (mne, ".end") == 0) {
            return;
        }

        if (strcasecmp (mne, ".endif") == 0) {
            if (iflevel <= 0) {
                errortok (token, ".endif without corresponding .if");
            } else {
                -- iflevel;
            }
            return;
        }

        if (strcasecmp (mne, ".extern") == 0) {
            return;
        }

        if (strcasecmp (mne, ".global") == 0) {
            Symbol *symbol;
            while ((token = token->next)->toktype != TT_ENDLINE) {
                if (token->toktype != TT_SYMBOL) {
                    errortok (token, "expecting global symbol name");
                }
                for (symbol = symbols; symbol != NULL; symbol = symbol->next) {
                    if (symbol->namcmp (symbol->name, token->strval) == 0) goto gotgblsym;
                }
                symbol = malloc (strlen (token->strval) + sizeof *symbol);
                symbol->next   = symbols;
                symbol->psect  = NULL;
                symbol->namcmp = symstrcmp;
                symbol->refs   = NULL;
                symbol->nrefs  = 0;
                symbol->value  = 0;
                symbol->defind = 0;
                symbol->hidden = 0;
                symbol->symtyp = ST_NORMAL;
                strcpy (symbol->name, token->strval);
                symbols = symbol;
            gotgblsym:;
                symbol->global = 1;
                addsymref (symbol, token, 'g');
                token = token->next;
                if (token->toktype == TT_ENDLINE) return;
                if ((token->toktype != TT_DELIM) || (token->intval != ',')) {
                    errortok (token, "expecting comma between global symbols");
                    return;
                }
            }
            return;
        }

        if (strcasecmp (mne, ".if") == 0) {
            token = token->next;
            token = decodexpr (token, &exprpsect, &exprvalue, &exprundsym);
            if ((exprpsect != NULL) || (exprundsym != NULL)) errortok (token, ".if values must be absolute");
            ++ iflevel;
            if (exprvalue == 0) ifblock = iflevel;
            return;
        }

        if (strcasecmp (mne, ".include") == 0) {
            token = token->next;
            if ((token->toktype != TT_STRING) || (token->next->toktype != TT_ENDLINE)) {
                errortok (token, "expecting filename string then end of line for .include");
                return;
            }
            if (passno == 2) {
                strcpy (lisdata + sizeof lisdata - 6, "enter");
                printf ("%s  %s  %6d:%s", lisaddr, lisdata, lineno, asmline);
            }
            doasmpass (token->strval);
            if (passno == 2) {
                strcpy (lisdata + sizeof lisdata - 6, "leave");
            }
            return;
        }

        if (strcasecmp (mne, ".linc") == 0) {
            dotsymbol->next = lincsyms;
            lincmode = 1;
            return;
        }

        if (strcasecmp (mne, ".long") == 0) {
            filloct (5, lisaddr, dotsymbol->value);
            p = lisdata;
            while ((token = token->next)->toktype != TT_ENDLINE) {
                token = decodexpr (token, &exprpsect, &exprvalue, &exprundsym);
                if ((exprpsect != NULL) || (exprundsym != NULL)) errortok (token, "long values must be absolute");
                emitlong (exprvalue);
                if (p <= lisdata) p = filloct (8, p, exprvalue);
                if (token->toktype == TT_ENDLINE) return;
                if ((token->toktype != TT_DELIM) || (token->intval != ',')) {
                    errortok (token, "expecting comma between long values");
                    return;
                }
            }
        }

        if (strcasecmp (mne, ".p2align") == 0) {
            emitflush ();
            token = decodexpr (token->next, &exprpsect, &exprvalue, &exprundsym);
            if (token->toktype != TT_ENDLINE) {
                errortok (token, "expecting end of line");
            }
            if ((exprpsect != NULL) || (exprundsym != NULL)) errortok (token, "alignment must be absolute");
            if (exprvalue > 12) {
                errortok (token, "alignment %u must be le 12", exprvalue);
                exprvalue = 0;
            }
            if ((dotsymbol->psect != NULL) && (dotsymbol->psect->p2al < exprvalue)) {
                dotsymbol->psect->p2al = (uint16_t) exprvalue;
            }
            exprvalue = (1 << exprvalue) - 1;
            emitflush ();
            setdotsymval ((uint16_t) ((dotsymbol->value + exprvalue) & ~ exprvalue));
            return;
        }

        if (strcasecmp (mne, ".pdp") == 0) {
            dotsymbol->next = pdpsyms;
            lincmode = 0;
            return;
        }

        if (strcasecmp (mne, ".psect") == 0) {
            token = token->next;
            if (token->toktype != TT_SYMBOL) {
                errortok (token, ".psect must be followed by psect name");
                return;
            }
            for (lpsect = &psects; (psect = *lpsect) != NULL; lpsect = &psect->next) {
                if (strcmp (psect->name, token->strval) == 0) goto gotpsect;
            }
            psect = malloc ((uint16_t) token->intval + sizeof *psect);
            psect->next = NULL;
            psect->pflg = 0;
            psect->p2al = 0;
            psect->offs = 0;
            psect->size = 0;
            strcpy (psect->name, token->strval);
            *lpsect = psect;
            while (((token = token->next)->toktype == TT_DELIM) && (token->intval == ',')) {
                token = token->next;
                if ((token->toktype != TT_SYMBOL) || (strcasecmp (token->strval, "ovr") != 0)) break;
                psect->pflg |= PF_OVR;
            }
            if (token->toktype != TT_ENDLINE) {
                errortok (token, "invalid .psect option");
            }
        gotpsect:;
            emitflush ();
            dotsymbol->psect = psect;
            dotsymbol->value = psect->offs;
            return;
        }

        if (strcasecmp (mne, ".word") == 0) {
            filloct (5, lisaddr, dotsymbol->value);
            p = lisdata;
            while ((token = token->next)->toktype != TT_ENDLINE) {
                token = decodexpr (token, &exprpsect, &exprvalue, &exprundsym);
                emitwordrel ((uint16_t) exprvalue, exprpsect, exprundsym);
                if (p <= lisdata + 4) p = filloct (4, p, exprvalue);
                if (token->toktype == TT_ENDLINE) return;
                if ((token->toktype != TT_DELIM) || (token->intval != ',')) {
                    errortok (token, "expecting comma between word values (have toktype %d, intval %d)", (int) token->toktype, (int) token->intval);
                    return;
                }
            }
        }

        errortok (token, "unknown directive %s", mne);
        return;
    }

    // check out opcodes
    filloct (5, lisaddr, dotsymbol->value);

    word = decodeopcode (token, 0);
    if (word == 0xFFFF) {

        // in palmode, can be an expression
        if (palmode) {
            token = decodexpr (token, &exprpsect, &exprvalue, &exprundsym);
            if (token->toktype != TT_ENDLINE) {
                errortok (token, "unknown opcode or malformed expression");
            }
            word = exprvalue;
            emitwordrel (word, exprpsect, exprundsym);
            goto done2;
        }

        // who knows
        errortok (token, "unknown opcode %s", mne);
        word = 07000;   // nop
        goto done;
    }

    if ((word & 0xC000) == 0x8000) {
        word &= 07777;
        goto done2;
    }
    word &= 07777;

    // output opcode to object file
done:;
    emitword (word);
done2:;
    filloct (4, lisdata, word);
}

// decode instruction and operand if any
//  input:
//   token = points to opcode token, assumed to be TT_SYMBOL
//   absonly = 0: decode anything, possibly emitting
//             1: only absolute instructions (no relocatables), do not emit, just return value
//  output:
//   returns
//    0xFFFF = unknown opcode
//    absonly = 0:
//     <15:14> = 10: value has already been emitted, just put <11:00> in listing
//             else: emit value <11:00> and put in listing
//    absonly = 1:
//        <15> = 1: not an opcode, nothing decoded
//            else: opcode decoded, use the returned value <11:00>
static uint16_t decodeopcode (Token *token, int absonly)
{
    char const *mne;
    PSect *exprpsect;
    Symbol *exprundsym, *symbol;
    uint16_t word;
    uint32_t exprvalue;

    mne = token->strval;

    // see if opcode symbol
    for (symbol = symbols; symbol != NULL; symbol = symbol->next) {
        if (symbol->namcmp (mne, symbol->name) == 0) break;
    }
    if (symbol != NULL) {
        switch (symbol->symtyp) {

            // just a normal symbol, no opcode processing
            case ST_NORMAL: break;

            // PDP-8 memory opcode (DCA, JMP, etc)
            case ST_PMEMOP: {
                addsymref (symbol, token, 'm');
                word  = symbol->value;
                token = token->next;

                // check for indirect flag ('I')
                if (palmode && (token->toktype == TT_SYMBOL) && (strcasecmp (token->strval, "I") == 0)) {
                    word |= 0400;
                    token = token->next;
                }

                // decode operand address expression
                token = decodexpr (token, &exprpsect, &exprvalue, &exprundsym);
                if (token->toktype != TT_ENDLINE) {
                    errortok (token, "expecting end of line");
                }

                if (exprundsym == NULL) {

                    // see if in first 128 words of same frame as instruction
                    if ((exprpsect == dotsymbol->psect) && ((exprpsect == NULL) || (exprpsect->p2al >= 12))) {
                        if ((exprvalue & 077600) == (dotsymbol->value & 070000)) {
                            word |= exprvalue & 00177;
                            return word;
                        }
                    }

                    // see if in same page as instruction
                    if ((exprpsect == dotsymbol->psect) && ((exprpsect == NULL) || (exprpsect->p2al >= 7))) {
                        if (((exprvalue ^ dotsymbol->value) & 07600) != 0) {
                            errortok (token, "operand out of range");
                        }
                        word |= 00200 | (exprvalue & 00177);
                        return word;
                    }
                }

                // if caller only wants absolutes, error out
                if (absonly) {
                    errortok (token, "operand must be absolute and on page zero of the frame or same page");
                    word |= 00200 | (exprvalue & 00177);
                    return word;
                }

                // pass on to linker to resolve it
                emitwordmem (word, exprpsect, exprvalue, exprundsym);
                return word | 0x8000;
            }

            // link direct address instruction
            case ST_LDIR: {
                addsymref (symbol, token, 'm');
                word  = symbol->value;
                token = token->next;
                token = decodexpr (token, &exprpsect, &exprvalue, &exprundsym);
                if (token->toktype != TT_ENDLINE) {
                    errortok (token, "expecting end of line");
                }
                if ((exprpsect != dotsymbol->psect) || ((exprpsect != NULL) && (exprpsect->p2al < 10)) || (((exprvalue ^ dotsymbol->value) & 06000) != 0)) {
                    errortok (token, "operand out of range");
                }
                word |= exprvalue & 01777;
                return word;
            }

            // linc beta instruction
            case ST_LBETA: {
                addsymref (symbol, token, 'm');
                word  = symbol->value;
                token = token->next;

                // following '@' or 'I' sets bit <04>
                if (((token->toktype == TT_DELIM) && (token->intval == '@')) ||
                        (palmode && (token->toktype == TT_SYMBOL) && (strcasecmp (token->strval, "I") == 0))) {
                    word |= 020;
                    token = token->next;
                }

                // if eol, then it's the null register (0)
                // otherwise, must be a symbol in same 10-bit page 001..017
                if (token->toktype != TT_ENDLINE) {
                    token = decodexpr (token, &exprpsect, &exprvalue, &exprundsym);
                    if (token->toktype != TT_ENDLINE) {
                        errortok (token, "expecting end of line");
                    }
                    if ((exprpsect != dotsymbol->psect) || ((exprpsect != NULL) && (exprpsect->p2al < 10)) ||
                            (((exprvalue ^ dotsymbol->value) & 06000) != 0) || ((exprvalue & 01777) < 001) || ((exprvalue & 01777) > 017)) {
                        errortok (token, "operand out of range");
                    }
                    word |= exprvalue & 01777;
                }

                return word;
            }

            // linc alpha instruction with frame number 0..31
            case ST_LALPHAF: {
                addsymref (symbol, token, 'm');
                word  = symbol->value;
                token = token->next;

                // there must be a number 000..037
                token = decodexpr (token, &exprpsect, &exprvalue, &exprundsym);
                if (token->toktype != TT_ENDLINE) {
                    errortok (token, "expecting end of line");
                }
                if ((exprpsect != NULL) || (exprundsym != NULL) || (exprvalue > 037)) {
                    errortok (token, "operand out of range");
                }
                word |= exprvalue & 00017;

                return word;
            }

            // linc alpha instruction with shift number 0..15
            case ST_LALPHAN: {
                addsymref (symbol, token, 'm');
                word  = symbol->value;
                token = token->next;

                // following '@' or 'I' sets bit <04>
                if (((token->toktype == TT_DELIM) && (token->intval == '@')) ||
                        (palmode && (token->toktype == TT_SYMBOL) && (strcasecmp (token->strval, "I") == 0))) {
                    word |= 00020;
                    token = token->next;
                }

                // there must be a number 000..017
                token = decodexpr (token, &exprpsect, &exprvalue, &exprundsym);
                if (token->toktype != TT_ENDLINE) {
                    errortok (token, "expecting end of line");
                }
                if ((exprpsect != NULL) || (exprundsym != NULL) || (exprvalue > 017)) {
                    errortok (token, "operand out of range");
                }
                word |= exprvalue & 00017;

                return word;
            }

            // linc alpha instruction with optional register 1..15
            case ST_LALPHAR: {
                addsymref (symbol, token, 'm');
                word  = symbol->value;
                token = token->next;

                // following '@' or 'I' sets bit <04>
                if (((token->toktype == TT_DELIM) && (token->intval == '@')) ||
                        (palmode && (token->toktype == TT_SYMBOL) && (strcasecmp (token->strval, "I") == 0))) {
                    word |= 00020;
                    token = token->next;
                }

                // then must be a symbol in same 10-bit page 001..017
                token = decodexpr (token, &exprpsect, &exprvalue, &exprundsym);
                if (token->toktype != TT_ENDLINE) {
                    errortok (token, "expecting end of line");
                }
                if ((exprpsect != dotsymbol->psect) || ((exprpsect != NULL) && (exprpsect->p2al < 10)) ||
                        (((exprvalue ^ dotsymbol->value) & 06000) != 0) || ((exprvalue & 01777) < 001) || ((exprvalue & 01777) > 017)) {
                    errortok (token, "operand out of range");
                }
                word |= exprvalue & 00017;

                return word;
            }

            // linc skip instruction
            case ST_LSKIP: {
                addsymref (symbol, token, 'm');
                word  = symbol->value;
                token = token->next;

                // following '@' or 'I' sets bit <04>
                if (((token->toktype == TT_DELIM) && (token->intval == '@')) ||
                        (palmode && (token->toktype == TT_SYMBOL) && (strcasecmp (token->strval, "I") == 0))) {
                    word |= 00020;
                    token = token->next;
                }
                if (token->toktype != TT_ENDLINE) {
                    errortok (token, "expecting end of line");
                }

                return word;
            }

            // linc miscellaneous
            case ST_LMISC: {
                addsymref (symbol, token, 'm');
                word  = symbol->value;
                token = token->next;

                if (token->toktype != TT_ENDLINE) {
                    errortok (token, "expecting end of line");
                }

                return word;
            }

            default: abort ();
        }
    }

    if (! palmode && (strcasecmp (mne, "IOT") == 0)) {
        token = token->next;
        token = decodexpr (token, &exprpsect, &exprvalue, &exprundsym);
        if (token->toktype != TT_ENDLINE) {
            errortok (token, "expecting end of line");
        }
        if ((exprpsect != NULL) || (exprundsym != NULL) || ((exprvalue & 07000) != 06000)) {
            errortok (token, "value for IOT must be 06xxx");
        }
        word = exprvalue & 07777;
        return word;
    }

    // can be a bunch of I/O opcodes or'd together
    word = 0;
    while (1) {
        switch (token->toktype) {
            case TT_ENDLINE: {
                if (word != 0) return word;
                goto notio;
            }
            case TT_INTEGER: {
                if ((word != 0) && ((token->intval & ~7) != (word & ~7))) {
                    errortok (token, "conflicting I/O codes");
                }
                if ((token->intval & 07000) != 06000) {
                    if (word == 0) goto notio;
                    errortok (token, "not an I/O value %04o", token->intval);
                }
                word |= token->intval & 07777;
                break;
            }
            case TT_SYMBOL: {
                for (symbol = symbols; symbol != NULL; symbol = symbol->next) {
                    if (symbol->namcmp (token->strval, symbol->name) == 0) {
                        if ((symbol->psect != NULL) || ! symbol->defind || ((word != 0) && ((symbol->value & ~7) != (word & ~7)))) {
                            if (word == 0) goto notio;
                            errortok (token, "conflicting I/O codes");
                        }
                        if ((symbol->value & 07000) != 06000) {
                            if (word == 0) goto notio;
                            errortok (token, "not an I/O value %04o", symbol->value);
                        }
                        word |= symbol->value;
                        addsymref (symbol, token, 'i');
                        goto nextio;
                    }
                }
                if (word == 0) goto notio;
                errortok (token, "undefined I/O code");
                break;
            }
            default: {
                if (word == 0) goto notio;
                return 0xFFFF;
            }
        }
    nextio:;
        token = token->next;
    }
notio:;

    // better be a micro op
    if (lincmode) return 0xFFFF;
    word = decomicro (token);
    return (word != 0) ? word : 0xFFFF;
}

static void processasciidirective (Token *token, int nulonend)
{
    StrCtx strctx;
    uint16_t i;

    // default bits-per-char is 12
    // override with '@' expr, either 6, 8 or 12
    memset (&strctx, 0, sizeof strctx);
    strctx.bpc = 12;
    if ((token->next->toktype == TT_DELIM) && (token->next->intval == '@') && (token->next->next->toktype == TT_INTEGER)) {
        token = token->next->next;
        if ((token->intval != 6) && (token->intval != 8) && (token->intval != 12)) {
            errortok (token, "bits-per-char must be 6, 8 or 12");
        } else {
            strctx.bpc = token->intval;
        }
    }

    filloct (5, lisaddr, dotsymbol->value);
    strctx.p = lisdata;
    while (1) {
        token = token->next;
        switch (token->toktype) {
            case TT_STRING: {
                for (i = 0; i < token->intval; i ++) {
                    emitchar (&strctx, token->strval[i]);
                }
                break;
            }
            case TT_INTEGER: {
                if (token->intval > 0377) errortok (token, "non-byte value %u", token->intval);
                emitchar (&strctx, (uint8_t) token->intval);
                break;
            }
            case TT_ENDLINE: {
                if (nulonend) emitchar (&strctx, 0);
                if ((strctx.bpc == 8) && (strctx.state == 1)) {
                    strctx.bpc = 6;
                }
                if (strctx.state != 0) emitchar (&strctx, 0);
                return;
            }
            default: {
                errortok (token, "non-integer non-string %s", token->strval);
                return;
            }
        }
    }
}

/**
 * Define symbol, replace previous definition if exists
 */
static Symbol *definesymbol (Token *token, char const *name, PSect *psect, uint16_t value)
{
    Symbol *symbol;

    for (symbol = symbols; symbol != NULL; symbol = symbol->next) {
        if (symbol->namcmp (symbol->name, name) == 0) {
            if (symbol == dotsymbol) {
                emitflush ();
                symbol->psect = psect;
                setdotsymval (value);
            } else if (symbol->defind && ((symbol->psect != psect) || (symbol->value != value))) {
                errortok (token, "multiple definitions of symbol '%s'", name);
                errortok (token, "  old value %05o psect %s", symbol->value, (symbol->psect == NULL) ? "*abs*" : (char const *) symbol->psect->name);
                errortok (token, "  new value %05o psect %s",         value, (        psect == NULL) ? "*abs*" : (char const *)         psect->name);
            }
            symbol->psect  = psect;
            symbol->value  = value;
            symbol->defind = 1;
            return symbol;
        }
    }

    symbol = malloc (strlen (name) + sizeof *symbol);
    symbol->next   = symbols;
    symbol->psect  = psect;
    symbol->namcmp = symstrcmp;
    symbol->refs   = NULL;
    symbol->nrefs  = 0;
    symbol->value  = value;
    symbol->defind = 1;
    symbol->global = 0;
    symbol->hidden = 0;
    symbol->symtyp = ST_NORMAL;
    strcpy (symbol->name, name);
    symbols = symbol;
    return symbol;
}

/**
 * Decode expression
 *  Input:
 *   token = starting token of expression
 *  Output:
 *   returns token past last one in expression
 *   *psect_r  = relocation or NULL if absolute
 *   *value_r  = offset in psect or absolute value
 *   *undsym_r = undefined symbol or NULL if none
 */
static Token *decodexpr (Token *token, PSect **psect_r, uint32_t *value_r, Symbol **undsym_r)
{
    char const *p;
    char preced;
    Stack *stack, *stackent;

    // get first value in expression
    stack = malloc (sizeof *stack);
    stack->next   = NULL;
    stack->preced = 0;
    stack->opctok = NULL;
    token = decodeval (token, &stack->psect, &stack->value, &stack->undsym);

    while ((token->toktype == TT_DELIM) && (token->intval <= 0377)) {
        p = strchr (precedence, (char) token->intval);
        if (p == NULL) break;
        preced = (char) (p - precedence) / 4;
        stackent = malloc (sizeof *stackent);
        stackent->next   = stack;
        stackent->preced = preced + 1;
        stackent->opctok = token;
        token = decodeval (token->next, &stackent->psect, &stackent->value, &stackent->undsym);
        stack = stackent;
        while (stack->preced <= stack->next->preced) {
            stack = dostackop (stack);
        }
    }

    while (stack->next != NULL) {
        stack = dostackop (stack);
    }

    *psect_r  = stack->psect;
    *value_r  = stack->value;
    *undsym_r = stack->undsym;
    free (stack);

    return token;
}

/**
 * Decode value from tokens
 *  Input:
 *   token = value token
 *  Output:
 *   returns token after value
 *   *psect_r = relocation or NULL if absolute
 *   *value_r = offset in psect or absolute value
 */
static Token *decodeval (Token *token, PSect **psect_r, uint32_t *value_r, Symbol **undsym_r)
{
    Symbol *symbol;

    *psect_r  = NULL;
    *value_r  = 0;
    *undsym_r = NULL;

    switch (token->toktype) {
        case TT_INTEGER: {
            *value_r = token->intval;
            break;
        }
        case TT_SYMBOL: {
            for (symbol = symbols; symbol != NULL; symbol = symbol->next) {
                if (symbol->namcmp (token->strval, symbol->name) == 0) goto gotsym;
            }
            symbol = malloc (strlen (token->strval) + sizeof *symbol);
            symbol->next   = symbols;
            symbol->psect  = NULL;
            symbol->namcmp = symstrcmp;
            symbol->refs   = NULL;
            symbol->nrefs  = 0;
            symbol->value  = 0;
            symbol->defind = 0;
            symbol->global = 0;
            symbol->hidden = 0;
            symbol->symtyp = ST_NORMAL;
            strcpy (symbol->name, token->strval);
            symbols = symbol;
            *undsym_r = symbol;
            addsymref (symbol, token, ' ');
            break;
        gotsym:;
            if (symbol->defind) {
                *psect_r  = symbol->psect;
                *value_r  = symbol->value;
            } else {
                *undsym_r = symbol;
            }
            addsymref (symbol, token, ' ');
            break;
        }
        case TT_DELIM: {
            switch (token->intval) {
                case '(': {
                    token = decodexpr (token->next, psect_r, value_r, undsym_r);
                    if ((token->toktype != TT_DELIM) || (token->intval != ')')) {
                        errortok (token, "expecting ) at end of ( expression");
                    } else {
                        token = token->next;
                    }
                    return token;
                }
                case '-': {
                    token = decodeval (token->next, psect_r, value_r, undsym_r);
                    if (*psect_r != NULL) {
                        errortok (token, "cannot negate a relocatable symbol");
                    }
                    if (*undsym_r != NULL) {
                        errortok (token, "cannot negate an undefined symbol");
                    }
                    *value_r = - *value_r;
                    return token;
                }
                case '~': {
                    token = decodeval (token->next, psect_r, value_r, undsym_r);
                    if (*psect_r != NULL) {
                        errortok (token, "cannot complement a relocatable symbol");
                    }
                    if (*undsym_r != NULL) {
                        errortok (token, "cannot complement an undefined symbol");
                    }
                    *value_r = ~ *value_r;
                    return token;
                }
            }
            // fallthrough
        }
        default: {
            errortok (token, "expecting integer, symbol or ( expr )");
            return token;
        }
    }

    return token->next;
}

/**
 * Do arithmetic on top two stack elements
 */
static Stack *dostackop (Stack *stackb)
{
    char opchr;
    Stack *stacka;
    Token *optok;

    stacka = stackb->next;
    optok  = stackb->opctok;
    opchr  = (char) optok->intval;

    switch (opchr) {
        case '|':
        case '^':
        case '&':
        case '*':
        case '/':
        case '%': {
            if ((stacka->psect != NULL) || (stackb->psect != NULL) || (stacka->undsym != NULL) || (stackb->undsym != NULL)) {
                errortok (optok, "operator %c cannot have any relocatables", opchr);
            }
            switch (opchr) {
                case '|': stacka->value |= stackb->value; break;
                case '^': stacka->value ^= stackb->value; break;
                case '&': stacka->value &= stackb->value; break;
                case '*': stacka->value *= stackb->value; break;
                case '/': {
                    if (stackb->value == 0) {
                        errortok (optok, "divide by zero");
                    } else {
                        stacka->value /= stackb->value;
                    }
                    break;
                }
                case '%': {
                    if (stackb->value == 0) {
                        errortok (optok, "divide by zero");
                    } else {
                        stacka->value %= stackb->value;
                    }
                    break;
                }
            }
            break;
        }
        case '+': {
            if (stackb->psect != NULL) {
                if (stacka->psect != NULL) {
                    errortok (optok, "operator + cannot have two relocatables");
                } else {
                    stacka->psect = stackb->psect;
                }
            }
            if (stackb->undsym != NULL) {
                if (stacka->undsym != NULL) {
                    errortok (optok, "operator + cannot have two undefineds");
                } else {
                    stacka->undsym = stackb->undsym;
                }
            }
            stacka->value += stackb->value;
            break;
        }
        case '-': {
            if (stackb->psect != NULL) {
                if (stackb->psect != stacka->psect) {
                    errortok (optok, "operator - cannot have two different relocatables");
                }
                stacka->psect = NULL;
            }
            if (stackb->undsym != NULL) {
                if (stackb->undsym != stacka->undsym) {
                    errortok (optok, "operator - cannot have two different undefineds");
                }
                stacka->undsym = NULL;
            }
            stacka->value -= stackb->value;
            break;
        }
        default: abort ();
    }
    free (stackb);
    return stacka;
}

/**
 * Output object code
 */
static void emitchar (StrCtx *strctx, uint8_t ch)
{
    switch (strctx->bpc + strctx->state) {

        // 6 bits per char; 0 buffered; buffer this char
        case 6: {
            strctx->word0 = ((uint16_t) ch << 6) & 07700;
            strctx->state = 1;
            break;
        }
        // 6 bits per char; 1 buffered; merge chars and emit
        case 7: {
            strctx->word0 |= ch & 00077;
            emitword (strctx->word0);
            if (strctx->p <= lisdata + 4) strctx->p = filloct (4, strctx->p, strctx->word0);
            strctx->state = 0;
            break;
        }

        // 8 bits per char; 0 buffered; buffer this char
        case 8: {
            strctx->word0 = ch;
            strctx->state = 1;
            break;
        }
        // 8 bits per char; 1 buffered; buffer this char
        case 9: {
            strctx->word1 = ch;
            strctx->state = 2;
            break;
        }
        // 8 bits per char; 2 buffered; merge chars and emit
        case 10: {
            strctx->word0 |= ((uint16_t) ch << 4) & 07400;
            strctx->word1 |= ((uint16_t) ch << 8) & 07400;
            emitword (strctx->word0);
            if (strctx->p <= lisdata + 4) strctx->p = filloct (4, strctx->p, strctx->word0);
            emitword (strctx->word1);
            if (strctx->p <= lisdata + 4) strctx->p = filloct (4, strctx->p, strctx->word1);
            strctx->state = 0;
            break;
        }

        // 12 bits per char, emit with zero padding
        case 12: {
            emitword (ch);
            if (strctx->p <= lisdata + 4) strctx->p = filloct (4, strctx->p, ch);
            break;
        }

        default: abort ();
    }
}

static void emitlong (uint32_t w)
{
    if (emitidx + 2 > sizeof emitbuf / sizeof emitbuf[0]) {
        emitflush ();
    }
    emitbuf[emitidx++] = w & 07777;
    emitbuf[emitidx++] = (w >> 12) & 07777;
    setdotsymval (dotsymbol->value + 2);
}

static void emitword (uint16_t w)
{
    if (emitidx + 1 > sizeof emitbuf / sizeof emitbuf[0]) {
        emitflush ();
    }
    emitbuf[emitidx++] = w & 07777;
    setdotsymval (dotsymbol->value + 1);
}

static void emitwordrel (uint16_t w, PSect *psect, Symbol *undsym)
{
    if ((psect == NULL) && (undsym == NULL)) {
        emitword (w);
    } else {
        emitflush ();
        if (passno == 2) {
            fprintf (octfile, "%04o", dotsymbol->value);
            if (dotsymbol->psect != NULL) {
                fprintf (octfile, "+>%s", dotsymbol->psect->name);
            }
            fprintf (octfile, "=(%04o", w);
            if (psect != NULL) {
                fprintf (octfile, "+>%s", psect->name);
            }
            if (undsym != NULL) {
                fprintf (octfile, "+?%s", undsym->name);
            }
            fprintf (octfile, ")\n");
        }
        setdotsymval (dotsymbol->value + 1);
    }
}

// emit memory-reference instruction with relocation
static void emitwordmem (uint16_t w, PSect *psect, uint32_t value, Symbol *undsym)
{
    emitflush ();
    if (passno == 2) {
        fprintf (octfile, "%04o", dotsymbol->value);
        if (dotsymbol->psect != NULL) {
            fprintf (octfile, "+>%s", dotsymbol->psect->name);
        }
        fprintf (octfile, "=(%04o", w);
        if (psect != NULL) {
            fprintf (octfile, "`>%s", psect->name);
        }
        if (undsym != NULL) {
            fprintf (octfile, "`?%s", undsym->name);
        }
        if (value != 0) {
            fprintf (octfile, "`%04o", value);
        }
        fprintf (octfile, ")\n");
    }
    setdotsymval (dotsymbol->value + 1);
}

static void emitflush ()
{
    int i;
    uint16_t addr;

    if ((passno == 2) && (emitidx > 0)) {
        addr = dotsymbol->value - emitidx;
        if (dotsymbol->psect == NULL) {
            fprintf (octfile, "%04o=", addr);
        } else {
            fprintf (octfile, "%04o+>%s=", addr, dotsymbol->psect->name);
        }
        for (i = 0; i < emitidx; i ++) {
            fprintf (octfile, "%04o", emitbuf[i] & 07777);
        }
        fprintf (octfile, "\n");
    }
    emitidx = 0;
}

/**
 * Set value of the "." symbol,
 * ie, address where the next object code goes
 */
static void setdotsymval (uint16_t val)
{
    dotsymbol->value = val;
    PSect *psect = dotsymbol->psect;
    if (psect != NULL) {
        psect->offs = val;
        if (psect->size < val) psect->size = val;
    }
}

/**
 * Fill buffer with octal characters
 *  Input:
 *   len = number of hex chars to output
 *   buf = at begining of string
 *   val = value to convert
 *  Output:
 *   returns pointer past end of string
 */
static char *filloct (int len, char *buf, uint32_t val)
{
    char j;
    int i;
    for (i = len * 3; (i -= 3) >= 0;) {
        j = (char) (val >> i) & 7;
        *(buf ++) = j + '0';
    }
    return buf;
}

/**
 * Print error message
 */
static void errortok (Token *tok, char const *fmt, ...)
{
    va_list ap;

    if (passno == 2) {
        fprintf (stderr, "*ERROR* %s:%d.%d:", tok->srcname, tok->srcline, tok->srcchar);
        va_start (ap, fmt);
        vfprintf (stderr, fmt, ap);
        va_end (ap);
        fprintf (stderr, "\n");
        if (stdoutdifferr) {
            fprintf (stdout, "*ERROR* %s:%d.%d:", tok->srcname, tok->srcline, tok->srcchar);
            va_start (ap, fmt);
            vfprintf (stdout, fmt, ap);
            va_end (ap);
            fprintf (stdout, "\n");
        }
        error = 1;
    }
}

static void errorloc (char const *srcname, int srcline, int srcchar, char const *fmt, ...)
{
    va_list ap;

    if (passno == 2) {
        fprintf (stderr, "*ERROR* %s:%d.%d:", srcname, srcline, srcchar);
        va_start (ap, fmt);
        vfprintf (stderr, fmt, ap);
        va_end (ap);
        fprintf (stderr, "\n");
        if (stdoutdifferr) {
            fprintf (stdout, "*ERROR* %s:%d.%d:", srcname, srcline, srcchar);
            va_start (ap, fmt);
            vfprintf (stdout, fmt, ap);
            va_end (ap);
            fprintf (stdout, "\n");
        }
        error = 1;
    }
}


#define O_ACS  0x0000000001ULL
#define O_ASR  0x0000000002ULL
#define O_BSW  0x0000000004ULL
#define O_CLA  0x0000000008ULL
#define O_CLL  0x0000000010ULL
#define O_CMA  0x0000000020ULL
#define O_CML  0x0000000040ULL
#define O_DAD  0x0000000080ULL
#define O_DCM  0x0000000100ULL
#define O_DPIC 0x0000000200ULL
#define O_DPSZ 0x0000000400ULL
#define O_DST  0x0000000800ULL
#define O_DVI  0x0000001000ULL
#define O_HLT  0x0000002000ULL
#define O_IAC  0x0000004000ULL
#define O_LSR  0x0000008000ULL
#define O_MQA  0x0000010000ULL
#define O_MQL  0x0000020000ULL
#define O_MUY  0x0000040000ULL
#define O_NMI  0x0000080000ULL
#define O_OSR  0x0000100000ULL
#define O_RAL  0x0000200000ULL
#define O_RAR  0x0000400000ULL
#define O_RTL  0x0000800000ULL
#define O_RTR  0x0001000000ULL
#define O_SAM  0x0002000000ULL
#define O_SCA  0x0004000000ULL
#define O_SCL  0x0008000000ULL
#define O_SHL  0x0010000000ULL
#define O_SKP  0x0020000000ULL
#define O_SMA  0x0040000000ULL
#define O_SNL  0x0080000000ULL
#define O_SWBA 0x0100000000ULL
#define O_SZA  0x0200000000ULL
#define O_SPA  0x0400000000ULL
#define O_SZL  0x0800000000ULL
#define O_SNA  0x1000000000ULL

#define GR1_SEQ1_ANY (O_CLA|O_CLL)
#define GR1_SEQ2_ANY (O_CMA|O_CML)
#define GR1_SEQ3     (O_IAC)
#define GR1_SEQ4_ONE (O_RAR|O_RAL|O_RTR|O_RTL|O_BSW)

#define GR1 (GR1_SEQ1_ANY|GR1_SEQ2_ANY|GR1_SEQ3|GR1_SEQ4_ONE)

#define GR2_SEQ1P_ANY (O_SMA|O_SZA|O_SNL)               // positive skips
#define GR2_SEQ1N_ANY (O_SPA|O_SNA|O_SZL)               // negative skips
#define GR2_SEQ1U     (O_SKP)                           // unconditional skip
#define GR2_SEQ2      (O_CLA)
#define GR2_SEQ3      (O_OSR)
#define GR2_SEQ4      (O_HLT)

#define GR2 (GR2_SEQ1P_ANY|GR2_SEQ1N_ANY|GR2_SEQ1U|GR2_SEQ2|GR2_SEQ3|GR2_SEQ4)

#define GR3A_SEQ1     (O_CLA)
#define GR3A_SEQ2_ANY (O_MQA|O_SCA|O_MQL)
#define GR3A_SEQ3_ONE (O_SCL|O_MUY|O_DVI|O_NMI|O_SHL|O_ASR|O_LSR)

#define GR3A (GR3A_SEQ1|GR3A_SEQ2_ANY|GR3A_SEQ3_ONE)

#define GR3B_SEQ1     (O_CLA)
#define GR3B_SEQ2_ANY (O_MQA|O_MQL)
#define GR3B_SEQ3_ONE (O_ACS|O_MUY|O_DVI|O_NMI|O_SHL|O_ASR|O_LSR|O_SCA|O_DAD|O_DST|O_SWBA|O_DPSZ|O_DPIC|O_DCM|O_SAM)

#define GR3B (GR3B_SEQ1|GR3B_SEQ2_ANY|GR3B_SEQ3_ONE)

typedef struct Micro {
    uint16_t opc;
    char const mne[6];
    uint64_t msk;
} Micro;

static Micro const microtable[] = {
    { 07403, "ACS",  O_ACS  },
    { 07415, "ASR",  O_ASR  },
    { 07002, "BSW",  O_BSW  },
    { 07621, "CAM",  O_CLA|O_MQL },
    { 07041, "CIA",  O_CMA|O_IAC },
    { 07200, "CLA",  O_CLA  },
    { 07100, "CLL",  O_CLL  },
    { 07040, "CMA",  O_CMA  },
    { 07020, "CML",  O_CML  },
    { 07443, "DAD",  O_DAD  },
    { 07455, "DCM",  O_DCM  },
    { 07573, "DPIC", O_DPIC },
    { 07451, "DPSZ", O_DPSZ },
    { 07445, "DST",  O_DST  },
    { 07407, "DVI",  O_DVI  },
    { 07402, "HLT",  O_HLT  },
    { 07001, "IAC",  O_IAC  },
    { 07604, "LAS",  O_CLA|O_OSR },
    { 07417, "LSR",  O_LSR  },
    { 07501, "MQA",  O_MQA  },
    { 07421, "MQL",  O_MQL  },
    { 07405, "MUY",  O_MUY  },
    { 07411, "NMI",   -1LL  },
    { 07000, "NOP",   -1LL  },
    { 07404, "OSR",  O_OSR  },
    { 07004, "RAL",  O_RAL  },
    { 07010, "RAR",  O_RAR  },
    { 07006, "RTL",  O_RTL  },
    { 07012, "RTR",  O_RTR  },
    { 07457, "SAM",  O_SAM  },
    { 07441, "SCA",  O_SCA  },
    { 07403, "SCL",  O_SCL  },
    { 07413, "SHL",  O_SHL  },
    { 07410, "SKP",  O_SKP  },
    { 07500, "SMA",  O_SMA  },
    { 07450, "SNA",  O_SNA  },
    { 07420, "SNL",  O_SNL  },
    { 07510, "SPA",  O_SPA  },
    { 07240, "STA",  O_CLA|O_CMA  },
    { 07120, "STL",  O_CLL|O_CML  },
    { 07431, "SWAB",  -1LL  },  // a: mql+nmi; b: mql+nmi
    { 07447, "SWBA",  -1LL  },  // a: sca+dvi; b: swba
    { 07440, "SZA",  O_SZA  },
    { 07430, "SZL",  O_SZL  },
    {     0, "",        0 } };

// decode micro opcodes
//  input:
//   token = points to possible micro opcode
//  output:
//   returns 0: token was not a micro opcode
//        else: decoded micro opcode 07xxx
static uint16_t decomicro (Token *token)
{
    int i, skipnext;
    uint16_t opcode;
    uint64_t already;

    already  = 0;
    opcode   = 0;
    skipnext = 0;

    do {

        // check for opcode
        // if first time through loop, don't print error, just return 0
        if (token->toktype != TT_SYMBOL) {
            if (opcode != 0) errortok (token, "expecting micro opcode");
            break;
        }

        // look it up in table
        Micro const *micro;
        for (i = 0;; i ++) {
            micro = &microtable[i];
            if (micro->mne[0] == 0) break;
            if (strcasecmp (token->strval, micro->mne) == 0) goto found;
        }
        if (opcode == 0) break;
        errortok (token, "unknown opcode %s", token->strval);

        // check for conflict with other micro ops
        // can only conflict if second or later so always print error message
        // only merge if good to avoid excessive error messages
    found:;
        if (skipnext && ! (micro->msk & (GR2_SEQ1P_ANY | GR2_SEQ1N_ANY))) {
            errortok (token, "+ * | & allowed only between conditional skips");
        }
        char const *conf = microconflict (already, micro->msk);
        if (conf != NULL) {
            errortok (token, "conflicting or out-of-order micro opcode %s (%s)", token->strval, conf);
        } else {
            already |= micro->msk;
            opcode  |= micro->opc;
        }

        // point to next in line
        token = token->next;

        // can have + * | & between conditional skip micro ops
        skipnext = 0;
        if (token->toktype == TT_DELIM) {
            switch (token->intval) {
                case '+':
                case '|': {
                    if (micro->msk & GR2_SEQ1P_ANY) {
                        skipnext = 1;
                    } else {
                        errortok (token, "+ | only allowed between pos skips SMA SZA SNL");
                    }
                    break;
                }
                case '*':
                case '&': {
                    if (micro->msk & GR2_SEQ1N_ANY) {
                        skipnext = 1;
                    } else {
                        errortok (token, "* & only allowed between neg skips SPA SNA SZL");
                    }
                    break;
                }
                default: {
                    errortok (token, "unknown delimiter");
                    break;
                }
            }
            token = token->next;
        }

    } while (token->toktype != TT_ENDLINE);

    // Group 2 CLA is one cycle faster than Group 1 CLA
    // Group 2 NOP is one cycle faster than Group 1 NOP
    // but only if not -pal cuz it might be using them as constants
    if (((opcode & 07577) == 07000) && ! palmode) opcode |= 00400;

    return opcode;
}

static char const *microconflict (uint64_t already, uint64_t adding)
{
    // can't have same thing twice
    if (already & adding) return "DUPLICATE";

    // if already have GR1_SEQ4_ONE (simple shift), can't add anything
    if (already & GR1_SEQ4_ONE) return "RAR RAL RTR RTL BSW";

    // if already have GR1_SEQ3 (iac), can't add GR1_SEQ2_ANY|GR1_SEQ1_ANY|GR2_*|GR3A_*|GR3B_*
    if ((already & GR1_SEQ3) && (adding & (GR1_SEQ2_ANY|GR1_SEQ1_ANY|GR2|GR3A|GR3B))) return "IAC";

    // if already have GR1_SEQ2_ANY (cma,cml), can't add GR1_SEQ1_ANY|GR2_*|GR3A_*|GR3B_*
    if ((already & GR1_SEQ2_ANY) && (adding & (GR1_SEQ1_ANY|GR2|GR3A|GR3B))) return "CMA CML";

    // if already have O_CLL, can't add GR2_*|GR3A_*|GR3B_*-O_CLA
    if ((already & O_CLL) && (adding & (GR2|GR3A|GR3B) & ~ O_CLA)) return "CLL";

    // if already have GR2_SEQ4 (hlt), can't add anything
    if (already & GR2_SEQ4) return "HLT";

    // if already have GR2_SEQ3 (osr), can't add GR1_*|GR2_SEQ1*|GR2_SEQ2|GR3A_*|GR3B_*
    if ((already & GR2_SEQ3) && (adding & (GR1|GR2_SEQ2|GR2_SEQ1U|GR2_SEQ1N_ANY|GR2_SEQ1P_ANY|GR3A|GR3B))) return "OSR";

    // if already have GR2_SEQ2 (cla), can't add GR2_SEQ1P_ANY|GR2_SEQ1N_ANY|GR2_SEQ1U
    if ((already & GR2_SEQ2) && (adding & (GR2_SEQ1U|GR2_SEQ1N_ANY|GR2_SEQ1P_ANY))) return "CLA";

    // if already have GR2_SEQ1U (skp), can't add GR1_*|GR2_SEQ1N_ANY|GR2_SEQ1P_ANY|GR3A_*|GR3B_*-O_CLA
    if ((already & GR2_SEQ1U) && (adding & (GR1|GR2_SEQ1N_ANY|GR2_SEQ1P_ANY|GR3A|GR3B) & ~ O_CLA)) return "SKP";

    // if already have GR2_SEQ1N_ANY (spa,sna,szl), can't add GR1_*|GR2_SEQ1P_ANY|GR2_SEQ1U|GR3A_*|GR3B_*-O_CLA
    if ((already & GR2_SEQ1N_ANY) && (adding & (GR1|GR2_SEQ1U|GR2_SEQ1P_ANY|GR3A|GR3B) & ~ O_CLA)) return "SPA SNA SZL";

    // if already have GR2_SEQ1P_ANY (sma,sza,snl), can't add GR1_*|GR2_SEQ1N_ANY|GR2_SEQ1U|GR3A_*|GR3B_*-O_CLA
    if ((already & GR2_SEQ1P_ANY) && (adding & (GR1|GR2_SEQ1U|GR2_SEQ1N_ANY|GR3A|GR3B) & ~ O_CLA)) return "SMA SZA SNL";

    // if already have GR3A_SEQ3_ONE (ops), can't add anything
    if (already & GR3A_SEQ3_ONE) return "SCL MUY DVI NMI SHL ASR LSR";

    // if already have GR3A_SEQ2_ANY (mqa,sca,mql), can't add GR1_*|GR2_*|GR3A_SEQ1|GR3B_*
    if ((already & GR3A_SEQ2_ANY) && (adding & (GR1|GR2|GR3A_SEQ1|GR3B))) return "MQA SCA MQL";

    // if already have GR3B_SEQ3_ONE (ops), can't add anything
    if (already & GR3A_SEQ3_ONE) return "SCL MUY DVI NMI SHL ASR LSR";

    // if already have GR3B_SEQ2_ANY (mqa,mql), can't add GR1_*|GR2_*|GR3B_SEQ1|GR3A_*
    if ((already & GR3B_SEQ2_ANY) && (adding & (GR1|GR2|GR3B_SEQ1|GR3A))) return "SCL MUY DVI NMI SHL ASR LSR";

    // no conflict
    return NULL;
}

static void addsymref (Symbol *symbol, Token *token, char type)
{
    if (! symbol->hidden && (passno == 2)) {
        Symref *symref = malloc (sizeof *symref);
        symref->next   = symbol->refs;
        symref->name   = token->srcname;
        symref->line   = token->srcline;
        symref->type   = type;
        symbol->refs   = symref;
        symbol->nrefs ++;
    }
}

#define LISW 120                // listing width
#define PADW 2                  // padding between columns
#define VALW 7                  // value width incl padding
#define PSEW 9                  // minimum psect width incl padding
#define NAMW 8                  // minimum name width incl padding
#define SYMW (VALW+PSEW+NAMW)   // overall minimum width for symbol

#define PRINTSYMNAME 1
#define PRINTSYMREF  2

static void symboltable ()
{
    int collens[LISW/SYMW];     // number of symbols in each column
    int coltops[LISW/SYMW+1];   // index at top of each column
    int namwids[LISW/SYMW];     // widest name in each column
    int psewids[LISW/SYMW];     // widest psect in each column
    char buf[12], colstates[LISW/SYMW];
    int colstart, icol, irow, isym, itop, len, linelen, linesincol, linesleft, namlen, ncols, nsyms, symsleft, totlines;
    PSect *psect;
    Symbol *colsymbls[LISW/SYMW];
    Symbol **symarray, *lsymbol, *symbol;
    Symref *colsymrfs[LISW/SYMW];
    Symref *lsymref, *nsymref, *symref;

    printf ("\f");

    if (psects != NULL) {
        printf ("\nSize    Align   Name\n\n");
        for (psect = psects; psect != NULL; psect = psect->next) {
            printf ("%05o   %05o   %s\n", psect->size, 1 << psect->p2al, psect->name);
        }
    }

    // get sorted symbol array
    nsyms = 0;
    for (symbol = symbols; symbol != NULL; symbol = symbol->next) {
        if (! symbol->hidden) nsyms ++;
    }
    symarray = malloc (nsyms * sizeof *symarray);
    isym = 0;
    for (symbol = symbols; symbol != NULL; symbol = symbol->next) {
        if (! symbol->hidden) symarray[isym++] = symbol;
    }
    qsort (symarray, nsyms, sizeof *symarray, sortsyms);

    // figure out how many columns by trial-and-error
    for (ncols = LISW / SYMW;; -- ncols) {

        // figure out how many symbols per column there would be
        symsleft = nsyms;
        for (icol = 0; icol < ncols; icol ++) {
            coltops[icol] = nsyms - symsleft;
            collens[icol] = (symsleft + ncols - icol - 1) / (ncols - icol);
            symsleft     -= collens[icol];
        }
        coltops[ncols] = nsyms;

        // figure out the psect and name width for each column
        for (icol = 0; icol < ncols; icol ++) {
            namwids[icol] = NAMW;
            psewids[icol] = PSEW;
            for (irow = 0; irow < collens[icol]; irow ++) {
                isym = coltops[icol] + irow;
                symbol = symarray[isym];
                len = strlen (symbol->name) + PADW;
                if (namwids[icol] < len) namwids[icol] = len;
                psect = symbol->psect;
                if (psect != NULL) {
                    len = strlen (psect->name) + PADW;
                    if (psewids[icol] < len) psewids[icol] = len;
                }
            }
        }

        if (ncols == 1) break;

        // compute line length and stop if narrow enough to fit
        len = -2 * PADW;
        for (icol = 0; icol < ncols; icol ++) {
            len += psewids[icol] + VALW + namwids[icol] + PADW;
        }
        if (len <= LISW) break;
    }

    // print symbol table headings
    printf ("\n");
    for (icol = 0; icol < ncols; icol ++) {
        if (icol > 0) printf ("%*s", namwids[icol-1] - 4 + PADW, "");
        printf ("%-*s%-*s%s", psewids[icol], "PSect", VALW, "Value", "Name");
    }
    printf ("\n\n");

    // print symbol table entries
    for (irow = 0; irow < collens[0]; irow ++) {
        namlen = 0;
        for (icol = 0; icol < ncols; icol ++) {
            isym = coltops[icol] + irow;
            if (isym >= coltops[icol+1]) break;
            symbol = symarray[isym];
            if (icol > 0) printf ("%*s", namwids[icol-1] - namlen + PADW, "");
            printf ("%-*s%05o%*s%s",
                    psewids[icol], (! symbol->defind ? "*UNDEF*" :
                        (symbol->psect != NULL ? symbol->name :
                            (symbol->symtyp == ST_PMEMOP ? "*MEMOP*" : ""))),
                    symbol->value, VALW - 5, "", symbol->name);
            namlen = strlen (symbol->name);
        }
        printf ("\n");
    }

    // compute total number of lines for cross-reference
    // ...if it were all done in one column
    totlines = 0;
    for (isym = 0; isym < nsyms; isym ++) {
        symbol = symarray[isym];
        totlines += symbol->nrefs + 2;
    }

    // figure out how many columns for cross-reference by trial-and-error
    for (ncols = LISW / SYMW;; -- ncols) {

        // figure out how many symbols per column there would be
        isym = 0;
        linesleft = totlines;
        for (icol = 0; icol < ncols; icol ++) {
            coltops[icol] = isym;
            linesincol = (linesleft + ncols - icol - 1) / (ncols - icol);
            for (irow = 0; linesincol > 0; irow ++) {
                symbol = symarray[isym+irow];
                linesincol -= symbol->nrefs + 2;
                linesleft  -= symbol->nrefs + 2;
            }
            collens[icol] = irow;
            isym += irow;
        }
        coltops[ncols] = nsyms;

        // get width required for each of those columns
        for (icol = 0; icol < ncols; icol ++) {
            namwids[icol] = 0;
            for (irow = 0; irow < collens[icol]; irow ++) {
                symbol = symarray[coltops[icol]+irow];
                len = strlen (symbol->name);
                if (namwids[icol] < len) namwids[icol] = len;
                for (symref = symbol->refs; symref != NULL; symref = symref->next) {
                    sprintf (buf, "%d", symref->line);
                    len = strlen (symref->name) + strlen (buf) + 5;
                    if (namwids[icol] < len) namwids[icol] = len;
                }
            }
        }

        if (ncols == 1) break;

        // compute line length and stop if narrow enough to fit
        len = -2 * PADW;
        for (icol = 0; icol < ncols; icol ++) {
            len += namwids[icol] + 2 * PADW;
        }
        if (len <= LISW) break;
    }

    // reverse order of symrefs on each symbol to put them in top-to-bottom order
    for (isym = 0; isym < nsyms; isym ++) {
        symbol = symarray[isym];
        lsymref = NULL;
        for (symref = symbol->refs; symref != NULL; symref = nsymref) {
            nsymref = symref->next;
            symref->next = lsymref;
            lsymref = symref;
        }
        symbol->refs = lsymref;
    }

    // get first symbol for each column and make linked list of symbols for each column
    for (icol = 0; icol < ncols; icol ++) {
        colstates[icol] = PRINTSYMNAME;
        itop = coltops[icol];
        if (itop < nsyms) {
            colsymbls[icol] = lsymbol = symarray[itop];
            for (irow = 0; ++ irow < collens[icol];) {
                symbol = symarray[coltops[icol]+irow];
                lsymbol->next = symbol;
                lsymbol = symbol;
            }
            lsymbol->next = NULL;
        } else {
            colsymbls[icol] = NULL;
        }
        colsymrfs[icol] = NULL;
    }

    // print cross-reference table
    printf ("\f\n");
    for (irow = 0;; irow ++) {

        // if no more symbols to print, we're all done
        for (icol = 0; icol < ncols; icol ++) {
            if (colsymbls[icol] != NULL) goto morexrefs;
        }
        break;
    morexrefs:;

        // print contents of each column
        linelen = 0;
        colstart = 0;
        for (icol = 0; icol < ncols; icol ++) {
            switch (colstates[icol]) {

                // print symbol name in the column (or blank if NULL cuz column is finished)
                // then step the column to symbol's first reference
                case PRINTSYMNAME: {
                    symbol = colsymbls[icol];
                    if (symbol != NULL) {
                        linelen += printf ("%*s%s", colstart - linelen, "", symbol->name);
                        colstates[icol] = PRINTSYMREF;
                        colsymrfs[icol] = symbol->refs;
                    }
                    break;
                }

                // print reference in this column (or blank if NULL)
                // then step to next reference of the symbol (or next symbol if NULL)
                case PRINTSYMREF: {
                    symref = colsymrfs[icol];
                    if (symref != NULL) {
                        linelen += printf ("%*s  %c %s:%d", colstart - linelen, "", symref->type, symref->name, symref->line);
                        colsymrfs[icol] = symref->next;
                    } else {
                        colsymbls[icol] = colsymbls[icol]->next;
                        colstates[icol] = PRINTSYMNAME;
                    }
                    break;
                }

                default: abort ();
            }
            colstart += namwids[icol] + 2 * PADW;
        }
        printf ("\n");
    }

    printf ("--\n");
}

static int sortsyms (void const *v1, void const *v2)
{
    Symbol const *s1 = *(Symbol const *const *)v1;
    Symbol const *s2 = *(Symbol const *const *)v2;
    return strcmp (s1->name, s2->name);
}
