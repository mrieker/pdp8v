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
#ifndef _DECL_H
#define _DECL_H

#include <vector>

struct Decl;
struct ValDecl;
struct FuncDecl;
struct NumDecl;
struct StrDecl;
struct AsmDecl;
struct EnumDecl;
struct VarDecl;
struct PtrDecl;
struct Type;
struct CVModType;
struct FloatType;
struct FuncType;
struct IntegType;
struct NumType;
struct PtrType;
struct StructType;
struct VoidType;

typedef std::vector<Decl *> DeclVec;

#include "expr.h"
#include "locn.h"
#include "mach.h"
#include "mytypes.h"
#include "prim.h"
#include "scope.h"
#include "stmt.h"
#include "token.h"

struct Decl {
    Decl (char const *name, Scope *scope, Token *deftok);

    char const *getName () { return name; }
    Scope *getScope () { return scope; }
    Token *getDefTok () { return deftok; }
    void setDefTok (Token *deftok) { this->deftok = deftok; }

    virtual Type *castType () { return nullptr; }
    virtual FloatType *castFloatType () { return nullptr; }
    virtual IntegType *castIntegType () { return nullptr; }
    virtual FuncType *castFuncType () { return nullptr; }
    virtual NumType *castNumType () { return nullptr; }
    virtual PtrType *castPtrType () { return nullptr; }
    virtual StructType *castStructType () { return nullptr; }
    virtual ValDecl *castValDecl () { return nullptr; }
    virtual FuncDecl *castFuncDecl () { return nullptr; }
    virtual NumDecl *castNumDecl () { return nullptr; }
    virtual StrDecl *castStrDecl () { return nullptr; }
    virtual VarDecl *castVarDecl () { return nullptr; }
    virtual PtrDecl *castPtrDecl () { return nullptr; }

    // for builtin types such as '__int8_t': returns this
    // for source-defined types such as struct/union: returns this
    // for typedefs: returns equivalent type
    // all others: returns nullptr
    virtual Type *getEquivType () { return nullptr; }

    virtual void dumpdecl () = 0;

private:
    char const *name;
    Scope *scope;
    Token *deftok;
};

//////////////
//  VALUES  //
//////////////

struct ValDecl : Decl {
    ValDecl (char const *name, Scope *scope, Token *deftok, Type *type, Keywd storclass);

    Type *getType () { return type; }
    Keywd getStorClass () { return storclass; }
    void setStorClass (Keywd sc) { storclass = sc; }

    virtual ValDecl *castValDecl () { return this; }
    virtual Type *getEquivType () { return (storclass == KW_TYPEDEF) ? type : nullptr; }
    virtual tsize_t getValSize (Token *errtok);
    virtual NumCat isNumConstVal (NumValue *nv_r) { return NC_NONE; }
    virtual char *isAsmConstExpr () = 0;
    virtual char *getConstantAddr () { return nullptr; }
    virtual bool isPtrDeclOf (ValDecl *svd) { return false; }
    virtual void replacePtrDeclOf (ValDecl *oldval, ValDecl *newval) { }

    virtual CallMach *callTo (MachFile *mf, Token *errtok);
    virtual Mach *ldaIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs);
    virtual Mach *loadIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs);
    virtual Mach *storeFromRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs);

    void setEncType (StructType *enctype);
    StructType *getEncType ();
    char const *getEncName ();  // for struct members, encname::name, else just name

private:
    Type *type;
    Keywd storclass;            // KW_ENUM, KW_EXTERN, KW_NONE, KW_STATIC, KW_STRUCT, KW_TYPEDEF, KW_VIRTUAL
    StructType *enctype;        // enclosing struct type (FuncDecl or VarDecl only) (or nullptr for non-struct)
    char *encname;
};

struct FuncDecl : ValDecl {
    FuncDecl (char const *name, Scope *scope, Token *deftok, FuncType *type, Keywd storclass, std::vector<VarDecl *> *params, ParamScope *paramscope);

    virtual FuncDecl *castFuncDecl () { return this; }

    Scope *getParamScope () { return paramscope; }
    std::vector<VarDecl *> const *getParams () { return params; }
    Stmt *getFuncBody () { return funcbody; }
    FuncType *getFuncType ();
    bool compatredecl (FuncType *functype, Keywd storclass, std::vector<VarDecl *> *params, ParamScope *paramscope);

    void parseFuncBody ();
    Stmt *parseStatement (Scope *scope, SwitchStmt *outerswitch);
    DeclStmt *parseDeclStmt (Scope *scope);
    ExprStmt *parseExprStmt (Scope *scope);
    Expr *parseParenExpr (Scope *scope);
    LabelStmt *newTempLabelStmt (Scope *scope, Token *tok);

    virtual void dumpdecl ();
    void generfunc ();
    void optprims ();
    void allocfunc ();

