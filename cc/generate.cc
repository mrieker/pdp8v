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
/////////////////////////////////////////////////////
//                                                 //
//  Generate primitives from abstract syntax tree  //
//                                                 //
/////////////////////////////////////////////////////

#include <assert.h>

#include "decl.h"
#include "error.h"
#include "prim.h"
#include "stmt.h"

// generate prims for a function
// fills in this->entryprim and all that goes with it,
// converting the abstract syntax tree to machine-level primitives
void FuncDecl::generfunc ()
{
    entryprim = new EntryPrim (paramscope, this->getDefTok ());
    entryprim->funcdecl = this;

    Prim *prevprim = funcbody->generstmt (entryprim);

    prevprim = retlabel->generstmt (prevprim);

    RetPrim *retprim = new RetPrim (paramscope, this->getDefTok ());
    retprim->funcdecl = this;
    retprim->rval = this->retvalue;
    prevprim->setLinext (retprim);
}

// helpers //

// if (condexpr == sense) goto gotolbl;
Prim *Stmt::ifgoto (Prim *prevprim, Expr *condexpr, bool sense, LabelPrim *gotolbl)
{
    // check for '!' enclosing the whole condition
    // if so, strip '!' and flip the sense
    UnopExpr *condunopexpr = condexpr->castUnopExpr ();
    if ((condunopexpr != nullptr) && (condunopexpr->opcode == OP_LOGNOT)) {
        return ifgoto (prevprim, condunopexpr->subexpr, ! sense, gotolbl);
    }

    // check for constant expression
    NumValue nv;
    NumCat nc = condexpr->isNumConstExpr (&nv);
    bool contrue;
    switch (nc) {
        case NC_FLOAT: contrue = nv.f != 0.0; break;
        case NC_SINT:  contrue = nv.s != 0;   break;
        case NC_UINT:  contrue = nv.u != 0;   break;
        default: goto notcon;
    }
    if (contrue == sense) {
        JumpPrim *jp = new JumpPrim (condexpr->exprscope, condexpr->exprtoken, gotolbl);
        prevprim = prevprim->setLinext (jp);
    }
    return prevprim;
notcon:;

    // check for outermost compares (most ifs) and || && ifs
    BinopExpr *condbinopexpr = condexpr->castBinopExpr ();
    if (condbinopexpr != nullptr) {
        switch (condbinopexpr->opcode) {

            // optimize compares to combine with compare-and-jump
            // CompPrim only works for small integers via CMP & Bxx instruction
            // larger ints and float/double get handled via BinopPrim & TestPrim via call __cmp_xx & Bxx instruction
            case OP_CMPEQ:
            case OP_CMPNE:
            case OP_CMPLT:
            case OP_CMPLE:
            case OP_CMPGT:
            case OP_CMPGE: {
                IntegType *leftinttype = condbinopexpr->leftexpr->getType ()->stripCVMod ()->castIntegType ();
                IntegType *riteinttype = condbinopexpr->riteexpr->getType ()->stripCVMod ()->castIntegType ();
                if ((leftinttype != nullptr) && (leftinttype->getTypeSize (nullptr) <= 4)) {
                    assert (leftinttype == riteinttype);    // upper layer cast to same type
                    Opcode opcode = sense ? condbinopexpr->opcode : revcmpop (condbinopexpr->opcode);

                    // non-zero right-hand operand is full compare
                    if (! condbinopexpr->riteexpr->isConstZero ()) {
                        CompPrim *cp = new CompPrim (condbinopexpr->exprscope, condbinopexpr->exprtoken);
                        cp->opcode = opcode;
                        prevprim = condbinopexpr->leftexpr->generexpr (prevprim, &cp->aval);
                        prevprim = condbinopexpr->riteexpr->generexpr (prevprim, &cp->bval);
                        cp->flonexts.push_back (gotolbl);
                        return prevprim->setLinext (cp);
                    }

                    // unsigned compares with zero
                    if (! leftinttype->getSign ()) {
                        switch (opcode) {

                            // unsigned can never be less than zero
                            case OP_CMPLT: return prevprim;

                            // unsigned is always greater than or equal to zero
                            case OP_CMPGE: {
                                JumpPrim *jp = new JumpPrim (condbinopexpr->exprscope, condbinopexpr->exprtoken, gotolbl);
                                return prevprim->setLinext (jp);
                            }

                            // less than or equal to zero => equal to zero
                            case OP_CMPLE: opcode = OP_CMPEQ; break;

                            // greater than zero => not equal to zero
                            case OP_CMPGT: opcode = OP_CMPNE; break;

                            default: break;
                        }
                    }

                    // compares with zero
                    TestPrim *tp = new TestPrim (condbinopexpr->exprscope, condbinopexpr->exprtoken);
                    tp->opcode = opcode;
                    prevprim = condbinopexpr->leftexpr->generexpr (prevprim, &tp->aval);
                    tp->flonexts.push_back (gotolbl);
                    return prevprim->setLinext (tp);
                }
                break;
            }

            // split || and && into separate ifs
            case OP_LOGAND: {
                if (sense) {
                    LabelPrim *contlbl = new LabelPrim (condbinopexpr->exprscope, condbinopexpr->exprtoken, "logand-cont");
                    prevprim = ifgoto (prevprim, condbinopexpr->leftexpr, false, contlbl);
                    prevprim = ifgoto (prevprim, condbinopexpr->riteexpr, true,  gotolbl);
                    prevprim = prevprim->setLinext (contlbl);
                } else {
                    prevprim = ifgoto (prevprim, condbinopexpr->leftexpr, false, gotolbl);
                    prevprim = ifgoto (prevprim, condbinopexpr->riteexpr, false, gotolbl);
                }
                return prevprim;
            }
            case OP_LOGOR: {
                if (sense) {
                    prevprim = ifgoto (prevprim, condbinopexpr->leftexpr, true, gotolbl);
                    prevprim = ifgoto (prevprim, condbinopexpr->riteexpr, true, gotolbl);
                } else {
                    LabelPrim *contlbl = new LabelPrim (condbinopexpr->exprscope, condbinopexpr->exprtoken, "logor-cont");
                    prevprim = ifgoto (prevprim, condbinopexpr->leftexpr, true,  contlbl);
                    prevprim = ifgoto (prevprim, condbinopexpr->riteexpr, false, gotolbl);
                    prevprim = prevprim->setLinext (contlbl);
                }
                return prevprim;
            }

            // something else, no optimization
            default: break;
        }
    }

    // no more optimizations, compute condition expression then test-and-jump on the result
    Expr *boolexpr = condexpr->exprscope->castToType (condexpr->exprtoken, booltype, condexpr);
    TestPrim *tp = new TestPrim (condexpr->exprscope, condexpr->exprtoken);
    prevprim = boolexpr->generexpr (prevprim, &tp->aval);
    tp->opcode = sense ? OP_CMPNE : OP_CMPEQ;
    tp->flonexts.push_back (gotolbl);
    return prevprim->setLinext (tp);
}

