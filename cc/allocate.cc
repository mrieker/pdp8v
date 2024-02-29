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
//////////////////////////////////////
//                                  //
//  allocate storage for variables  //
//                                  //
//////////////////////////////////////

#include <assert.h>

#include "decl.h"
#include "error.h"
#include "locn.h"
#include "mytypes.h"
#include "scope.h"

// allocate storage for variables on a function's stack
void FuncDecl::allocfunc ()
{
    // set offsets of parameters on stack
    // varargs represented by final 'void __varargs' entry
    tsize_t paramoffs = 0;
    for (VarDecl *param : *params) {
        assert (! param->isStaticMem ());
        tsize_t al = param->getVarAlign (param->getDefTok ());
        paramoffs = (paramoffs + al - 1) & - al;
        ParamLocn *pl = new ParamLocn ();
        pl->funcdecl = this;
        pl->offset = paramoffs;
        param->setVarLocn (pl);
        paramoffs += param->getValSize (param->getDefTok ());
    }

    // allocate stack space for automatic variables
    // - just sets up a StackLocn struct for automatic vars
    //   they get filled in by optimizer
    // output assemblycode for static memory variables
    paramscope->allocscope ();
}

// allocate storage for variables in this scope and all inner scopes
void Scope::allocscope ()
{
    // allocate space for all inner frames
    for (Scope *inner : inners) {
        inner->allocscope ();
    }

    // assign addresses to variables in this scope
    DeclMap const *declmap = getDecls ();
    for (DeclMap::const_iterator it = declmap->begin (); it != declmap->end (); it ++) {
        Decl *decl = it->second;
        VarDecl *vardecl = decl->castVarDecl ();
        if ((vardecl != nullptr) && (vardecl->getVarLocn () == nullptr)) {
            switch (vardecl->getStorClass ()) {

                // stack-based variable
                // they don't have 'extern' 'static' 'typedef' storage class
                // if the top-level global scope, they are global variables
                case KW_NONE: {
                    if (this == globalscope) {
                        assert (vardecl->isStaticMem ());
                        vardecl->allocstaticvar ();
                        vardecl->outputstaticvar ();
                    } else {
                        assert (! vardecl->isStaticMem ());
                        // offset and such gets filled in by optimizer
                        StackLocn *sl = new StackLocn ();
                        vardecl->setVarLocn (sl);
                    }
                    break;
                }

                case KW_EXTERN:
                case KW_STATIC: {
                    assert (vardecl->isStaticMem ());
                    vardecl->allocstaticvar ();
                    vardecl->outputstaticvar ();
                    break;
                }

                case KW_ENUM:
                case KW_TYPEDEF: break;

                default: assert_false ();
            }
        }
    }
}

// called for extern, static, global variables
// output assembler directives to reserve/initialize storage
// - global/extern vars have same label as given in source code
//   static vars are prefixed with lcl.%u. in case more than one static var of same name
// - non-initialized vars get .blkb of the size
//   .byte/.word/.long/.quad init value for initialization
void VarDecl::allocstaticvar ()
{
    LabelLocn *labelocn = new LabelLocn ();
    locn = labelocn;

    Keywd storclass = getStorClass ();
    assert ((storclass == KW_EXTERN) || (storclass == KW_NONE) || (storclass == KW_STATIC));

    if (getEncType () != nullptr) {

        // static struct member, label is structtype::membername
        assert (storclass == KW_STATIC);
        labelocn->label = this->getEncName ();
    } else if (storclass == KW_STATIC) {

        // static, use unique label in case there is more than one internal static of same name
        static tsize_t lclseqno = 100;
        char *buf;
        asprintf (&buf, "lcl.%u.%s", ++ lclseqno, this->getName ());
        labelocn->label = buf;
    } else {

        // extern/global, use given name as they should be unique
        labelocn->label = this->getName ();
    }
}

