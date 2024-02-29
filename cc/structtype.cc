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

/////////////////////
//  struct layout  //
/////////////////////

/*
    leaf comes first
    vtable pointer at the beginning (hasVTable() is true)

    __VTable_childtype  const *__vtable;    // virtual functions for child, followed by parent1 then parent2
    child variables

    __VTable_parent1type const *__vtable;   // virtual functions for parent1 as overidden by child within __VTable_childtype
    parent1 variables

    __VTable_parent2type const *__vtable;   // virtual functions for parent2 as overidden by child within __VTable_childtype
    parent2 variables

        .
        .
        .

vtable format:
    void (*child_f1) (...);
    int (*child_f2) (...);
        .
        .
        .
    void (*parent1_f1) (...);
    int (*parent1_f2) (...);
        .
        .
        .
    void (*parent2_f1) (...);
    int (*parent2_f2) (...);
        .
        .
        .


Example source code:

    struct Stmt {
        Stmt *nextstmt;
        virtual void execute ();
    };

    struct LoopStmt : Stmt {
        LabelStmt *breaklbl;
        LabelStmt *contlbl;
        virtual void execure ();
    };

    struct DoStmt : LoopStmt {
        Expr *condexpr;
        virtual void execute ();
    };


Internally we get:

    struct __VTable_Stmt {
        void (*execute) ();
    } const __vtable_Stmt = {
        Stmt::execute
    };

    struct Stmt {
        __VTable_Stmt const *__vtable;  // = &__vtable_Stmt
        Stmt *nextstmt;
    };


    struct __VTable_LoopStmt {
        void (*execute) ();
        struct __VTable_Stmt;
    } const __vtable_DoStmt = {
        LoopStmt::execute,
        LoopStmt::execute::Stmt
    };

    struct LoopStmt {
        __VTable_LoopStmt const *__vtable;  // = &__vtable_LoopStmt
        LabelStmt *breaklbl;
        LabelStmt *contlbl;
        struct Stmt;                        // = &__vtable_LoopStmt.Stmt
    };


    struct __VTable_DoStmt {
        void (*execute) ();
        struct __VTable_LoopStmt;
    } const __vtable_DoStmt = {
        DoStmt::execute,
        DoStmt::execute::LoopStmt,          // overrides LoopStmt::execute()
        DoStmt::execute::LoopStmt::Stmt     // overrides Stmt::execute()
    };

    struct DoStmt : LoopStmt {
        __VTable_DoStmt const *__vtable;    // = &__vtable_DoStmt
        Expr *condexpr;
        struct LoopStmt;                    // = &__vtable_DoStmt.LoopStmt
                                            // = &__vtable_DoStmt.LoopStmt.Stmt
    };


    DoStmt::execute::LoopStmt::Stmt:        // R2 points to Stmt struct within struct DoStmt
        LDA R2,-6(R2)                       //  back up over contlbl,breaklbl,__vtable at beg of LoopStmt
    DoStmt::execute::LoopStmt:              // R2 points to LoopStmt struct within struct DoStmt
        LDA R2,-4(R2)                       //  back up over condexpr,__vtable at beg of DoStmt
    DoStmt::execute:                        // R2 points to struct DoStmt
        LDA R6,-x(R6)
        STW R2,this(R6)
            .
            .
            .
*/

#include "decl.h"
#include "error.h"

StructType::StructType (char const *name, Scope *scope, Token *deftok, bool unyun)
    : Type (name, scope, deftok)
{
    this->unyun        = unyun;
    this->memscope     = nullptr;
    this->functions    = nullptr;
    this->variables    = nullptr;
    this->initfuncdecl = nullptr;
    this->termfuncdecl = nullptr;
    this->vtableptr    = nullptr;
    this->vtablevar    = nullptr;
}