Opcode Stmt::revcmpop (Opcode cmpop)
{
    switch (cmpop) {
        case OP_CMPEQ: return OP_CMPNE;
        case OP_CMPNE: return OP_CMPEQ;
        case OP_CMPLT: return OP_CMPGE;
        case OP_CMPLE: return OP_CMPGT;
        case OP_CMPGT: return OP_CMPLE;
        case OP_CMPGE: return OP_CMPLT;
        default: assert_false ();
    }
}

// goto gotolbl;
static Prim *alwaysgoto (Prim *prevprim, LabelPrim *gotolbl)
{
    JumpPrim *jp = new JumpPrim (gotolbl->primscope, gotolbl->primtoken, gotolbl);
    return prevprim->setLinext (jp);
}

//////////////////
//  STATEMENTS  //
//////////////////

// all generstmt() methods:
//  input:
//   prevprim = previous prim that falls into this statement (nullptr if unreachable)
//   breaklbl = label to jump to if there is a 'break' statement herein (nullptr if not in breakable context)
//   contlbl  = label to jump to if there is a 'continue' statement herein (nullptr if not in continueable context)
//  output:
//   returns prim that block falls out on (nullptr if code does not fall out the bottom)

Prim *BlockStmt::generstmt (Prim *prevprim)
{
    if (prevprim != nullptr) {
        for (Stmt *stmt : stmts) {
            prevprim = stmt->generstmt (prevprim);
        }
    }
    return prevprim;
}

Prim *CaseStmt::generstmt (Prim *prevprim)
{
    if (labelprim != nullptr) {
        prevprim = prevprim->setLinext (labelprim);
    } else {
        printerror (getStmtToken (), "case/default not part of switch");
    }
    return prevprim;
}

// declaration statement within a function
// output code for all enclosed stack/register-based variable initializations
Prim *DeclStmt::generstmt (Prim *prevprim)
{
    for (Decl *decl : alldecls) {
        VarDecl *vardecl = decl->castVarDecl ();
        if ((vardecl != nullptr) && (vardecl->getStorClass () == KW_NONE)) {
            Expr *initexpr = vardecl->getInitExpr ();
            if (initexpr != nullptr) {
                tsize_t arrsize = vardecl->isArray () ? vardecl->getArrSize () : 0;
                prevprim = initexpr->outputStackdInit (prevprim, vardecl, vardecl->isArray (), arrsize);
            }
        }
    }
    return prevprim;
}

Prim *ExprStmt::generstmt (Prim *prevprim)
{
    return expr->generexpr (prevprim, nullptr);
}

Prim *GotoStmt::generstmt (Prim *prevprim)
{
    JumpPrim *jumpprim = new JumpPrim (this->getStmtScope (), this->getStmtToken (), label->getLabelPrim ());
    prevprim = prevprim->setLinext (jumpprim);

    return prevprim;
}

Prim *IfStmt::generstmt (Prim *prevprim)
{
    assert (gotostmt->label != nullptr);
    return ifgoto (prevprim, condexpr, ! condrev, gotostmt->label->getLabelPrim ());
}

Prim *LabelStmt::generstmt (Prim *prevprim)
{
    // make sure it has a labelprim
    if (labelprim == nullptr) {
        char *buf;
        asprintf (&buf, "lbl.%s", name);
        labelprim = new LabelPrim (this->getStmtScope (), this->getStmtToken (), buf);
    }
    prevprim = prevprim->setLinext (labelprim);
    return labelprim;
}

