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
////////////////////////////////////////////////////////////////
//  Optimization at assembly-language level                   //
//  Also assigns hardware registers and maps stack variables  //
////////////////////////////////////////////////////////////////

#include <algorithm>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "mach.h"

void MachFile::optmachs ()
{
    bool didsomething;

    int passno = 0;

    do {
        opt_dump (passno ++);
        didsomething = false;
        didsomething = opt_01 (didsomething);
        didsomething = opt_02 (didsomething);
        didsomething = opt_03 (didsomething);
        didsomething = opt_04 (didsomething);
        didsomething = opt_05 (didsomething);
        didsomething = opt_06 (didsomething);
    } while (didsomething);

    // assign stack variable offsets
    opt_asnstack ();

    do {
        opt_dump (passno ++);

        didsomething = false;

        // optimize register constants using stepRegState() calls
        didsomething = opt_20 (didsomething);

        // strip out write-only registers
        didsomething = opt_21 (didsomething);

        // maybe stick in some LDAs for ADDs
        didsomething = opt_22 (didsomething);
    } while (didsomething);

    // assign hardware registers
    opt_asnhregs ();

    opt_40 ();
    opt_41 ();
    opt_42 ();

    // shorten branches
    do didsomething = opt_50 (false);
    while (didsomething);

    // move immediate constants after no-fall-through instructions
    opt_51 ();

    opt_dump (passno ++);
}

// print instructions to stdout
void MachFile::opt_dump (int passno)
{
    printf ("; ------------------------------\n");
    printf ("; OPTMACHS %5d %s\n", passno, funcdecl->getEncName ());
    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {
        Token *token = mach->token;
        printf (";; %p  %s:%d\n", mach, token->getFile (), token->getLine ());
        mach->dumpmach (stdout);
    }
    printf ("; ------------------------------\n");
}

// remove LDW from
//  STW  rn,stk:x
//  ...
//  LDW  rn,stk:x
// remove LDB? from
//  STB  rn,stk:x   rn->regcont==RC_?EXTBYTE
//  ...
//  LDB? rn,stk:x
bool MachFile::opt_01 (bool didsomething)
{
    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {

        mach->stepRegState ();

        Mach *loadsubsequentmach, *storesubsequentmach;
        VarDecl *loadstackvar, *storestackvar;
        MachOp loadopcode, storeopcode;
        Reg *loadregister, *storeregister;
        tsize_t loadstackvaroff, storestackvaroff;

        // check for a STW from a fake register onto the stack
        // fake cuz if we rip out the LDW we want to rename the store's register
        // can also do STB if we know what top bits of register are
        if (! storeOntoStack (mach, &storeopcode, &storeregister, &storestackvar, &storestackvaroff, &storesubsequentmach)) continue;
        if (storeopcode != MO_STW) {
            assert (storeopcode == MO_STB);
            if ((storeregister->regcont != RC_CONST) && (storeregister->regcont != RC_SEXTBYTE) && (storeregister->regcont != RC_ZEXTBYTE)) continue;
        }
        if (! storeregister->regFake ()) continue;
        if (storesubsequentmach == nullptr) continue;

        // check for following LDW/LDBS/LDBU from same var on the stack
        // allow a few intervening instructions but not too many so we can't pile up a lot of these andvrun out of registers
        Mach *interveneendmach = storesubsequentmach;
        for (int i = 15; -- i >= 0;) {
            if (interveneendmach->castLabelMach () != nullptr) goto keepit;
            if (! loadFromStack (interveneendmach, &loadopcode, &loadregister, &loadstackvar, &loadstackvaroff, &loadsubsequentmach)) goto skipld;
            if ((loadstackvar != storestackvar) || (loadstackvaroff != storestackvaroff)) goto skipld;
            if ((loadopcode == MO_LDW)  && (storeopcode == MO_STW)) goto gotload;
            if ((loadopcode == MO_LDBS) && ((storeregister->regcont == RC_CONST) || (storeregister->regcont == RC_SEXTBYTE))) goto gotload;
            if ((loadopcode == MO_LDBU) && ((storeregister->regcont == RC_CONST) || (storeregister->regcont == RC_ZEXTBYTE))) goto gotload;
        skipld:;
            if (storeOntoStack (interveneendmach, &loadopcode, &loadregister, &loadstackvar, &loadstackvaroff, &loadsubsequentmach) && (loadstackvar == storestackvar) && (loadstackvaroff == storestackvaroff)) goto keepit;
            if (! interveneendmach->fallsThrough ()) goto keepit;
            interveneendmach = interveneendmach->nextmach;
        }
        goto keepit;
    gotload:;

        // check for no references to loadregister in intervening instructions
        // check for no writes to storeregister in intervening instructions
        for (Mach *m = storesubsequentmach; m != interveneendmach; m = m->nextmach) {
            if (m->readsReg  (loadregister))  goto keepit;
            if (m->writesReg (loadregister))  goto keepit;
            if (m->writesReg (storeregister)) goto keepit;
        }

        // we have
        //  STW %r101,stk:temp1
        //  ...no references to %r102
        //  ...no writes to %r101
        //  LDW %r102,stk:temp1
        //  ...other uses of %r102
        // replace refs to %r101 (storeregister) with refs to %r102 (loadregister)
        //  STW %r102,stk:temp1
        //  ...
        //  LDW %r102,stk:temp1
        //  ...other uses of %r102
        // we know that storeregister is a fake register so can be renamed
        replacereg (storeregister, loadregister);

        // now it is safe to remove the LDW
        for (Mach *justbeforeload = interveneendmach->prevmach; justbeforeload->nextmach != loadsubsequentmach;) {
            remove (justbeforeload->nextmach);
        }

        didsomething = true;
    keepit:;
    }

    return didsomething;
}

// remove all Stores that write to a stack variable that isn't read anywhere
//TODO track down that variable isn't read before written on all paths
bool MachFile::opt_02 (bool didsomething)
{
    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {

        // check for a store onto a variable on the stack
        MachOp storeopcode;
        Reg *storeregister;
        VarDecl *storestackvar;
        tsize_t storestackvaroff;
        Mach *storesubsequentmach;
        if (! storeOntoStack (mach, &storeopcode, &storeregister, &storestackvar, &storestackvaroff, &storesubsequentmach)) continue;

        // don't optimize out stores that are writing call arguments
        if (storestackvar->iscallarg != nullptr) continue;

        // see if that variable is accessed anywhere else in the function
        for (Mach *mach2 = firstmach; mach2 != nullptr; mach2 = mach2->nextmach) {
            MachOp loadopcode;
            Reg *loadregister;
            VarDecl *loadstackvar;
            tsize_t loadstackvaroff;
            Mach *loadsubsequentmach;
            if (loadFromStack (mach2, &loadopcode, &loadregister, &loadstackvar, &loadstackvaroff, &loadsubsequentmach) && (loadstackvar == storestackvar) && (loadstackvaroff == storestackvaroff)) goto keepit;
            if (addrOfStack (mach2, &loadregister, &loadstackvar, &loadstackvaroff, &loadsubsequentmach) && (loadstackvar == storestackvar)) goto keepit;
        }

        // not referenced anywhere else, remove the store instruction
        mach = mach->prevmach;
        while (mach->nextmach != storesubsequentmach) {
            remove (mach->nextmach);
        }

        didsomething = true;

    keepit:;
    }
    return didsomething;
}

