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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "token.h"

struct Kwent {
    Keywd kwd;
    char str[9];
};

static Kwent const kwtbl[] = {
    { KW_ALIGNOF,   "alignof"   },
    { KW_BREAK,     "break"     },
    { KW_CASE,      "case"      },
    { KW_CATCH,     "catch"     },
    { KW_CONST,     "const"     },
    { KW_CONTINUE,  "continue"  },
    { KW_DEFAULT,   "default"   },
    { KW_DELETE,    "delete"    },
    { KW_DO,        "do"        },
    { KW_ELSE,      "else"      },
    { KW_ENUM,      "enum"      },
    { KW_EXTERN,    "extern"    },
    { KW_FINALLY,   "finally"   },
    { KW_FOR,       "for"       },
    { KW_GOTO,      "goto"      },
    { KW_IF,        "if"        },
    { KW_NEW,       "new"       },
    { KW_RETURN,    "return"    },
    { KW_SIGNED,    "signed"    },
    { KW_SIZEOF,    "sizeof"    },
    { KW_STATIC,    "static"    },
    { KW_STRUCT,    "struct"    },
    { KW_SWITCH,    "switch"    },
    { KW_THROW,     "throw"     },
    { KW_TRY,       "try"       },
    { KW_TYPEDEF,   "typedef"   },
    { KW_UNION,     "union"     },
    { KW_UNSIGNED,  "unsigned"  },
    { KW_VIRTUAL,   "virtual"   },
    { KW_VOLATILE,  "volatile"  },
    { KW_WHILE,     "while"     } };

static Kwent const dltbl[] = {
    { KW_DOTDOTDOT,   "..."  },
    { KW_GTGTEQ,      ">>="  },
    { KW_LTLTEQ,      "<<="  },

    { KW_ANDAND,      "&&"   },
    { KW_ANDEQ,       "&="   },
    { KW_CIRCUMEQ,    "^="   },
    { KW_COLON2,      "::"   },
    { KW_EQEQ,        "=="   },
    { KW_GTEQ,        ">="   },
    { KW_GTGT,        ">>"   },
    { KW_LTEQ,        "<="   },
    { KW_LTLT,        "<<"   },
    { KW_MINUSEQ,     "-="   },
    { KW_MINUSMINUS,  "--"   },
    { KW_NOTEQ,       "!="   },
    { KW_OREQ,        "|="   },
    { KW_OROR,        "||"   },
    { KW_PERCENTEQ,   "%="   },
    { KW_PLUSEQ,      "+="   },
    { KW_PLUSPLUS,    "++"   },
    { KW_POINT,       "->"   },
    { KW_SLASHEQ,     "/="   },
    { KW_STAREQ,      "*="   },

    { KW_AMPER,       "&"    },
    { KW_CBRACE,      "}"    },
    { KW_CBRKT,       "]"    },
    { KW_CIRCUM,      "^"    },
    { KW_COLON,       ":"    },
    { KW_COMMA,       ","    },
    { KW_CPAREN,      ")"    },
    { KW_DOT,         "."    },
    { KW_EQUAL,       "="    },
    { KW_EXCLAM,      "!"    },
    { KW_GTANG,       ">"    },
    { KW_LTANG,       "<"    },
    { KW_MINUS,       "-"    },
    { KW_OBRACE,      "{"    },
    { KW_OBRKT,       "["    },
    { KW_OPAREN,      "("    },
    { KW_ORBAR,       "|"    },
    { KW_PERCENT,     "%"    },
    { KW_PLUS,        "+"    },
    { KW_QMARK,       "?"    },
    { KW_SEMI,        ";"    },
    { KW_SLASH,       "/"    },
    { KW_STAR,        "*"    },
    { KW_TILDE,       "~"    } };

std::vector<char const *> srcfilenames;

static char *line;
static char *next;
static char const *srcname;     // source filename being processed
static FILE *srcfile;           // source file being processed
static int srcline;             // line number within srcname being processed
static std::vector<Token *> pushedtokens;