Prim *NullStmt::generstmt (Prim *prevprim)
{
    return prevprim;
}

Prim *SwitchStmt::generstmt (Prim *prevprim)
{
    // compute index value
    ValDecl *valuval;
    prevprim = valuexpr->generexpr (prevprim, &valuval);

    // if pointer to value, read into temp var
    if (valuval->castPtrDecl () != nullptr) {
        VarDecl *tempvar = VarDecl::newtemp (valuexpr->exprscope, valuexpr->exprtoken, valuval->getType ());
        AsnopPrim *asnop = new AsnopPrim (valuexpr->exprscope, valuexpr->exprtoken);
        asnop->aval = tempvar;
        asnop->bval = valuval;
        prevprim = prevprim->setLinext (asnop);
        valuval = tempvar;
    }

    ValExpr *valuexp = new ValExpr (valuexpr->exprscope, valuexpr->exprtoken, valuval);

    // if there is a default: label in enclosed body, make a label for it
    if (defstmt != nullptr) {
        defstmt->labelprim = new LabelPrim (defstmt->getStmtScope (), defstmt->getStmtToken (), "switch-default");
    }

    // output CompPrims for each case
    //TODO optimize to an indexed jump table
    while (true) {

        // find lowest case that we haven't done yet
        CaseStmt *lowestcase = nullptr;
        for (CaseStmt *casestmt : casestmts) {

            // aready handled this case
            if (casestmt->labelprim != nullptr) continue;

            // first unhandled case, it's the lowest so far
            if (lowestcase == nullptr) {
                lowestcase = casestmt;
                continue;
            }

            // use it if it's lower than what we found before
            if ((casestmt->lonc == NC_SINT) && (lowestcase->lonc == NC_SINT)) {
                if (casestmt->lonv.s < lowestcase->lonv.s) {
                    lowestcase = casestmt;
                }
            }
            if ((casestmt->lonc == NC_UINT) && (lowestcase->lonc == NC_SINT)) {
                if ((lowestcase->lonv.s >= 0) && (casestmt->lonv.u < (uint64_t) lowestcase->lonv.s)) {
                    lowestcase = casestmt;
                }
            }
            if ((casestmt->lonc == NC_SINT) && (lowestcase->lonc == NC_UINT)) {
                if ((casestmt->lonv.s < 0) || ((uint64_t) casestmt->lonv.s < lowestcase->lonv.u)) {
                    lowestcase = casestmt;
                }
            }
            if ((casestmt->lonc == NC_UINT) && (lowestcase->lonc == NC_UINT)) {
                if (casestmt->lonv.u < lowestcase->lonv.u) {
                    lowestcase = casestmt;
                }
            }
        }
        if (lowestcase == nullptr) break;

        // make a labelprim that we can jump to for the lowest unprocessed case
        char *buf;
        asprintf (&buf, "case-%lld", (long long) lowestcase->lonv.s);
        lowestcase->labelprim = new LabelPrim (lowestcase->getStmtScope (), this->getStmtToken (), buf);

        // output compare and branch instructions
        if (lowestcase->hinc == NC_NONE) {

            // single value case, do an 'if (val == loval) goto caselabel'
            BinopExpr *condexpr = new BinopExpr (this->getStmtScope (), this->getStmtToken ());
            condexpr->opcode    = OP_CMPEQ;
            condexpr->leftexpr  = valuexp;
            condexpr->riteexpr  = ValExpr::createnumconst (this->getStmtScope (), this->getStmtToken (), lowestcase->lont, lowestcase->lonc, lowestcase->lonv);
            prevprim = ifgoto (prevprim, condexpr, true, lowestcase->labelprim);
        } else {

            // ranged value case, do an 'if ((val <= hival) && (val >= loval)) goto caselabel'
            BinopExpr *hicondexpr = new BinopExpr (this->getStmtScope (), this->getStmtToken ());
            hicondexpr->opcode    = OP_CMPLE;
            hicondexpr->leftexpr  = valuexp;
            hicondexpr->riteexpr  = ValExpr::createnumconst (this->getStmtScope (), this->getStmtToken (), lowestcase->hint, lowestcase->hinc, lowestcase->hinv);

            BinopExpr *locondexpr = new BinopExpr (this->getStmtScope (), this->getStmtToken ());
            locondexpr->opcode    = OP_CMPGE;
            locondexpr->leftexpr  = valuexp;
            locondexpr->riteexpr  = ValExpr::createnumconst (this->getStmtScope (), this->getStmtToken (), lowestcase->lont, lowestcase->lonc, lowestcase->lonv);

            BinopExpr *andexpr = new BinopExpr (this->getStmtScope (), this->getStmtToken ());
            andexpr->opcode    = OP_LOGAND;
            andexpr->leftexpr  = hicondexpr;
            andexpr->riteexpr  = locondexpr;

            prevprim = ifgoto (prevprim, andexpr, true, lowestcase->labelprim);
        }
    }

    // no more compares, jump to the default label
    // if no default label, jump past enclosed body
    LabelPrim *jumpto = (defstmt == nullptr) ? this->breaklbl->getLabelPrim () : defstmt->labelprim;
    JumpPrim *jumpprim = new JumpPrim (this->getStmtScope (), this->getStmtToken (), jumpto);
    prevprim = prevprim->setLinext (jumpprim);

    // next comes the body of the switch
    // includes case labels and default label
    for (Stmt *bodystmt : bodystmts) {
        prevprim = bodystmt->generstmt (prevprim);
    }

    // finally comes the break-to label
    prevprim = this->breaklbl->generstmt (prevprim);

    return prevprim;
}