// set up the function members
//  input:
//   memscope = scope the members were declared in
//   funcs = member functions (normal, static, virtual) (not including function pointers)
//   vars  = member variables (includes function pointers)
// assumes exttypes is already filled in with parent types
static std::vector<FuncDecl *> nullvtablefuncs;
void StructType::setMembers (MemberScope *memscope, std::vector<FuncDecl *> *funcs, std::vector<VarDecl *> *vars)
{
    this->memscope  = memscope;
    this->functions = funcs;
    this->variables = vars;

    // mark all the members as being part of this struct type
    for (FuncDecl *f : *funcs) f->setEncType (this);
    for (VarDecl  *v : *vars)  v->setEncType (this);

    // see if any parent has __init() or __term() functions
    bool parentinit = false;
    bool parentterm = false;
    for (StructType *exttype : exttypes) {
        if (exttype->initfuncdecl != nullptr) parentinit = true;
        if (exttype->termfuncdecl != nullptr) parentterm = true;
    }

    // see if there is a constructor or any virtual functions
    hasconstructor = false;
    hasdestructor  = false;
    bool hasvfuncs = false;
    for (FuncDecl *f : *funcs) {
        if (strcmp (f->getName (), "__ctor") == 0) hasconstructor = true;
        if (strcmp (f->getName (), "__dtor") == 0) hasdestructor  = true;
        if (f->getStorClass () == KW_VIRTUAL) hasvfuncs = true;
    }

    // having a destructor means this one needs an __term() function
    if (hasdestructor | parentterm) {

        // set up parameter:
        //  (tsize_t numelem)
        ParamScope *paramscope = new ParamScope (globalscope);
        std::vector<VarDecl *> *termfuncparams = new std::vector<VarDecl *> ();
        termfuncparams->push_back (new VarDecl ("numelem", paramscope, this->getDefTok (), sizetype, KW_NONE));

        // create function type
        //  void * (tsize_t numelem)
        FuncType *termfunctype = FuncType::create (this->getDefTok (), voidtype->getPtrType (), termfuncparams);

        // declare function as a virtual struct member
        termfuncdecl = new FuncDecl ("__term", memscope, this->getDefTok (), termfunctype, KW_VIRTUAL, termfuncparams, paramscope);
        termfuncdecl->setEncType (this);

        // add it to struct's list of functions
        functions->push_back (termfuncdecl);

        hasvfuncs = true;
    }

    // virtual functions means it has a vtable
    if (hasvfuncs) {

        // create vtable struct definition
        char *vtablename;
        asprintf (&vtablename, "__VTable_%s", this->getName ());
        this->vtabletype = new StructType (vtablename, this->getScope (), this->getDefTok (), false);

        // it inherits from the vtables from all of main struct parents
        for (StructType *pt : this->exttypes) {
            if (pt->vtabletype != nullptr) {
                this->vtabletype->exttypes.push_back (pt->vtabletype);
            }
        }

        // fill in member variables for child vtable
        MemberScope *vtscope = new MemberScope (this->getScope ());
        std::vector<VarDecl *> *vtablevars = new std::vector<VarDecl *> ();

        // - put in a 'functype *funcname' member for each virtual function
        for (FuncDecl *f : *funcs) {
            if (f->getStorClass () == KW_VIRTUAL) {
                VarDecl *vtablefunc = new VarDecl (f->getName (), vtscope, this->getDefTok (), f->getFuncType ()->getPtrType (), KW_NONE);
                vtablevars->push_back (vtablefunc);
            }
        }

        // fill in vtable members
        this->vtabletype->setMembers (vtscope, &nullvtablefuncs, vtablevars);

        // splice in a 'vtable const *const __vtableptr' on the front of child struct
        VarDecl *vtableptr = new VarDecl ("__vtableptr", this->memscope, this->getDefTok (), this->vtabletype->getConstType ()->getPtrType (), KW_NONE);
        this->variables->insert (this->variables->begin (), vtableptr);
    }

    // having a constructor and/or virtual functions and/or parent with __init(), means this one needs an __init() function
    // the __init() function calls parent __init()s, fills in vtable pointer if any and calls __ctor() if any
    if (hasconstructor | hasvfuncs | parentinit) {

        // set up parameters:
        //  (structtype *pointer, tsize_t numelem, vtabletype *vtablep)
        ParamScope *paramscope = new ParamScope (globalscope);
        std::vector<VarDecl *> *initfuncparams = new std::vector<VarDecl *> ();
        initfuncparams->push_back (new VarDecl ("pointer", paramscope, this->getDefTok (), this->getPtrType (), KW_NONE));
        initfuncparams->push_back (new VarDecl ("numelem", paramscope, this->getDefTok (), sizetype, KW_NONE));
        if (vtabletype != nullptr) {
            initfuncparams->push_back (new VarDecl ("vtablep", paramscope, this->getDefTok (), vtabletype->getConstType ()->getPtrType (), KW_NONE));
        }

        // create function type
        //  structtype * (structtype *pointer, tsize_t numelem, vtabletype *vtablep)
        FuncType *initfunctype = FuncType::create (this->getDefTok (), this->getPtrType (), initfuncparams);

        // declare function as a static struct member
        initfuncdecl = new FuncDecl ("__init", memscope, this->getDefTok (), initfunctype, KW_STATIC, initfuncparams, paramscope);
        initfuncdecl->setEncType (this);

        // add it to struct's list of functions
        functions->push_back (initfuncdecl);
    }
}

// see if this type is identical or is leafward of the given rootward type
// if found, return offset of given root within this leaf type
bool StructType::isLeafwardOf (Type *roottype, tsize_t *offset_r)
{
    if (roottype == this) {
        *offset_r = 0;
        return true;
    }
    int extindex = 0;
    for (StructType *exttype : exttypes) {
        if (exttype->isLeafwardOf (roottype, offset_r)) {
            *offset_r += getExtOffset (extindex);
            return true;
        }
        extindex ++;
    }
    return false;
}