    virtual char *isAsmConstExpr ();

    virtual Mach *loadIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs);
    virtual CallMach *callTo (MachFile *mf, Token *errtok);

    void appendMach (Mach *mach);

    bool purevirt;      // virtual ... = 0;

    LabelStmt *breaklbl;    // where to go on a break statement
    LabelStmt *contlbl;     // where to go on a continue statement
    VarDecl *retaddr;       // temp var that holds return address
    LabelStmt *retlabel;    // where to jump for a return statement
    ValDecl *retvalue;      // temp var that holds return value
    VarDecl *thisptr;       // temp var that holds 'this' pointer
    tsize_t stacksize;      // used by stack vars and temps (incl return address, excl parameters)

private:
    std::vector<GotoStmt *> gotostmts;
    std::vector<VarDecl *> *params;     // does not include return-by-ref nor 'this' entries
    ParamScope *paramscope;
    Stmt *funcbody;
    EntryPrim *entryprim;
    MachFile *machfile;

    void swallowsemi ();
};

struct NumDecl : ValDecl {
    static NumDecl *createsizeint (tsize_t val, bool sined);
    static NumDecl *createtyped (Token *deftok, Type *type, NumCat nc, NumValue nv);
    static NumDecl *create (NumToken *token);

    virtual NumDecl *castNumDecl () { return this; }

    virtual void dumpdecl ();
    virtual NumCat isNumConstVal (NumValue *nv_r);
    virtual char *isAsmConstExpr ();

    virtual Mach *ldaIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs);
    virtual Mach *loadIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs);

private:
    NumDecl (char const *name, Token *deftok, Type *type);

    NumCat   numcat;
    NumValue numval;
    char     asmlabel[24];
};

struct StrDecl : ValDecl {
    static StrDecl *create (StrToken *token);

    StrToken *strtok;

    virtual StrDecl *castStrDecl () { return this; }
    virtual tsize_t getValSize (Token *errtok);
    virtual void dumpdecl ();
    virtual char *isAsmConstExpr ();

    virtual Mach *loadIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs);

private:
    StrDecl (char const *name, Scope *scope, StrToken *strtok);
    tsize_t seqno;
};

// value is an assembly language expression
//  eg, at top level:
//   int intarray[10];
//   int main()
//   {
//     int *p = &intarray[3];
//   }
//  this contains name='intarray+6'
//  and its type is type='__int16_t *'
struct AsmDecl : ValDecl {
    AsmDecl (char const *name, Token *deftok, Type *type);

    virtual void dumpdecl ();
    virtual char *isAsmConstExpr ();
    virtual CallMach *callTo (MachFile *mf, Token *errtok);
    virtual Mach *loadIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs);
};

// an enumeration value (not the enum type itself)
struct EnumDecl : ValDecl {
    EnumDecl (char const *name, Scope *scope, Token *deftok, Type *type, int64_t value);

    virtual void dumpdecl ();
    virtual NumCat isNumConstVal (NumValue *nv_r);
    virtual char *isAsmConstExpr ();
    virtual Mach *loadIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs);

private:
    int64_t value;
};

// static/global variables
// stack variables
// function parameters
// typedef definitions
struct VarDecl : ValDecl {
    static VarDecl *newtemp (Scope *scope, Token *deftok, Type *type);

    VarDecl (char const *name, Scope *scope, Token *deftok, Type *type, Keywd storclass);

    void setArrSize (tsize_t arrsize);
    bool isArray () { return isarray; }
    bool isStaticMem ();        // true: extern/global/static; false: stack (incl func params)
    tsize_t getArrSize ();
    tsize_t getVarAlign (Token *errtok);
    void setInitExpr (Expr *initexpr) { this->initexpr = initexpr; }
    Expr *getInitExpr () { return this->initexpr; }
    void allocstaticvar ();
    void outputstaticvar ();

    virtual VarDecl *castVarDecl () { return this; }

    virtual void dumpdecl ();

    void setVarLocn (Locn *locn) { this->locn = locn; }
    Locn *getVarLocn () { return this->locn; }

    virtual tsize_t getValSize (Token *errtok);
    virtual char *isAsmConstExpr ();
    virtual char *getConstantAddr ();

    virtual Mach *ldaIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs);
    virtual Mach *loadIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs);
    virtual Mach *storeFromRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs);

    bool outspoffssym;
    CallPrim *iscallarg;
    uint32_t numvarefs;

private:
    bool isarray;
    Expr *initexpr;
    Locn *locn;
    tsize_t arrsize;
};

struct PtrDecl : ValDecl {
    PtrDecl (Scope *scope, Token *deftok, ValDecl *subvaldecl);

    virtual PtrDecl *castPtrDecl () { return this; }

    virtual void dumpdecl ();

