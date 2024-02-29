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

#include <math.h>
#include <assert.h>
#include <string.h>

#include "decl.h"
#include "error.h"

Decl::Decl (char const *name, Scope *scope, Token *deftok)
{
    assert (name   != nullptr);
    assert (deftok != nullptr);
    this->name   = name;
    this->scope  = scope;
    this->deftok = deftok;
    if (this->scope != nullptr) {
        this->scope->enterdecl (this);
    }
}

ValDecl::ValDecl (char const *name, Scope *scope, Token *deftok, Type *type, Keywd storclass)
    : Decl (name, scope, deftok)
{
    assert (type != nullptr);
    assert ((storclass == KW_ENUM) || (storclass == KW_EXTERN) || (storclass == KW_NONE) || (storclass == KW_STRUCT) ||
                (storclass == KW_STATIC) || (storclass == KW_TYPEDEF) || (storclass == KW_VIRTUAL));
    this->type      = type;
    this->storclass = storclass;
    this->enctype   = nullptr;
    this->encname   = nullptr;
}

void ValDecl::setEncType (StructType *enctype)
{
    assert (this->enctype == nullptr);
    this->enctype = enctype;
}

StructType *ValDecl::getEncType ()
{
    return this->enctype;
}

char const *ValDecl::getEncName ()
{
    if (enctype == nullptr) return this->getName ();
    if (encname == nullptr) {
        asprintf (&encname, "%s::%s", enctype->getName (), this->getName ());
    }
    return encname;
}

tsize_t ValDecl::getValSize (Token *errtok)
{
    return getType ()->getTypeSize (errtok);
}

Mach *ValDecl::ldaIntoRn (MachFile *mf, Token *tok, Reg *rn, tsize_t raoffs)
{
    throwerror (tok, "cannot take address of operand");
}

Mach *ValDecl::loadIntoRn (MachFile *mf, Token *tok, Reg *rn, tsize_t raoffs)
{
    throwerror (tok, "cannot be loaded into a register");
}

Mach *ValDecl::storeFromRn (MachFile *mf, Token *tok, Reg *rn, tsize_t raoffs)
{
    throwerror (tok, "cannot be stored from a register");
}

FuncDecl::FuncDecl (char const *name, Scope *scope, Token *deftok, FuncType *type, Keywd storclass, std::vector<VarDecl *> *params, ParamScope *paramscope)
    : ValDecl (name, scope, deftok, type->getConstType (), storclass)
{
    assert (params != nullptr);
    assert (paramscope != nullptr);
    this->params     = params;
    this->paramscope = paramscope;
    this->funcbody   = nullptr;
    this->purevirt   = false;
    this->entryprim  = nullptr;
    this->machfile   = nullptr;
    this->breaklbl   = nullptr;
    this->contlbl    = nullptr;
    this->retaddr    = nullptr;
    this->retlabel   = nullptr;
    this->retvalue   = nullptr;
    this->thisptr    = nullptr;
    this->stacksize  = 0;
    paramscope->funcdecl = this;
}

FuncType *FuncDecl::getFuncType ()
{
    return getType ()->stripCVMod ()->castFuncType ();
}

void FuncDecl::dumpdecl ()
{
    switch (getStorClass ()) {
        case KW_EXTERN:  printf ("extern ");  break;
        case KW_STATIC:  printf ("static ");  break;
        case KW_TYPEDEF: printf ("typedef "); break;
        case KW_VIRTUAL: printf ("virtual "); break;
        case KW_NONE: break;
        default: assert_false ();
    }
    FuncType *functype = getFuncType ();
    functype->getRetType ()->dumpdecl ();
    printf (" %s (", getEncName ());
    bool first = true;
    for (VarDecl *param : *params) {
        if (! first) printf (", ");
        param->dumpdecl ();
        first = false;
    }
    printf (")");
    if (funcbody != nullptr) {
        printf (" ");
        funcbody->dumpstmt (0);
    }
}

// get compile-time constant string
//  the value of a function is the address of the function
//  suitable for assembly-language '.word value' or 'ldw %r0,#value'
char *FuncDecl::isAsmConstExpr ()
{
    return strdup (getEncName ());
}