Prim *ThrowStmt::generstmt (Prim *prevprim)
{
    ThrowPrim *tp = new ThrowPrim (getStmtScope (), getStmtToken ());
    if (thrownexpr != nullptr) {
        prevprim = thrownexpr->generexpr (prevprim, &tp->thrownval);
    }
    return prevprim->setLinext (tp);
}

Prim *TryStmt::generstmt (Prim *prevprim)
{
    Scope *stmtscope = this->getStmtScope ();
    Token *stmttoken = this->getStmtToken ();

    FinRetPrim *frp = new FinRetPrim (stmtscope, stmttoken);
    LabelPrim *endlbl = new LabelPrim (stmtscope, stmttoken, "try-end");
    LabelPrim *finlbl = new LabelPrim (stmtscope, stmttoken, "try-fin");
    LabelPrim *finendlbl = new LabelPrim (stmtscope, stmttoken, "try-fin-end");
    VarDecl *trymarkvar = VarDecl::newtemp (stmtscope, stmttoken, trymarktype);

    // __trymark.outer = __trystack;
    // __trymark.descr = &trydescr;
    // __trystack      = &__trymark;
    TryBegPrim *tbp = new TryBegPrim (stmtscope, stmttoken);
    tbp->trymarkvar = trymarkvar;
    tbp->finlblprim = finlbl;
    tbp->flonexts.push_back (finlbl);
    prevprim = prevprim->setLinext (tbp);

    // output try body
    prevprim = this->bodystmt->generstmt (prevprim);

    // call finally(), returning to endlbl
    prevprim = prevprim->setLinext (new JumpPrim (stmtscope, stmttoken, finendlbl));

    // catch blocks
    //  try-cat:
    //      save thrown value in catchvar
    //      catch body statements
    //      call finally(), returning to endlbl
    for (auto it : this->catches) {
        VarDecl *vardecl  = it.first.first;
        VarDecl *tvardecl = it.first.second;
        Stmt *catchbody   = it.second;
        LabelPrim *catlbl = new LabelPrim (stmtscope, stmttoken, "try-cat");
        prevprim = prevprim->setLinext (catlbl);
        CatEntPrim *cep = new CatEntPrim (stmtscope, vardecl->getDefTok ());
        cep->catlblprim = catlbl;
        cep->thrownvar  = vardecl;
        cep->ttypevar   = tvardecl;
        prevprim = prevprim->setLinext (cep);
        prevprim = catchbody->generstmt (prevprim);
        prevprim = prevprim->setLinext (new JumpPrim (stmtscope, stmttoken, finendlbl));
        tbp->flonexts.push_back (catlbl);
        tbp->catlblprims.push_back ({ vardecl->getType (), catlbl });
    }

    // insert 'innerlabel: call finally(), returning to outerlabel' for each wrapped goto
    for (auto it : stmtscope->castTryScope ()->trylabels) {
        TryLabelStmt *tls = it.second;
        prevprim = prevprim->setLinext (tls->getLabelPrim ());
        prevprim = prevprim->setLinext (new FinCallPrim (stmtscope, stmttoken, finlbl, tls->outerlabel->getLabelPrim ()));
        frp->flonexts.push_back (tls->outerlabel->getLabelPrim ());
    }

    // last comes a 'finendlbl: call finally(), returning to endlbl'
    prevprim = prevprim->setLinext (finendlbl);
    prevprim = prevprim->setLinext (new FinCallPrim (stmtscope, stmttoken, finlbl, endlbl));
    frp->flonexts.push_back (endlbl);

    // finally:
    //  __trystack = __trymark.outer;
    //  __trymark.outer = return address;
    prevprim = prevprim->setLinext (finlbl);
    FinEntPrim *fep = new FinEntPrim (stmtscope, fintoken);
    fep->finlblprim = finlbl;
    fep->trymarkvar = trymarkvar;
    prevprim = prevprim->setLinext (fep);

    // output source-specified finally code
    if (this->finstmt != nullptr) {
        prevprim = this->finstmt->generstmt (prevprim);
    }

    //  jump *__trymark.outer
    // end:
    frp->trymarkvar = trymarkvar;
    prevprim = prevprim->setLinext (frp);
    return prevprim->setLinext (endlbl);
}

Prim *TryLabelStmt::generstmt (Prim *prevprim)
{
    printerror (this->getStmtToken (), "trylabelstmt not implemented"); //TODO
    return prevprim;
}

///////////////////
//  EXPRESSIONS  //
///////////////////

Prim *ArrayInitExpr::generexpr (Prim *prevprim, ValDecl **exprval_r)
{
    assert_false ();   // handled by ArrayInitExpr::outputStackdInit()
}