static char *getsrcline ();
static void error (char const *fmt, ...);

void pushtoken (Token *tok)
{
    pushedtokens.push_back (tok);
}

Token *gettoken ()
{
    if (! pushedtokens.empty ()) {
        Token *pushed = pushedtokens.back ();
        pushedtokens.pop_back ();
        return pushed;
    }

    while (true) {

        // maybe read next source line
        if (next == nullptr) {
            line = next = getsrcline ();
            if (line == nullptr) break;

            // '# ' linenumber "sourcefilename" otherstuff...
            if (line[0] == '#') {
                next ++;
                Token *lno = gettoken ();
                NumToken *lnonum = lno->castNumToken ();
                if ((lnonum != nullptr) && (lnonum->getNumCat () == NC_SINT)) {
                    Token *sfn = gettoken ();
                    StrToken *sfnstr = sfn->castStrToken ();
                    if (sfnstr != nullptr) {
                        NumValue nv = lnonum->getValue ();
                        srcname = sfnstr->getString ();
                        srcline = nv.s - 1;
                    }
                }
                if (line != nullptr) delete[] line;
                line = next = nullptr;
                continue;
            }
        }

        // get next char from source line
        char ch = *next;
        if (ch == 0) {
            delete[] line;
            line = next = nullptr;
            continue;
        }

        // skip whitespace
        if (ch <= ' ') {
            next ++;
            continue;
        }

        // skip comments
        if (ch == '/') {
            if (next[1] == '/') {
                delete[] line;
                line = next = nullptr;
                continue;
            }
            if (next[1] == '*') {
                char *p;
                while ((p = strstr (next, "*/")) == nullptr) {
                    delete[] line;
                    line = next = getsrcline ();
                    if (line == nullptr) {
                        error ("eof in comment");
                        goto eofincom;
                    }
                }
                next = p + 2;
            eofincom:
                continue;
            }
        }

        // strings
        if ((ch == '"') || (ch == '\'')) {

            // gather string into buffer without the quotes and with escapes decoded
            char *strbuf = new char[strlen(next)];
            char *p = next;
            char *q = strbuf;
            while (true) {
                char dh = *(++ p);
                if (dh == 0) {
                    error ("unterminated string");
                    break;
                }
                if (dh == ch) {
                    ++ p;
                    break;
                }
                if (dh == '\\') {
                    dh = *(++ p);
                    if (dh == 0) {
                        error ("unterminated string");
                        break;
                    }
                    switch (dh) {
                        case 'b': dh = '\b'; break;
                        case 'n': dh = '\n'; break;
                        case 'r': dh = '\r'; break;
                        case 't': dh = '\t'; break;
                        case 'z': dh =    0; break;
                        case '0' ... '7': {
                            dh -= '0';
                            for (int i = 0; (i < 2) && (dh <= 31); i ++) {
                                char eh = p[1];
                                if ((eh < '0') || (eh > '7')) break;
                                p ++;
                                dh = dh * 8 + eh - '0';
                            }
                            break;
                        }
                    }
                }
                *(q ++) = dh;
            }
            *q = 0;
            int length = q - strbuf;

            int srccoln = next - line + 1;
            next = p;

            // if 'string', make an integer constant
            if (ch == '\'') {
                assert (chartype->getSign ());
                NumValue numval;
                numval.s = 0;
                for (int i = length; -- i >= 0;) {
                    numval.s = (numval.s << 8) | (unsigned char) strbuf[i];
                }
                return new NumToken (srcname, srcline, srccoln, NC_SINT, numval);
            }

            // "string", make new string token
            return new StrToken (srcname, srcline, srccoln, strbuf, q - strbuf);
        }

        // numerics
        if (((ch >= '0') && (ch <= '9')) || ((ch == '.') && (next[1] >= '0') && (next[1] <= '9'))) {
            int  srccoln = next - line + 1;
            char *pi, *pd;
            NumCat   numcat = NC_NONE;
            NumValue numval;
            double   dval = strtod   (next, &pd);
            uint64_t uval = strtoull (next, &pi, 0);
            if (pd > pi) {
                numcat   = NC_FLOAT;
                numval.f = dval;
                next = pd;
            } else {
                if ((*pi == 'u') || (*pi == 'U')) {
                    numcat   = NC_UINT;
                    numval.u = uval;
                    pi ++;
                } else {
                    numcat   = NC_SINT;
                    numval.s = uval;
                }
                while ((*pi == 'l') || (*pi == 'L')) pi ++;
                next = pi;
            }
            return new NumToken (srcname, srcline, srccoln, numcat, numval);
        }

        // identifiers & keywords
        {
            char *p;
            for (p = next;; p ++) {
                ch = *p;
                if ((ch >= 'a') && (ch <= 'z')) continue;
                if ((ch >= 'A') && (ch <= 'Z')) continue;
                if ((ch >= '0') && (ch <= '9')) continue;
                if ((ch == '_') || (ch == '$')) continue;
                break;
            }
            int i = p - next;
            if (i > 0) {

                // check for keyword
                int j = 0;
                int k = sizeof kwtbl / sizeof kwtbl[0];
                do {
                    int m = (j + k) / 2;
                    for (int n = 0; n < i; n ++) {
                        int c = next[n] - kwtbl[m].str[n];
                        if (c < 0) goto lowerhalf;
                        if (c > 0) goto upperhalf;
                    }
                    if (kwtbl[m].str[i] == 0) {
                        KwToken *tok = new KwToken (srcname, srcline, next - line + 1, kwtbl[m].kwd);
                        next = p;
                        return tok;
                    }
                lowerhalf:
                    k = m;
                    continue;
                upperhalf:
                    j = m + 1;
                } while (j < k);

                // not a keyword, make a symbol token
                char *buf = new char[i+1];
                memcpy (buf, next, i);
                buf[i] = 0;
                SymToken *tok = new SymToken (srcname, srcline, next - line + 1, buf);
                next = p;
                return tok;
            }
        }

        // delimeters
        for (unsigned i = 0; i < sizeof dltbl / sizeof dltbl[0]; i ++) {
            for (int j = 0;; j ++) {
                char c = dltbl[i].str[j];
                if (c == 0) {
                    KwToken *tok = new KwToken (srcname, srcline, next - line + 1, dltbl[i].kwd);
                    next += j;
                    return tok;
                }
                if (next[j] != c) break;
            }
        }

        // unknown
        error ("bad char %02X <%c>", (uint8_t) ch, ch);
        next ++;
    }

    return new KwToken (srcname, srcline, 0, KW_EOF);
}