void VarDecl::outputstaticvar ()
{
    Keywd storclass = getStorClass ();

    if (storclass == KW_EXTERN) {
        if (initexpr != nullptr) {
            printerror (initexpr->exprtoken, "initialization not allowed on extern variable\n");
        }
        return;
    }

    assert ((storclass == KW_NONE) || (storclass == KW_STATIC));

    bool ro = getType ()->isConstType ();
    char const *tn = getType ()->getName ();
    char const *ar = "";
    if (isArray ()) {
        ro = getType ()->stripCVMod ()->castPtrType ()->getBaseType ()->isConstType ();
        tn = getType ()->stripCVMod ()->castPtrType ()->getBaseType ()->getName ();
        ar = "[]";
    }
    fprintf (sfile, "; -------------- %s%s\n", tn, ar);
    fprintf (sfile, "\t.psect\t%s\n", ro ? TEXTPSECT : DATAPSECT);

    // maybe the label is global
    // - global if top-level declaration and no qualifier like extern or static
    // - global if static struct member
    if ((storclass == KW_NONE) || (getEncType () != nullptr)) {
        fprintf (sfile, "\t.global\t%s\n", locn->castLabelLocn ()->label);
    }

    // output alignment and label
    fprintf (sfile, "\t.align\t%u\n", this->getVarAlign (this->getDefTok ()));
    fprintf (sfile, "%s:\n", locn->castLabelLocn ()->label);

    // fill up the storage
    if (initexpr == nullptr) {

        // no initialization, just reserve a block of zeroes for the size of the var
        fprintf (sfile, "\t.blkb\t%u\n", this->getValSize (this->getDefTok ()));
    } else {

        // initialization, output static initialization
        tsize_t initsize = initexpr->outputStaticInit (this->getType ()->stripCVMod ());
        assert (initsize == getValSize (initexpr->exprtoken));
    }
}

// initialize a static scalar variable (numeric, pointer)
// returns number of bytes written to assembly file = sizeof scalar variable
tsize_t Expr::outputStaticInit (Type *vartype)
{
    // see if expression is a simple constant or other assembly-language expression
    // get assembly-language expression if so
    char *constval = isNumOrAsmConstExpr ();
    if (constval == nullptr) {
        throwerror (exprtoken, "compile-time constant expression required");
    }

    // this expression must be exactly one of these types
    FloatType *floattype = vartype->castFloatType ();
    IntegType *integtype = vartype->castIntegType ();
    PtrType   *ptrtype   = vartype->castPtrType   ();

    // check for floating point constant
    if (floattype != nullptr) {
        assert (integtype == nullptr);
        assert (ptrtype   == nullptr);
        NumValue nv;
        NumCat nc = isNumConstExpr (&nv);
        assert (nc == NC_FLOAT);
        tsize_t siz = floattype->getTypeSize (exprtoken);
        switch (siz) {
            case 4: {
                union { float f; uint32_t u; } v;
                v.f = nv.f;
                fprintf (sfile, "\t.long\t0x%08X ; .float %g\n", v.u, v.f);
                break;
            }
            case 8: {
                fprintf (sfile, "\t.long\t0x%08X,0x%08X ; .double %g\n", (uint32_t) nv.u, (uint32_t) (nv.u >> 32), nv.f);
                break;
            }
            default: assert_false ();
        }
        free (constval);
        return siz;
    }

    // check for integer constant
    if (integtype != nullptr) {
        assert (ptrtype == nullptr);
        char const *dir;
        tsize_t siz = integtype->getTypeSize (exprtoken);
        switch (siz) {
            case 1: dir = ".byte"; break;
            case 2: dir = ".word"; break;
            case 4: dir = ".long"; break;
            case 8: dir = ".quad"; break;
            default: assert_false ();
        }
        fprintf (sfile, "\t%s\t%s\n", dir, constval);
        free (constval);
        return siz;
    }

    // check for pointer constant
    if (ptrtype != nullptr) {
        assert (sizeof (tsize_t) == 2);
        fprintf (sfile, "\t.word\t%s\n", constval);
        free (constval);
        return 2;
    }

    // must be one of the above
    throwerror (exprtoken, "unknown initialization type '%s'", vartype->getName ());
}

// initialize a whole static array
// returns number of bytes written to assembly file = number of bytes in the whole array
tsize_t ArrayInitExpr::outputStaticInit (Type *vartype)
{
    assert (vartype->stripCVMod ()->castPtrType ()->getBaseType ()->stripCVMod () == valuetype);

    tsize_t elementsize = valuetype->getTypeSize (exprtoken);
    tsize_t outputindex = 0;
    while (true) {

        // find lowest array index we haven't initialized yet
        tsize_t lowestindex = 0;
        Expr *lowestexpr = nullptr;
        for (std::pair<tsize_t,Expr *> pair : exprs) {
            tsize_t index = pair.first;
            if ((index >= outputindex) && ((lowestexpr == nullptr) || (index < lowestindex))) {
                lowestindex = index;
                lowestexpr  = pair.second;
            }
        }

        // stop if all inits have been done
        if (lowestexpr == nullptr) break;

        // output any padding between elements
        if (lowestindex > outputindex) {
            fprintf (sfile, "\t.blkb\t%u\n", (lowestindex - outputindex) * elementsize);
        }

        // output element's initialization value
        tsize_t esize = lowestexpr->outputStaticInit (valuetype);
        assert (esize == elementsize);
        outputindex = lowestindex + 1;
    }

    // output padding to end of array
    assert (outputindex <= numarrayelems);
    if (numarrayelems > outputindex) {
        fprintf (sfile, "\t.blkb\t\%u\n", (numarrayelems - outputindex) * elementsize);
    }

    return numarrayelems * elementsize;
}

