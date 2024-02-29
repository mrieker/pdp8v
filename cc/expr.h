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
#ifndef _EXPR_H
#define _EXPR_H

struct Expr;
struct ArrayInitExpr;
struct AsnopExpr;
struct BinopExpr;
struct BlockExpr;
struct CallExpr;
struct CastExpr;
struct MFuncExpr;
struct QMarkExpr;
struct SizeofExpr;
struct StringInitExpr;
struct StructInitExpr;
struct UnopExpr;
struct ValExpr;

#include "decl.h"
#include "opcode.h"
#include "prim.h"
#include "scope.h"
#include "token.h"

struct Expr {
    Expr (Scope *exprscope, Token *exprtoken);

    Scope *exprscope;
    Token *exprtoken;

    virtual BinopExpr *castBinopExpr () { return nullptr; }
    virtual CastExpr  *castCastExpr  () { return nullptr; }
    virtual MFuncExpr *castMFuncExpr () { return nullptr; }
    virtual UnopExpr  *castUnopExpr  () { return nullptr; }
    virtual ValExpr   *castValExpr   () { return nullptr; }

    virtual Type *getType () = 0;
    virtual void dumpexpr () = 0;
    virtual Prim *generexpr (Prim *prevprim, ValDecl **exprval_r) = 0;
    virtual NumCat isNumConstExpr (NumValue *nv_r) { return NC_NONE; }
    virtual tsize_t getExprSize ();
    virtual tsize_t outputStaticInit (Type *vartype);
    virtual Prim *outputStackdInit (Prim *prevprim, ValDecl *valdecl, bool isarray, tsize_t arrsize);
    virtual char *isAsmConstExpr () { return nullptr; }
    virtual char *getConstantAddr () { return nullptr; }

    bool isConstZero ();
    bool checkConstExpr (ValDecl **exprval_r);
    int getIntConPowOf2 ();
    char *isNumOrAsmConstExpr ();
    VarDecl *newTempVarDecl (Type *type);
    Prim *stackdMemberInit (Prim *prevprim, ValDecl *ptrvar, tsize_t memofs, Type *memtype, bool memisarray, tsize_t memarrsize, Expr *expr);
};

struct ArrayInitExpr : Expr {
    ArrayInitExpr (Scope *exprscope, Token *exprtoken);

    virtual Type *getType ();
    virtual void dumpexpr ();
    virtual tsize_t outputStaticInit (Type *vartype);
    virtual Prim *outputStackdInit (Prim *prevprim, ValDecl *valdecl, bool isarray, tsize_t arrsize);
    virtual Prim *generexpr (Prim *prevprim, ValDecl **exprval_r);

    Type *valuetype;
    tsize_t numarrayelems;
    std::vector<std::pair<tsize_t,Expr *>> exprs;  // pairs of (index,value)
};

struct AsnopExpr : Expr {
    AsnopExpr (Scope *exprscope, Token *exprtoken);

    Expr *leftexpr;
    Expr *riteexpr;

    virtual Type *getType ();
    virtual void dumpexpr ();
    virtual Prim *generexpr (Prim *prevprim, ValDecl **exprval_r);
    virtual NumCat isNumConstExpr (NumValue *nv_r) { return NC_NONE; }
};

struct BinopExpr : Expr {
    BinopExpr (Scope *exprscope, Token *exprtoken);

    Opcode opcode;
    Expr *leftexpr;
    Expr *riteexpr;
    Type *restype;

    virtual BinopExpr *castBinopExpr () { return this; }

    virtual Type *getType () { return restype; }
    virtual void dumpexpr ();
    virtual Prim *generexpr (Prim *prevprim, ValDecl **exprval_r);
    virtual NumCat isNumConstExpr (NumValue *nv_r);
    virtual char *isAsmConstExpr ();

private:
    Prim *genshortcirc (Prim *prevprim, ValDecl **exprval_r, bool defval, char const *lblname);
};

struct BlockExpr : Expr {
    BlockExpr (Scope *exprscope, Token *exprtoken);

    BlockScope *blockscope;
    std::vector<Stmt *> topstmts;
    ExprStmt *laststmt;

    virtual Type *getType ();
    virtual void dumpexpr ();
    virtual Prim *generexpr (Prim *prevprim, ValDecl **exprval_r);
    virtual NumCat isNumConstExpr (NumValue *nv_r);
    virtual tsize_t getExprSize ();
    virtual char *isAsmConstExpr ();
    virtual char *getConstantAddr ();
};

struct CallExpr : Expr {
    CallExpr (Scope *exprscope, Token *exprtoken);

    Expr *funcexpr;
    std::vector<Expr *> argexprs;

    virtual Type *getType ();   // gets the return type, ie, the value of this expression
    virtual void dumpexpr ();
    virtual Prim *generexpr (Prim *prevprim, ValDecl **exprval_r);
    //TODO maybe some builtins return constant virtual NumCat isNumConstExpr (NumValue *nv_r);
};

