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
//////////////////////////////////////////////////
//                                              //
//  various things to do with binary operators  //
//                                              //
//////////////////////////////////////////////////

#include <assert.h>

#include "error.h"
#include "expr.h"
#include "opcode.h"

// get type of asnop result
Type *AsnopExpr::getType ()
{
    return leftexpr->getType ()->stripCVMod ()->getConstType ();
}

// generate prims for the binop expression
Prim *BinopExpr::generexpr (Prim *prevprim, ValDecl **exprval_r)
{
    // handle cases of both operands constant
    if (checkConstExpr (exprval_r)) return prevprim;

    // handle cases of one operand constant
    switch (opcode) {

        // discard left value (constant or otherwise), then use right value
        case OP_COMMA: {
            prevprim = leftexpr->generexpr (prevprim, nullptr);
            return riteexpr->generexpr (prevprim, exprval_r);
        }

        // if left is a constant zero, just do right
        // if right is a constant zero, just do left
        case OP_BITOR:
        case OP_BITXOR:
        case OP_ADD: {
            if (leftexpr->isConstZero ()) {
                return riteexpr->generexpr (prevprim, exprval_r);
            }
            // fallthrough
        }
        case OP_SHL:
        case OP_SHR:
        case OP_SUB: {
            if (riteexpr->isConstZero ()) {
                return leftexpr->generexpr (prevprim, exprval_r);
            }
            break;
        }

        // if either is a constant zero, just use the zero
        case OP_BITAND:
        case OP_MUL: {
            if (riteexpr->isConstZero ()) {
                return riteexpr->generexpr (prevprim, exprval_r);
            }
            // fallthrough
        }
        // if left is a constant zero, just use the zero
        case OP_DIV:
        case OP_MOD: {
            if (leftexpr->isConstZero ()) {
                return leftexpr->generexpr (prevprim, exprval_r);
            }
            break;
        }

        // short-circuiters
        case OP_LOGOR: {
            return genshortcirc (prevprim, exprval_r, true, "logor-true");
        }
        case OP_LOGAND: {
            return genshortcirc (prevprim, exprval_r, false, "logand-false");
        }

        default: break;
    }

    Type *lefttype = leftexpr->getType ()->stripCVMod ();
    IntegType *leftint = lefttype->castIntegType ();
    switch (opcode) {

        // multiply, maybe we can use shift-left instead
        case OP_MUL: {
            int ritep2 = riteexpr->getIntConPowOf2 ();
            if (ritep2 >= 0) {
                opcode   = OP_SHL;
                riteexpr = ValExpr::createsizeint (riteexpr->exprscope, riteexpr->exprtoken, ritep2, false);
                break;
            }
            int leftp2 = leftexpr->getIntConPowOf2 ();
            if (leftp2 >= 0) {
                Expr *rx = riteexpr;
                opcode   = OP_SHL;
                riteexpr = ValExpr::createsizeint (leftexpr->exprscope, leftexpr->exprtoken, leftp2, false);
                leftexpr = rx;
                break;
            }
            break;
        }

        // divide, maybe we can use shift-right instead
        case OP_DIV: {
            if (leftint == nullptr) break;
            if (leftint->getSign ()) break;
            int ritep2 = riteexpr->getIntConPowOf2 ();
            if (ritep2 >= 0) {
                opcode   = OP_SHR;
                riteexpr = ValExpr::createsizeint (riteexpr->exprscope, riteexpr->exprtoken, ritep2, false);
            }
            break;
        }

        // modulus, maybe we can use anding instead
        case OP_MOD: {
            if (leftint == nullptr) break;
            if (leftint->getSign ()) break;
            int ritep2 = riteexpr->getIntConPowOf2 ();
            //TODO support bigger sizes
            if ((ritep2 >= 0) && (ritep2 <= 8 * (int) sizeof (tsize_t))) {
                opcode   = OP_BITAND;
                riteexpr = ValExpr::createsizeint (riteexpr->exprscope, riteexpr->exprtoken, (1U << ritep2) - 1, false);
            }
            break;
        }

        // shifts, always use tsize_t for shift count so lower layers don't have to support all types
        case OP_SHR:
        case OP_SHL: {
            riteexpr = exprscope->castToType (exprtoken, sizetype, riteexpr);
            break;
        }

        default: break;
    }

    // generate prims
    if (exprval_r == nullptr) {
        prevprim = leftexpr->generexpr (prevprim, nullptr);
        prevprim = riteexpr->generexpr (prevprim, nullptr);
    } else {
        BinopPrim *bp = new BinopPrim (exprscope, exprtoken);
        bp->opcode = opcode;
        prevprim = leftexpr->generexpr (prevprim, &bp->aval);
        prevprim = riteexpr->generexpr (prevprim, &bp->bval);
        *exprval_r = bp->ovar = newTempVarDecl (this->getType ());
        prevprim = prevprim->setLinext (bp);
    }
    return prevprim;
}

// short-circuit operator '||' or '&&'
//  defval = default value for operation
//  lblname = label name
Prim *BinopExpr::genshortcirc (Prim *prevprim, ValDecl **exprval_r, bool defval, char const *lblname)
{
    // create a result var
    VarDecl *resvar = newTempVarDecl (booltype);
    *exprval_r = resvar;

    // set result var to defval to begin with
    assert (! booltype->getSign ());
    NumValue nvtrue;
    nvtrue.u = defval;
    AsnopPrim *unopdef = new AsnopPrim (exprscope, exprtoken);
    unopdef->aval = resvar;
    unopdef->bval = NumDecl::createtyped (exprtoken, booltype, NC_UINT, nvtrue);
    prevprim = prevprim->setLinext (unopdef);

    // if left expression is defval, leave result var as defval
    LabelPrim *sslabel = new LabelPrim (exprscope, exprtoken, lblname);
    prevprim = Stmt::ifgoto (prevprim, leftexpr, defval, sslabel);

    // otherwise, set result var to right expression result
    ValDecl *riteval;
    prevprim = riteexpr->generexpr (prevprim, &riteval);
    AsnopPrim *unoprite = new AsnopPrim (exprscope, exprtoken);
    unoprite->aval = resvar;
    unoprite->bval = riteval;
    prevprim = prevprim->setLinext (unoprite);

    return prevprim->setLinext (sslabel);
}

// if both operands are constants, hopefully they combine to a constant
//  the isNumConstExpr() function will combine two numeric constants
//  this version is only used if isNumConstExpr() failed, ie, one of the constants is an assembly-language label
// call isNumOrAsmConstExpr() to try isNumConstExpr() first then isAsmConstExpr()
char *BinopExpr::isAsmConstExpr ()
{
    char *leftcon = leftexpr->isNumOrAsmConstExpr ();
    char *ritecon = riteexpr->isNumOrAsmConstExpr ();

    char *buf = nullptr;
    if ((leftcon != nullptr) && (ritecon != nullptr)) {
        asprintf (&buf, "(%s)%s(%s)", leftcon, opcodestr (opcode), ritecon);
    }

    if (leftcon != nullptr) free (leftcon);
    if (ritecon != nullptr) free (ritecon);

    return buf;
}