static char *getsrcline ()
{
    int len = 256;
    char *buf = new char[len];

opensrc:
    if (srcfile == nullptr) {
        if (srcfilenames.empty ()) {
            delete[] buf;
            return nullptr;
        }
        srcname = srcfilenames[0];
        srcfilenames.erase (srcfilenames.begin ());
        srcfile = fopen (srcname, "r");
        if (srcfile == nullptr) {
            fprintf (stderr, "error opening %s: %m\n", srcname);
            exit (1);
        }
        srcline = 0;
    }

    srcline ++;
    if (fgets (buf, len, srcfile) == nullptr) {
        fclose (srcfile);
        srcfile = nullptr;
        goto opensrc;
    }

    len = strlen (buf);
    while ((len <= 0) || (buf[len-1] != '\n')) {
        int newlen = len * 3 / 2;
        char *newbuf = new char[newlen];
        memcpy (newbuf, buf, len);
        delete[] buf;
        if (fgets (newbuf + len, newlen - len, srcfile) == nullptr) return newbuf;
        len += strlen (newbuf + len);
        buf  = newbuf;
    }

    return buf;
}

static void error (char const *fmt, ...)
{
    va_list ap;
    char *buf;
    va_start (ap, fmt);
    vasprintf (&buf, fmt, ap);
    va_end (ap);
    fprintf (stderr, "%s:%d.%ld: %s\n", srcname, srcline, (long) (next - line + 1), buf);
    free (buf);
    errorflag = true;
}