Prim *AsnopExpr::generexpr (Prim *prevprim, ValDecl **exprval_r)
{
    if (checkConstExpr (exprval_r)) return prevprim;

    // make up primitive and fill in right-hand operand value
    ValDecl *riteval;
    prevprim = riteexpr->generexpr (prevprim, &riteval);

    // make up primitive and fill in left-hand operand value
    ValDecl *leftval;
    prevprim = leftexpr->generexpr (prevprim, &leftval);

    if (exprval_r == nullptr) {
        AsnopPrim *ap = new AsnopPrim (exprscope, exprtoken);
        ap->aval = leftval;
        ap->bval = riteval;
        return prevprim->setLinext (ap);
    }

    *exprval_r = newTempVarDecl (this->getType ());

    AsnopPrim *ap1 = new AsnopPrim (exprscope, exprtoken);
    ap1->aval = *exprval_r;
    ap1->bval = riteval;
    prevprim  = prevprim->setLinext (ap1);

    AsnopPrim *ap2 = new AsnopPrim (exprscope, exprtoken);
    ap2->aval = leftval;
    ap2->bval = *exprval_r;
    return prevprim->setLinext (ap2);
}

Prim *BlockExpr::generexpr (Prim *prevprim, ValDecl **exprval_r)
{
    for (Stmt *stmt : topstmts) {
        prevprim = stmt->generstmt (prevprim);
    }
    return laststmt->expr->generexpr (prevprim, exprval_r);
}

Prim *CallExpr::generexpr (Prim *prevprim, ValDecl **exprval_r)
{
    if (checkConstExpr (exprval_r)) return prevprim;

    CallStartPrim *csp = new CallStartPrim (exprscope, exprtoken);
    CallPrim *cp = new CallPrim (exprscope, exprtoken);

    csp->callprim = cp;
    cp->callstartprim = csp;

    prevprim = prevprim->setLinext (csp);

    for (Expr *argexpr : argexprs) {

        // generate code to compute argument
        ValDecl *argval;
        prevprim = argexpr->generexpr (prevprim, &argval);
        prevprim = cp->pushargval (prevprim, argval);
    }

    // see if there is a 'this' pointer to pass to function
    MFuncExpr *mfuncexpr = funcexpr->castMFuncExpr ();
    if (mfuncexpr == nullptr) {

        // compute non-member function entrypoint
        prevprim = funcexpr->generexpr (prevprim, &cp->entval);
    } else {

        // compute 'this' pointer
        prevprim = mfuncexpr->ptrexpr->generexpr (prevprim, &cp->thisval);

        // call member function
        switch (mfuncexpr->memfunc->getStorClass ()) {

            // handle virtual function
            case KW_VIRTUAL: {
                if (! mfuncexpr->ignvirt) {
                    prevprim = mfuncexpr->lookupvirtualfunction (prevprim, cp);
                    break;
                }
                // fallthrough
            }

            // non-virtual function
            case KW_NONE: {
                cp->entval = mfuncexpr->memfunc;
                break;
            }

            default: {
                throwerror (exprtoken, "cannot call static function with an instance pointer");
            }
        }
    }

    // set up return value
    cp->retvar = nullptr;
    if (exprval_r != nullptr) {
        *exprval_r = cp->retvar = newTempVarDecl (this->getType ());
    }

    return prevprim->setLinext (cp);
}

// find virtual function's entry in vtable
//  input:
//   cp->thisval = 'this' value being passed to function
//  output:
//   cp->entval  = filled in with vtable entry
Prim *MFuncExpr::lookupvirtualfunction (Prim *prevprim, CallPrim *cp)
{
    // type 'this' was declared as - assumed to be a struct type
    StructType *structtype = cp->thisval->getType ()->stripCVMod ()->castPtrType ()->getBaseType ()->stripCVMod ()->castStructType ();

    // read vtable pointer into a temp variable
    //  vtablepointer = thisval->__vtableptr
    assert (structtype->getMemberOffset (exprtoken, "__vtableptr") == 0);
    VarDecl *castedstructpointer = newTempVarDecl (structtype->vtabletype->getPtrType ()->getPtrType ());
    CastPrim *structptrcasted = new CastPrim (exprscope, exprtoken);
    structptrcasted->ovar = castedstructpointer;
    structptrcasted->aval = cp->thisval;
    prevprim = prevprim->setLinext (structptrcasted);
    PtrDecl *vtablepointer = new PtrDecl (exprscope, exprtoken, castedstructpointer);

    // get offset of desired function pointer within the vtable struct
    tsize_t memvtoffset = structtype->vtabletype->getMemberOffset (exprtoken, this->memfunc->getName ());

    // get a pointer to the vtable entry
    VarDecl *vfuncentrypointer = newTempVarDecl (this->memfunc->getType ()->getPtrType ()->getPtrType ());
    BinopPrim *getvfuncentrypointer = new BinopPrim (exprscope, exprtoken);
    getvfuncentrypointer->opcode = OP_ADD;
    getvfuncentrypointer->ovar   = vfuncentrypointer;
    getvfuncentrypointer->aval   = vtablepointer;
    getvfuncentrypointer->bval   = NumDecl::createsizeint (memvtoffset, false);
    prevprim = prevprim->setLinext (getvfuncentrypointer);

    // read function pointer from vtable entry
    cp->entval = new PtrDecl (exprscope, exprtoken, vfuncentrypointer);

    return prevprim;
}

