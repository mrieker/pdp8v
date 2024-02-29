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
/////////////////////////////////////////////////
//                                             //
//  various things to do with unary operators  //
//                                             //
/////////////////////////////////////////////////

#include <assert.h>

#include "error.h"
#include "expr.h"
#include "opcode.h"

// generate prims for the binop expression
Prim *UnopExpr::generexpr (Prim *prevprim, ValDecl **exprval_r)
{
    if (checkConstExpr (exprval_r)) return prevprim;

    switch (opcode) {
        case OP_MOVE:   return subexpr->generexpr (prevprim, exprval_r);

        case OP_ADDROF: {

            // optimize &*p => p
            UnopExpr *subunop = subexpr->castUnopExpr ();
            if ((subunop != nullptr) && (subunop->opcode == OP_DEREF)) {
                return subunop->subexpr->generexpr (prevprim, exprval_r);
            }

            // optimize &a[i] => a + i
            BinopExpr *subbinop = subexpr->castBinopExpr ();
            if ((subbinop != nullptr) && (subbinop->opcode == OP_SUBSCR)) {
                BinopExpr *addexpr = new BinopExpr (exprscope, exprtoken);
                addexpr->opcode = OP_ADD;
                addexpr->leftexpr = subbinop->leftexpr;
                addexpr->riteexpr = subbinop->riteexpr;
                return addexpr->generexpr (prevprim, exprval_r);
            }

            break;
        }

        case OP_DEREF: {

            PtrType *ptrtype = subexpr->getType ()->stripCVMod ()->castPtrType ();
            if (ptrtype == nullptr) {
                throwerror (exprtoken, "dereferencing non-pointer type '%s'", subexpr->getType ()->getName ());
            }
            Type *valtype = ptrtype->getBaseType ();

            // optimize *&p => p
            UnopExpr *subunop = subexpr->castUnopExpr ();
            if ((subunop != nullptr) && (subunop->opcode == OP_ADDROF)) {
                return subunop->subexpr->generexpr (prevprim, exprval_r);
            }

            // optimize *funcpointer => funcpointer
            FuncType *functype = valtype->stripCVMod ()->castFuncType ();
            if (functype != nullptr) {
                return subexpr->generexpr (prevprim, exprval_r);
            }

            // make a dereferenced pointer value
            ValDecl *subvaldecl;
            prevprim = subexpr->generexpr (prevprim, &subvaldecl);
            if (exprval_r != nullptr) {
                *exprval_r = new PtrDecl (exprscope, exprtoken, subvaldecl);
            }
            return prevprim;
        }

        case OP_LOGNOT:
        case OP_SIZEOF:
        case OP_BITNOT:
        case OP_NEGATE: break;

        default:        assert_false ();
    }

    if (exprval_r == nullptr) {
        prevprim = subexpr->generexpr (prevprim, nullptr);
    } else {
        UnopPrim *up = new UnopPrim (exprscope, exprtoken);
        up->opcode = opcode;
        prevprim = subexpr->generexpr (prevprim, &up->aval);
        *exprval_r = up->ovar = newTempVarDecl (this->getType ());
        prevprim = prevprim->setLinext (up);
    }
    return prevprim;
}
