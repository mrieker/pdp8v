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
/////////////////////////////////////////////////////////
//  Optimize primitives (intermediate representation)  //
/////////////////////////////////////////////////////////

#include "decl.h"
#include "prim.h"

void FuncDecl::optprims ()
{
    bool didsomething;
    int passno = 0;

    do {
        printf ("; ------------------------------\n");
        printf ("; OPTPRIMS %5d %s\n", ++ passno, getEncName ());
        for (Prim *prim = entryprim; prim != nullptr; prim = prim->linext) {
            prim->dumpprim ();
        }

        didsomething = false;

        // remove redundant labels
        for (Prim *prim = entryprim; prim != nullptr; prim = prim->linext) {
            LabelPrim *lp = prim->castLabelPrim ();
            if (lp != nullptr) {
                LabelPrim *lq = lp->linext->castLabelPrim ();
                if (lq != nullptr) {
                    lp->resetLinext (lq->linext);
                    for (Prim *p = entryprim; p != nullptr; p = p->linext) {
                        p->replaceLabel (lq, lp);
                    }
                    delete lq;
                    didsomething = true;
                }
            }
        }

        // remove unreferenced labels
        for (Prim *prim = entryprim; prim != nullptr; prim = prim->linext) {
            LabelPrim *lp = prim->castLabelPrim ();
            if (lp != nullptr) {
                lp->lblrefd = false;
            }
        }
        for (Prim *prim = entryprim; prim != nullptr; prim = prim->linext) {
            prim->markLabelRefs ();
        }
        Prim *prevprim = nullptr;
        for (Prim *prim = entryprim; prim != nullptr; prim = prim->linext) {
            LabelPrim *lp = prim->castLabelPrim ();
            if ((lp != nullptr) && ! lp->lblrefd && (prevprim != nullptr)) {
                prevprim->resetLinext (lp->linext);
                delete lp;
                prim = prevprim;
            }
            prevprim = prim;
        }

        // other optimizations
        for (Prim *prim = entryprim; prim != nullptr; prim = prim->linext) {

            // remove write-only stack-based variables
            // eg,
            //  ASNOP tempx = something
            //  no other refs to tempx
            ValDecl *outva = prim->writesTemp ();
            VarDecl *tempx = (outva == nullptr) ? nullptr : outva->castVarDecl ();
            if ((tempx != nullptr) && ! tempx->isStaticMem () && ! prim->hasSideEffects ()) {
                Prim *prevprim = nullptr;
                Prim *pp = nullptr;
                for (Prim *p = entryprim; p != nullptr; p = p->linext) {
                    if (p == prim) {
                        prevprim = pp;
                        continue;
                    }

                    // can't optimize if anything takes address of tempx
                    UnopPrim *up = p->castUnopPrim ();
                    if ((up != nullptr) && (up->opcode == OP_DEREF) && (up->aval == tempx)) goto refdelsewhere_b;

                    // can't optimize if tempx read or written anywhere else
                    if (p->readsOrWritesVar (tempx)) goto refdelsewhere_b;

                    pp = p;
                }

                prevprim->resetLinext (prim->linext);
                didsomething = true;
                prim = prevprim;
                continue;
            refdelsewhere_b:;
            }

            // look for
            //  BINOP tempx = whatever    ... or UNOP, CAST, etc
            //  ASNOP vary  = tempx
            // no other references to tempx
            // replace with
            //  BINOP vary = whatever
            if (tempx != nullptr) {
                AsnopPrim *ap = prim->linext->castAsnopPrim ();
                if ((ap != nullptr) && (ap->bval == tempx)) {

                    bool inblock = false;
                    for (Prim *p = entryprim; p != nullptr; p = p->linext) {

                        // can't optimize if anything takes address of tempx
                        UnopPrim *up = p->castUnopPrim ();
                        if ((up != nullptr) && (up->opcode == OP_DEREF) && (up->aval == tempx)) goto refdelsewhere_a;

                        // can't optimize if tempx read or written anywhere except in rest of this basic block
                        if (p == prim) {
                            inblock = true;
                            continue;
                        }
                        if (p->castLabelPrim () != nullptr) {
                            inblock = false;
                            continue;
                        }
                        if (! inblock && p->readsOrWritesVar (tempx)) goto refdelsewhere_a;
                    }

                    {
                        ValDecl *valy = ap->aval;

                        // replace write of tempx in the binop with write to valy
                        prim->replaceWrites (tempx, valy);

                        // remove the ASNOP from instruction stream
                        prim->resetLinext (ap->linext);

                        // replace reads of tempx with valy in the rest of the basic block up to next write of tempx
                        for (Prim *p = prim; (p = p->linext) != nullptr;) {
                            if (p->castLabelPrim () != nullptr) break;
                            if (p->writesTemp () == tempx) break;
                            p->replaceReads (tempx, valy);
                        }
                    }
                    didsomething = true;
                refdelsewhere_a:;
                }
            }

            // look for
            //   COMP   if (compexpr) goto truelbl
            //   JUMP   falselbl
            //  truelbl:
            // replace with
            //   COMP   if (! compexpr) goto falselbl
            //  truelbl:
            CompPrim *compprim = prim->castCompPrim ();
            if (compprim != nullptr) {
                JumpPrim *jumpprim = compprim->linext->castJumpPrim ();
                if (jumpprim != nullptr) {
                    LabelPrim *labelprim = jumpprim->linext->castLabelPrim ();
                    if (labelprim == compprim->flonexts[0]) {
                        compprim->opcode = Stmt::revcmpop (compprim->opcode);
                        compprim->flonexts[0] = jumpprim->flonexts[0];
                        compprim->resetLinext (labelprim);
                        didsomething = true;
                    }
                }
            }

            // look for
            //   TEST   if (testexpr) goto truelbl
            //   JUMP   falselbl
            //  truelbl:
            // replace with
            //   TEST   if (! testexpr) goto falselbl
            //  truelbl:
            TestPrim *testprim = prim->castTestPrim ();
            if (testprim != nullptr) {
                JumpPrim *jumpprim = testprim->linext->castJumpPrim ();
                if (jumpprim != nullptr) {
                    LabelPrim *labelprim = jumpprim->linext->castLabelPrim ();
                    if (labelprim == testprim->flonexts[0]) {
                        testprim->opcode = Stmt::revcmpop (testprim->opcode);
                        testprim->flonexts[0] = jumpprim->flonexts[0];
                        testprim->resetLinext (labelprim);
                        didsomething = true;
                    }
                }
            }

            // look for
            //    JUMP  somelabel
            //   somelabel:
            // replace with
            //   somelabel:
            Prim *nextprim = prim->linext;
            JumpPrim *jumpprim = (nextprim == nullptr) ? nullptr : nextprim->castJumpPrim ();
            LabelPrim *labelprim = (jumpprim == nullptr) ? nullptr : jumpprim->linext->castLabelPrim ();
            if ((labelprim != nullptr) && (labelprim == jumpprim->flonexts[0])) {
                prim->resetLinext (labelprim);
                didsomething = true;
            }

            // look for
            //  JUMP  somelabel
            //  deadcode-not-a-label
            // replace with
            //  JUMP  somelabel
            while (! prim->hasFallthru () && (prim->linext != nullptr) && (prim->linext->castLabelPrim () == nullptr)) {
                prim->resetLinext (prim->linext->linext);
                didsomething = true;
            }
        }
        printf ("; ------------------------------\n");
    } while (didsomething);
}