// see if the instruction pointed to by mach is a store onto the stack
//  input:
//   mach = first instruction to look at
//  output:
//   returns false: not a store onto stack
//            true: is a store onto stack
//                  *storeopcode = MO_STB, MO_STW
//                  *storeregister = which register is being stored from
//                  *storestackvar = which variable is being stored into
//                  *storestackvaroff = offset within storestackvar
//                  *storesubsequentmach = instruction following the store
bool MachFile::storeOntoStack (Mach *mach, MachOp *storeopcode, Reg **storeregister, VarDecl **storestackvar, tsize_t *storestackvaroff, Mach **storesubsequentmach)
{
    // check direct store onto stack
    //  STx rd,offs(SP)
    StoreStkMach *ssm = mach->castStoreStkMach ();
    if (ssm != nullptr) {
        *storeopcode   = ssm->mop;
        *storeregister = ssm->getRd ();
        *storestackvar = ssm->stk;
        *storestackvaroff = ssm->raoffs;
        *storesubsequentmach = ssm->nextmach;
        return true;
    }

    // check for long store onto stack
    //  LDW rt,#offs
    //  ADD ru,SP,rt
    //  STx rd,0(ru)
    LdImmStkMach *lism = mach->castLdImmStkMach ();
    if (lism != nullptr) {
        Arith3Mach *a3m = lism->nextmach->castArith3Mach ();
        if (a3m != nullptr) {
            StoreMach *sm = a3m->nextmach->castStoreMach ();
            if (sm != nullptr) {
                Reg *rt = lism->getRd ();
                Reg *ru = a3m->getRd ();
                if ((a3m->getRa () == this->spreg) &&
                        (a3m->getRb () == rt) &&
                        (sm->getRa () == ru) &&
                        (sm->offs == 0)) {
                    *storeopcode   = sm->mop;
                    *storeregister = sm->getRd ();
                    *storestackvar = lism->stk;
                    *storestackvaroff = lism->raoffs;
                    *storesubsequentmach = sm->nextmach;
                    return true;
                }
            }
        }
    }

    return false;
}

// see if the instruction pointed to by mach is a load from something (stack var, imm value, global/static var)
//  input:
//   mach = first instruction to look at
//  output:
//   returns false: not such a load
//            true: is such a load
//                  *loadopcode = MO_LDBS, MO_LDBU, MO_LDW
//                  *loadregister = which register is being loaded into
//                  *loadvalue = which value is being loaded from
//                  *loadvalueoff = offset within loadvalue
//                  *loadsubsequentmach = instruction following the load
bool MachFile::loadFromValue (Mach *mach, MachOp *loadopcode, Reg **loadregister, ValDecl **loadvalue, tsize_t *loadvalueoff, Mach **loadsubsequentmach)
{
    // loading from a stack variable
    VarDecl *lsv;
    if (loadFromStack (mach, loadopcode, loadregister, &lsv, loadvalueoff, loadsubsequentmach)) {
        *loadvalue = lsv;
        return true;
    }

    // loading an immediate value
    LdImmValMach *livm = mach->castLdImmValMach ();
    if (livm != nullptr) {
        *loadopcode   = livm->mop;
        *loadregister = livm->getRd ();
        *loadvalue    = NumDecl::createsizeint (livm->val, false);
        *loadvalueoff = 0;
        *loadsubsequentmach = mach->nextmach;
        return true;
    }

    // loading address of a global/static variable
    LdImmStrMach *lism = mach->castLdImmStrMach ();
    if (lism != nullptr) {
        *loadopcode   = lism->mop;
        *loadregister = lism->getRd ();
        *loadvalue    = lism->val;
        *loadvalueoff = 0;
        *loadsubsequentmach = lism->nextmach;
        return true;
    }

    return false;
}

bool MachFile::isModifyValue (Mach *mach, ValDecl *loadvalue, tsize_t loadvalueoff)
{
    MachOp modifyopcode;
    Reg *modifyregister;
    VarDecl *modifystackvar;
    tsize_t modifystackvaroff;
    Mach *modifysubsequentmach;

    // storing into same offset of stack variable
    if (storeOntoStack (mach, &modifyopcode, &modifyregister, &modifystackvar, &modifystackvaroff, &modifysubsequentmach)) {
        return (modifystackvar == loadvalue) && ((modifystackvaroff & -2) == (loadvalueoff & -2));
    }

    // taking the address of stack variable
    if (addrOfStack (mach, &modifyregister, &modifystackvar, &modifystackvaroff, &modifysubsequentmach)) {
        return modifystackvar == loadvalue;
    }

    // taking the address of global/static variable
    LdImmStrMach *lism = mach->castLdImmStrMach ();
    if (lism != nullptr) {
        return lism->val == loadvalue;
    }

    return false;
}

// see if the instruction pointed to by mach is a load from the stack
//  input:
//   mach = first instruction to look at
//  output:
//   returns false: not a load from stack
//            true: is a load from stack
//                  *loadopcode = MO_LDBS, MO_LDBU, MO_LDW
//                  *loadregister = which register is being loaded into
//                  *loadstackvar = which variable is being loaded from
//                  *loadstackvaroff = offset within loadstackvar
//                  *loadsubsequentmach = instruction following the load
bool MachFile::loadFromStack (Mach *mach, MachOp *loadopcode, Reg **loadregister, VarDecl **loadstackvar, tsize_t *loadstackvaroff, Mach **loadsubsequentmach)
{
    // check direct load from stack
    //  LDx rd,offs(SP)
    LoadStkMach *lsm = mach->castLoadStkMach ();
    if (lsm != nullptr) {
        *loadopcode   = lsm->mop;
        *loadregister = lsm->getRd ();
        *loadstackvar = lsm->stk;
        *loadstackvaroff = lsm->raoffs;
        *loadsubsequentmach = lsm->nextmach;
        return true;
    }

    // check for long load from stack
    //  LDW rt,#offs
    //  ADD ru,SP,rt
    //  LDx rd,0(ru)
    LdImmStkMach *lism = mach->castLdImmStkMach ();
    if (lism != nullptr) {
        Arith3Mach *a3m = lism->nextmach->castArith3Mach ();
        if (a3m != nullptr) {
            LoadMach *lm = a3m->nextmach->castLoadMach ();
            if (lm != nullptr) {
                Reg *rt = lism->getRd ();
                Reg *ru = a3m->getRd ();
                if ((a3m->getRa () == this->spreg) &&
                        (a3m->getRb () == rt) &&
                        (lm->getRa () == ru) &&
                        (lm->offs == 0)) {
                    *loadopcode   = lm->mop;
                    *loadregister = lm->getRd ();
                    *loadstackvar = lism->stk;
                    *loadstackvaroff = lism->raoffs;
                    *loadsubsequentmach = lm->nextmach;
                    return true;
                }
            }
        }
    }

    return false;
}

