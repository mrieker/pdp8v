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
////////////////////////////////////////////////////
//                                                //
//  evaluation compile-time constant expressions  //
//                                                //
////////////////////////////////////////////////////

#include <assert.h>

#include "expr.h"
#include "token.h"

// return boolean value
static NumCat boolret (NumValue *nv_r, bool istrue)
{
    if (booltype->getSign ()) {
        nv_r->s = istrue;
        return NC_SINT;
    } else {
        nv_r->u = istrue;
        return NC_UINT;
    }
}

// test boolean value
static bool booltest (NumCat numcat, NumValue numval)
{
    switch (numcat) {
        case NC_FLOAT: return numval.f != 0;
        case NC_SINT:  return numval.s != 0;
        case NC_UINT:  return numval.u != 0;
        default: assert_false ();
    }
}

// return size value
static NumCat sizeret (NumValue *nv_r, tsize_t value)
{
    if (sizetype->getSign ()) {
        nv_r->s = value;
        return NC_SINT;
    } else {
        nv_r->u = value;
        return NC_UINT;
    }
}

// compute binop value
//  BINOPF - one or both operands might be floatingpoint, result numeric
//  BINOPC - one or both operands might be floatingpoint, result boolean
//  BINOPI - both operands are integers
#define BINOPF(op) do { \
    switch (leftnc) { \
        case NC_FLOAT: { \
            switch (ritenc) { \
                case NC_FLOAT: nv_r->f = leftnv.f op ritenv.f; return NC_FLOAT; \
                case NC_SINT:  nv_r->f = leftnv.f op ritenv.s; return NC_FLOAT; \
                case NC_UINT:  nv_r->f = leftnv.f op ritenv.u; return NC_FLOAT; \
                default: return NC_NONE; \
            } \
        } \
        case NC_SINT: { \
            switch (ritenc) { \
                case NC_FLOAT: nv_r->f = leftnv.s op ritenv.f; return NC_FLOAT; \
                case NC_SINT:  nv_r->s = leftnv.s op ritenv.s; return NC_SINT; \
                case NC_UINT:  nv_r->u = leftnv.s op ritenv.u; return NC_UINT; \
                default: return NC_NONE; \
            } \
        } \
        case NC_UINT: { \
            switch (ritenc) { \
                case NC_FLOAT: nv_r->f = leftnv.u op ritenv.f; return NC_FLOAT; \
                case NC_SINT:  nv_r->u = leftnv.u op ritenv.s; return NC_UINT; \
                case NC_UINT:  nv_r->u = leftnv.u op ritenv.u; return NC_UINT; \
                default: return NC_NONE; \
            } \
        } \
        default: return NC_NONE; \
    } \
} while (0)

#define BINOPC(op) do { \
    bool istrue; \
    switch (leftnc) { \
        case NC_FLOAT: { \
            switch (ritenc) { \
                case NC_FLOAT: istrue = leftnv.f op ritenv.f; break; \
                case NC_SINT:  istrue = leftnv.f op ritenv.s; break; \
                case NC_UINT:  istrue = leftnv.f op ritenv.u; break; \
                default: return NC_NONE; \
            } \
            break; \
        } \
        case NC_SINT: { \
            switch (ritenc) { \
                case NC_FLOAT: istrue = leftnv.s op ritenv.f; break; \
                case NC_SINT:  istrue = leftnv.s op ritenv.s; break; \
                case NC_UINT:  istrue = (uint64_t) leftnv.s op ritenv.u; break; \
                default: return NC_NONE; \
            } \
            break; \
        } \
        case NC_UINT: { \
            switch (ritenc) { \
                case NC_FLOAT: istrue = leftnv.u op ritenv.f; break; \
                case NC_SINT:  istrue = leftnv.u op (uint64_t) ritenv.s; break; \
                case NC_UINT:  istrue = leftnv.u op ritenv.u; break; \
                default: return NC_NONE; \
            } \
            break; \
        } \
        default: return NC_NONE; \
    } \
    return boolret (nv_r, istrue); \
} while (0)

#define BINOPI(op) do { \
    switch (leftnc) { \
        case NC_SINT: { \
            switch (ritenc) { \
                case NC_SINT:  nv_r->s = leftnv.s op ritenv.s; return NC_SINT; \
                case NC_UINT:  nv_r->u = leftnv.s op ritenv.u; return NC_UINT; \
                default: return NC_NONE; \
            } \
        } \
        case NC_UINT: { \
            switch (ritenc) { \
                case NC_SINT:  nv_r->u = leftnv.u op ritenv.s; return NC_UINT; \
                case NC_UINT:  nv_r->u = leftnv.u op ritenv.u; return NC_UINT; \
                default: return NC_NONE; \
            } \
        } \
        default: return NC_NONE; \
    } \
} while (0)