// create number declaration of the given value
NumDecl *NumDecl::createsizeint (tsize_t val, bool sined)
{
    if (sined) {
        NumValue nv;
        nv.s = val;
        return createtyped (intdeftok, ssizetype, NC_SINT, nv);
    } else {
        NumValue nv;
        nv.u = val;
        return createtyped (intdeftok, sizetype, NC_UINT, nv);
    }
}

// create typed number declaration of the given value
// if type null, get smallest type that holds value
NumDecl *NumDecl::createtyped (Token *deftok, Type *type, NumCat nc, NumValue nv)
{
    Type *typenc = (type == nullptr) ? nullptr : type->stripCVMod ();
    char *name;
    switch (nc) {
        case NC_FLOAT: {
            if (typenc != nullptr) {
                FloatType *ft = typenc->castFloatType ();
                assert (ft != nullptr);
            } else {
                typenc = ((double) (float) nv.f == nv.f) ? flt32type : flt64type;
            }
            asprintf (&name, "(%s)%A", typenc->getName (), nv.f);
            break;
        }
        case NC_SINT: {
            if (typenc != nullptr) {
                if (sizetype->getSign ()) {
                    PtrType *pt = typenc->castPtrType ();
                    if (pt != nullptr) goto goodsint;
                }
                IntegType *it = typenc->castIntegType ();
                assert (it != nullptr);
                assert (it->getSign ());
            } else {
                typenc = ((int64_t) (int8_t)  nv.s == nv.s) ? int8type  :
                         ((int64_t) (int16_t) nv.s == nv.s) ? int16type :
                         ((int64_t) (int32_t) nv.s == nv.s) ? int32type : int64type;
            }
        goodsint:;
            asprintf (&name, "(%s)%lld", typenc->getName (), (long long) nv.s);
            break;
        }
        case NC_UINT: {
            if (typenc != nullptr) {
                if (! sizetype->getSign ()) {
                    PtrType *pt = typenc->castPtrType ();
                    if (pt != nullptr) goto gooduint;
                }
                IntegType *it = typenc->castIntegType ();
                assert (it != nullptr);
                assert (! it->getSign ());
            } else {
                typenc = ((uint64_t) (uint8_t)  nv.u == nv.u) ? uint8type  :
                         ((uint64_t) (uint16_t) nv.u == nv.u) ? uint16type :
                         ((uint64_t) (uint32_t) nv.u == nv.u) ? uint32type : uint64type;
            }
        gooduint:;
            asprintf (&name, "(%s)%lluU", typenc->getName (), (unsigned long long) nv.u);
            break;
        }
        default: assert_false ();
    }

    Decl *d = globalscope->lookupdecl (name);
    if (d != nullptr) {
        free (name);
        NumDecl *nd = d->castNumDecl ();
        assert (nd != nullptr);
        assert (nd->numcat == nc);
        assert (nd->getType ()->stripCVMod () == typenc);
        switch (nc) {
            case NC_FLOAT: assert ((isnan (nd->numval.f) && isnan (nv.f)) || (nd->numval.f == nv.f)); break;
            case NC_SINT:  assert (nd->numval.s == nv.s); break;
            case NC_UINT:  assert (nd->numval.u == nv.u); break;
            default: assert_false ();
        }
        return nd;
    }

    NumDecl *nd = new NumDecl (name, deftok, typenc->getConstType ());
    nd->numcat = nc;
    nd->numval = nv;
    return nd;
}

// create instance if no such constant already exists
// or get current existing one
NumDecl *NumDecl::create (NumToken *numtok)
{
    char const *name = numtok->getName ();
    Decl *d = globalscope->lookupdecl (name);
    if (d != nullptr) {
        NumDecl *nd = d->castNumDecl ();
        assert (nd != nullptr);
        return nd;
    }

    NumDecl *nd = new NumDecl (name, numtok, numtok->getNumType ());
    nd->numcat = numtok->getNumCat ();
    nd->numval = numtok->getValue ();
    return nd;
}