// see if the instruction pointed to by mach is taking address of a stack variable
//  input:
//   mach = first instruction to look at
//  output:
//   returns false: not an address of stack
//            true: is an address of stack
//                  *addrregister = which register is being loaded into
//                  *addrstackvar = which variable is being loaded from
//                  *addrstackvaroff = offset within addrstackvar
//                  *addrsubsequentmach = instruction following the load
bool MachFile::addrOfStack (Mach *mach, Reg **addrregister, VarDecl **addrstackvar, tsize_t *addrstackvaroff, Mach **addrsubsequentmach)
{
    // check direct load from stack
    //  LDA rd,offs(SP)
    LdAddrStkMach *lasm = mach->castLdAddrStkMach ();
    if (lasm != nullptr) {
        *addrregister = lasm->getRd ();
        *addrstackvar = lasm->stk;
        *addrstackvaroff = lasm->raoffs;
        *addrsubsequentmach = lasm->nextmach;
        return true;
    }

    // check for long load from stack
    //  LDW rt,#offs
    //  ADD ru,SP,rt
    // no LDx/STx rd,0(ru)
    LdImmStkMach *lism = mach->castLdImmStkMach ();
    if (lism != nullptr) {
        Reg *rt = lism->getRd ();
        Arith3Mach *a3m = lism->nextmach->castArith3Mach ();
        if ((a3m != nullptr) && (a3m->getRa () == this->spreg) && (a3m->getRb () == rt)) {
            Reg *ru = a3m->getRd ();
            LoadMach  *lm = a3m->nextmach->castLoadMach  ();
            StoreMach *sm = a3m->nextmach->castStoreMach ();
            if (((lm == nullptr) || (lm->getRa () != ru)) && ((sm == nullptr) || (sm->getRa () != ru))) {
                *addrregister = a3m->getRd ();
                *addrstackvar = lism->stk;
                *addrstackvaroff = lism->raoffs;
                *addrsubsequentmach = a3m->nextmach;
                return true;
            }
        }
    }

    return false;
}

// look for:
//  STW rn,temp     rn might be a fixed register, eg, R0 return value
//  LDW rm,temp
//                  no other references to temp
// replace with:
//  LDA rm,0(rn)
bool MachFile::opt_03 (bool didsomething)
{
    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {

        Mach *loadsubsequentmach, *storesubsequentmach;
        VarDecl *loadstackvar, *storestackvar;
        MachOp loadopcode, storeopcode;
        Reg *loadregister, *storeregister;
        tsize_t loadstackvaroff, storestackvaroff;

        // check for a STW onto the stack
        if (! storeOntoStack (mach, &storeopcode, &storeregister, &storestackvar, &storestackvaroff, &storesubsequentmach)) continue;
        if (storeopcode != MO_STW) continue;
        if (storesubsequentmach == nullptr) continue;

        int line = storesubsequentmach->line;
        Token *token = storesubsequentmach->token;

        // check for LDW from same stack location
        if (! loadFromStack (storesubsequentmach, &loadopcode, &loadregister, &loadstackvar, &loadstackvaroff, &loadsubsequentmach)) continue;
        if (loadopcode != MO_LDW) continue;
        if (loadstackvar != storestackvar) continue;
        if (loadstackvaroff != storestackvaroff) continue;

        // check for no other referecnes to stack location
        for (Mach *mach2 = firstmach; mach2 != nullptr; mach2 = mach2->nextmach) {
            if (mach2 == mach) continue;
            if (mach2 == storesubsequentmach) continue;
            MachOp otheropcode;
            Reg *otherregister;
            VarDecl *otherstackvar;
            tsize_t otherstackvaroff;
            Mach *othersubsequentmach;
            if (loadFromStack (mach2, &otheropcode, &otherregister, &otherstackvar, &otherstackvaroff, &othersubsequentmach) && (otherstackvar == storestackvar) && (otherstackvaroff == storestackvaroff)) goto keepit;
            if (storeOntoStack (mach2, &otheropcode, &otherregister, &otherstackvar, &otherstackvaroff, &othersubsequentmach) && (otherstackvar == storestackvar) && (otherstackvaroff == storestackvaroff)) goto keepit;
            if (addrOfStack (mach2, &otherregister, &otherstackvar, &otherstackvaroff, &othersubsequentmach) && (otherstackvar == storestackvar)) goto keepit;
        }

        // remove the old STW/LDW

        mach = mach->prevmach;
        while (mach->nextmach != loadsubsequentmach) {
            remove (mach->nextmach);
        }

        // insert an LDA loadregister,0(storeregister)
        if (storeregister != loadregister) {
            LdAddrMach *ldam = new LdAddrMach (line, token, loadregister, 0, storeregister);
            insafter (ldam, mach);
        }

        didsomething = true;

    keepit:;
    }
    return didsomething;
}

// remove some redundant LDBS/LDBU/LDWs
bool MachFile::opt_04 (bool didsomething)
{
    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {
        Mach *loadsubsequentmach, *load2subsequentmach;
        ValDecl *loadvalue, *load2value;
        MachOp loadopcode, load2opcode;
        Reg *loadregister, *load2register;
        tsize_t loadvalueoff, load2valueoff;

        //  ... no references to rx
        //  LD?  rx,value
        //  ... no stores or addrof value, no writes to rx, no references to ry
        //  LD?  ry,value
        //  ... no references to rx
        if (! loadFromValue (mach, &loadopcode, &loadregister, &loadvalue, &loadvalueoff, &loadsubsequentmach)) continue;
        if (! loadregister->regFake ()) continue;

        // look for the second LD? within the next few instructions in the same basic block
        Mach *mach2 = loadsubsequentmach->prevmach;
        for (int i = 0; i < 10; i ++) {
            if (! mach2->fallsThrough ()) break;
            mach2 = mach2->nextmach;
            if (mach2->hasSideEffects ()) break;
            if (mach2->castLabelMach () != nullptr) break;
            if (! loadFromValue (mach2, &load2opcode, &load2register, &load2value, &load2valueoff, &load2subsequentmach)) continue;
            if (load2opcode != loadopcode) continue;
            if (! load2register->regFake ()) continue;
            if (load2value != loadvalue) continue;
            if (load2valueoff == loadvalueoff) goto foundload2;
        }

        // couldn't find second LD?, go look for another first LD? to check out
        continue;

        // found second LD? matching first LD?, check intervening instructions that might mess it all up
    foundload2:;
        Reg *rx = loadregister;
        Reg *ry = load2register;
        for (Mach *m = loadsubsequentmach; m != mach2; m = m->nextmach) {

            // intervening instructions cannot modify the load value
            if (isModifyValue (m, loadvalue, loadvalueoff)) goto keepit;

            // intervening instructions cannot modify the first register (rx)
            if (m->writesReg (rx)) goto keepit;

            // intervening instructions cannot access the second register (ry)
            if (m->readsReg (ry)) goto keepit;
            if (m->writesReg (ry)) goto keepit;
        }

        // instructions before first LD? and after second LD? cannot access rx
        for (Mach *m = firstmach; m != mach; m = m->nextmach) {
            if (m->readsReg (rx)) goto keepit;
            if (m->writesReg (rx)) goto keepit;
        }
        for (Mach *m = load2subsequentmach; m != nullptr; m = m->nextmach) {
            if (m->readsReg (rx)) goto keepit;
            if (m->writesReg (rx)) goto keepit;
        }

        // replace all references to ry with references to rx
        replacereg (ry, rx);

        // remove the second load
        mach2 = mach2->prevmach;
        while (mach2->nextmach != load2subsequentmach) {
            remove (mach2->nextmach);
        }

        didsomething = true;
    keepit:;
    }
    return didsomething;
}