Token::Token (char const *file, int line, int coln)
{
    this->file = file;
    this->line = line;
    this->coln = coln;
}

KwToken::KwToken (char const *file, int line, int coln, Keywd kw)
    : Token (file, line, coln)
{
    this->kw = kw;
}

NumToken::NumToken (char const *file, int line, int coln, NumCat category, NumValue value)
    : Token (file, line, coln)
{
    this->category = category;
    this->value    = value;
    this->name     = nullptr;
}

char const *NumToken::getName ()
{
    if (name == nullptr) {
        char const *typenm = getNumType ()->getName ();
        switch (category) {
            case NC_FLOAT: asprintf (&name, "(%s)%g",    typenm, value.f); break;
            case NC_SINT:  asprintf (&name, "(%s)%lld",  typenm, (long long) value.s); break;
            case NC_UINT:  asprintf (&name, "(%s)%lluU", typenm, (unsigned long long) value.u); break;
            default: assert_false ();
        }
    }
    return name;
}

NumType *NumToken::getNumType ()
{
    NumType *t;
    switch (category) {
        case NC_FLOAT: {
            t = (value.f == (float) value.f) ? flt32type : flt64type;
            break;
        }
        case NC_SINT: {
            t = (value.s == (int64_t) (int8_t) value.s) ? int8type :
                        (value.s == (int64_t) (int16_t) value.s) ? int16type :
                        (value.s == (int64_t) (int32_t) value.s) ? int32type : int64type;
            break;
        }
        case NC_UINT: {
            t = (value.u == (uint64_t) (uint8_t) value.u) ? uint8type :
                        (value.u == (uint64_t) (uint16_t) value.u) ? uint16type :
                        (value.u == (uint64_t) (uint32_t) value.u) ? uint32type : uint64type;
            break;
        }
        default: assert_false ();
    }
    return t;
}

StrToken::StrToken (char const *file, int line, int coln, char *value, int valen)
    : Token (file, line, coln)
{
    this->value = value;
    this->valen = valen;
    this->name  = nullptr;
}

// append on subsequent string constant tokens
void StrToken::extend ()
{
    while (true) {
        Token *exttok = gettoken ();
        StrToken *strtok = exttok->castStrToken ();
        if (strtok == nullptr) {
            pushtoken (exttok);
            return;
        }
        int newlen = valen + strtok->valen;
        char *newbuf = new char[newlen];
        memcpy (newbuf, value, valen);
        memcpy (newbuf + valen, strtok->value, strtok->valen);
        delete[] value;
        value = newbuf;
        valen = newlen;
        delete strtok;
        if (name != nullptr) {
            delete name;
            name = nullptr;
        }
    }
}

StrToken::~StrToken ()
{
    if (name  != nullptr) delete   name;
    if (value != nullptr) delete[] value;
    name  = nullptr;
    value = nullptr;
}

char const *StrToken::getName ()
{
    if (name == nullptr) {
        name = new std::string ();
        name->push_back ('"');
        for (int i = 0; i < valen; i ++) {
            char c = value[i];
            switch (c) {
                case '\b': name->append ("\\b"); break;
                case '\n': name->append ("\\n"); break;
                case '\r': name->append ("\\r"); break;
                case '\t': name->append ("\\t"); break;
                case    0: name->append ("\\z"); break;
                default:   name->push_back (c); break;
            }
        }
        name->push_back ('"');
    }
    return name->c_str ();
}

SymToken::SymToken (char const *file, int line, int coln, char const *value)
    : Token (file, line, coln)
{
    this->value = value;
}