struct CastExpr : Expr {
    CastExpr (Scope *exprscope, Token *exprtoken);

    virtual CastExpr *castCastExpr () { return this; }

    Type *casttype;
    Expr *subexpr;

    virtual Type *getType ();
    virtual void dumpexpr ();
    virtual Prim *generexpr (Prim *prevprim, ValDecl **exprval_r);
    virtual NumCat isNumConstExpr (NumValue *nv_r);
    virtual char *isAsmConstExpr ();
};

// accessing a member function
//   pointer->[memstruct::]memfunc (...)
//                            ^-we are here
struct MFuncExpr : Expr {
    MFuncExpr (Scope *exprscope, Token *exprtoken);

    virtual MFuncExpr *castMFuncExpr () { return this; }

    virtual Type *getType ();
    virtual void dumpexpr ();
    virtual Prim *generexpr (Prim *prevprim, ValDecl **exprval_r);
    Prim *lookupvirtualfunction (Prim *prevprim, CallPrim *cp);

    Expr *ptrexpr;          // pointer expression, ie, 'this' value passed to member function
    StructType *memstruct;  // struct it was found in (might be a parent)
    FuncDecl *memfunc;      // member function being accessed
    bool ignvirt;           // false: call func via vtable
                            //  true: bypass vtable, call func directly
};

struct QMarkExpr : Expr {
    QMarkExpr (Scope *exprscope, Token *exprtoken);

    Expr *condexpr;
    Expr *trueexpr;
    Expr *falsexpr;

    virtual Type *getType ();
    virtual void dumpexpr ();
    virtual Prim *generexpr (Prim *prevprim, ValDecl **exprval_r);
    virtual NumCat isNumConstExpr (NumValue *nv_r);
};

struct SizeofExpr : Expr {
    SizeofExpr (Scope *exprscope, Token *exprtoken);

    Opcode opcode;  // OP_ALIGNOF, OP_SIZEOF
    Expr *subexpr;
    Type *subtype;

    virtual Type *getType ();
    virtual void dumpexpr ();
    virtual Prim *generexpr (Prim *prevprim, ValDecl **exprval_r);
    virtual NumCat isNumConstExpr (NumValue *nv_r);
};

struct StringInitExpr : Expr {
    StringInitExpr (Scope *exprscope, Token *exprtoken);

    virtual Type *getType ();
    virtual void dumpexpr ();
    virtual tsize_t outputStaticInit (Type *vartype);
    virtual Prim *outputStackdInit (Prim *prevprim, ValDecl *valdecl, bool isarray, tsize_t arrsize);
    virtual Prim *generexpr (Prim *prevprim, ValDecl **exprval_r);

    StrToken *strtok;
    tsize_t arrsize;
};

// fills in values given by ... in 'structtype structvar = { ... };' declaration
// also fills in vtable pointers and anything given by struct constructor
struct StructInitExpr : Expr {
    StructInitExpr (Scope *exprscope, Token *exprtoken);

    virtual Type *getType ();
    virtual void dumpexpr ();
    virtual tsize_t outputStaticInit (Type *vartype);
    virtual Prim *outputStackdInit (Prim *prevprim, ValDecl *valdecl, bool isarray, tsize_t arrsize);
    virtual Prim *generexpr (Prim *prevprim, ValDecl **exprval_r);

    StructType *structype;
    std::map<tsize_t,Expr *> exprs;  // pairs of (index,value)
};

struct UnopExpr : Expr {
    UnopExpr (Scope *exprscope, Token *exprtoken);

    Opcode opcode;
    Expr *subexpr;
    Type *restype;

    virtual UnopExpr *castUnopExpr () { return this; }

    virtual Type *getType () { return restype; }
    virtual void dumpexpr ();
    virtual Prim *generexpr (Prim *prevprim, ValDecl **exprval_r);
    virtual NumCat isNumConstExpr (NumValue *nv_r);
    virtual char *isAsmConstExpr ();
};

struct ValExpr : Expr {
    static ValExpr *createnumconst (Scope *exprscope, Token *exprtoken, NumType *nt, NumCat nc, NumValue nv);
    static ValExpr *createsizeint (Scope *exprscope, Token *exprtoken, tsize_t value, bool sined);

    ValExpr (Scope *exprscope, Token *exprtoken, ValDecl *valdecl);

    ValDecl *valdecl;

    virtual ValExpr *castValExpr () { return this; }

    virtual Type *getType ();
    virtual void dumpexpr ();
    virtual Prim *generexpr (Prim *prevprim, ValDecl **exprval_r);
    virtual NumCat isNumConstExpr (NumValue *nv_r);
    virtual tsize_t getExprSize ();
    virtual char *isAsmConstExpr ();
    virtual char *getConstantAddr ();
};

#endif