Prim *CastExpr::generexpr (Prim *prevprim, ValDecl **exprval_r)
{
    if (exprval_r == nullptr) return subexpr->generexpr (prevprim, nullptr);

    Type      *castnc = casttype->stripCVMod ();
    FloatType *castft = castnc->castFloatType ();
    IntegType *castit = castnc->castIntegType ();
    PtrType   *castpt = castnc->castPtrType ();

    // maybe a numeric constant is being cast to a particular numeric type
    // - if so, create a numeric constant of that specific type
    NumValue nv;
    NumCat nc = subexpr->isNumConstExpr (&nv);
    switch (nc) {
        case NC_NONE: break;
        case NC_FLOAT: {
            //TODO warn of overflow
            if (castft != nullptr) {
                *exprval_r = NumDecl::createtyped (exprtoken, castft, NC_FLOAT, nv);
                return prevprim;
            }
            if (castit != nullptr) {
                if (castit->getSign ()) {
                    nv.s = (int64_t) nv.f;
                    *exprval_r = NumDecl::createtyped (exprtoken, castit, NC_SINT, nv);
                } else {
                    nv.u = (uint64_t) nv.f;
                    *exprval_r = NumDecl::createtyped (exprtoken, castit, NC_UINT, nv);
                }
                return prevprim;
            }
            throwerror (exprtoken, "invalid cast of constant '%g' to '%s'", nv.f, castnc->getName ());
        }
        case NC_SINT: {
            //TODO warn of overflow
            if (castft != nullptr) {
                nv.f = nv.s;
                *exprval_r = NumDecl::createtyped (exprtoken, castft, NC_FLOAT, nv);
                return prevprim;
            }
            if (castit != nullptr) {
                if (castit->getSign ()) {
                    *exprval_r = NumDecl::createtyped (exprtoken, castit, NC_SINT, nv);
                } else {
                    nv.u = nv.s;
                    *exprval_r = NumDecl::createtyped (exprtoken, castit, NC_UINT, nv);
                }
                return prevprim;
            }
            if (castpt != nullptr) {
                assert (! sizetype->getSign ());
                nv.u = nv.s;
                *exprval_r = NumDecl::createtyped (exprtoken, castpt, NC_UINT, nv);
                return prevprim;
            }
            throwerror (exprtoken, "invalid cast of constant '%lld' to '%s'", (long long) nv.s, castnc->getName ());
        }
        case NC_UINT: {
            //TODO warn of overflow
            if (castft != nullptr) {
                nv.f = nv.u;
                *exprval_r = NumDecl::createtyped (exprtoken, castft, NC_FLOAT, nv);
                return prevprim;
            }
            if (castit != nullptr) {
                if (castit->getSign ()) {
                    nv.s = nv.u;
                    *exprval_r = NumDecl::createtyped (exprtoken, castit, NC_SINT, nv);
                } else {
                    *exprval_r = NumDecl::createtyped (exprtoken, castit, NC_UINT, nv);
                }
                return prevprim;
            }
            if (castpt != nullptr) {
                assert (! sizetype->getSign ());
                *exprval_r = NumDecl::createtyped (exprtoken, castpt, NC_UINT, nv);
                return prevprim;
            }
            throwerror (exprtoken, "invalid cast of constant '%llu' to '%s'", (unsigned long long) nv.u, castnc->getName ());
        }
        default: break;
    }

    if (checkConstExpr (exprval_r)) return prevprim;

    CastPrim *cp = new CastPrim (exprscope, exprtoken);
    prevprim = subexpr->generexpr (prevprim, &cp->aval);
    assert (cp->aval != nullptr);
    *exprval_r = cp->ovar = newTempVarDecl (casttype);
    return prevprim->setLinext (cp);
}

Prim *MFuncExpr::generexpr (Prim *prevprim, ValDecl **exprval_r)
{
    throwerror (exprtoken, "member function can only be used in a calling context");
}

Prim *QMarkExpr::generexpr (Prim *prevprim, ValDecl **exprval_r)
{
    if (checkConstExpr (exprval_r)) return prevprim;

    LabelPrim *donelbl = new LabelPrim (exprscope, exprtoken, "cond-done");
    LabelPrim *elselbl = new LabelPrim (exprscope, exprtoken, "cond-else");

    VarDecl *ovar = nullptr;
    if (exprval_r != nullptr) {
        *exprval_r = ovar = newTempVarDecl (this->getType ());
    }

    // if condition is false goto elselbl
    prevprim = Stmt::ifgoto (prevprim, condexpr, false, elselbl);

    // fall through to true expression if condition is true
    if (ovar == nullptr) {
        prevprim = trueexpr->generexpr (prevprim, nullptr);
    } else {
        AsnopPrim *tmp = new AsnopPrim (exprscope, exprtoken);
        tmp->aval = ovar;
        prevprim = trueexpr->generexpr (prevprim, &tmp->bval);
        prevprim = prevprim->setLinext (tmp);
    }

    // jump over the false statement
    prevprim = alwaysgoto (prevprim, donelbl);

    // next comes the else label
    prevprim = prevprim->setLinext (elselbl);

    // compute false value
    if (ovar == nullptr) {
        prevprim = falsexpr->generexpr (prevprim, nullptr);
    } else {
        AsnopPrim *fmp = new AsnopPrim (exprscope, exprtoken);
        fmp->aval = ovar;
        prevprim = falsexpr->generexpr (prevprim, &fmp->bval);
        prevprim = prevprim->setLinext (fmp);
    }

    // finally the done label
    prevprim = prevprim->setLinext (donelbl);

    return prevprim;
}