// if struct has an init function, insert a call
// this is used when a struct type variable or array is __malloc2()'d with new
Expr *StructType::callInitFunc (Scope *scope, Token *token, Expr *mempointer)
{
    if (initfuncdecl != nullptr) {

        // get mem pointer into temp cuz mempointer is complicated
        VarDecl *ptrtemp = VarDecl::newtemp (scope, token, this->getPtrType ());
        AsnopExpr *asnptrtemp = new AsnopExpr (scope, token);
        asnptrtemp->leftexpr  = new ValExpr (scope, token, ptrtemp);
        asnptrtemp->riteexpr  = mempointer;

        // get array subscript from CARROFFS on the pointer
        BinopExpr *offsetted = new BinopExpr (scope, token);
        offsetted->opcode    = OP_ADD;
        offsetted->leftexpr  = scope->castToType (token, sizetype->getPtrType (), new ValExpr (scope, token, ptrtemp));
        offsetted->riteexpr  = ValExpr::createsizeint (scope, token, CARROFFS, true);
        offsetted->restype   = sizetype->getPtrType ();
        UnopExpr *arrsubscr  = new UnopExpr (scope, token);
        arrsubscr->opcode    = OP_DEREF;
        arrsubscr->subexpr   = offsetted;

        // call structtype::__init(mempointer,arraysubscript[,&vtable])
        CallExpr *callinit = new CallExpr (scope, token);
        callinit->funcexpr = new ValExpr (scope, token, initfuncdecl);
        callinit->argexprs.push_back (asnptrtemp);
        callinit->argexprs.push_back (arrsubscr);
        if (vtabletype != nullptr) {
            callinit->argexprs.push_back (this->getVTablePtr ());
        }

        // it returns mempointer exactly as passed
        mempointer = callinit;
    }
    return mempointer;
}

// generate code to initialize a stack variable or array
// - fills in vtable pointers
// - calls constructor
// if array, repeats for each element of the array
Prim *StructType::genInitCall (Scope *scope, Token *token, Prim *prevprim, ValDecl *valdecl, bool isarray, tsize_t arrsize)
{
    if (initfuncdecl != nullptr) {

        // set up pointer to variable to be initialized
        ValDecl *addrofval  = VarDecl::newtemp (scope, token, this->getPtrType ());
        UnopPrim *addrofexp = new UnopPrim (scope, token);
        addrofexp->opcode   = OP_ADDROF;
        addrofexp->aval     = valdecl;
        addrofexp->ovar     = addrofval;
        prevprim = prevprim->setLinext (addrofexp);

        // set up pointer to vtable
        ValDecl *vtblptrval = nullptr;
        if (vtabletype != nullptr) {
            vtblptrval = VarDecl::newtemp (scope, token, vtabletype->getConstType ()->getPtrType ());
            UnopPrim *vtaddrof = new UnopPrim (scope, token);
            vtaddrof->opcode   = OP_ADDROF;
            vtaddrof->aval     = this->getVTableVarDecl ();
            vtaddrof->ovar     = vtblptrval;
            prevprim = prevprim->setLinext (vtaddrof);
        }

        // array subscript defaults to 1
        if (! isarray) arrsize = 1;

        // call structtype::__init(mempointer,arraysubscript,&vtable)
        // we don't need the return value
        CallStartPrim *csp = new CallStartPrim (scope, token);
        CallPrim *callprim = new CallPrim (scope, token);
        csp->callprim      = callprim;
        callprim->entval   = initfuncdecl;
        callprim->callstartprim = csp;
        prevprim = prevprim->setLinext  (csp);
        prevprim = callprim->pushargval (prevprim, addrofval);
        prevprim = callprim->pushargval (prevprim, NumDecl::createsizeint (arrsize, false));
        if (vtblptrval != nullptr) {
            prevprim = callprim->pushargval (prevprim, vtblptrval);
        }
        prevprim = prevprim->setLinext  (callprim);
    }
    return prevprim;
}

// if struct has a __term() terminator function, insert a call
// this is used when a struct type variable or array of structs is freed with delete
Expr *StructType::callTermFunc (Scope *scope, Token *token, Expr *mempointer, bool arrstyle)
{
    if (termfuncdecl != nullptr) {
        Expr *numelems;

        // delete[] can be an array or scalar, but it *MUST* be the leafmost type and *MUST* be element[0] if an array
        // delete will work for scalar or 1-element array and can be leafmost or any rootward type of the object
        if (arrstyle) {

            // make a temp to hold mempointer in case mempointer is complicated
            VarDecl *memtempdecl = VarDecl::newtemp (scope, token, this->getPtrType ());
            AsnopExpr *asntemp = new AsnopExpr (scope, token);
            asntemp->leftexpr  = new ValExpr (scope, token, memtempdecl);
            asntemp->riteexpr  = mempointer;

            // cast temp to (__size_t const *)
            Expr *casted = scope->castToType (token, sizetype->getConstType ()->getPtrType (), asntemp);

            // offset the casted pointer to # of array elements as set up by __malloc2()
            BinopExpr *offsptr = new BinopExpr (scope, token);
            offsptr->opcode    = OP_ADD;
            offsptr->leftexpr  = casted;
            offsptr->riteexpr  = ValExpr::createsizeint (scope, token, CARROFFS, true);
            offsptr->restype   = sizetype->getConstType ()->getPtrType ();

            // get number of array elements (or 1 for scalar)
            // this will get incorrect value if not leafmost type or if not element[0]
            UnopExpr *nex = new UnopExpr (scope, token);
            nex->opcode   = OP_DEREF;
            nex->subexpr  = offsptr;
            nex->restype  = sizetype;
            numelems = nex;

            // use temp var for array pointer from now on
            mempointer = new ValExpr (scope, token, memtempdecl);
        } else {

            // assume scalar
            // if a rootward type, its virtual __term() will be overridden with leafmost __term()
            numelems = ValExpr::createsizeint (scope, token, 1, false);
        }

        // mempointer->__term
        MFuncExpr *mfx = new MFuncExpr (scope, token);
        mfx->ptrexpr   = mempointer;
        mfx->memstruct = this;
        mfx->memfunc   = termfuncdecl;

        // call mempointer->__term(numelems)
        CallExpr *callterm = new CallExpr (scope, token);
        callterm->funcexpr = mfx;
        callterm->argexprs.push_back (numelems);

        // it returns mempointer pointing to beginning of struct
        // if deleted from a rootward pointer, its virtual __term() was overridden by leafmost __term()
        // ...which returns the leafmost struct pointer
        mempointer = callterm;
    }
    return mempointer;
}