// initialize character array from string
// returns number of bytes written to assembly file = number of bytes in the whole character array
tsize_t StringInitExpr::outputStaticInit (Type *vartype)
{
    assert (vartype->stripCVMod ()->castPtrType ()->getBaseType ()->stripCVMod () == chartype);

    // make escaped version of given string
    // ...but only up to length of the array
    std::string outstr;
    int strlen = strtok->getStrlen ();
    char const *string = strtok->getString ();
    for (int i = 0; (i < strlen) && (i < arrsize); i ++) {
        char c = string[i];
        switch (c) {
            case '\"': outstr.append ("\\\""); break;
            case '\'': outstr.append ("\\\'"); break;
            case '\\': outstr.append ("\\\\"); break;
            case '\b': outstr.append ("\\b"); break;
            case '\n': outstr.append ("\\n"); break;
            case '\r': outstr.append ("\\r"); break;
            case '\t': outstr.append ("\\t"); break;
            case    0: outstr.append ("\\z"); break;
            default: outstr.push_back (c); break;
        }
    }

    // output a .ascii directive
    // use .asciz if there is still an extra byte
    char const *mne = ".ascii";
    if (strlen < arrsize) {
        mne = ".asciz";
        strlen ++;
    } else {
        printerror (exprtoken, "string length %d+1 greater than array size %u", strlen, arrsize);
    }
    fprintf (sfile, "\t%s\t\"%s\"\n", mne, outstr.c_str ());

    // fill in extra zero bytes if there is more room in array being filled
    assert (strlen <= arrsize);
    if (arrsize > strlen) {
        fprintf (sfile, "\t.blkb\t%u\n", arrsize - strlen);
    }

    return arrsize;
}

// initialize a whole static struct
// returns number of bytes written to assembly file = number of bytes in the whole struct
tsize_t StructInitExpr::outputStaticInit (Type *vartype)
{
    assert (vartype->stripCVMod () == structype);

    tsize_t outputoffset = 0;
    while (true) {

        // find lowest struct member we haven't initialized yet
        tsize_t lowestoffset = (tsize_t) -1;
        Expr *lowestexpr = nullptr;
        Type *lowesttype = nullptr;
        for (std::pair<tsize_t,Expr *> pair : exprs) {
            tsize_t index = pair.first;
            Expr *expr = pair.second;

            StructType *memstruct;
            VarDecl *memvardecl;
            tsize_t memoffset;
            if (! structype->getMemberByIndex (expr->exprtoken, index, &memstruct, &memvardecl, &memoffset)) {
                assert_false ();
            }

            if ((memoffset >= outputoffset) && ((lowestexpr == nullptr) || (memoffset < lowestoffset))) {
                lowestoffset = memoffset;
                lowestexpr   = expr;
                lowesttype   = memvardecl->getType ();
            }
        }

        // see if there is a vtable entry before the lowest found if any
        tsize_t structoffset = 0;
        tsize_t vtableoffset = 0;
        if (structype->getVTableLabel (exprtoken, outputoffset, lowestoffset, &structoffset, &vtableoffset)) {
            assert (! (structoffset & 1));
            assert (! (vtableoffset & 1));
            if (structoffset > outputoffset) {
                fprintf (sfile, "\t.blkb\t%u\n", structoffset - outputoffset);
            }
            char *vtablelabel = structype->getVTableVarName ();
            fprintf (sfile, "\t.word\t%s+%u\n", vtablelabel, vtableoffset);
            free (vtablelabel);
            outputoffset = structoffset + 2;
            continue;
        }

        // stop if all inits have been done
        if (lowestexpr == nullptr) break;

        // output any padding between members
        if (lowestoffset > outputoffset) {
            fprintf (sfile, "\t.blkb\t%u\n", lowestoffset - outputoffset);
        }

        // output member's initialization value
        tsize_t msize = lowestexpr->outputStaticInit (lowesttype);
        outputoffset = lowestoffset + msize;
    }

    // output padding to end of struct
    tsize_t structsize = structype->getTypeSize (exprtoken);
    assert (outputoffset <= structsize);
    if (structsize > outputoffset) {
        fprintf (sfile, "\t.blkb\t\%u\n", structsize - outputoffset);
    }

    return structsize;
}