bool MachFile::opt_05 (bool didsomething)
{
    return didsomething;
}

// convert
//      LDA rd,#lbl_a
//          ...
//  lbl_a:
//      BR  lbl_b
// to
//      LDA rd,#lbl_b
//          ...
//  lbl_a:
//      BR  lbl_b
bool MachFile::opt_06 (bool didsomething)
{
    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {
        LdImmLblMach *lilm = mach->castLdImmLblMach ();
        if (lilm != nullptr) {
            LabelMach *lbl_a = lilm->lbl;
            BrMach *brm = lbl_a->nextmach->castBrMach ();
            if (brm != nullptr) {
                lilm->lbl = brm->lbl;
                lilm->madeshort = false;
                didsomething = true;
            }
        }
    }

    // strip out unreferenced labels
    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {
        LabelMach *lm = mach->castLabelMach ();
        if (lm != nullptr) lm->labelmachrefd = false;
    }
    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {
        mach->markLabelsRefd ();
    }
    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {
        LabelMach *lm = mach->castLabelMach ();
        if ((lm != nullptr) && ! lm->labelmachrefd) {
            mach = mach->prevmach;
            remove (lm);
            didsomething = true;
        }
    }

    // strip out dead code
    bool lastfallsthrough = true;
    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {
        LabelMach *lm = mach->castLabelMach ();
        if (lm != nullptr) {
            lastfallsthrough = true;
            continue;
        }
        if (lastfallsthrough) {
            lastfallsthrough = mach->fallsThrough ();
            continue;
        }
        mach = mach->prevmach;
        remove (mach->nextmach);
        didsomething = true;
    }

    return didsomething;
}

// use direct stack addressing where offset is small enough
bool MachFile::opt_20 (bool didsomething)
{
    // check for long-style stack oriented loads/stores with small offsets
    // long-styles are created by bigLoadStkMach() and similar
    //  LDW rt,#stkoffset       LdImmStkMach
    //  ADD ru,R6,rt            Arith3Mach
    //  LD? rd,0(ru)            LoadMach, StoreMach
    // no other references to rt,ru
    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {

        // check for 'LDW rt,#stkoffset' with -64<=stkoffset<=63
        LdImmStkMach *lism = mach->castLdImmStkMach ();
        if (lism != nullptr) {
            int16_t lismoffs = lism->getStkOffs ();
            if ((lismoffs >= LSOFFMIN) && (lismoffs <= LSOFFMAX)) {
                Reg *rt = lism->getRd ();

                // check for 'ADD ru,R6,rt'
                Arith3Mach *a3m = lism->nextmach->castArith3Mach ();
                if ((a3m != nullptr) && (a3m->mop == MO_ADD) && (a3m->getRa () == this->spreg) && (a3m->getRb () == rt)) {

                    // check for no other references to rt
                    //TODO check for no _subsequent_ references to rt
                    for (Mach *m = firstmach; m != nullptr; m = m->nextmach) {
                        if ((m != lism) && (m != a3m)) {
                            if (m->readsReg  (rt)) goto keepit;
                            if (m->writesReg (rt)) goto keepit;
                        }
                    }

                    Reg *ru = a3m->getRd ();

                    // check for 'LD? rd,0(ru)'
                    LoadMach  *lm = a3m->nextmach->castLoadMach  ();
                    StoreMach *sm = a3m->nextmach->castStoreMach ();
                    int16_t lsoffs;
                    if ((lm != nullptr) && (lm->getRa () == ru) && lm->getOffs (&lsoffs) && (lsoffs == 0)) {

                        // check for no other references to ru
                        //TODO check for no _subsequent_ references to ru
                        for (Mach *m = firstmach; m != nullptr; m = m->nextmach) {
                            if ((m != lism) && (m != a3m)) {
                                if (m->readsReg  (ru)) goto repwlda;
                                if (m->writesReg (ru)) goto repwlda;
                            }
                        }

                        // replace with 'LD? rd,stkoffset(SP)'
                        LoadStkMach *lsm = new LoadStkMach (lism->line, lism->token, lm->mop, lm->getRd (), lism->stk, lism->spoffs, lism->raoffs);
                        this->replace (lism, lsm);
                        this->remove (a3m);
                        this->remove (lm);
                        mach = lsm;
                        didsomething = true;
                        continue;
                    }

                    // check for 'ST? rd,0(ru)'
                    if ((sm != nullptr) && (sm->getRa () == ru) && sm->getOffs (&lsoffs) && (lsoffs == 0)) {

                        // replace with 'ST? rd,stkoffset(SP)'
                        StoreStkMach *ssm = new StoreStkMach (lism->line, lism->token, sm->mop, sm->getRd (), lism->stk, lism->spoffs, lism->raoffs);
                        this->replace (lism, ssm);
                        this->remove (a3m);
                        this->remove (sm);
                        mach = ssm;
                        didsomething = true;
                        continue;
                    }

                    // just LDW,ADD
                    // replace with 'LDA ru,stkoffset(SP)'
                repwlda:;
                    LdAddrStkMach *lasm = new LdAddrStkMach (lism->line, lism->token, ru, lism->stk, lism->spoffs, lism->raoffs);
                    this->replace (lism, lasm);
                    this->remove (a3m);
                    mach = lasm;
                    didsomething = true;
                }
            }
        }
    keepit:;
    }

    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {

        // look for
        //  LDx/STx Rd,x(Ra)
        // where Ra is based on stack pointer + offset
        // change to
        //  LDx/STx Rd,x+offset(SP)
        // ...where x+offset is in range
        int16_t oldoffs;
        Reg *ra = mach->getRa ();
        if ((ra != nullptr) && (ra->regplace != RP_R6) && (ra->regcont == RC_CONSTSP) && mach->getOffs (&oldoffs)) {
            int16_t newoffs = oldoffs + ra->regvalue;
            if ((newoffs >= LSOFFMIN) && (newoffs <= LSOFFMAX)) {
                mach->setOffsRa (newoffs, this->spreg);
                didsomething = true;
            }
        }

        // update register state to end of instruction
        mach->stepRegState ();
    }

    return didsomething;
}