bool AsnopPrim::readsOrWritesVar (VarDecl *var)
{
    return (aval == var) || (bval == var) || aval->isPtrDeclOf (var) || bval->isPtrDeclOf (var);
}

void AsnopPrim::replaceReads (ValDecl *oldtemp, ValDecl *newtemp)
{
    if (bval == oldtemp) bval = newtemp;
    aval->replacePtrDeclOf (oldtemp, newtemp);
    bval->replacePtrDeclOf (oldtemp, newtemp);
}

void AsnopPrim::replaceWrites (ValDecl *oldtemp, ValDecl *newtemp)
{
    if (aval == oldtemp) aval = newtemp;
}

bool BinopPrim::readsOrWritesVar (VarDecl *var)
{
    return (ovar == var) || (aval == var) || (bval == var) || ovar->isPtrDeclOf (var) || aval->isPtrDeclOf (var) || bval->isPtrDeclOf (var);
}

void BinopPrim::replaceReads (ValDecl *oldtemp, ValDecl *newtemp)
{
    if (aval == oldtemp) aval = newtemp;
    if (bval == oldtemp) bval = newtemp;
    ovar->replacePtrDeclOf (oldtemp, newtemp);
    aval->replacePtrDeclOf (oldtemp, newtemp);
    bval->replacePtrDeclOf (oldtemp, newtemp);
}

void BinopPrim::replaceWrites (ValDecl *oldtemp, ValDecl *newtemp)
{
    if (ovar == oldtemp) ovar = newtemp;
}

bool CallPrim::readsOrWritesVar (VarDecl *var)
{
    if ((retvar != nullptr) && ((retvar == var) || retvar->isPtrDeclOf (var))) return true;
    if ((thisval != nullptr) && ((thisval == var) || thisval->isPtrDeclOf (var))) return true;
    if ((entval == var) || entval->isPtrDeclOf (var)) return true;
    for (VarDecl *argvar : argvars) {
        if ((argvar == var) || argvar->isPtrDeclOf (var)) return true;
    }
    return false;
}

void CallPrim::replaceReads (ValDecl *oldtemp, ValDecl *newtemp)
{
    if (retvar != nullptr) retvar->replacePtrDeclOf (oldtemp, newtemp);
    if (thisval == oldtemp) thisval = newtemp;
    if (entval == oldtemp) entval = newtemp;
    entval->replacePtrDeclOf (oldtemp, newtemp);
    for (int i = argvars.size (); -- i >= 0;) {
        ValDecl *argvar = argvars[i];
        assert (argvar != oldtemp);
    }
}