// sizeof should always be a compile-time constant
Prim *SizeofExpr::generexpr (Prim *prevprim, ValDecl **exprval_r)
{
    if (exprval_r == nullptr) return prevprim;
    if (! checkConstExpr (exprval_r)) assert_false ();
    return prevprim;
}

Prim *StringInitExpr::generexpr (Prim *prevprim, ValDecl **exprval_r)
{
    assert_false ();   // handled by StringInitExpr::outputStackdInit()
}

Prim *StructInitExpr::generexpr (Prim *prevprim, ValDecl **exprval_r)
{
    assert_false ();   // handled by StructInitExpr::outputStackdInit()
}

// for variable declarations (VarDecl), we must return the variable itself so AsnopExpr will see the original variable itself
Prim *ValExpr::generexpr (Prim *prevprim, ValDecl **exprval_r)
{
    if (exprval_r == nullptr) return prevprim;
    if (checkConstExpr (exprval_r)) return prevprim;
    *exprval_r = valdecl;
    return prevprim;
}

// helpers //

// see if the expression is a compile-time constant
// if so, create a numeric token and return that as the expression's value
bool Expr::checkConstExpr (ValDecl **exprval_r)
{
    // check for numeric constant
    NumValue nv;
    NumCat nc = isNumConstExpr (&nv);
    if (nc != NC_NONE) {
        if (exprval_r != nullptr) {
            *exprval_r = NumDecl::createtyped (exprtoken, getType ()->stripCVMod ()->castNumType (), nc, nv);
        }
        return true;
    }

    // check for assembly label constant such as 'printf' or 'intarray+6'
    char *constval = isAsmConstExpr ();
    if (constval == nullptr) return false;

    // make a value of that location
    if (exprval_r != nullptr) {
        *exprval_r = new AsmDecl (constval, exprtoken, getType ());
    }
    return true;
}

// create a temp var for an expression's computation result
VarDecl *Expr::newTempVarDecl (Type *type)
{
    return VarDecl::newtemp (exprscope, exprtoken, type);
}

VarDecl *VarDecl::newtemp (Scope *scope, Token *token, Type *type)
{
    static uint32_t tempnum = 0;

    char *name = new char[18];
    sprintf (name, "tmp.%u", ++ tempnum);
    return new VarDecl (name, scope, token, type->stripCVMod (), KW_NONE);
}

//////////////////////////////////////////////////////////////////
//  generate stack/register-based variable initialization code  //
//////////////////////////////////////////////////////////////////

// output code to initialize stack/register-based scalar (numeric, pointer) variable
// use a simple assignment, ie, valdecl = initexpr
Prim *Expr::outputStackdInit (Prim *prevprim, ValDecl *valdecl, bool isarray, tsize_t arrsize)
{
    assert (! isarray);
    AsnopPrim *ap = new AsnopPrim (this->exprscope, this->exprtoken);
    ap->aval = valdecl;
    prevprim = this->generexpr (prevprim, &ap->bval);
    prevprim = prevprim->setLinext (ap);
    return prevprim;
}

// output code to initialize stack-based array variable
// output initializations for each specified element of the array
Prim *ArrayInitExpr::outputStackdInit (Prim *prevprim, ValDecl *valdecl, bool isarray, tsize_t arrsize)
{
    assert (isarray);
    PtrType *ptrtype = valdecl->getType ()->stripCVMod ()->castPtrType ();
    assert (valuetype->stripCVMod () == ptrtype->getBaseType ()->stripCVMod ());

    //TODO maybe zero array out

    // go through all initializations
    for (std::pair<tsize_t,Expr *> pair : exprs) {
        tsize_t index = pair.first;
        Expr *expr = pair.second;
        tsize_t memofs = index * valuetype->getTypeSize (exprtoken);
        prevprim = stackdMemberInit (prevprim, valdecl, memofs, valuetype, false, 0, expr);
    }

    return prevprim;
}