// remove any instructions that write registers that are never read
//TODO delete if written again before read
bool MachFile::opt_21 (bool didsomething)
{
    for (Reg *reg : allregs) reg->isread = false;

    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {
        for (Reg *reg : mach->regreads) reg->isread = true;
    }

    for (Mach *mach = firstmach; mach != nullptr;) {
        Mach *nextmach = mach->nextmach;

        // don't delete calls or stores
        if (mach->hasSideEffects ()) goto keepit;
        if (mach->regwrites.size () == 0) goto keepit;

        // if anything it writes is read by something else, keep the instruction
        for (Reg *reg : mach->regwrites) {
            if (reg->isread) goto keepit;
        }

        remove (mach);
        didsomething = true;

    keepit:;
        mach = nextmach;
    }

    return didsomething;
}

// optimize out some LDAs
bool MachFile::opt_22 (bool didsomething)
{
    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {

        // update register state to end of instruction
        mach->stepRegState ();

        // look for
        //  ADD/SUB Rd,Ra,Rb=const
        // where const is in range
        // change to
        //  LDA Rd,const(Ra)
        Arith3Mach *am = mach->castArith3Mach ();
        if ((am != nullptr) && ((am->mop == MO_ADD) || (am->mop == MO_SUB))) {
            Reg *rb = am->getRb ();
            if (rb->regcont == RC_CONST) {
                int16_t val = rb->regvalue;
                if (am->mop == MO_SUB) val = - val;
                if ((val >= LSOFFMIN) && (val <= LSOFFMAX)) {
                    mach = replace (mach, new LdAddrMach (am->line, am->token, am->getRd (), val, am->getRa ()));
                    didsomething = true;
                }
            }
        }
    }

    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {

        // look for
        //  LDA rt,x(ra)
        //  ... no writes to ra, rt
        //  LD? rd,y(rt)  or  ST? rd,y(rt)  or  CALL y(rt)
        // change to
        //  LDA rt,x(ra)
        //  ... no writes to ra, rt
        //  LD? rd,x+y(ra)  or  ST? rd,x+y(ra)  or  CALL x+y(ra)
        // the LDA will be eliminated if possible by other optimization
        LdAddrMach *lam1 = mach->castLdAddrMach ();
        if (lam1 != nullptr) {
            Reg *rt = lam1->getRd ();
            Reg *ra = lam1->getRa ();
            didsomething = opt_22_b (didsomething, mach, rt, lam1->offs, ra);
        }
        LdAddrStkMach *lasm1 = mach->castLdAddrStkMach ();
        if (lasm1 != nullptr) {
            Reg *rt = lasm1->getRd ();
            didsomething = opt_22_b (didsomething, mach, rt, lasm1->getStkOffs (), this->spreg);
        }
    }

    return didsomething;
}

bool MachFile::opt_22_b (bool didsomething, Mach *mach, Reg *rt, int16_t ldaoffs, Reg *ra)
{
    if (rt->regeq (ra)) return didsomething;

    // allow for a handful of intervening instructions
    Mach *mach2 = mach;
    for (int i = 0; i < 5; i ++) {
        if (! mach2->fallsThrough ()) break;
        mach2 = mach2->nextmach;
        if (mach2 == nullptr) break;
        if (mach2->castLabelMach () != nullptr) break;

        LdAddrMach  *lam2 = mach2->castLdAddrMach  ();
        LoadMach    *lm2  = mach2->castLoadMach    ();
        StoreMach   *sm2  = mach2->castStoreMach   ();
        CallPtrMach *cpm2 = mach2->castCallPtrMach ();

        for (Mach *m = mach; (m = m->nextmach) != mach2;) {
            if (m->writesReg (ra)) return didsomething;
            if (m->writesReg (rt)) return didsomething;
        }

        if ((lam2 != nullptr) && (lam2->getRa () == rt)) {
            int16_t offs = ldaoffs + lam2->offs;
            if ((offs >= LSOFFMIN) && (offs <= LSOFFMAX)) {
                LdAddrMach *lamt = new LdAddrMach (lam2->line, lam2->token, lam2->getRd (), offs, ra);
                mach = this->replace (lam2, lamt);
                didsomething = true;
            }
            break;
        }

        if ((lm2 != nullptr) && (lm2->getRa () == rt)) {
            int16_t offs = ldaoffs + lm2->offs;
            if ((offs >= LSOFFMIN) && (offs <= LSOFFMAX)) {
                LoadMach *lmt = new LoadMach (lm2->line, lm2->token, lm2->mop, lm2->getRd (), offs, ra);
                mach = this->replace (lm2, lmt);
                didsomething = true;
            }
            break;
        }

        if ((sm2 != nullptr) && (sm2->getRa () == rt)) {
            int16_t offs = ldaoffs + sm2->offs;
            if ((offs >= LSOFFMIN) && (offs <= LSOFFMAX)) {
                StoreMach *smt = new StoreMach (sm2->line, sm2->token, sm2->mop, sm2->getRd (), offs, ra);
                mach = this->replace (sm2, smt);
                didsomething = true;
            }
            break;
        }

        if ((cpm2 != nullptr) && (cpm2->getRa () == rt)) {
            int16_t offs = ldaoffs + cpm2->offs;
            if ((offs >= LSOFFMIN) && (offs <= LSOFFMAX)) {

                // set up new call instruction with combined offset
                CallPtrMach *cpmt = new CallPtrMach (cpm2->line, cpm2->token, offs, ra);

                // copy registers it reads except the first is the base register
                int i = 0;
                for (Reg *rr : cpm2->regreads) {
                    if (i == 0) {
                        assert (rr == rt);
                        assert (cpmt->regreads.at (0) == ra);
                    } else {
                        cpmt->regreads.push_back (rr);
                    }
                    i ++;
                }
                assert ((i > 0) && ((int) cpmt->regreads.size () == i));

                // calls never write registers, the return label writes the registers
                assert (cpm2->regwrites.size () == 0);
                assert (cpmt->regwrites.size () == 0);

                // replace old call instruction with new one
                mach = this->replace (cpm2, cpmt);
                didsomething = true;
            }
            break;
        }
    }
    return didsomething;
}