tsize_t StructType::getTypeAlign (Token *errtok)
{
    if (variables == nullptr) {
        if (errtok != nullptr) printerror (errtok, "struct/union '%s' contents undefined", getName ());
        return 1;
    }

    tsize_t alin = 1;

    for (VarDecl *vd : *variables) {
        tsize_t va = vd->getVarAlign (errtok);
        if (alin < va) alin = va;
    }

    for (StructType *st : exttypes) {
        tsize_t xa = st->getTypeAlign (errtok);
        if (alin < xa) alin = xa;
    }

    assert (alin <= MAXALIGN);
    return alin;
}

tsize_t StructType::getTypeSize (Token *errtok)
{
    if (variables == nullptr) {
        if (errtok != nullptr) printerror (errtok, "struct/union '%s' contents undefined", getName ());
        return 1;
    }

    tsize_t alin = 1;
    tsize_t size = 0;

    for (VarDecl *vd : *variables) {
        tsize_t vs = vd->getValSize (errtok);
        tsize_t va = vd->getVarAlign (errtok);
        if (unyun) {
            if (size < vs) size = vs;
        } else {
            size  = (size + va - 1) & - va;
            size += vs;
        }
        if (alin < va) alin = va;
    }

    for (StructType *st : exttypes) {
        tsize_t xs = st->getTypeSize  (errtok);
        tsize_t xa = st->getTypeAlign (errtok);
        if (alin < xa) alin = xa;
        if (unyun) {
            if (size < xs) size = xs;
        } else {
            size  = (size + xa - 1) & - xa;
            size += xs;
        }
    }

    return (size + alin - 1) & - alin;
}

// get overall number of variable members including parents
// excludes functions
tsize_t StructType::getNumMembers ()
{
    if (variables == nullptr) return 0;

    tsize_t index = variables->size ();

    for (StructType *exttype : exttypes) {
        index += exttype->getNumMembers ();
    }

    return index;
}

// get offset of member variable
// assert fail if not found
tsize_t StructType::getMemberOffset (Token *errtok, char const *name)
{
    StructType *memstruct;
    FuncDecl *memfuncdecl;
    VarDecl *memvardecl;
    tsize_t memoffset, memindex;
    bool ok = getMemberByName (errtok, nullptr, name, &memstruct, &memfuncdecl, &memvardecl, &memoffset, &memindex);
    assert (ok && (memvardecl != nullptr));
    return memoffset;
}

// get member of the struct/union
//  input:
//   parenttype = nullptr: look in this or any rootward struct we inherit from
//                   else: only look in this rootward struct
//   name = name of member to find
//  output:
//   returns false: nothing found
//            true: member found
//                  *memstruct_r   = extension struct found in or 'this' if this child
//                  *memfuncdecl_r = member function declaration or nullptr
//                  *memvardecl_r  = member variable declaration or nullptr
//                                   *memoffset_r = offset of variable member in 'this' struct
//                                   *memindex_r  = index of variable member within 'this' struct
bool StructType::getMemberByName (Token *errtok, char const *parenttype, char const *name, StructType **memstruct_r, FuncDecl **memfuncdecl_r, VarDecl **memvardecl_r, tsize_t *memoffset_r, tsize_t *memindex_r)
{
    if (functions == nullptr) return false;

    tsize_t index  = 0;
    tsize_t offset = 0;

    // search this one first
    // accumulate offset as we search through
    tsize_t alin = this->getTypeAlign (errtok);
    offset = (offset + alin - 1) & - alin;
    for (VarDecl *v : *variables) {
        tsize_t alin = v->getVarAlign (errtok);
        offset = (offset + alin - 1) & - alin;
        if (((parenttype == nullptr) || (strcmp (this->getName (), parenttype) == 0)) && (strcmp (v->getName (), name) == 0)) {
            *memstruct_r   = this;
            *memfuncdecl_r = nullptr;
            *memvardecl_r  = v;
            *memoffset_r   = offset;
            *memindex_r    = index;
            return true;
        }
        index ++;
        if (! this->getUnyun ()) offset += v->getValSize (errtok);
    }

    // functions do not consume any index values or offset space
    for (FuncDecl *f : *functions) {
        if (((parenttype == nullptr) || (strcmp (this->getName (), parenttype) == 0)) && (strcmp (f->getName (), name) == 0)) {
            *memstruct_r   = this;
            *memfuncdecl_r = f;
            *memvardecl_r  = nullptr;
            *memoffset_r   = -1;
            *memindex_r    = -1;
            return true;
        }
    }

    // search extensions, in order of declaration
    // accumulate offset as we search through
    for (StructType *exttype : exttypes) {
        tsize_t alin = exttype->getTypeAlign (errtok);
        offset = (offset + alin - 1) & - alin;
        if (((parenttype == nullptr) || (strcmp (exttype->getName (), parenttype) == 0)) &&
                exttype->getMemberByName (errtok, nullptr, name, memstruct_r, memfuncdecl_r, memvardecl_r, memoffset_r, memindex_r)) {
            *memoffset_r += offset;
            *memindex_r  += index;
            return true;
        }
        index += exttype->getNumMembers ();
        if (! this->getUnyun ()) offset += exttype->getTypeSize (errtok);
    }

    return false;
}

