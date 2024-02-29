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

#include "decl.h"
#include "error.h"
#include "expr.h"
#include "token.h"

////////////////////
//  constructors  //
////////////////////

Expr::Expr (Scope *exprscope, Token *exprtoken)
{
    assert (exprscope != nullptr);
    assert (exprtoken != nullptr);
    this->exprscope = exprscope;
    this->exprtoken = exprtoken;
}

tsize_t Expr::getExprSize ()
{
    return getType ()->getTypeSize (exprtoken);
}

// see if the expression is a compile-time constant zero
bool Expr::isConstZero ()
{
    NumValue nv;
    switch (this->isNumConstExpr (&nv)) {
        case NC_FLOAT: return nv.f == 0;
        case NC_SINT:  return nv.s == 0;
        case NC_UINT:  return nv.u == 0;
        default: {

            // check for something like (((0))) for NULL
            char *asmconst = isAsmConstExpr ();
            if (asmconst == nullptr) return false;
            int len = strlen (asmconst);
            int nop, ncp;
            for (nop = 0; nop < len; nop ++) {
                if (asmconst[nop] != '(') break;
            }
            for (ncp = 0; ncp < len; ncp ++) {
                if (asmconst[len-ncp-1] != ')') break;
            }
            bool ok = (ncp == nop) && (ncp + nop + 1 == len) && (asmconst[nop] == '0');
            free (asmconst);
            return ok;
        }
    }
}

// see if the expression is a compile-time constant power-of-2
//  returns < 0: isn't so
//         else: log2 of expression value
int Expr::getIntConPowOf2 ()
{
    NumValue nv;
    switch (this->isNumConstExpr (&nv)) {
        case NC_SINT: {
            if (nv.s <= 0) return -1;
            if ((nv.s & - nv.s) != nv.s) return -1;
            return __builtin_ffsll (nv.s) - 1;
        }
        case NC_UINT: {
            if (nv.u == 0) return -1;
            if ((nv.u & - nv.u) != nv.u) return -1;
            return __builtin_ffsll (nv.u) - 1;
        }
        default: return -1;
    }
}

// check for compile-time constant
//  first check for a single numeric value
//  then check for an assembly-time expression such as somestaticarray+12
char *Expr::isNumOrAsmConstExpr ()
{
    char *buf;
    NumValue nv;
    switch (this->isNumConstExpr (&nv)) {
        case NC_FLOAT: asprintf (&buf, "%A",   nv.f); break;
        case NC_SINT:  asprintf (&buf, "%lld", (long long) nv.s); break;
        case NC_UINT:  asprintf (&buf, "%llu", (unsigned long long) nv.u); break;
        default: buf = isAsmConstExpr (); break;
    }
    return buf;
}

ArrayInitExpr::ArrayInitExpr (Scope *exprscope, Token *exprtoken)
    : Expr (exprscope, exprtoken)
{ }

AsnopExpr::AsnopExpr (Scope *exprscope, Token *exprtoken)
    : Expr (exprscope, exprtoken)
{ }

BinopExpr::BinopExpr (Scope *exprscope, Token *exprtoken)
    : Expr (exprscope, exprtoken)
{ }

BlockExpr::BlockExpr (Scope *exprscope, Token *exprtoken)
    : Expr (exprscope, exprtoken)
{ }

Type *BlockExpr::getType ()
{
    return laststmt->expr->getType ();
}

tsize_t BlockExpr::getExprSize ()
{
    return laststmt->expr->getExprSize ();
}

char *BlockExpr::isAsmConstExpr ()
{
    if (! topstmts.empty ()) return nullptr;
    return laststmt->expr->isAsmConstExpr ();
}

char *BlockExpr::getConstantAddr ()
{
    if (! topstmts.empty ()) return nullptr;
    return laststmt->expr->getConstantAddr ();
}

CallExpr::CallExpr (Scope *exprscope, Token *exprtoken)
    : Expr (exprscope, exprtoken)
{ }

CastExpr::CastExpr (Scope *exprscope, Token *exprtoken)
    : Expr (exprscope, exprtoken)
{ }

QMarkExpr::QMarkExpr (Scope *exprscope, Token *exprtoken)
    : Expr (exprscope, exprtoken)
{
    condexpr = nullptr;
    trueexpr = nullptr;
    falsexpr = nullptr;
}

SizeofExpr::SizeofExpr (Scope *exprscope, Token *exprtoken)
    : Expr (exprscope, exprtoken)
{
    opcode  = OP_END;
    subexpr = nullptr;
    subtype = nullptr;
}

StringInitExpr::StringInitExpr (Scope *exprscope, Token *exprtoken)
    : Expr (exprscope, exprtoken)
{ }

StructInitExpr::StructInitExpr (Scope *exprscope, Token *exprtoken)
    : Expr (exprscope, exprtoken)
{ }

MFuncExpr::MFuncExpr (Scope *exprscope, Token *exprtoken)
    : Expr (exprscope, exprtoken)
{
    ptrexpr   = nullptr;
    memstruct = nullptr;
    memfunc   = nullptr;
    ignvirt   = false;
}

Type *MFuncExpr::getType ()
{
    return memfunc->getType ();
}

UnopExpr::UnopExpr (Scope *exprscope, Token *exprtoken)
    : Expr (exprscope, exprtoken)
{
    opcode  = OP_END;
    subexpr = nullptr;
    restype = nullptr;
}

// get assembly-language constant string
char *UnopExpr::isAsmConstExpr ()
{
    if (opcode == OP_ADDROF) {
        // try to return address of the sub-expression
        return subexpr->getConstantAddr ();
    }

    // try to do compile-tile computation on numeric constants
    return Expr::isAsmConstExpr ();
}