// output code to initialize stack-based char array variable
// output a memcpy() call to copy string const to array
Prim *StringInitExpr::outputStackdInit (Prim *prevprim, ValDecl *valdecl, bool isarray, tsize_t arrsize)
{
    static FuncDecl *memcpyfuncdecl;

    if (memcpyfuncdecl == nullptr) {
        ParamScope *memcpypscope = new ParamScope (globalscope);
        std::vector<VarDecl *> *memcpyparams = new std::vector<VarDecl *> ();
        memcpyparams->push_back (new VarDecl ("dstptr", memcpypscope, intdeftok, chartype->getPtrType (), KW_NONE));
        memcpyparams->push_back (new VarDecl ("srcptr", memcpypscope, intdeftok, chartype->getConstType ()->getPtrType (), KW_NONE));
        memcpyparams->push_back (new VarDecl ("length", memcpypscope, intdeftok, sizetype, KW_NONE));
        FuncType *memcpytype = FuncType::create (intdeftok, voidtype, memcpyparams);
        memcpyfuncdecl = new FuncDecl ("memcpy", globalscope, intdeftok, memcpytype, KW_EXTERN, memcpyparams, memcpypscope);
    }

    tsize_t strlennum = strtok->getStrlen () + 1;

    assert (isarray);
    if (strlennum > arrsize) {
        printerror (exprtoken, "string length %lu+1 greater than array size %lu", (unsigned long) strlennum, (unsigned long) arrsize);
        strlennum = arrsize;
    }

    CallStartPrim *csp = new CallStartPrim (exprscope, exprtoken);
    CallPrim *cp = new CallPrim (exprscope, exprtoken);
    csp->callprim = cp;
    cp->callstartprim = csp;
    cp->retvar = nullptr;
    cp->entval = memcpyfuncdecl;

    prevprim = prevprim->setLinext (csp);
    prevprim = cp->pushargval (prevprim, valdecl);
    prevprim = cp->pushargval (prevprim, StrDecl::create (strtok));

    assert (! sizetype->getSign ());
    NumValue nv;
    nv.u = strlennum;
    prevprim = cp->pushargval (prevprim, NumDecl::createtyped (exprtoken, sizetype, NC_UINT, nv));

    return prevprim->setLinext (cp);
}

// output code to initialize stack-based struct variable
// output initializations for each specified member of the struct
Prim *StructInitExpr::outputStackdInit (Prim *prevprim, ValDecl *valdecl, bool isarray, tsize_t arrsize)
{
    PtrType *ptrtype = structype->getPtrType ();

    // make a pointer to the struct
    VarDecl *ptrvar = newTempVarDecl (ptrtype);
    UnopPrim *ptrprim = new UnopPrim (exprscope, exprtoken);
    ptrprim->opcode = OP_ADDROF;
    ptrprim->ovar = ptrvar;
    ptrprim->aval = valdecl;
    prevprim = prevprim->setLinext (ptrprim);

    // maybe call function to fill in vtable pointers and call constructor
    prevprim = structype->genInitCall (exprscope, exprtoken, prevprim, valdecl, isarray, arrsize);

    // go through all initializations
    for (std::pair<tsize_t,Expr *> pair : exprs) {
        tsize_t index = pair.first;
        Expr *expr = pair.second;
        StructType *memstruct;
        VarDecl *memvar;
        tsize_t memofs;
        if (! structype->getMemberByIndex (expr->exprtoken, index, &memstruct, &memvar, &memofs)) {
            assert_false ();
        }
        tsize_t memarrsize = memvar->isArray () ? memvar->getArrSize () : 0;
        prevprim = stackdMemberInit (prevprim, ptrvar, memofs, memvar->getType (), memvar->isArray (), memarrsize, expr);
    }

    return prevprim;
}

// initialize member of stack-based array or struct
//  input:
//   prevprim = previously output prim
//   ptrvar = points to beginning of array or struct
//   memofs = offset in bytes of member to init
//   memvar = variable describing member to init
//   memtype = type of member to init
//   expr = initialization value
//  output:
//   returns prim stream updated with code to init member
Prim *Expr::stackdMemberInit (Prim *prevprim, ValDecl *ptrvar, tsize_t memofs, Type *memtype, bool memisarray, tsize_t memarrsize, Expr *expr)
{
    if (memisarray) {
        VarDecl *elemptr = newTempVarDecl (memtype->getConstType ());
        BinopPrim *bp = new BinopPrim (exprscope, exprtoken);
        bp->opcode = OP_ADD;
        bp->ovar = elemptr;
        bp->aval = ptrvar;
        bp->bval = NumDecl::createsizeint (memofs, false);
        prevprim = prevprim->setLinext (bp);
        prevprim = expr->outputStackdInit (prevprim, elemptr, memisarray, memarrsize);
    } else {
        BinopPrim *bp = new BinopPrim (exprscope, exprtoken);
        bp->opcode = OP_ADD;
        bp->ovar = newTempVarDecl (memtype->getPtrType ());
        bp->aval = ptrvar;
        bp->bval = NumDecl::createsizeint (memofs, false);
        prevprim = prevprim->setLinext (bp);
        PtrDecl *elemptr = new PtrDecl (exprscope, exprtoken, bp->ovar);
        prevprim = expr->outputStackdInit (prevprim, elemptr, memisarray, memarrsize);
    }
    return prevprim;
}

// put in temp var that only gets passed to function as part of argument list
// this temp is actually in the arg list and gets wiped from stack when called function returns
Prim *CallPrim::pushargval (Prim *prevprim, ValDecl *argval)
{
    VarDecl *argvar = VarDecl::newtemp (primscope, primtoken, argval->getType ());
    argvars.push_back (argvar);
    argvar->iscallarg = this;
    AsnopPrim *ap = new AsnopPrim (primscope, primtoken);
    ap->aval = argvar;
    ap->bval = argval;
    return prevprim->setLinext (ap);
}