// get variable member of the struct/union
//  input:
//   index = index of variable member as returned by getMemberByName()
//  output:
//   returns false: nothing found
//            true: member found
//                  *memstruct_r  = extension struct found in or 'this' if this child
//                  *memvardecl_r = member variable declaration
//                  *memoffset_r  = offset of member in 'this' struct
bool StructType::getMemberByIndex (Token *errtok, tsize_t index, StructType **memstruct_r, VarDecl **memvardecl_r, tsize_t *memoffset_r)
{
    tsize_t offset = 0;

    // search extensions for the index, stop if found
    // accumulate offset as we search through
    for (StructType *exttype : exttypes) {
        tsize_t alin = exttype->getTypeAlign (errtok);
        offset = (offset + alin - 1) & - alin;
        if (exttype->getMemberByIndex (errtok, index, memstruct_r, memvardecl_r, memoffset_r)) {
            *memoffset_r += offset;
            return true;
        }
        tsize_t n = exttype->getNumMembers ();
        if (n > index) return false;
        index -= n;
        if (! this->getUnyun ()) offset += exttype->getTypeSize (errtok);
    }

    // search this one, override any previous one found
    // accumulate offset as we search through
    tsize_t alin = this->getTypeAlign (errtok);
    offset = (offset + alin - 1) & - alin;
    for (VarDecl *v : *variables) {
        tsize_t alin = v->getVarAlign (errtok);
        offset = (offset + alin - 1) & - alin;
        if (index == 0) {
            *memstruct_r   = this;
            *memvardecl_r  = v;
            *memoffset_r   = offset;
            return true;
        }
        -- index;
        if (! this->getUnyun ()) offset += v->getValSize (errtok);
    }

    // index runs off the end, not found
    return false;
}

// see if this struct directly contains the given virtual function
// do not check inheriteds
bool StructType::hasVirtFunc (char const *name)
{
    if (functions == nullptr) return false;

    for (FuncDecl *f : *functions) {
        if (strcmp (f->getName (), name) == 0) {
            return f->getStorClass () == KW_VIRTUAL;
        }
    }
    return false;
}

// get offset of the given parent within this struct
tsize_t StructType::getExtOffset (int extindex)
{
    assert (extindex >= 0);

    if (this->getUnyun ()) return 0;

    tsize_t offset = 0;

    // accumulate offset to first extension
    tsize_t alin = this->getTypeAlign (nullptr);
    offset = (offset + alin - 1) & - alin;
    for (VarDecl *v : *variables) {
        tsize_t alin = v->getVarAlign (nullptr);
        offset  = (offset + alin - 1) & - alin;
        offset += v->getValSize (nullptr);
    }

    // search extensions, in order of declaration
    // accumulate offset as we search through
    for (StructType *exttype : exttypes) {
        if (-- extindex < 0) return offset;
        tsize_t alin = exttype->getTypeAlign (nullptr);
        offset  = (offset + alin - 1) & - alin;
        offset += exttype->getTypeSize (nullptr);
    }

    assert_false ();
}

// get pointer to vtable
Expr *StructType::getVTablePtr ()
{
    if (vtableptr == nullptr) {
        Scope *scope = getScope ();
        Token *token = getDefTok ();

        UnopExpr *addrof = new UnopExpr (scope, token);
        addrof->opcode   = OP_ADDROF;
        addrof->subexpr  = new ValExpr (scope, token, this->getVTableVarDecl ());
        addrof->restype  = vtabletype->getConstType ()->getPtrType ();
        vtableptr = addrof;
    }
    return vtableptr;
}

// get vtable variable (the vtable itself)
VarDecl *StructType::getVTableVarDecl ()
{
    if ((vtablevar == nullptr) && (vtabletype != nullptr)) {
        Scope *scope = getScope ();
        Token *token = getDefTok ();

        StructInitExpr *vtinitexpr = new StructInitExpr (scope, token);
        vtinitexpr->structype = vtabletype;
        this->fillvtableinit (0, vtinitexpr);

        vtablevar = new VarDecl ("__vtable", memscope, token, vtabletype->getConstType (), KW_STATIC);
        vtablevar->setEncType (vtabletype);
        vtablevar->setInitExpr (vtinitexpr);
        vtablevar->allocstaticvar ();
    }
    return vtablevar;
}

char *StructType::getVTableVarName ()
{
    char *buf;
    asprintf (&buf, "%s::__vtable", vtabletype->getName ());
    return buf;
}

