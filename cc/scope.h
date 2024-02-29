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
#ifndef _SCOPE_H
#define _SCOPE_H

#include <map>
#include <string.h>

enum PvEnv {
    PE_BLOCK,   // within a '{' block '}' statement
    PE_GLOBAL,  // top-level declaration
    PE_PARAM,   // a function parameter definition
    PE_STRUCT,  // within a struct/union definition
    PE_TCAST    // doing a type cast parse
};

struct Scope;
struct BlockScope;
struct GlobalScope;
struct MemberScope;
struct ParamScope;
struct TryScope;

#include "cmp_str.h"
#include "decl.h"
#include "expr.h"
#include "mytypes.h"
#include "stmt.h"
#include "token.h"

typedef std::map<char const *,Decl *,cmp_str> DeclMap;
typedef std::map<char const *,LabelStmt *,cmp_str> LabelMap;

struct Scope {

    Scope (Scope *outer);

    virtual TryScope *castTryScope () { return nullptr; }

    Scope *getOuter () { return outer; }

    virtual DeclMap const *getDecls () = 0;
    virtual Decl *lookupdecl (char const *name) = 0;
    virtual void enterdecl (Decl *decl) = 0;
    virtual void erasedecl (char const *name) = 0;
    virtual LabelStmt *lookuplabel (char const *name) = 0;
    virtual void enterlabel (LabelStmt *label) = 0;
    virtual FuncDecl *getOuterFuncDecl () = 0;

    void allocscope ();
    tsize_t parseintconstexpr ();
    Expr *parsexpression (bool stoponcomma, bool stoponcolon);
    void parsevaldecl (PvEnv pvenv, DeclVec *declvec);
    Type *parsetypecast ();
    Expr *parseinitexpr (Type *vartype, bool isarray, tsize_t *arrsize_r);
    void maybeoutputstructinit (VarDecl *vardecl);
    bool istypenext ();
    bool istypetoken (Token *tok);
    void checkAsneqCompat (Token *optok, Type *lefttype, Type *ritetype);
    Expr *castToType (Token *optok, Type *totype, Expr *expr);
    StructType *getStructType (char const *name);

private:
    Scope *outer;
    std::vector<Scope *> inners;

    void parsedecl (PvEnv pvenv, DeclVec *declvec);
    bool parsearrsize (tsize_t *arrsize_r);
    VarDecl *declarevariable (char const *varname, Token *varnametok, StructType *enctype, Type *vartype, Keywd storclass, bool hasarrsize, tsize_t arrsize);
    FuncType *parsefunctype (Type *rettype, std::vector<VarDecl *> **params_r, ParamScope **paramscope_r);
    Type *parsetypemods (Type *type);
    Type *parsecvmoddedtype ();
    Type *parsenamedtype ();
    BlockExpr *parseblockexpr (Token *obracetok);

    Expr *postIncDec (Token *optok, Opcode addsub, Expr *subexpr);
    Expr *preIncDec (Token *optok, Opcode addsub, Expr *subexpr);
    Expr *getIncrement (Token *optok, Expr *valexpr);
    Expr *newBinopArith (Token *optok, Opcode opcode, Expr *leftexpr, Expr *riteexpr);
    Expr *newBinopAdd (Token *optok, Expr *leftexpr, Expr *riteexpr);
    Expr *newBinopSub (Token *optok, Expr *leftexpr, Expr *riteexpr);
    Expr *ptrPlusMinusInt (Token *optok, Opcode opcode, Expr *leftexpr, Expr *riteexpr);
    Expr *ptrMinusPtr (Token *optok, Expr *leftexpr, Expr *riteexpr);
    Type *getStrongerArith (Token *optok, Type *lefttype, Type *ritetype);
    Type *getStrongerIArith (Token *optok, Type *lefttype, Type *ritetype);
    Type *getWeakerIArith (Token *optok, Type *lefttype, Type *ritetype);
};

// variables and labels declared inside { ... } of a function
struct BlockScope : Scope {
    BlockScope (Scope *outer);

    virtual DeclMap const *getDecls ();
    virtual Decl *lookupdecl (char const *name);
    virtual void enterdecl (Decl *decl);
    virtual void erasedecl (char const *name);
    virtual LabelStmt *lookuplabel (char const *name);
    virtual void enterlabel (LabelStmt *label);
    virtual FuncDecl *getOuterFuncDecl ();

private:
    DeclMap decls;      // all variables (incl stack, extern, static) declared in the block
    LabelMap labels;    // all labels declared in the block
};

// variables and functions declared at top level
struct GlobalScope : Scope {
    GlobalScope ();

    virtual DeclMap const *getDecls ();
    virtual Decl *lookupdecl (char const *name);
    virtual void enterdecl (Decl *decl);
    virtual void erasedecl (char const *name);
    virtual LabelStmt *lookuplabel (char const *name);
    virtual void enterlabel (LabelStmt *label);
    virtual FuncDecl *getOuterFuncDecl ();

private:
    DeclMap decls;      // all top-level variables and functions
};

// variables and functions declared as members of a struct/union
struct MemberScope : Scope {
    MemberScope (Scope *outer);

    virtual DeclMap const *getDecls ();
    virtual Decl *lookupdecl (char const *name);
    virtual void enterdecl (Decl *decl);
    virtual void erasedecl (char const *name);
    virtual LabelStmt *lookuplabel (char const *name);
    virtual void enterlabel (LabelStmt *label);
    virtual FuncDecl *getOuterFuncDecl ();

private:
    DeclMap decls;      // all struct members (variables, methods, static variables & methods)
};

// variables declared as parameters of a function
struct ParamScope : Scope {
    ParamScope (Scope *outer);

    virtual DeclMap const *getDecls ();
    virtual Decl *lookupdecl (char const *name);
    virtual void enterdecl (Decl *decl);
    virtual void erasedecl (char const *name);
    virtual LabelStmt *lookuplabel (char const *name);
    virtual void enterlabel (LabelStmt *label);
    virtual FuncDecl *getOuterFuncDecl ();

    FuncDecl *funcdecl;

private:
    DeclMap decls;      // all function parameters + __retaddr
};

// wraps labels outside the try block that are referenced from inside the try block
// causes the finally block to be executed before the jump completes
// includes wrapping break, continue, return targets
struct TryScope : Scope {
    TryScope (Scope *outer);

    virtual TryScope *castTryScope () { return this; }

    virtual DeclMap const *getDecls ();
    virtual Decl *lookupdecl (char const *name);
    virtual void enterdecl (Decl *decl);
    virtual void erasedecl (char const *name);
    virtual LabelStmt *lookuplabel (char const *name);
    virtual void enterlabel (LabelStmt *label);
    virtual FuncDecl *getOuterFuncDecl ();

    TryLabelStmt *wraplabel (LabelStmt *outerlabelstmt);

    std::map<char const *,TryLabelStmt *,cmp_str> trylabels; // label wrappers for referenced labels outside the try block

private:
    DeclMap decls;      // __trymark
};

extern GlobalScope *globalscope;

#endif