    virtual tsize_t getValSize (Token *errtok);
    virtual char *isAsmConstExpr ();
    virtual char *getConstantAddr ();
    virtual bool isPtrDeclOf (ValDecl *svd) { return (svd == subvaldecl) || subvaldecl->isPtrDeclOf (svd); }
    virtual void replacePtrDeclOf (ValDecl *oldval, ValDecl *newval) { if (subvaldecl == oldval) subvaldecl = newval; }

    virtual CallMach *callTo (MachFile *mf, Token *errtok);
    virtual Mach *ldaIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs);
    virtual Mach *loadIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs);
    virtual Mach *storeFromRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs);

    ValDecl *subvaldecl;

private:
    static char const *getptrdeclname (ValDecl *subvaldecl);
    static Type *getptrdecltype (Token *deftok, ValDecl *subvaldecl);
};

/////////////
//  TYPES  //
/////////////

struct Type : Decl {
    Type (char const *name, Scope *scope, Token *deftok);

    virtual Type *castType () { return this; }
    virtual Type *getEquivType () { return this; }
    virtual Type *stripCVMod () { return this; }

    CVModType *getConstType ();
    CVModType *getVolType ();
    PtrType *getPtrType ();
    virtual bool isConstType () { return false; }
    virtual bool isVolType () { return false; }
    virtual tsize_t getTypeSize (Token *errtok) = 0;
    virtual tsize_t getTypeAlign (Token *errtok) = 0;
    virtual char const *getThrowID (Token *errtok);
    virtual bool isLeafwardOf (Type *roottype, tsize_t *offset_r) { return false; }
    virtual bool isAbstract (Token *errtok, StructType *childtype) { return false; }
    virtual Expr *callInitFunc (Scope *scope, Token *token, Expr *mempointer) { return mempointer; }
    virtual Expr *callTermFunc (Scope *scope, Token *token, Expr *mempointer, bool arrstyle) { return mempointer; }
    virtual void gatherThrowSubIDs (Token *errtok, std::vector<std::pair<char const *,tsize_t>> *subtypeids, tsize_t offset) { }

    virtual void dumpdecl ();

private:
    CVModType *constType;   // this type with ' const' suffix
    CVModType *volType;     // this type with ' volatile' suffix
    PtrType *ptrType;       // this type with ' *' suffix
};

// name: '<basetypename> const' or '<basetypename> volatile'
struct CVModType : Type {
    CVModType (char const *name, Scope *scope, Token *deftok, Type *basetype, Keywd cvmod);

    Type *getBaseType () { return basetype; }
    Type *stripCVMod () { return basetype->stripCVMod (); }

    virtual bool isConstType () { return cvmod == KW_CONST; }
    virtual bool isVolType () { return cvmod == KW_VOLATILE; }
    virtual tsize_t getTypeAlign (Token *errtok) { return basetype->getTypeAlign (errtok); }
    virtual tsize_t getTypeSize  (Token *errtok) { return basetype->getTypeSize  (errtok); }
    virtual char const *getThrowID (Token *errtok) { return basetype->getThrowID  (errtok); }

private:
    Type *basetype;
    Keywd cvmod;        // KW_CONST or KW_VOLATILE
};

// note there is only one instance of FuncType for a given set of rettype/paramtypes
struct FuncType : Type {
    static FuncType *create (Token *deftok, Type *rettype, std::vector<VarDecl *> *params);

    Type *getRetType () { return rettype; }
    bool hasVarArgs () { return hasvarargs; }
    tsize_t getNumParams () { return params->size () - hasvarargs; }
    Type *getParamType (tsize_t i) { return params->at (i)->getType (); }
    std::vector<VarDecl *> *getRawParams () { return params; }  // includes __varargs
    bool isRetByRef (Token *errtok);
    virtual tsize_t getTypeAlign (Token *errtok);    // sizetype->getTypeAlign ()
    virtual tsize_t getTypeSize  (Token *errtok);    // sizetype->getTypeSize  ()

    virtual FuncType *castFuncType () { return this; }

private:
    int hasvarargs;
    Type *rettype;
    std::vector<VarDecl *> *params;     // varargs represented by a final 'void __varargs'

    FuncType (char const *name, Scope *scope, Token *deftok, Type *rettype, std::vector<VarDecl *> *params);
    bool equals (Type *rettype, std::vector<VarDecl *> *params);
};

// name: '<basetypename> *'
struct PtrType : Type {
    PtrType (char const *name, Scope *scope, Token *deftok, Type *basetype);

    virtual PtrType *castPtrType () { return this; }

    Type *getBaseType () { return basetype; }
    virtual tsize_t getTypeAlign (Token *errtok);    // sizetype->getTypeAlign ()
    virtual tsize_t getTypeSize  (Token *errtok);    // sizetype->getTypeSize  ()
    virtual char const *getThrowID (Token *errtok);

private:
    Type *basetype;
    char *throwid;
};