// fill in vtable entries for this struct and any inherited-from structs herein
// performs overriding of function pointers
//  input:
//   vtindex = next index in vtable to fill in
//   vtinitexpr = vtable being filled in
//  output:
//   returns vtindex just past last entry filled in
tsize_t StructType::fillvtableinit (tsize_t vtindex, StructInitExpr *vtinitexpr)
{
    if (functions == nullptr) return vtindex;

    Scope *scope = getScope ();
    Token *token = getDefTok ();

    // fill in the 'functype *funcname' vtable member for each virtual function
    for (FuncDecl *f : *functions) {
        if (f->getStorClass () == KW_VIRTUAL) {
            ValExpr *vx = new ValExpr (scope, token, f);
            auto ok = vtinitexpr->exprs.insert ({ vtindex, vx });
            assert (ok.second);
            vtindex ++;
        }
    }

    for (StructType *exttype : this->exttypes) {

        // fill in parent versions of parent vtable entries
        tsize_t oldindex = vtindex;
        vtindex = exttype->fillvtableinit (vtindex, vtinitexpr);

        // now do any overriding
        for (; oldindex < vtindex; oldindex ++) {
            FuncDecl *vtentrydecl = vtinitexpr->exprs.at (oldindex)->castValExpr ()->valdecl->castFuncDecl ();
            char const *vtentryname = vtentrydecl->getName ();
            FuncType *vtentrytype = vtentrydecl->getFuncType ();
            for (FuncDecl *childfuncdecl : *functions) {
                // function name and type must match exactly (type does not include 'this' type)
                if ((strcmp (vtentryname, childfuncdecl->getName ()) == 0) && (vtentrytype == childfuncdecl->getFuncType ())) {

                    ValExpr *vx = vtinitexpr->exprs.at (oldindex)->castValExpr ();
                    assert (vx != nullptr);
                    FuncDecl *parentfuncdecl = vx->valdecl->castFuncDecl ();
                    assert (parentfuncdecl != nullptr);

                    ////fprintf (stderr, "StructType::fillvtableinit*: %s[%u] %s::%s: function %s::%s being overidden by %s::%s__%s\n",
                    ////            vtinitexpr->structype->getName (), oldindex,
                    ////            exttype->getName (), vtentryname,
                    ////            parentfuncdecl->getEncType ()->getName (), parentfuncdecl->getName (),
                    ////            exttype->getName (), this->getName (), childfuncdecl->getName ());

                    // childfuncdecl has 'this' type of child
                    // it will be called with 'this' type of parent
                    // so we need version of child func that casts 'this' from parent to child
                    // EntMach::dumpmach() generates the casting code as part of the child function,
                    //  so all we need to do is set up a function declaration with name to match it
                    // it is named parenttype::childtype__function

                    char *castingname;
                    asprintf (&castingname, "%s__%s", this->getName (), childfuncdecl->getName ());
                    ParamScope *castingparamscope = new ParamScope (childfuncdecl->getParamScope ()->getOuter ());
                    std::vector<VarDecl *> *castingparams = new std::vector<VarDecl *> ();
                    for (VarDecl *oldparam : *childfuncdecl->getParams ()) {
                        VarDecl *newparam = new VarDecl (oldparam->getName (), castingparamscope, oldparam->getDefTok (),
                                oldparam->getType (), oldparam->getStorClass ());
                        castingparams->push_back (newparam);
                    }
                    FuncDecl *castingfunc = new FuncDecl (castingname, childfuncdecl->getScope (), childfuncdecl->getDefTok (),
                            childfuncdecl->getFuncType (), childfuncdecl->getStorClass (), castingparams, castingparamscope);
                    castingfunc->setEncType (parentfuncdecl->getEncType ());  // set 'this' type
                    vx->valdecl = castingfunc;
                    break;
                }
            }
        }
    }

    return vtindex;
}