ValExpr *ValExpr::createnumconst (Scope *exprscope, Token *exprtoken, NumType *nt, NumCat nc, NumValue nv)
{
    NumDecl *nd = NumDecl::createtyped (exprtoken, nt, nc, nv);
    return new ValExpr (exprscope, exprtoken, nd);
}

ValExpr *ValExpr::createsizeint (Scope *exprscope, Token *exprtoken, tsize_t value, bool sined)
{
    NumDecl *nd = NumDecl::createsizeint (value, sined);
    return new ValExpr (exprscope, exprtoken, nd);
}

ValExpr::ValExpr (Scope *exprscope, Token *exprtoken, ValDecl *valdecl)
    : Expr (exprscope, exprtoken)
{
    this->valdecl = valdecl;
}

tsize_t ValExpr::getExprSize ()
{
    return valdecl->getValSize (exprtoken);
}

char *ValExpr::isAsmConstExpr ()
{
    return valdecl->isAsmConstExpr ();
}

char *ValExpr::getConstantAddr ()
{
    return valdecl->getConstantAddr ();
}

/////////////////////////////////
//  get the expression's type  //
/////////////////////////////////

Type *ArrayInitExpr::getType ()
{
    return valuetype->getPtrType ()->getConstType ();
}

Type *CallExpr::getType ()
{
    assert (funcexpr != this);
    FuncType *functype = funcexpr->getType ()->stripCVMod ()->castFuncType ();
    return functype->getRetType ();
}

Type *CastExpr::getType ()
{
    return casttype;
}

char *CastExpr::isAsmConstExpr ()
{
    char *subcon = subexpr->isAsmConstExpr ();
    if (subcon == nullptr) return nullptr;
    char *buf;
    asprintf (&buf, "(%s)", subcon);
    free (subcon);
    return buf;
}

Type *QMarkExpr::getType ()
{
    Type *truetype = trueexpr->getType ()->stripCVMod ();
    Type *falstype = falsexpr->getType ()->stripCVMod ();
    if (truetype != falstype) {
        printerror (exprtoken, "differing types '%s' : '%s' on ? : expressions", truetype->getName (), falstype->getName ());
    }
    return truetype->getConstType ();
}

Type *SizeofExpr::getType ()
{
    return sizetype;
}

Type *StringInitExpr::getType ()
{
    return chartype->getConstType ()->getPtrType ()->getConstType ();
}

Type *StructInitExpr::getType ()
{
    return structype->getConstType ();
}

Type *ValExpr::getType ()
{
    return valdecl->getType ();
}

///////////////////////
//  dump expression  //
///////////////////////

void ArrayInitExpr::dumpexpr ()
{
    printf ("{ ");
    bool first = true;
    for (std::pair<tsize_t,Expr *> pair : exprs) {
        if (! first) printf (", ");
        tsize_t index = pair.first;
        printf ("[%u] = ", index);
        Expr *expr = pair.second;
        expr->dumpexpr ();
        first = false;
    }
    printf (" }");
}

void AsnopExpr::dumpexpr ()
{
    leftexpr->dumpexpr ();
    printf (" = ");
    riteexpr->dumpexpr ();
}

void BinopExpr::dumpexpr ()
{
    if (opcode == OP_SUBSCR) {
        leftexpr->dumpexpr ();
        printf ("[");
        riteexpr->dumpexpr ();
        printf ("]");
    } else {
        leftexpr->dumpexpr ();
        printf (" %s ", opcodestr (opcode));
        riteexpr->dumpexpr ();
    }
}

void BlockExpr::dumpexpr ()
{
    printf ("({");
    for (Stmt *stmt : topstmts) {
        printf ("  ");
        stmt->dumpstmt (2);
    }
    printf ("  ");
    laststmt->expr->dumpexpr ();
    printf (" })");
}

void CallExpr::dumpexpr ()
{
    funcexpr->dumpexpr ();
    printf (" (");
    bool first = true;
    for (Expr *argexpr : argexprs) {
        if (! first) printf (", ");
        argexpr->dumpexpr ();
    }
    printf (")");
}

void CastExpr::dumpexpr ()
{
    printf ("(%s) ", casttype->getName ());
    subexpr->dumpexpr ();
}

void MFuncExpr::dumpexpr ()
{
    ptrexpr->dumpexpr ();
    printf ("->%s", memfunc->getName ());
}

void QMarkExpr::dumpexpr ()
{
    condexpr->dumpexpr ();
    printf (" ? ");
    trueexpr->dumpexpr ();
    printf (" : ");
    falsexpr->dumpexpr ();
}

void SizeofExpr::dumpexpr ()
{
    printf ("%s ", opcodestr (opcode));
    if (subexpr != nullptr) {
        subexpr->dumpexpr ();
    }
    if (subtype != nullptr) {
        printf ("(%s)", subtype->getName ());
    }
}

void StringInitExpr::dumpexpr ()
{
    printf ("%s", strtok->getName ());
}

void StructInitExpr::dumpexpr ()
{
    printf ("{ ");
    bool first = true;
    for (std::pair<tsize_t,Expr *> pair : exprs) {
        if (! first) printf (", ");

        tsize_t index = pair.first;
        StructType *memstruct;
        VarDecl *memvardecl;
        tsize_t memoffset;
        if (! structype->getMemberByIndex (exprtoken, index, &memstruct, &memvardecl, &memoffset)) {
            assert_false ();
        }
        printf (".%s = ", memvardecl->getName  ());

        Expr *expr = pair.second;
        expr->dumpexpr ();
        first = false;
    }
    printf (" }");
}

void UnopExpr::dumpexpr ()
{
    printf ("%s ", opcodestr (opcode));
    subexpr->dumpexpr ();
}

void ValExpr::dumpexpr ()
{
    printf ("%s", valdecl->getName ());
}