void CallPrim::replaceWrites (ValDecl *oldtemp, ValDecl *newtemp)
{
    if (retvar == oldtemp) retvar = newtemp;
}

bool CastPrim::readsOrWritesVar (VarDecl *var)
{
    return (ovar == var) || (aval == var) || ovar->isPtrDeclOf (var) || aval->isPtrDeclOf (var);
}

void CastPrim::replaceReads (ValDecl *oldtemp, ValDecl *newtemp)
{
    if (aval == oldtemp) aval = newtemp;
    ovar->replacePtrDeclOf (oldtemp, newtemp);
    aval->replacePtrDeclOf (oldtemp, newtemp);
}

void CastPrim::replaceWrites (ValDecl *oldtemp, ValDecl *newtemp)
{
    if (ovar == oldtemp) ovar = newtemp;
}

bool CompPrim::readsOrWritesVar (VarDecl *var)
{
    return (aval == var) || (bval == var) || aval->isPtrDeclOf (var) || bval->isPtrDeclOf (var);
}

void CompPrim::replaceReads (ValDecl *oldtemp, ValDecl *newtemp)
{
    if (aval == oldtemp) aval = newtemp;
    if (bval == oldtemp) bval = newtemp;
    aval->replacePtrDeclOf (oldtemp, newtemp);
    bval->replacePtrDeclOf (oldtemp, newtemp);
}

bool RetPrim::readsOrWritesVar (VarDecl *var)
{
    return (rval != nullptr) && ((rval == var) || rval->isPtrDeclOf (var));
}

void RetPrim::replaceReads (ValDecl *oldtemp, ValDecl *newtemp)
{
    if (rval != nullptr) {
        if (rval == oldtemp) rval = newtemp;
        rval->replacePtrDeclOf (oldtemp, newtemp);
    }
}

bool TestPrim::readsOrWritesVar (VarDecl *var)
{
    return (aval == var) || aval->isPtrDeclOf (var);
}

void TestPrim::replaceReads (ValDecl *oldtemp, ValDecl *newtemp)
{
    if (aval == oldtemp) aval = newtemp;
    aval->replacePtrDeclOf (oldtemp, newtemp);
}

bool ThrowPrim::readsOrWritesVar (VarDecl *var)
{
    if (thrownval == nullptr) return false;
    return (thrownval == var) || thrownval->isPtrDeclOf (var);
}

void ThrowPrim::replaceReads (ValDecl *oldtemp, ValDecl *newtemp)
{
    if (thrownval != nullptr) {
        if (thrownval == oldtemp) thrownval = newtemp;
        thrownval->replacePtrDeclOf (oldtemp, newtemp);
    }
}

bool UnopPrim::readsOrWritesVar (VarDecl *var)
{
    return (ovar == var) || (aval == var) || ovar->isPtrDeclOf (var) || aval->isPtrDeclOf (var);
}

void UnopPrim::replaceReads (ValDecl *oldtemp, ValDecl *newtemp)
{
    if (aval == oldtemp) aval = newtemp;
    ovar->replacePtrDeclOf (oldtemp, newtemp);
    aval->replacePtrDeclOf (oldtemp, newtemp);
}

void UnopPrim::replaceWrites (ValDecl *oldtemp, ValDecl *newtemp)
{
    if (ovar == oldtemp) ovar = newtemp;
}

// modify the next-in-flow primitive
void Prim::resetLinext (Prim *nextprim)
{
    int numflonexts = flonexts.size ();
    for (int i = 0; i < numflonexts; i ++) {
        if (flonexts[i] == linext) flonexts[i] = nextprim;
    }
    linext = nextprim;

    if (this->castTestPrim () != nullptr) {
        assert (flonexts.size () == 2);
        assert (flonexts[1] == linext);
        assert (flonexts[0]->castLabelPrim () != nullptr);
    }
}

// replace references to old label with references to new label
void Prim::replaceLabel (LabelPrim *oldlabel, LabelPrim *newlabel)
{
    int numflonexts = flonexts.size ();
    for (int i = 0; i < numflonexts; i ++) {
        if (flonexts[i] == oldlabel) flonexts[i] = newlabel;
    }
    assert (linext != oldlabel);
}

void FinCallPrim::replaceLabel (LabelPrim *oldlabel, LabelPrim *newlabel)
{
    if (finlblprim == oldlabel) finlblprim = newlabel;
    if (retlblprim == oldlabel) retlblprim = newlabel;
    this->Prim::replaceLabel (oldlabel, newlabel);
}

// mark any referenced labels as referenced
void Prim::markLabelRefs ()
{
    for (Prim *prim : flonexts) {
        LabelPrim *lp = prim->castLabelPrim ();
        if (lp != nullptr) lp->lblrefd = true;
    }
}