struct StructType : Type {
    StructType (char const *name, Scope *scope, Token *deftok, bool unyun);

    virtual StructType *castStructType () { return this; }

    bool isDefined () { return variables != nullptr; }
    bool getUnyun () { return unyun; }
    MemberScope *getMemScope () { return memscope; }
    bool hasinitfunc () { return initfuncdecl != nullptr; }
    virtual tsize_t getTypeAlign (Token *errtok);
    virtual tsize_t getTypeSize  (Token *errtok);
    virtual bool isLeafwardOf (Type *roottype, tsize_t *offset_r);
    virtual Expr *callInitFunc (Scope *scope, Token *token, Expr *mempointer);
    virtual Expr *callTermFunc (Scope *scope, Token *token, Expr *mempointer, bool arrstyle);
    virtual bool isAbstract (Token *errtok, StructType *childtype);
    virtual void gatherThrowSubIDs (Token *errtok, std::vector<std::pair<char const *,tsize_t>> *subtypeids, tsize_t offset);
    tsize_t getNumMembers ();
    bool getMemberByName (Token *errtok, char const *parenttype, char const *name, StructType **memstruct_r, FuncDecl **memfuncdecl_r, VarDecl **memvardecl_r, tsize_t *memoffset_r, tsize_t *memindex_r);
    bool getMemberByIndex (Token *errtok, tsize_t index, StructType **memstruct_r, VarDecl **memvardecl_r, tsize_t *memoffset_r);
    tsize_t getMemberOffset (Token *errtok, char const *name);  // assert fails if not found
    void setMembers (MemberScope *memscope, std::vector<FuncDecl *> *funcs, std::vector<VarDecl *> *vars);
    bool hasVirtFunc (char const *name);
    tsize_t getExtOffset (int extindex);
    void genInitFunc ();
    void genTermFunc ();
    Prim *genInitCall (Scope *scope, Token *token, Prim *prevprim, ValDecl *valdecl, bool isarray, tsize_t arrsize);
    Expr *getVTablePtr ();
    char *getVTableVarName ();
    VarDecl *getVTableVarDecl ();
    tsize_t fillvtableinit (tsize_t vtindex, StructInitExpr *vtinitexpr);
    bool getVTableLabel (Token *errtok, tsize_t lowoffs, tsize_t highoffs, tsize_t *structoffs_r, tsize_t *vtableoffs_r);

    std::vector<StructType *> exttypes;
    StructType *vtabletype;

private:
    bool unyun;                         // false: struct; true: union
    bool hasconstructor;                // has a constructor declared
    bool hasdestructor;                 // has a destructor declared
    MemberScope *memscope;              // scope the members are declared in
    std::vector<FuncDecl *> *functions;
    std::vector<VarDecl *> *variables;
    FuncDecl *initfuncdecl;
    FuncDecl *termfuncdecl;
    Expr *vtableptr;
    VarDecl *vtablevar;
};

struct VoidType : Type {
    VoidType (char const *name, Scope *scope, Token *deftok);
    virtual tsize_t getTypeAlign (Token *errtok) { return 1; }
    virtual tsize_t getTypeSize  (Token *errtok) { return 0; }
};

struct NumType : Type {
    NumType (char const *name, Scope *scope, Token *deftok);

    virtual NumType *castNumType () { return this; }

    char sizechar;  // Float: f d; Integ/signed: b w l q; Integ/unsigned: B W L Q
};

struct FloatType : NumType {
    FloatType (char const *name, Scope *scope, Token *deftok, int size);

    virtual FloatType *castFloatType () { return this; }

    virtual tsize_t getTypeAlign (Token *errtok) { return (size < MAXALIGN) ? size : MAXALIGN; }
    virtual tsize_t getTypeSize  (Token *errtok) { return size; }

private:
    int size;           // 4 or 8
};

struct IntegType : NumType {
    IntegType (char const *name, Scope *scope, Token *deftok, int size, bool sign);

    virtual IntegType *castIntegType () { return this; }

    virtual tsize_t getTypeAlign (Token *errtok) { return (size < MAXALIGN) ? size : MAXALIGN; }
    virtual tsize_t getTypeSize  (Token *errtok) { return size; }
    bool getSign () { return sign; }

private:
    int size;   // 1, 2, 4 or 8
    bool sign;
};

extern FloatType  *flt32type, *flt64type;
extern FuncDecl   *freefunc, *malloc2func;
extern IntegType  *int8type, *int16type, *int32type, *int64type;
extern IntegType  *uint8type, *uint16type, *uint32type, *uint64type;
extern IntegType  *booltype, *chartype, *inttype, *sizetype, *ssizetype;
extern StructType *trymarktype;
extern tsize_t     trashsize;
extern VarDecl    *trystackvar;
extern VoidType   *voidtype;

#endif