// optimize post-decrement/increment
//  LD? rx,off(ra)      LD? rx,off(ra)
//  LD? ry,off(ra)
//  LDA ry,inc(ry)      LDA ry,inc(rx)
// also eliminate any LDA rn,0(rn)
void MachFile::opt_40 ()
{
    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {

        // check for LD? rx,off(ra)
        LoadMach *lm1 = mach->castLoadMach ();
        if ((lm1 != nullptr) && (lm1->nextmach != nullptr)) {

            // check for LD? ry,off(ra)
            LoadMach *lm2 = lm1->nextmach->castLoadMach ();
            if ((lm2 != nullptr) && (lm2->mop == lm1->mop) && (lm2->offs == lm1->offs) && (lm2->getRa ()->regplace == lm1->getRa ()->regplace)) {

                // check for LDA ry,inc(ry)
                LdAddrMach *lam = mach->nextmach->nextmach->castLdAddrMach ();
                if ((lam != nullptr) && (lam->getRa ()->regplace == lm2->getRd ()->regplace) && (lam->getRd ()->regplace == lam->getRa ()->regplace)) {

                    // remove second LD? and fix LDA
                    remove (lm2);
                    lam->setOffsRa (lam->offs, lm1->getRd ());
                }
            }
        }

        // check for LD? rx,offs(SP)
        LoadStkMach *lsm1 = mach->castLoadStkMach ();
        if (lsm1 != nullptr) {

            // check for LD? ry,offs(SP)
            LoadStkMach *lsm2 = mach->nextmach->castLoadStkMach ();
            if ((lsm2 != nullptr) && (lsm2->mop == lsm1->mop) && (lsm2->stk == lsm1->stk) && (lsm2->raoffs == lsm1->raoffs)) {

                // stack pointer offset can't have changed between the two LD?s
                assert (lsm2->spoffs == lsm1->spoffs);

                // check for LDA ry,inc(ry)
                LdAddrMach *lam = mach->nextmach->nextmach->castLdAddrMach ();
                if ((lam != nullptr) && (lam->getRa ()->regplace == lsm2->getRd ()->regplace) && (lam->getRd ()->regplace == lam->getRa ()->regplace)) {

                    // remove second LD? and fix LDA
                    remove (lsm2);
                    lam->setOffsRa (lam->offs, lsm1->getRd ());
                }
            }
        }

        // eliminate any LDA rn,0(rn)
        LdAddrMach *lam1 = mach->castLdAddrMach ();
        if ((lam1 != nullptr) && (lam1->offs == 0) && (lam1->getRd ()->regplace == lam1->getRa ()->regplace)) {
            mach = mach->prevmach;
            remove (lam1);
        }
    }
}

// look for
//  LD? rx,#imm1
//    ... no writes to rx
//  LD? ry,#imm2
// replace with
//  LD? rx,#imm1
//    ... no writes to rx
//  LDA ry,imm2-imm1(rx)
void MachFile::opt_41 ()
{
    Reg *valids[6];
    int16_t values[6];
    for (int i = 0; i < 6; i ++) {
        valids[i] = nullptr;
        values[i] = 0;
    }

    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {
        if (mach->castLabelMach () != nullptr) {

            // labels invalidate all register contents
            for (int i = 0; i < 6; i ++) valids[i] = nullptr;
        } else {

            // check for a Load Immediate Value instruction
            LdImmValMach *livm = mach->castLdImmValMach ();
            if (livm != nullptr) {

                // see if some register has a nearby constant
                Reg *ry = livm->getRd ();
                for (int i = 0; i < 5; i ++) {
                    Reg *rx = valids[i];
                    if (rx != nullptr) {
                        int16_t offs = livm->val - values[i];
                        if ((offs >= LSOFFMIN) && (offs <= LSOFFMAX)) {

                            // ok, replace the LDIMM with an LDA ry,offs(rx)
                            LdAddrMach *lam = new LdAddrMach (livm->line, livm->token, ry, offs, rx);
                            replace (livm, lam);
                            mach = lam;
                            break;
                        }
                    }
                }

                // replaced or not, the ry contains the given value
                int i = ry->regplace - RP_R0;
                assert ((i >= 0) && (i <= 5));
                valids[i] = ry;
                values[i] = livm->val;
            } else {

                // some other instruction, invalidate whatever the instruction writes
                for (Reg *r : mach->regwrites) {
                    int i = r->regplace - RP_R0;
                    if ((i >= 0) && (i <= 5)) {
                        valids[i] = nullptr;
                    }
                }

                int i = 0;
                for (uint32_t trashes = mach->trashesHwRegs () >> RP_R0; (trashes != 0) && (i <= 5); trashes >>= 1) {
                    if (trashes & 1) {
                        valids[i] = nullptr;
                    }
                    i ++;
                }
            }
        }
    }
}

// look for
//  LDA SP,x(SP)
//  LDA SP,y(SP)
// replace with
//  LDA SP,x+y(SP)
// happens for nested calls
void MachFile::opt_42 ()
{
    bool didsomething;
    do {
        didsomething = false;
        for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {
            LdAddrMach *lda1 = mach->castLdAddrMach ();
            if ((lda1 != nullptr) && (lda1->getRd () == this->spreg) && (lda1->getRa () == this->spreg)) {
                LdAddrMach *lda2 = lda1->nextmach->castLdAddrMach ();
                if ((lda2 != nullptr) && (lda2->getRd () == this->spreg) && (lda2->getRa () == this->spreg)) {
                    int16_t offset = lda1->offs + lda2->offs;
                    if ((offset >= LSOFFMIN) && (offset <= LSOFFMAX)) {
                        lda1->offs = offset;
                        remove (lda2);
                        didsomething = true;
                    }
                }
            }
        }
    } while (didsomething);
}

// shorten branches
bool MachFile::opt_50 (bool didsomething)
{
    // get where all the instructions are before shortening anything
    tsize_t relpc = 0;
    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {
        mach->relpc = relpc;
        relpc += mach->getisize ();
    }

    // try to shorten some branches
    Mach *next;
    for (Mach *mach = firstmach; mach != nullptr; mach = next) {
        next = mach->nextmach;
        BrMach *brmach = mach->castBrMach ();
        if (brmach != nullptr) {
            int offset = brmach->lbl->relpc - brmach->relpc - brmach->getisize ();
            if (offset == 0) {
                remove (brmach);
                didsomething = true;
            } else {
                bool sh = (offset >= BROFFMIN) && (offset <= BROFFMAX);
                if (brmach->shortbr != sh) {
                    brmach->shortbr = sh;
                    didsomething = true;
                }
            }
        }
    }

    return didsomething;
}

#define NBEF (-LSOFFMIN/2)

