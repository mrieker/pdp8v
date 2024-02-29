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
#ifndef _TOKEN_H
#define _TOKEN_H

#include <stdint.h>
#include <string>
#include <vector>

enum Keywd {
    KW_ALIGNOF,     // alignof
    KW_AMPER,       // &
    KW_ANDAND,      // &&
    KW_ANDEQ,       // &=
    KW_BREAK,       // break
    KW_CASE,        // case
    KW_CATCH,       // catch
    KW_CBRACE,      // }
    KW_CBRKT,       // ]
    KW_CIRCUM,      // ^
    KW_CIRCUMEQ,    // ^=
    KW_COLON,       // :
    KW_COLON2,      // ::
    KW_COMMA,       // ,
    KW_CONST,       // const
    KW_CONTINUE,    // continue
    KW_CPAREN,      // )
    KW_DEFAULT,     // default
    KW_DELETE,      // delete
    KW_DO,          // do
    KW_DOT,         // .
    KW_DOTDOTDOT,   // ...
    KW_ELSE,        // else
    KW_ENUM,        // enum
    KW_EOF,         // end-of-file
    KW_EQEQ,        // ==
    KW_EQUAL,       // =
    KW_EXCLAM,      // !
    KW_EXTERN,      // extern
    KW_FINALLY,     // finally
    KW_FOR,         // for
    KW_GOTO,        // goto
    KW_GTANG,       // >
    KW_GTEQ,        // >=
    KW_GTGT,        // >>
    KW_GTGTEQ,      // >>=
    KW_IF,          // if
    KW_LTANG,       // <
    KW_LTEQ,        // <=
    KW_LTLT,        // <<
    KW_LTLTEQ,      // <<=
    KW_MINUS,       // -
    KW_MINUSEQ,     // -=
    KW_MINUSMINUS,  // --
    KW_NEW,         // new
    KW_NONE,        // not a keyword
    KW_NOTEQ,       // !=
    KW_OBRACE,      // {
    KW_OBRKT,       // [
    KW_OPAREN,      // (
    KW_ORBAR,       // |
    KW_OREQ,        // |=
    KW_OROR,        // ||
    KW_PERCENT,     // %
    KW_PERCENTEQ,   // %=
    KW_PLUS,        // +
    KW_PLUSEQ,      // +=
    KW_PLUSPLUS,    // ++
    KW_POINT,       // ->
    KW_QMARK,       // ?
    KW_RETURN,      // return
    KW_SEMI,        // ;
    KW_SIGNED,      // signed
    KW_SIZEOF,      // sizeof
    KW_SLASH,       // /
    KW_SLASHEQ,     // /=
    KW_STAR,        // *
    KW_STAREQ,      // *=
    KW_STATIC,      // static
    KW_STRUCT,      // struct
    KW_SWITCH,      // switch
    KW_THROW,       // throw
    KW_TILDE,       // ~
    KW_TRY,         // try
    KW_TYPEDEF,     // typedef
    KW_UNION,       // union
    KW_UNSIGNED,    // unsigned
    KW_VIRTUAL,     // virtual
    KW_VOLATILE,    // volatile
    KW_WHILE,       // while
};

enum NumCat {
    NC_NONE,
    NC_FLOAT,
    NC_SINT,
    NC_UINT
};

union NumValue {
    double   f; // NC_FLOAT
    int64_t  s; // NC_SINT
    uint64_t u; // NC_UINT
};

enum TType {
    TT_EOF,
    TT_NUM,
    TT_KW,
    TT_STR,
    TT_SYM,
};

struct Token;
struct KwToken;
struct NumToken;
struct StrToken;
struct SymToken;

#include "decl.h"
#include "mytypes.h"

Token *gettoken ();
void pushtoken (Token *tok);

struct Token {
    Token (char const *file, int line, int coln);

    char const *getFile () { return file; }
    int getLine () { return line; }
    int getColn () { return coln; }

    virtual bool isEof () { return false; }
    virtual bool isNum () { return false; }
    virtual bool isKw  () { return false; }
    virtual bool isStr () { return false; }
    virtual bool isSym () { return false; }

    virtual bool isKw (Keywd keywd) { return false; }

    virtual NumToken *castNumToken () { return nullptr; }
    virtual StrToken *castStrToken () { return nullptr; }
    virtual SymToken *castSymToken () { return nullptr; }

    virtual TType getTType () = 0;
    virtual Keywd getKw () { return KW_NONE; }
    virtual NumCat getNumValue (NumValue *nv_r) { return NC_NONE; }
    virtual char const *getString () { return nullptr; }
    virtual int getStrlen () { return -1; }
    virtual char const *getSym () { return nullptr; }

private:
    char const *file;
    int line;
    int coln;
};

struct KwToken : Token {
    KwToken (char const *file, int line, int coln, Keywd kw);

    virtual bool isKw () { return true; }
    virtual bool isKw (Keywd keywd) { return keywd == kw; }
    virtual TType getTType () { return TT_KW; }
    virtual Keywd getKw () { return kw; }

private:
    Keywd kw;
};

struct NumToken : Token {
    NumToken (char const *file, int line, int coln, NumCat category, NumValue value);

    virtual NumToken *castNumToken () { return this; }

    NumCat getNumCat () { return category; }
    NumValue getValue () { return value; }
    char const *getName ();
    NumType *getNumType ();

    virtual bool isNum () { return true; }
    virtual TType getTType () { return TT_NUM; }
    virtual NumCat getNumValue (NumValue *nv_r) { *nv_r = value; return category; }

private:
    NumCat category;
    NumValue value;
    char *name;
};

struct StrToken : Token {
    StrToken (char const *file, int line, int coln, char *value, int valen);
    virtual ~StrToken ();

    virtual StrToken *castStrToken () { return this; }

    virtual bool isStr () { return true; }
    virtual TType getTType () { return TT_STR; }
    virtual char const *getString () { return value; }
    virtual int getStrlen () { return valen; }

    void extend ();
    char const *getName ();

    friend Token *gettoken ();

private:
    char *value;
    int valen;
    std::string *name;
};

struct SymToken : Token {
    SymToken (char const *file, int line, int coln, char const *value);

    virtual SymToken *castSymToken () { return this; }

    virtual bool isSym () { return true; }
    virtual TType getTType () { return TT_SYM; }
    virtual char const *getSym () { return value; }

private:
    char const *value;
};

extern Token *intdeftok;
extern std::vector<char const *> srcfilenames;

#endif