NumDecl::NumDecl (char const *name, Token *deftok, Type *type)
    : ValDecl (name, globalscope, deftok, type->getConstType (), KW_NONE)
{
    numcat = NC_NONE;
    asmlabel[0] = 0;
}

void NumDecl::dumpdecl ()
{
    printf ("%s", getName ());
}

NumCat NumDecl::isNumConstVal (NumValue *nv_r)
{
    *nv_r = numval;
    return numcat;
}

// get compile-time constant string
//  so it is just the digits suitable for .word, .float, etc
//  for word integers, suitable for assembly-language 'ldw %r0,#value'
char *NumDecl::isAsmConstExpr ()
{
    char *buf;
    switch (numcat) {
        case NC_FLOAT: asprintf (&buf, "%A",  numval.f); break;
        case NC_SINT:  asprintf (&buf, "%lld", (long long) numval.s); break;
        case NC_UINT:  asprintf (&buf, "%llu", (unsigned long long) numval.u); break;
        default: assert_false ();
    }
    return buf;
}

// create instance if no such constant already exists
// or get current existing one
StrDecl *StrDecl::create (StrToken *strtok)
{
    char const *name = strtok->getName ();
    Decl *d = globalscope->lookupdecl (name);
    StrDecl *sd;
    if (d == nullptr) {
        sd = new StrDecl (name, globalscope, strtok);
        fprintf (sfile, "; --------------\n");
        fprintf (sfile, "\t.psect\t%s\n", TEXTPSECT);
        char *strlbl = sd->isAsmConstExpr ();
        fprintf (sfile, "%s: .asciz %s\n", strlbl, name);
        free (strlbl);
    } else {
        sd = d->castStrDecl ();
        assert (sd != nullptr);
    }
    return sd;
}

StrDecl::StrDecl (char const *name, Scope *scope, StrToken *strtok)
    : ValDecl (name, scope, strtok, chartype->getConstType ()->getPtrType ()->getConstType (), KW_NONE)
{
    static tsize_t strseqno = 100;
    this->strtok = strtok;
    this->seqno = ++ strseqno;
}

tsize_t StrDecl::getValSize (Token *errtok)
{
    return strtok->getStrlen () + 1;
}

void StrDecl::dumpdecl ()
{
    printf ("%s", getName ());
}

// get compile-time constant string
//  the value of a string is the address of the string
//  ...so it is the label of the .asciz directive
//  suitable for assembly-language '.word value' or 'ldw %r0,#value'
char *StrDecl::isAsmConstExpr ()
{
    char *buf;
    asprintf (&buf, "str.%u", seqno);
    return buf;
}

AsmDecl::AsmDecl (char const *name, Token *deftok, Type *type)
    : ValDecl (name, nullptr, deftok, type, KW_STATIC)
{ }

char *AsmDecl::isAsmConstExpr ()
{
    return strdup (getName ());
}

void AsmDecl::dumpdecl ()
{
    printf ("%s", getName ());
}

EnumDecl::EnumDecl (char const *name, Scope *scope, Token *deftok, Type *type, int64_t value)
    : ValDecl (name, scope, deftok, type, KW_ENUM)
{
    this->value = value;
}

void EnumDecl::dumpdecl ()
{
    printf ("%s", getName ());
}

NumCat EnumDecl::isNumConstVal (NumValue *nv_r)
{
    nv_r->s = value;
    return NC_SINT;
}

char *EnumDecl::isAsmConstExpr ()
{
    char *buf;
    asprintf (&buf, "%lld", (long long) value);
    return buf;
}

VarDecl::VarDecl (char const *name, Scope *scope, Token *deftok, Type *type, Keywd storclass)
    : ValDecl (name, scope, deftok, type, storclass)
{
    assert (type->stripCVMod ()->castFuncType () == nullptr); // use FuncDecl instead

    outspoffssym = false;
    iscallarg = nullptr;
    isarray   = false;
    initexpr  = nullptr;
    locn      = nullptr;
    arrsize   = 0;
}