void MachFile::opt_51 ()
{
    // we can insert constants just before the function entrypoint
    Mach *insertbefore = firstmach;
    char const *inbeforeblockstrngs[NBEF];
    uint16_t inbeforeblockvalues[NBEF];
    LabelMach *inbeforeblocklabels[NBEF];
    int ninbeforeblock = 0;

    // scan through the instructions looking for LDx Rd,#immval
    tsize_t relpc = 0;
    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {
        mach->relpc = relpc;

        // if the instruction doesn't fall through, insert any after constants and restart the before constants
        //TODO allow inserts in CallMach's
        if (! mach->fallsThrough ()) {
            ninbeforeblock = 0;
            insertbefore = mach->nextmach;
            goto nextmach;
        }

        // see if it is some form of 'LDx Rd,#...'
        // but don't bother with #0 cuz they get optimized to 'CLR Rd'
        MachOp immmop;
        char const *immstr;
        uint16_t immval;
        if (mach->isLdImmAny (&immmop, &immstr, &immval) && ((immstr != nullptr) || (immval != 0))) {

            // see if we already have that constant in the before block
            for (int i = 0; i < ninbeforeblock; i ++) {
                if (((immstr != nullptr) && (inbeforeblockstrngs[i] != nullptr) && (strcmp (immstr, inbeforeblockstrngs[i]) == 0)) ||
                        ((immstr == nullptr) && (inbeforeblockvalues[i] == immval))) {
                    uint16_t foundaddr = insertbefore->relpc - (ninbeforeblock - i) * 2;
                    int16_t pcoffset = foundaddr - relpc - 2;
                    if (pcoffset < LSOFFMIN) goto notbefore;
                    mach = this->replace (mach, new LdPCRelMach (mach->line, mach->token, immmop, mach->getRd (), inbeforeblocklabels[i]));
                    mach->relpc = relpc;
                    goto nextmach;
                }
            }

            // if not, try to insert into before block
            if (ninbeforeblock < NBEF) {
                LabelMach *conlabel = new LabelMach (mach->line, intdeftok);
                WordMach  *convalue = new WordMach  (mach->line, intdeftok, immstr, immval);
                LdPCRelMach *lpcrel = new LdPCRelMach (mach->line, mach->token, immmop, mach->getRd (), conlabel);
                inbeforeblockstrngs[ninbeforeblock] = immstr;
                inbeforeblockvalues[ninbeforeblock] = immval;
                inbeforeblocklabels[ninbeforeblock] = conlabel;
                this->insbefore (conlabel, insertbefore);
                this->insbefore (convalue, insertbefore);
                this->replace (mach, lpcrel);
                for (Mach *m = firstmach; m != nullptr; m = m->nextmach) {
                    if (m->brokenPCRel ()) {
                        this->remove (conlabel);
                        this->remove (convalue);
                        this->replace (lpcrel, mach);
                        //delete conlabel;
                        //delete convalue;
                        //delete lpcrel;
                        goto notbefore;
                    }
                }
                ninbeforeblock ++;
                mach = lpcrel;
                mach->relpc = relpc;
                goto nextmach;
            }
        notbefore:;

            //TODO maybe it will fit in an after block
        }
    nextmach:;
        relpc = mach->relpc + mach->getisize ();
    }
}

// assign stack locations to stack variables
static bool compareStackVarLiveRanges (VarDecl *a, VarDecl *b);
void MachFile::opt_asnstack ()
{
    // get liveness range of all the stack variables
    // also count the number of references
    //TODO use actual branches to chase them all down instead of simple sequence, maybe it will be better optimized
    std::vector<Mach *> allmachs;
    std::vector<VarDecl *> allvars;
    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {
        mach->instnum = allmachs.size ();
        allmachs.push_back (mach);

        // ldaddr marks them as 'addrtaken' - so they have dedicated addresses on the stack
        //TODO - give them live range = its scope
        LdAddrStkMach *lasm = mach->castLdAddrStkMach ();
        if (lasm != nullptr) {
            StackLocn *stklocn = lasm->stk->getVarLocn ()->castStackLocn ();
            if (! stklocn->addrtaken) {
                if ((stklocn->lastread == nullptr) && (stklocn->firstwrite == nullptr)) {
                    lasm->stk->numvarefs = 0;
                    allvars.push_back (lasm->stk);
                }
                stklocn->addrtaken = true;
            }
            lasm->stk->numvarefs ++;
        }

        // ldimm offset - also mark as 'addrtaken'
        //TODO - used as part of sequence to lda/load/store sequence
        //       ...so can be used for better analysis
        LdImmStkMach *lism = mach->castLdImmStkMach ();
        if (lism != nullptr) {
            StackLocn *stklocn = lism->stk->getVarLocn ()->castStackLocn ();
            if ((stklocn != nullptr) && ! stklocn->addrtaken) {
                if ((stklocn->lastread == nullptr) && (stklocn->firstwrite == nullptr)) {
                    lism->stk->numvarefs = 0;
                    allvars.push_back (lism->stk);
                }
                stklocn->addrtaken = true;
            }
            lism->stk->numvarefs ++;
        }

        // load marks them as last read - ending the range the variable is active
        LoadStkMach *lsm = mach->castLoadStkMach ();
        if (lsm != nullptr) {
            StackLocn *stklocn = lsm->stk->getVarLocn ()->castStackLocn ();
            if ((stklocn != nullptr) && ! stklocn->addrtaken) {
                if ((stklocn->lastread == nullptr) && (stklocn->firstwrite == nullptr)) {
                    lsm->stk->numvarefs = 0;
                    allvars.push_back (lsm->stk);
                }
                if (lsm->stk->isArray ()) {
                    // reading an array variable is really getting the array address
                    stklocn->addrtaken = true;
                } else {
                    // reading non-array, mark last time read so far
                    stklocn->lastread = mach;
                }
            }
            lsm->stk->numvarefs ++;
        }

        // store marks them as first written - starting the range the variable is active
        StoreStkMach *ssm = mach->castStoreStkMach ();
        if (ssm != nullptr) {
            StackLocn *stklocn = ssm->stk->getVarLocn ()->castStackLocn ();
            if ((stklocn != nullptr) && ! stklocn->addrtaken && (stklocn->firstwrite == nullptr)) {
                if (stklocn->lastread == nullptr) {
                    ssm->stk->numvarefs = 0;
                    allvars.push_back (ssm->stk);
                }
                assert (! ssm->stk->isArray ());
                stklocn->firstwrite = mach;
            }
            ssm->stk->numvarefs ++;
        }
    }

    // don't assign locations for variables that have strange ranges
    // - some are written but not read
    // - some are read but not written
    // - some are read before written
    for (int i = allvars.size (); -- i >= 0;) {
        VarDecl *stk = allvars[i];
        StackLocn *stklocn = stk->getVarLocn ()->castStackLocn ();
        if (stklocn->addrtaken) continue;
        Mach *fw = stklocn->firstwrite;
        Mach *lr = stklocn->lastread;
        if (fw == nullptr) {
            printerror (stk->getDefTok (), "variable '%s' read but not written", stk->getName ());
            printerror (lr->token, "... read at this point");
            allvars.erase (allvars.begin () + i);
            continue;
        }
        if (lr == nullptr) {
            printerror (stk->getDefTok (), "variable '%s' written but not read", stk->getName ());
            printerror (fw->token, "... written at this point");
            allvars.erase (allvars.begin () + i);
            continue;
        }
        if (lr->instnum <= fw->instnum) {
            printerror (stk->getDefTok (), "variable '%s' read before written", stk->getName ());
            printerror (lr->token, "... read at this point");
            printerror (fw->token, "... written at this point");
            allvars.erase (allvars.begin () + i);
        }
    }

    // sort stack variables by variable size, number of references, liveness length, starting instruction number
    std::sort (allvars.begin (), allvars.end (), compareStackVarLiveRanges);

    // assign offsets for stack variables other than addrtaken
    // - overlap them on stack as long as their live ranges don't overlap
    uint32_t nunassnd = 0;
    tsize_t offset = 0;
    do {
        nunassnd = 0;
        tsize_t sizeused = 0;
        for (VarDecl *stk : allvars) {

            // check for variable that hasn't been assigned yet
            StackLocn *stklocn = stk->getVarLocn ()->castStackLocn ();
            if (! stklocn->addrtaken && ! stklocn->assigned) {

                // range of instructions the stack variable is live
                uint32_t stkbeg = stklocn->firstwrite->instnum;
                uint32_t stkend = stklocn->lastread->instnum;

                // get alinged offset for the variable and size on stack this var will take up
                tsize_t alin = stk->getType ()->getTypeAlign (nullptr);
                tsize_t alignedoffset = (offset + alin - 1) & - alin;
                tsize_t size = alignedoffset + stk->getValSize (nullptr) - offset;

                // see if its live range overlaps on something else already assigned
                for (VarDecl *overlapstk : allvars) {

                    // only look at vars that poke into this range of stack locations
                    StackLocn *overlapstklocn = overlapstk->getVarLocn ()->castStackLocn ();
                    if (! overlapstklocn->addrtaken && overlapstklocn->assigned && (overlapstklocn->offset + overlapstk->getValSize (nullptr) > alignedoffset)) {
                        uint32_t overlapbeg = overlapstklocn->firstwrite->instnum;
                        uint32_t overlapend = overlapstklocn->lastread->instnum;
                        if ((overlapbeg < stkend) && (stkbeg < overlapend)) {
                            nunassnd ++;
                            goto nextvar;
                        }
                    }
                }

                // assign this variable's stack location
                stk->getVarLocn ()->castStackLocn ()->offset = alignedoffset;
                stk->getVarLocn ()->castStackLocn ()->assigned = true;
                if (sizeused < size) sizeused = size;
            nextvar:;
            }
        }
        offset += sizeused;
    } while (nunassnd > 0);

    // assign offsets for addrtaken stack variables
    for (VarDecl *stk : allvars) {
        if (stk->getVarLocn ()->castStackLocn ()->addrtaken) {
            assert (! stk->getVarLocn ()->castStackLocn ()->assigned);
            tsize_t alin = stk->getType ()->getTypeAlign (nullptr);
            tsize_t size = stk->getValSize (nullptr);
            offset  = (offset + alin - 1) & - alin;
            stk->getVarLocn ()->castStackLocn ()->offset = offset;
            stk->getVarLocn ()->castStackLocn ()->assigned = true;
            offset += size;
        }
    }

    // declare stack size required for stack variables
    funcdecl->stacksize = (offset + MAXALIGN - 1) & - MAXALIGN;

    // maybe dump some of the assignments to assembler file as symbols
    for (VarDecl *stk : allvars) {
        if (stk->outspoffssym) {
            assert (stk->getVarLocn ()->castStackLocn ()->assigned);
            fprintf (sfile, "\t%s.spoffs = %d\n", stk->getName (), stk->getVarLocn ()->castStackLocn ()->offset - SPOFFSET);
        }
    }
}