NumCat BinopExpr::isNumConstExpr (NumValue *nv_r)
{
    NumValue leftnv, ritenv;
    NumCat leftnc = leftexpr->isNumConstExpr (&leftnv);
    NumCat ritenc = riteexpr->isNumConstExpr (&ritenv);

    // for all ops, left must be a constant
    if (leftnc == NC_NONE) return NC_NONE;

    // maybe || or && is short-circuiting the right operand
    // ...so we don't care if right operand is a constant or not
    switch (opcode) {
        case OP_LOGOR: {
            if (booltest (leftnc, leftnv)) return boolret (nv_r, true);
            *nv_r = ritenv;
            return ritenc;
        }
        case OP_LOGAND: {
            if (! booltest (leftnc, leftnv)) return boolret (nv_r, false);
            *nv_r = ritenv;
            return ritenc;
        }
        default: break;
    }

    // for all others, right must also be a constant
    if (ritenc == NC_NONE) return NC_NONE;

    switch (opcode) {
        case OP_COMMA: {
            *nv_r = ritenv;
            return ritenc;
        }

        case OP_BITOR:  BINOPI(|);
        case OP_BITXOR: BINOPI(^);
        case OP_BITAND: BINOPI(&);
        case OP_CMPEQ:  BINOPC(==);
        case OP_CMPNE:  BINOPC(!=);
        case OP_CMPGE:  BINOPC(>=);
        case OP_CMPGT:  BINOPC(>);
        case OP_CMPLE:  BINOPC(<=);
        case OP_CMPLT:  BINOPC(<);
        case OP_SHL:    BINOPI(<<);
        case OP_SHR:    BINOPI(>>);
        case OP_ADD:    BINOPF(+);
        case OP_SUB:    BINOPF(-);
        case OP_DIV:    BINOPF(/);
        case OP_MUL:    BINOPF(*);
        case OP_MOD:    BINOPI(%);

        default: return NC_NONE;
    }
}

NumCat BlockExpr::isNumConstExpr (NumValue *nv_r)
{
    if (! topstmts.empty ()) return NC_NONE;
    return laststmt->expr->isNumConstExpr (nv_r);
}

// compute typecast constant
// - must be casting a numeric constant to a numeric type
NumCat CastExpr::isNumConstExpr (NumValue *nv_r)
{
    NumValue subnv;
    NumCat subnc = subexpr->isNumConstExpr (&subnv);
    if (subnc != NC_NONE) {
        FloatType *newft = casttype->castFloatType ();
        if (newft != nullptr) {
            switch (subnc) {
                case NC_FLOAT: nv_r->f = subnv.f; break;
                case NC_SINT:  nv_r->f = subnv.s; break;
                case NC_UINT:  nv_r->f = subnv.u; break;
                default: assert_false ();
            }
            return NC_FLOAT;
        }
        IntegType *newit = casttype->castIntegType ();
        if (newit != nullptr) {
            if (newit->getSign ()) {
                switch (subnc) {
                    case NC_FLOAT: nv_r->s = subnv.f; break;
                    case NC_SINT:  nv_r->s = subnv.s; break;
                    case NC_UINT:  nv_r->s = subnv.u; break;
                    default: assert_false ();
                }
                return NC_SINT;
            } else {
                switch (subnc) {
                    case NC_FLOAT: nv_r->u = subnv.f; break;
                    case NC_SINT:  nv_r->u = subnv.s; break;
                    case NC_UINT:  nv_r->u = subnv.u; break;
                    default: assert_false ();
                }
                return NC_UINT;
            }
        }
    }
    return NC_NONE;
}

// compute question mark constant
NumCat QMarkExpr::isNumConstExpr (NumValue *nv_r)
{
    bool condtrue;
    NumValue condnv;
    NumCat condnc = condexpr->isNumConstExpr (&condnv);
    switch (condnc) {
        case NC_FLOAT: condtrue = condnv.f != 0; break;
        case NC_SINT:  condtrue = condnv.s != 0; break;
        case NC_UINT:  condtrue = condnv.u != 0; break;
        default: return NC_NONE;
    }
    return (condtrue ? trueexpr : falsexpr)->isNumConstExpr (nv_r);
}

// compute sizeof expression or type
// - always results in integer constant
NumCat SizeofExpr::isNumConstExpr (NumValue *nv_r)
{
    tsize_t size = 0;
    switch (opcode) {
        case OP_ALIGNOF: {
            if (subexpr != nullptr) {
                size = subexpr->getType ()->getTypeAlign (exprtoken);
            } else {
                size = subtype->getTypeAlign (exprtoken);
            }
            break;
        }
        case OP_SIZEOF: {
            if (subexpr != nullptr) {
                size = subexpr->getExprSize ();
            } else {
                size = subtype->getTypeSize (exprtoken);
            }
            break;
        }
        default: assert_false ();
    }
    return sizeret (nv_r, size);
}

// unary operator constant
// - operand must be constant
NumCat UnopExpr::isNumConstExpr (NumValue *nv_r)
{
    NumValue subnv;
    NumCat subnc = subexpr->isNumConstExpr (&subnv);
    if (subnc == NC_NONE) return NC_NONE;

    switch (opcode) {
        case OP_MOVE: {
            *nv_r = subnv;
            return subnc;
        }
        case OP_NEGATE: {
            switch (subnc) {
                case NC_FLOAT: nv_r->f = - subnv.f; break;
                case NC_SINT:  nv_r->s = - subnv.s; break;
                case NC_UINT:  nv_r->u = - subnv.u; break;
                default: assert_false ();
            }
            return subnc;
        }
        case OP_LOGNOT: {
            bool istrue;
            switch (subnc) {
                case NC_FLOAT: istrue = subnv.f != 0; break;
                case NC_SINT:  istrue = subnv.s != 0; break;
                case NC_UINT:  istrue = subnv.u != 0; break;
                default: assert_false ();
            }
            return boolret (nv_r, istrue);
        }
        case OP_BITNOT: {
            switch (subnc) {
                case NC_FLOAT: return NC_NONE;
                case NC_SINT:  nv_r->s = ~ subnv.s; break;
                case NC_UINT:  nv_r->u = ~ subnv.u; break;
                default: assert_false ();
            }
            return subnc;
        }
        default: break;
    }

    return NC_NONE;
}

// compute whether a simple value (function, string, number, variable) is constant
// - so just numeric constants are numeric constants
NumCat ValExpr::isNumConstExpr (NumValue *nv_r)
{
    return valdecl->isNumConstVal (nv_r);
}