// generate initialization function
// fills in vtable pointers
// calls constructor
// repeats for all elements of an array
//  input:
//   pointer = pointer to struct or array of structs
//   numelem = 1: scalar or single-element array
//          else: numer of elements in array
//   vtablep = vtable pointer (can be override from child's vtable)
//  output:
//   returns pointer to scalar or first array element
/*
    structtype *structtype::__init (structtype *pointer, __size_t numelem, __VTable_structtype const *vtablep)
    {
        for (__size_t i = 0; i < numelem; i ++) {
            structtype *s = pointer + i;
            for (__size_t j = 0; j < s->exttypes.size (); j ++)
                    exttype::__init (s, 1, vtablep);
            if (vtabletype != nullptr) s->__vtableptr = vtablep;
            if (hasconstructor) s->__ctor ();
        }
        return pointer;
    }
*/
void StructType::genInitFunc ()
{
    if (initfuncdecl == nullptr) return;
    if (initfuncdecl->getFuncBody () != nullptr) return;

    // push tokens for init function source in reverse order

    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_CBRACE));

    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_SEMI));
    pushtoken (new SymToken (__FILE__, __LINE__, 0, "pointer"));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_RETURN));

    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_CBRACE));

    if (hasconstructor) {
        pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_SEMI));
        pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_CPAREN));
        pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_OPAREN));
        pushtoken (new SymToken (__FILE__, __LINE__, 0, "__ctor"));
        pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_POINT));
        pushtoken (new SymToken (__FILE__, __LINE__, 0, "s"));
    }

    if (vtabletype != nullptr) {
        pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_SEMI));
        pushtoken (new SymToken (__FILE__, __LINE__, 0, "vtablep"));
        pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_EQUAL));
        pushtoken (new SymToken (__FILE__, __LINE__, 0, "__vtableptr"));
        pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_POINT));
        pushtoken (new SymToken (__FILE__, __LINE__, 0, "s"));
    }

    // output code to initialize inherited structs
    // use our vtable overrides to fill in the inherited structs
    // implicit casting of the pointer to rootward type adds the offset as part of the implicit cast
    // likewise for implicit casting of the vtable pointer
    for (StructType *exttype : exttypes) {
        if (exttype->initfuncdecl != nullptr) {
            pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_SEMI));
            pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_CPAREN));
            pushtoken (new SymToken (__FILE__, __LINE__, 0, "vtablep"));
            pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_COMMA));
            NumValue nvone;
            nvone.u = 1;
            pushtoken (new NumToken (__FILE__, __LINE__, 0, NC_UINT, nvone));
            pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_COMMA));
            pushtoken (new SymToken (__FILE__, __LINE__, 0, "s"));
            pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_OPAREN));
            pushtoken (new SymToken (__FILE__, __LINE__, 0, "__init"));
            pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_COLON2));
            pushtoken (new SymToken (__FILE__, __LINE__, 0, exttype->getName ()));
        }
    }

    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_SEMI));
    pushtoken (new SymToken (__FILE__, __LINE__, 0, "i"));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_PLUS));
    pushtoken (new SymToken (__FILE__, __LINE__, 0, "pointer"));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_EQUAL));
    pushtoken (new SymToken (__FILE__, __LINE__, 0, "s"));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_STAR));
    pushtoken (new SymToken (__FILE__, __LINE__, 0, this->getName ()));

    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_OBRACE));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_CPAREN));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_PLUSPLUS));
    pushtoken (new SymToken (__FILE__, __LINE__, 0, "i"));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_SEMI));
    pushtoken (new SymToken (__FILE__, __LINE__, 0, "numelem"));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_LTANG));
    pushtoken (new SymToken (__FILE__, __LINE__, 0, "i"));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_SEMI));
    NumValue nvzero;
    nvzero.u = 0;
    pushtoken (new NumToken (__FILE__, __LINE__, 0, NC_UINT, nvzero));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_EQUAL));
    pushtoken (new SymToken (__FILE__, __LINE__, 0, "i"));
    pushtoken (new SymToken (__FILE__, __LINE__, 0, sizetype->getName ()));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_OPAREN));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_FOR));

    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_OBRACE));

    initfuncdecl->parseFuncBody ();

    // also output the vtable contents if any
    // all structs that have vtables always have __init() functions to fill in the vtable pointer
    // don't output if abstract functions cuz it will never be used and would have null function pointers
    if (vtabletype != nullptr) {
        for (FuncDecl *f : *functions) {
            if (f->purevirt) goto isabstract;
        }
        {
            VarDecl *vtv = this->getVTableVarDecl ();
            vtv->outputstaticvar ();
        }
    isabstract:;
    }
}

// see if there is a vtable entry for a struct
//  input:
//   lowoffs  = lowest offset within the struct to search
//   highoffs = highest offset within the struct to search
//   *structoffs_r = zero for top-level call
//   *vtableoffs_r = zero for top-level call
//  output:
//   returns false: nothing in that range found
//                  *structoffs_r = incremented to end of the struct
//                  *vtableoffs_r = incremented to end of the vtable
//            true: vtable entry in that range found
//                  *structoffs_r = filled in with offset within struct of the vtable entry
//                  *vtableoffs_r = filled in with offset within the vtable of the entry
bool StructType::getVTableLabel (Token *errtok, tsize_t lowoffs, tsize_t highoffs, tsize_t *structoffs_r, tsize_t *vtableoffs_r)
{
    // search this one first
    // accumulate offsets as we search through
    tsize_t alin = this->getTypeAlign (errtok);
    *structoffs_r = (*structoffs_r + alin - 1) & - alin;
    for (VarDecl *v : *variables) {
        tsize_t alin = v->getVarAlign (errtok);
        *structoffs_r = (*structoffs_r + alin - 1) & - alin;
        if ((strcmp (v->getName (), "__vtableptr") == 0) && (*structoffs_r >= lowoffs) && (*structoffs_r < highoffs)) return true;
        if (! this->getUnyun ()) *structoffs_r += v->getValSize (errtok);
    }

    // increment vtable offset over all local virtual function entries
    // ...so it will then point to override entries for the first extension
    for (FuncDecl *f : *functions) {
        if (f->getStorClass () == KW_VIRTUAL) {
            *vtableoffs_r += 2;
        }
    }

    // search extensions, in order of declaration
    for (StructType *exttype : exttypes) {
        if (exttype->getVTableLabel (errtok, lowoffs, highoffs, structoffs_r, vtableoffs_r)) return true;
    }

    return false;
}