void VarDecl::setArrSize (tsize_t arrsize)
{
    // type should be something like 'char * const'
    // 'const' cuz it's a pointer that cannot be written,
    // ie,
    //  char buf[80];
    //  buf = "abc";
    //  ...doesn't work
    assert (getType ()->isConstType ());
    assert (getType ()->stripCVMod ()->castPtrType () != nullptr);

    // remember it is an array and how big it is
    this->isarray = true;
    this->arrsize = arrsize;
}

bool VarDecl::isStaticMem ()
{
    // mirrors what Scope::allocscope() does
    return (this->getScope () == globalscope) || (this->getStorClass () == KW_EXTERN) || (this->getStorClass () == KW_STATIC);
}

tsize_t VarDecl::getArrSize ()
{
    assert (this->isarray);
    return this->arrsize;
}

tsize_t VarDecl::getValSize (Token *errtok)
{
    Type *t = getType ()->stripCVMod ();
    return isarray ? t->castPtrType ()->getBaseType ()->getTypeSize (errtok) * arrsize : t->getTypeSize (errtok);
}

tsize_t VarDecl::getVarAlign (Token *errtok)
{
    Type *t = getType ()->stripCVMod ();
    return isarray ? t->castPtrType ()->getBaseType ()->getTypeAlign (errtok) : t->getTypeAlign (errtok);
}

// get variable's compile-time constant value
// - returns static/global array address suitable for a .word
//   everything else is nullptr cuz the variable's value is not an compile-time constant
//   suitable for assembly-language '.word value' or 'ldw %r0,#value'
char *VarDecl::isAsmConstExpr ()
{
    if (isArray ()) {
        // get variable's location
        // - stack/register-based vars may not have any assignment yet
        Locn *vl = getVarLocn ();
        if (vl != nullptr) {
            LabelLocn *ll = vl->castLabelLocn ();
            if (ll != nullptr) {
                return strdup (ll->label);
            }
        }
    }
    return nullptr;
}

// get variable's compile-time constant address
// - returns nullptr for stack/register-based variables
//   returns compile-time label for static/global variables
// example:
//   static SomeStruct *firststruct;
//   static SomeStruct **laststruct = &firststruct;
//     gets the 'firststruct' for a '.word firststruct'
char *VarDecl::getConstantAddr ()
{
    if (locn == nullptr) return nullptr;
    LabelLocn *ll = locn->castLabelLocn ();
    if (ll == nullptr) return nullptr;
    return strdup (ll->label);
}

void VarDecl::dumpdecl ()
{
    switch (getStorClass ()) {
        case KW_EXTERN:  printf ("extern ");  break;
        case KW_STATIC:  printf ("static ");  break;
        case KW_TYPEDEF: printf ("typedef "); break;
        case KW_VIRTUAL: printf ("virtual "); break;
        case KW_NONE: break;
        default: assert_false ();
    }
    if (isArray ()) {
        getType ()->stripCVMod ()->castPtrType ()->getBaseType ()->dumpdecl ();
        printf (" %s[%u]", getName (), arrsize);
    } else {
        getType ()->dumpdecl ();
        printf (" %s", getName ());
    }
}

// subvaldecl is a pointer variable
// this is the value being pointed to
PtrDecl::PtrDecl (Scope *scope, Token *deftok, ValDecl *subvaldecl)
    : ValDecl (getptrdeclname (subvaldecl), scope, deftok, getptrdecltype (deftok, subvaldecl), KW_NONE)
{
    this->subvaldecl = subvaldecl;
}
char const *PtrDecl::getptrdeclname (ValDecl *subvaldecl)
{
    static uint32_t ptrnum = 100;

    char *ptrname;
    asprintf (&ptrname, "<ptr.%u:*%s>", ++ ptrnum, subvaldecl->getName ());
    return ptrname;
}
Type *PtrDecl::getptrdecltype (Token *deftok, ValDecl *subvaldecl)
{
    Type *type = subvaldecl->getType ();
    PtrType *ptrtype = type->stripCVMod ()->castPtrType ();
    if (ptrtype == nullptr) throwerror (deftok, "must be a pointer type '%s'", type->getName ());
    return ptrtype->getBaseType ();
}