// sort stack variables for assignment to stack locations
// higher priority gets lower stack address hopefully sp offset -64..+63
// - shortest span has highest priority
// - higher number of references is higher priority
// - increasing position in code next to that
// - addrtaken vars have lowest priority
static bool compareStackVarLiveRanges (VarDecl *a, VarDecl *b)
{
    tsize_t asize = a->getValSize (nullptr);
    tsize_t bsize = b->getValSize (nullptr);
    if (asize < bsize) return true;
    if (asize > bsize) return false;

    if (a->numvarefs > b->numvarefs) return true;
    if (a->numvarefs < b->numvarefs) return false;

    StackLocn *alocn = a->getVarLocn ()->castStackLocn ();
    StackLocn *blocn = b->getVarLocn ()->castStackLocn ();

    if (alocn->addrtaken && blocn->addrtaken) return alocn < blocn;
    if (alocn->addrtaken) return false;
    if (blocn->addrtaken) return true;

    uint32_t aspan = alocn->lastread->instnum - alocn->firstwrite->instnum;
    uint32_t bspan = blocn->lastread->instnum - blocn->firstwrite->instnum;
    if (aspan < bspan) return true;
    if (aspan > bspan) return false;

    return alocn->firstwrite->instnum < blocn->firstwrite->instnum;
}

// assign hardware registers to fake registers
static bool compareRegisterLiveRanges (Reg *a, Reg *b);
void MachFile::opt_asnhregs ()
{
    // get liveness range of all the registers
    //TODO use actual branches to chase them all down instead of simple sequence, maybe it will be better optimized
    std::vector<Mach *> allmachs;
    std::vector<Reg *> allregs;
    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {
        mach->instnum = allmachs.size ();
        allmachs.push_back (mach);

        for (Reg *reg : mach->regwrites) {
            if (reg->firstwrite == nullptr) {
                reg->firstwrite = mach;
                allregs.push_back (reg);
            }
        }
        for (Reg *reg : mach->regreads) {
            reg->lastread = mach;
        }
    }

    // some registers get written without being read
    // ...such as the R0 coming out of a call instruction
    //    when the return value is not used
    for (int i = allregs.size (); -- i >= 0;) {
        Reg *reg = allregs[i];
        if (reg->lastread == nullptr) {
            allregs.erase (allregs.begin () + i);
        }
    }

    // sort registers by liveness length, starting instruction number
    std::sort (allregs.begin (), allregs.end (), compareRegisterLiveRanges);

    // assign registers
    uint32_t nunassnd = 0;
    for (uint32_t regnum = RP_R0; regnum <= RP_R5; regnum ++) {

        // make mask of registers active going into instruction and coming out of instruction
        // ...for those registers that are already assigned
        for (Mach *mach : allmachs) {
            mach->acthregsout = 0;
        }
        for (Reg *reg : allregs) {
            if (reg->regplace <= RP_R7) {
                assert (reg->firstwrite != nullptr);
                assert (reg->lastread   != nullptr);
                assert (reg->lastread->instnum > reg->firstwrite->instnum);
                assert (reg->lastread->instnum < allmachs.size ());
                for (uint32_t i = reg->firstwrite->instnum; i < reg->lastread->instnum; i ++) {
                    allmachs[i]->acthregsout |= 1U << reg->regplace;
                }
            }
        }

        // assign register 'regnum' to unassigned starting with smallest range
        // ...that don't overlap with existing assignments
        nunassnd = 0;
        for (Reg *reg : allregs) {
            if (reg->regplace == RP_R05) {
                for (uint32_t i = reg->firstwrite->instnum; i < reg->lastread->instnum; i ++) {
                    uint32_t written = allmachs[i]->acthregsout | allmachs[i]->trashesHwRegs ();
                    if (written & (1U << regnum)) {
                        nunassnd ++;
                        goto nextreg;
                    }
                }
                reg->regplace = (RegPlace) regnum;
                for (uint32_t i = reg->firstwrite->instnum; i < reg->lastread->instnum; i ++) {
                    allmachs[i]->acthregsout |= 1U << regnum;
                }
            nextreg:;
            }
        }
        if (nunassnd == 0) break;
    }

    if (nunassnd > 0) {
        fprintf (stderr, "opt_asnhregs*: ended with %u unassigned\n", nunassnd);
    }
}

static bool compareRegisterLiveRanges (Reg *a, Reg *b)
{
    uint32_t aspan = a->lastread->instnum - a->firstwrite->instnum;
    uint32_t bspan = b->lastread->instnum - b->firstwrite->instnum;

    if (aspan < bspan) return true;
    if (aspan > bspan) return false;

    return a->firstwrite->instnum < b->firstwrite->instnum;
}