// generate termination function
// calls destructor
// repeats for all elements of an array
//  input:
//   pointer = pointer to struct or array of structs
//   numelem = 1: scalar or single-element array
//          else: numer of elements in array
//  output:
//   returns pointer to scalar or first array element
/*
    void *structtype::__term (__size_t numelem)
    {
        for (__size_t i = 0; i < numelem; i ++) {
            structtype *s = this + i;
            for (__size_t j = 0; j < s->exttypes.size (); j ++)
                    s->exttype::__term (1);
            if (hasdestructor) s->__dtor ();
        }
        return (void *) this;
    }
*/
void StructType::genTermFunc ()
{
    if (termfuncdecl == nullptr) return;
    if (termfuncdecl->getFuncBody () != nullptr) return;

    // push tokens for term function source in reverse order

    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_CBRACE));

    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_SEMI));
    pushtoken (new SymToken (__FILE__, __LINE__, 0, "this"));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_CPAREN));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_STAR));
    pushtoken (new SymToken (__FILE__, __LINE__, 0, "void"));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_OPAREN));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_RETURN));

    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_CBRACE));

    if (hasdestructor) {
        pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_SEMI));
        pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_CPAREN));
        pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_OPAREN));
        pushtoken (new SymToken (__FILE__, __LINE__, 0, "__dtor"));
        pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_POINT));
        pushtoken (new SymToken (__FILE__, __LINE__, 0, "s"));
    }

    // output code to terminate inherited structs
    // implicit casting of the pointer to rootward type adds the offset as part of the implicit cast
    for (StructType *exttype : exttypes) {
        if (exttype->termfuncdecl != nullptr) {
            pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_SEMI));
            pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_CPAREN));
            NumValue nvone;
            nvone.u = 1;
            pushtoken (new NumToken (__FILE__, __LINE__, 0, NC_UINT, nvone));
            pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_OPAREN));
            pushtoken (new SymToken (__FILE__, __LINE__, 0, "__term"));
            pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_COLON2));
            pushtoken (new SymToken (__FILE__, __LINE__, 0, exttype->getName ()));
            pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_POINT));
            pushtoken (new SymToken (__FILE__, __LINE__, 0, "s"));
        }
    }

    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_SEMI));
    pushtoken (new SymToken (__FILE__, __LINE__, 0, "i"));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_PLUS));
    pushtoken (new SymToken (__FILE__, __LINE__, 0, "this"));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_EQUAL));
    pushtoken (new SymToken (__FILE__, __LINE__, 0, "s"));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_STAR));
    pushtoken (new SymToken (__FILE__, __LINE__, 0, this->getName ()));

    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_OBRACE));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_CPAREN));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_PLUSPLUS));
    pushtoken (new SymToken (__FILE__, __LINE__, 0, "i"));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_SEMI));
    pushtoken (new SymToken (__FILE__, __LINE__, 0, "numelem"));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_LTANG));
    pushtoken (new SymToken (__FILE__, __LINE__, 0, "i"));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_SEMI));
    NumValue nvzero;
    nvzero.u = 0;
    pushtoken (new NumToken (__FILE__, __LINE__, 0, NC_UINT, nvzero));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_EQUAL));
    pushtoken (new SymToken (__FILE__, __LINE__, 0, "i"));
    pushtoken (new SymToken (__FILE__, __LINE__, 0, sizetype->getName ()));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_OPAREN));
    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_FOR));

    pushtoken (new KwToken  (__FILE__, __LINE__, 0, KW_OBRACE));

    termfuncdecl->parseFuncBody ();
}

// returns true iff struct contains a pure virtual function
//  input:
//   errtok = token for error messages
//   childtype = nullptr: top-level inquiry
//                  else: next step leafward that may fill in pure virtuals
bool StructType::isAbstract (Token *errtok, StructType *childtype)
{
    if (functions == nullptr) {
        printerror (errtok, "struct/union '%s' not yet defined", this->getName ());
        return false;
    }

    bool isabs = false;

    // see if there is a pure virtual function that is not overridden by the child type
    for (FuncDecl *f : *functions) {

        // see if we have a pure virtual function
        if (f->purevirt) {
            assert (f->getStorClass () == KW_VIRTUAL);

            // see if the given child type has a virtual function that overrides this one.
            // the child function can be either pure virtual or concrete virtual, if it is
            //  pure, it was checked by an outer-level call aready.
            if (childtype != nullptr) {
                for (FuncDecl *cf : *childtype->functions) {
                    if ((cf->getStorClass () == KW_VIRTUAL) && (strcmp (f->getName (), cf->getName ()) == 0)) goto overidden;
                }
            }

            // unoverridden pure virtual
            if (! isabs) {
                printerror (errtok, "cannot instantiate abstract struct type '%s'", this->getName ());
                isabs = true;
            }
            printerror (errtok, "  abstract function '%s'", f->getName ());
        overidden:;
        }
    }

    // see if all our parents are complete by substituting our virtual functions in for their pure virtual functions
    // the parents can use either our concrete virtual or pure virtual functions,
    //  cuz all our abstract virtual functions were checked in the above loop
    for (StructType *exttype : exttypes) {
        isabs |= exttype->isAbstract (errtok, this);
    }

    return isabs;
}

// gather list of all subtypeids and corresponding offset within struct
//  input:
//   subtypeids = where to add the subtypeids
//   offset = offset of start of this struct
//  output:
//   any substructs added to subtypeids
void StructType::gatherThrowSubIDs (Token *errtok, std::vector<std::pair<char const *,tsize_t>> *subtypeids, tsize_t offset)
{
    int extindex = 0;
    for (StructType *exttype : exttypes) {
        tsize_t extofs = getExtOffset (extindex ++) + offset;
        subtypeids->push_back ({ exttype->getPtrType ()->getThrowID (errtok), extofs });
        exttype->gatherThrowSubIDs (errtok, subtypeids, extofs);
    }
}