void PtrDecl::dumpdecl ()
{
    printf ("*");
    subvaldecl->dumpdecl ();
}

tsize_t PtrDecl::getValSize (Token *errtok)
{
    return getType ()->getTypeSize (errtok);
}

char *PtrDecl::isAsmConstExpr ()
{
    return nullptr;
}

char *PtrDecl::getConstantAddr ()
{
    return nullptr;
}

Type::Type (char const *name, Scope *scope, Token *deftok)
    : Decl (name, scope, deftok)
{
    this->constType = nullptr;
    this->volType   = nullptr;
    this->ptrType   = nullptr;
}

char const *Type::getThrowID (Token *errtok)
{
    printerror (errtok, "type '%s' not throwable, must be a pointer", getName ());
    return "";
}

void Type::dumpdecl ()
{
    printf ("%s", getName ());
}

CVModType *Type::getConstType ()
{
    if (constType == nullptr) {
        char *cname = new char[strlen(this->getName())+7];
        strcpy (cname, this->getName());
        strcat (cname, " const");
        constType = new CVModType (cname, this->getScope (), this->getDefTok (), this, KW_CONST);
    }
    return constType;
}

CVModType *Type::getVolType ()
{
    if (volType == nullptr) {
        char *vname = new char[strlen(this->getName())+10];
        strcpy (vname, this->getName());
        strcat (vname, " volatile");
        volType = new CVModType (vname, this->getScope (), this->getDefTok (), this, KW_VOLATILE);
    }
    return volType;
}

PtrType *Type::getPtrType ()
{
    if (ptrType == nullptr) {
        char *pname = new char[strlen(this->getName())+3];
        strcpy (pname, this->getName ());
        strcat (pname, " *");
        ptrType = new PtrType (pname, this->getScope (), this->getDefTok (), this);
    }
    return ptrType;
}

CVModType::CVModType (char const *name, Scope *scope, Token *deftok, Type *basetype, Keywd cvmod)
    : Type (name, scope, deftok)
{
    assert (basetype != nullptr);
    assert ((cvmod == KW_CONST) || (cvmod == KW_VOLATILE));
    this->basetype = basetype;
    this->cvmod = cvmod;
}

FloatType::FloatType (char const *name, Scope *scope, Token *deftok, int size)
    : NumType (name, scope, deftok)
{
    this->size = size;
    switch (size) {
        case 4: sizechar = 'f'; break;
        case 8: sizechar = 'd'; break;
        default: assert_false ();
    }
}

FuncType *FuncType::create (Token *deftok, Type *rettype, std::vector<VarDecl *> *params)
{
    static std::vector<FuncType *> allfunctypes;
    static uint32_t funcnum;

    // see if we already have this exact same type defined
    // all function types are in a single list for all scopes
    for (FuncType *ft : allfunctypes) {
        if (ft->equals (rettype, params)) return ft;
    }

    // no such function type known, make a new one
    char *functypename = new char[18];
    sprintf (functypename, "<func-%u>", ++ funcnum);

    FuncType *ft = new FuncType (functypename, globalscope, deftok, rettype, params);

    allfunctypes.push_back (ft);

    return ft;
}

FuncType::FuncType (char const *name, Scope *scope, Token *deftok, Type *rettype, std::vector<VarDecl *> *params)
    : Type (name, scope, deftok)
{
    assert (rettype != nullptr);
    assert (params  != nullptr);
    this->rettype    = rettype;
    this->params     = params;
    this->hasvarargs = (params->size () > 0) && (strcmp ((*params)[params->size()-1]->getName(), "__varargs") == 0);
}

tsize_t FuncType::getTypeAlign (Token *errtok)
{
    // align 1 so struct members aren't affected when computing member offsets
    return 1;
}

tsize_t FuncType::getTypeSize (Token *errtok)
{
    // size 0 so struct members don't take any room when computing member offsets
    return 0;
}

bool FuncType::equals (Type *rettype, std::vector<VarDecl *> *params)
{
    if (this->rettype != rettype) return false;
    if (this->params->size () != params->size ()) return false;
    for (int i = this->params->size (); -- i >= 0;) {
        ValDecl *parami   = (*params)[i];
        ValDecl *ftparami = (*this->params)[i];
        if (ftparami->getType () != parami->getType ()) return false;
    }
    return true;
}

bool FuncType::isRetByRef (Token *errtok)
{
    return getRetType ()->getTypeSize (errtok) > RETBYVALSIZE;
}

IntegType::IntegType (char const *name, Scope *scope, Token *deftok, int size, bool sign)
    : NumType (name, scope, deftok)
{
    this->size = size;
    this->sign = sign;
    switch (size) {
        case 1: sizechar = sign ? 'b' : 'B'; break;
        case 2: sizechar = sign ? 'w' : 'W'; break;
        case 4: sizechar = sign ? 'l' : 'L'; break;
        case 8: sizechar = sign ? 'q' : 'Q'; break;
        default: assert_false ();
    }
}

NumType::NumType (char const *name, Scope *scope, Token *deftok)
    : Type (name, scope, deftok)
{ }

PtrType::PtrType (char const *name, Scope *scope, Token *deftok, Type *basetype)
    : Type (name, scope, deftok)
{
    assert (basetype != nullptr);
    this->basetype = basetype;
    this->throwid  = nullptr;
}

tsize_t PtrType::getTypeAlign (Token *errtok)
{
    return sizetype->getTypeAlign (errtok);
}

tsize_t PtrType::getTypeSize (Token *errtok)
{
    return sizetype->getTypeSize (errtok);
}

char const *PtrType::getThrowID (Token *errtok)
{
    if (throwid == nullptr) {
        char const *bnstr = basetype->getName ();
        int bnlen = strlen (bnstr);
        throwid = new char[bnlen*2+10];
        memcpy (throwid, "throwid.", 8);
        int j = 8;
        for (int i = 0; i < bnlen; i ++) {
            char c = bnstr[i];
            switch (c) {
                case ' ': {
                    throwid[j++] = '$';
                    throwid[j++] = 's';
                    break;
                }
                case '*': {
                    throwid[j++] = '$';
                    throwid[j++] = 'a';
                    break;
                }
                case '$': {
                    throwid[j++] = '$';
                    throwid[j++] = 'd';
                    break;
                }
                default: {
                    throwid[j++] = c;
                    break;
                }
            }
        }
        throwid[j] = 0;

        // get list of types a object of this type can be caught by
        std::vector<std::pair<char const *,tsize_t>> subtypeids;
        basetype->gatherThrowSubIDs (errtok, &subtypeids, 0);
        if (basetype != voidtype) {
            subtypeids.push_back ({ voidtype->getPtrType ()->getThrowID (errtok), 0 });
        }

        // output throw type id block
        //  label:
        //      .word   asciztypename
        //      .word   subtypeid,subtypeoffset     repeated for each subtype
        //      .word   0
        char const *basename = basetype->getName ();
        int basenamelen = strlen (basename) + 1;
        fprintf (sfile, "; --------------\n");
        fprintf (sfile, "\t.psect\t.40.%s,OVR\n", throwid);
        fprintf (sfile, "\t.align\t2\n");
        fprintf (sfile, "\t.asciz\t\"%s\"\n", basename);
        if (basenamelen & 1) {
            fprintf (sfile, "\t.byte\t0\n");
            basenamelen ++;
        }
        fprintf (sfile, "\t.global\t%s\n", throwid);
        fprintf (sfile, "%s:\n", throwid);
        fprintf (sfile, "\t.word\t.-%d\n", basenamelen);

        // struct object pointers can be caught by 'subtype *' catches
        // any object pointer can be caught by a 'void *' catch
        for (auto it : subtypeids) {
            fprintf (sfile, "\t.word\t%s,%u\n", it.first, it.second);
        }

        // a thrown pointer of this type doesn't match any other catch types
        fprintf (sfile, "\t.word\t0\n");
        fprintf (sfile, "; --------------\n");
    }
    return throwid;
}

VoidType::VoidType (char const *name, Scope *scope, Token *deftok)
    : Type (name, scope, deftok)
{ }
