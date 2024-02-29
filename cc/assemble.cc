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
////////////////////////////////////////////////
//                                            //
//  output assembly language code from prims  //
//                                            //
////////////////////////////////////////////////

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"
#include "locn.h"
#include "prim.h"

static Mach *bigLdAddrMach (MachFile *mf, int line, Token *token, Reg *rd, int offs, Reg *ra);
static Mach *bigLdAddrStkMach (MachFile *mf, int line, Token *token, Reg *rd, VarDecl *stk);
static Mach *bigLoadStkMach (MachFile *mf, int line, Token *token, MachOp mop, Reg *rd, VarDecl *stk, tsize_t raoffs);
static Mach *bigStoreMach (MachFile *mf, int line, Token *token, MachOp mop, Reg *rd, int offs, Reg *ra);
static Mach *bigStoreStkMach (MachFile *mf, int line, Token *token, MachOp mop, Reg *rd, VarDecl *stk, tsize_t raoffs);

// output assembly language to perform simple direct assignment
void AsnopPrim::assemprim (MachFile *mf)
{
    // aval = left-hand operand
    // bval = right-hand operand

    Type *btype = bval->getType ()->stripCVMod ();
    tsize_t alin = btype->getTypeAlign (primtoken);
    tsize_t size = btype->getTypeSize  (primtoken);

    // upper layer should make sure type casting has been done
    // but all we care is that alignment and size match
    Type *avtype = aval->getType ()->stripCVMod ();
    assert (avtype->getTypeAlign (primtoken) == alin);
    assert (avtype->getTypeSize  (primtoken) == size);

    switch (size) {
        case 0: break;
        case 1: {
            Reg *rbval = new Reg (mf, RP_R05);
            bval->loadIntoRn (mf, primtoken, rbval, 0);
            aval->storeFromRn (mf, primtoken, rbval, 0);
            break;
        }
        case 2: {
            Reg *rbval = new Reg (mf, RP_R05);
            bval->loadIntoRn (mf, primtoken, rbval, 0);
            aval->storeFromRn (mf, primtoken, rbval, 0);
            break;
        }
        case 4: {
            Reg *rbval = new Reg (mf, RP_R05);
            bval->loadIntoRn (mf, primtoken, rbval, 0);
            aval->storeFromRn (mf, primtoken, rbval, 0);
            Reg *rcval = new Reg (mf, RP_R05);
            bval->loadIntoRn (mf, primtoken, rcval, 2);
            aval->storeFromRn (mf, primtoken, rcval, 2);
            break;
        }
        default: {
            Reg *raptr = new Reg (mf, RP_R0);
            aval->ldaIntoRn (mf, primtoken, raptr, 0);

            Reg *rbptr = new Reg (mf, RP_R1);
            bval->ldaIntoRn (mf, primtoken, rbptr, 0);

            gencallmemcpy (mf, raptr, rbptr, alin, size);
            break;
        }
    }

    assert ((flonexts.size () == 1) && (flonexts[0] == linext));
}

// in mulDivMod(), we extend all byte operands to words,
// so we don't need any byte-sized functions
static char btow (char sizechar)
{
    if (sizechar == 'b') return 'w';
    if (sizechar == 'B') return 'W';
    return sizechar;
}

// for mulDivMod(), functions with unsigned operands are usually more efficient than signed operands,
// so use unsigned operands for signed when constant .ge. 0.
static char getsizechar (ValDecl *val)
{
    char sc = btow (val->getType ()->stripCVMod ()->castNumType ()->sizechar);
    NumValue nv;
    NumCat nc = val->isNumConstVal (&nv);
    if ((nc == NC_SINT) && (nv.s >= 0)) {
        assert (('A' == 0x41) && ('a' == 0x61));
        sc &= 0xDF;
    }
    return sc;
}

// output assembly language to perform two-operand arithmetic function
void BinopPrim::assemprim (MachFile *mf)
{
    // see if either operand is unsigned
    IntegType *ainttype = aval->getType ()->stripCVMod ()->castIntegType ();
    IntegType *binttype = bval->getType ()->stripCVMod ()->castIntegType ();
    bool aunsigned = (ainttype != nullptr) && ! ainttype->getSign ();
    bool bunsigned = (binttype != nullptr) && ! binttype->getSign ();

    // if so, compares are unsigned
    MachOp _lt_ = (aunsigned | bunsigned) ? MO_BLO  : MO_BLT;
    MachOp _le_ = (aunsigned | bunsigned) ? MO_BLOS : MO_BLE;
    MachOp _gt_ = (aunsigned | bunsigned) ? MO_BHI  : MO_BGT;
    MachOp _ge_ = (aunsigned | bunsigned) ? MO_BHIS : MO_BGE;

    switch (opcode) {
        case OP_ADD:    { addSubAndXorOr (mf, MO_ADD, "__add"); break; }
        case OP_SUB:    { addSubAndXorOr (mf, MO_SUB, "__sub"); break; }
        case OP_BITAND: { addSubAndXorOr (mf, MO_AND, "__and"); break; }
        case OP_BITXOR: { addSubAndXorOr (mf, MO_XOR, "__xor"); break; }
        case OP_BITOR:  { addSubAndXorOr (mf, MO_OR,  "__or");  break; }
        case OP_MUL:    { mulDivMod (mf, "__mul"); break; }
        case OP_DIV:    { mulDivMod (mf, "__div"); break; }
        case OP_MOD:    { mulDivMod (mf, "__mod"); break; }
        case OP_SHL:    { shifts (mf, MO_SHL, "__shl"); break; }
        case OP_SHR:    { shifts (mf, aunsigned ? MO_LSR : MO_ASR, "__shr"); break; }
        case OP_CMPEQ:  { compares (mf, MO_BNE); break; }
        case OP_CMPNE:  { compares (mf, MO_BEQ); break; }
        case OP_CMPGE:  { compares (mf, _lt_); break; }
        case OP_CMPGT:  { compares (mf, _le_); break; }
        case OP_CMPLE:  { compares (mf, _gt_); break; }
        case OP_CMPLT:  { compares (mf, _ge_); break; }

        default: assert_false ();
    }

    assert ((flonexts.size () == 1) && (flonexts[0] == linext));
}

void BinopPrim::addSubAndXorOr (MachFile *mf, MachOp mop, char const *mnestr)
{
    Type *atype = aval->getType ()->stripCVMod ();
    Type *btype = bval->getType ()->stripCVMod ();

    IntegType *ainttype = atype->castIntegType ();
    IntegType *binttype = btype->castIntegType ();

    // maybe it can be handled as a standard integer arithmetic opcode
    if (atype->getTypeSize (primtoken) <= 2) {

        // output single arithmetic instruction with loads & stores
        Reg *raval = new Reg (mf, RP_R05);
        Reg *rbval = new Reg (mf, RP_R05);
        Reg *rovar = new Reg (mf, RP_R05);
        aval->loadIntoRn (mf, primtoken, raval, 0);
        bval->loadIntoRn (mf, primtoken, rbval, 0);
        mf->append (new Arith3Mach (__LINE__, primtoken, mop, rovar, raval, rbval));
        ovar->storeFromRn (mf, primtoken, rovar, 0);
        return;
    }

    // 32-bit integer arithmetic
    if ((ainttype != nullptr) && (binttype != nullptr) && (atype->getTypeSize (primtoken) == 4) && (btype->getTypeSize (primtoken) == 4)) {

        // should at least be word aligned
        assert (atype->getTypeAlign (primtoken) >= 2);
        assert (btype->getTypeAlign (primtoken) >= 2);

        // output two arithmetic instructions with loads & stores
        Reg *raptr = new Reg (mf, RP_R05);
        Reg *rbptr = new Reg (mf, RP_R05);
        Reg *roptr = new Reg (mf, RP_R05);
        Reg *raval = new Reg (mf, RP_R05);
        Reg *rbval = new Reg (mf, RP_R05);
        aval->ldaIntoRn (mf, primtoken, raptr, 0);
        bval->ldaIntoRn (mf, primtoken, rbptr, 0);
        ovar->ldaIntoRn (mf, primtoken, roptr, 0);
        mf->append (new LoadMach   (__LINE__, primtoken, MO_LDW, raval, 0, raptr));
        mf->append (new LoadMach   (__LINE__, primtoken, MO_LDW, rbval, 0, rbptr));
        mf->append (new Arith3Mach (__LINE__, primtoken, mop, raval, raval, rbval));
        mf->append (new StoreMach  (__LINE__, primtoken, MO_STW, raval, 0, roptr));
        if (mop == MO_ADD) mop = MO_ADC;
        if (mop == MO_SUB) mop = MO_SBB;
        mf->append (new LoadMach   (__LINE__, primtoken, MO_LDW, raval, 2, raptr));
        mf->append (new LoadMach   (__LINE__, primtoken, MO_LDW, rbval, 2, rbptr));
        mf->append (new Arith3Mach (__LINE__, primtoken, mop, raval, raval, rbval));
        mf->append (new StoreMach  (__LINE__, primtoken, MO_STW, raval, 2, roptr));
        return;
    }

    // everything else (floats, doubles, quads) handled via function call
    mulDivMod (mf, mnestr);
}

void BinopPrim::shifts (MachFile *mf, MachOp mop, char const *mnestr)
{
    tsize_t bni;

    Type *atype = aval->getType ()->stripCVMod ();
    IntegType *ainttype = atype->castIntegType ();
    tsize_t asize = ainttype->getTypeSize (primtoken);
    if (asize > 2) goto nonconst;

    {
        NumValue bnv;
        NumCat bnc = bval->isNumConstVal (&bnv);
        switch (bnc) {
            case NC_SINT: bni = bnv.s; break;
            case NC_UINT: bni = bnv.u; break;
            default: goto nonconst;
        }
    }

    bni &= (1U << (asize * 8)) - 1U;
    if (bni > 4) goto nonconst;

    {
        Reg *rr = new Reg (mf, RP_R05);
        aval->loadIntoRn  (mf, primtoken, rr, 0);
        while (bni > 0) {
            mf->append (new Arith2Mach (__LINE__, primtoken, mop, rr, rr));
            -- bni;
        }
        ovar->storeFromRn (mf, primtoken, rr, 0);
    }
    return;

    // everything else handled via function call
nonconst:
    mulDivMod (mf, mnestr);
}

void BinopPrim::mulDivMod (MachFile *mf, char const *mnestr)
{
    char *fullstr = (char *) malloc (10);

    int mnelen = strlen (mnestr);
    assert (mnelen <= 5);
    memcpy (fullstr, mnestr, mnelen);
    fullstr[mnelen] = '_';

    // ovar = aval * bval
    NumType *otype = ovar->getType ()->stripCVMod ()->castNumType ();
    NumType *atype = aval->getType ()->stripCVMod ()->castNumType ();
    NumType *btype = bval->getType ()->stripCVMod ()->castNumType ();
    assert ((atype != nullptr) && (btype != nullptr));

    fullstr[mnelen+1] = btow (otype->sizechar);
    fullstr[mnelen+2] = getsizechar (aval);
    fullstr[mnelen+3] = getsizechar (bval);
    fullstr[mnelen+4] = 0;

    Reg *r0reg;     // result
    if (otype->getTypeSize (primtoken) > 2) {
        // result doesn't fit in register, get address in R0
        r0reg = new Reg (mf, RP_R0);
        ovar->ldaIntoRn (mf, primtoken, r0reg, 0);
    } else {
        // result fits in register, set up R0 to capture result
        r0reg = new Reg (mf, RP_R0);
    }

    Reg *r1reg;     // left
    if (atype->getTypeSize (primtoken) > 2) {
        // left doesn't fit in register, get address in R1
        r1reg = new Reg (mf, RP_R1);
        aval->ldaIntoRn (mf, primtoken, r1reg, 0);
    } else {
        // left fits in register, get value in R1
        r1reg = new Reg (mf, RP_R1);
        aval->loadIntoRn (mf, primtoken, r1reg, 0);
    }

    Reg *r2reg;     // right
    if (btype->getTypeSize (primtoken) > 2) {
        // right doesn't fit in register, get address in R2
        r2reg = new Reg (mf, RP_R2);
        bval->ldaIntoRn (mf, primtoken, r2reg, 0);
    } else {
        // right fits in register, get value in R2
        r2reg = new Reg (mf, RP_R2);
        bval->loadIntoRn (mf, primtoken, r2reg, 0);
    }

    // output call, and it always reads R1, R2
    LabelMach *retlbl = new LabelMach (__LINE__, primtoken);
    Reg *r3reg = new Reg (mf, RP_R3);
    mf->append (new LdImmLblMach (__LINE__, primtoken, r3reg, retlbl));
    CallImmMach *callmach = new CallImmMach (__LINE__, primtoken, fullstr);
    mf->append (callmach);
    callmach->regreads.push_back (r3reg);
    callmach->regreads.push_back (r2reg);
    callmach->regreads.push_back (r1reg);
    mf->append (retlbl);

    if (otype->getTypeSize (primtoken) > 2) {
        // result doesn't fit in register, call reads R0
        callmach->regreads.push_back (r0reg);
        // function stored result via R0
    } else {
        // result fits in register, call writes R0
        callmach->regwrites.push_back (r0reg);
        ovar->storeFromRn (mf, primtoken, r0reg, 0);
    }
}

// given two same-typed numeric operands, create a boolean (0/1) result
void BinopPrim::compares (MachFile *mf, MachOp notbop)
{
    LabelMach *notlbl = new LabelMach (__LINE__, primtoken);

    // higher level should make sure both are same numeric type
    NumType *anumtype = aval->getType ()->stripCVMod ()->castNumType ();
    NumType *bnumtype = bval->getType ()->stripCVMod ()->castNumType ();
    assert ((anumtype != nullptr) && (bnumtype == anumtype));

    if ((anumtype->castIntegType () != nullptr) && (bnumtype->castIntegType () != nullptr)) {

        // for byte/word compare, output direct code
        tsize_t asize = anumtype->getTypeSize (primtoken);
        if (asize <= 2) {
            Reg *raval = new Reg (mf, RP_R05);
            Reg *rbval = new Reg (mf, RP_R05);
            Reg *rovar = new Reg (mf, RP_R05);
            aval->loadIntoRn (mf, primtoken, raval, 0);
            bval->loadIntoRn (mf, primtoken, rbval, 0);
            mf->append (new Arith1Mach (__LINE__, primtoken, MO_CLR, rovar));
            mf->append (new CmpBrMach  (__LINE__, primtoken, notbop, raval, rbval, notlbl, nullptr));
            mf->append (new Arith2Mach (__LINE__, primtoken, MO_INC, rovar, rovar));
            mf->append (notlbl);
            ovar->storeFromRn (mf, primtoken, rovar, 0);
            return;
        }

        // for long compare, output direct code as two compares
        if (asize == 4) {
            LabelMach *onelbl = new LabelMach (__LINE__, primtoken);
            Reg *raval = new Reg (mf, RP_R05);
            Reg *rbval = new Reg (mf, RP_R05);
            Reg *rovar = new Reg (mf, RP_R05);
            mf->append (new Arith1Mach (__LINE__, primtoken, MO_CLR, rovar));
            aval->loadIntoRn (mf, primtoken, raval, 2);
            bval->loadIntoRn (mf, primtoken, rbval, 2);
            mf->append (new CmpBrMach  (__LINE__, primtoken, MO_BNE, raval, rbval, onelbl, nullptr));
            aval->loadIntoRn (mf, primtoken, raval, 0);
            bval->loadIntoRn (mf, primtoken, rbval, 0);
            mf->append (new CmpBrMach  (__LINE__, primtoken, notbop, raval, rbval, notlbl, onelbl));
            mf->append (new Arith2Mach (__LINE__, primtoken, MO_INC, rovar, rovar));
            mf->append (notlbl);
            ovar->storeFromRn (mf, primtoken, rovar, 0);
            return;
        }
    }

    // all others, call a function
    //  input:
    //   R0 = branch opcode
    //   R1 = left operand address
    //   R2 = right operand address
    //  output:
    //   R0 = 0: comparison false; 1: comparison true
    char *fullstr = (char *) malloc (10);
    memcpy (fullstr, "__cmp_", 6);

    fullstr[6] = btow (anumtype->sizechar);
    fullstr[7] = btow (bnumtype->sizechar);
    fullstr[8] = 0;

    Reg *r0reg = new Reg (mf, RP_R0);
    uint16_t op;
    switch (notbop) {
        case MO_BLT:  { op =  0; break; }
        case MO_BLO:  { op =  4; break; }
        case MO_BGE:  { op =  8; break; }
        case MO_BHIS: { op = 12; break; }
        case MO_BLE:  { op = 16; break; }
        case MO_BLOS: { op = 20; break; }
        case MO_BGT:  { op = 24; break; }
        case MO_BHI:  { op = 28; break; }
        case MO_BEQ:  { op = 32; break; }
        case MO_BNE:  { op = 40; break; }
        default: assert_false ();
    }
    mf->append (new LdImmValMach (__LINE__, primtoken, MO_LDW, r0reg, op));

    Reg *r1reg = new Reg (mf, RP_R1);
    aval->ldaIntoRn (mf, primtoken, r1reg, 0);

    Reg *r2reg = new Reg (mf, RP_R2);
    bval->ldaIntoRn (mf, primtoken, r2reg, 0);

    // output call, and it always reads R0, R1, R2 and always writes R0
    LabelMach *retlbl = new LabelMach (__LINE__, primtoken);
    Reg *r3reg = new Reg (mf, RP_R3);
    mf->append (new LdImmLblMach (__LINE__, primtoken, r3reg, retlbl));
    CallImmMach *callmach = new CallImmMach (__LINE__, primtoken, fullstr);
    mf->append (callmach);
    callmach->regreads.push_back (r0reg);
    callmach->regreads.push_back (r1reg);
    callmach->regreads.push_back (r2reg);
    callmach->regreads.push_back (r3reg);
    mf->append (retlbl);
    retlbl->regwrites.push_back (r0reg);

    // store boolean result from R0
    ovar->storeFromRn (mf, primtoken, r0reg, 0);
}

// just before computation of all call arguments
// make room of stack for call arguments
// compute the arguments directly into the stack area
void CallStartPrim::assemprim (MachFile *mf)
{
    CallPrim *cp = this->callprim;
    Type *enttype = cp->entval->getType ()->stripCVMod ();
    PtrType *ptrtype = enttype->castPtrType ();
    if (ptrtype != nullptr) enttype = ptrtype->getBaseType ()->stripCVMod ();
    FuncType *functype = enttype->castFuncType ();
    if (functype == nullptr) {
        throwerror (primtoken, "calling non-function type '%s'", cp->entval->getType ());
    }
    retbyref = functype->isRetByRef (primtoken);

    // count number of stack bytes needed for arguments including varargs
    // - for return-by-reference, leave room to slip in a &retvar at the beginning
    tsize_t prmindex  = 0;
    tsize_t numparams = functype->getNumParams ();
    stacksize = 0;
    ssizenova = 0;
    if (retbyref) ssizenova = stacksize = 2;
    for (VarDecl *argvar : cp->argvars) {
        Type   *type = argvar->getType ()->stripCVMod ();
        tsize_t alin = type->getTypeAlign (primtoken);
        tsize_t size = type->getTypeSize  (primtoken);

        // upper layer should have made sure the types are the same by inserting casts
        // but all we care about is the alignment and size match
        if (prmindex < numparams) {
            Type *ptype = functype->getParamType (prmindex)->stripCVMod ();
            assert (ptype->getTypeAlign (primtoken) == alin);
            assert (ptype->getTypeSize  (primtoken) == size);
        } else {
            assert (functype->hasVarArgs ());
        }
        prmindex ++;

        // assign stack offset of call argument
        stacksize = (stacksize + alin - 1) & - alin;
        assert (argvar->iscallarg == cp);
        argvar->setVarLocn (new CalarLocn (stacksize));

        // accumulate stack size needed
        stacksize += size;
        if (prmindex == numparams) {
            stacksize = (stacksize + MAXALIGN - 1) & - MAXALIGN;
            ssizenova = stacksize;
        }
    }
    assert (prmindex >= numparams);
    stacksize = (stacksize + MAXALIGN - 1) & - MAXALIGN;

    // set up return-by-reference arg if needed
    // do this before decrementing stack pointer in case we are in this situation:
    //   printsomething ( ..., sin (x), ... )
    //  we used to be compiling the args for printsomething()
    //  we are starting to compile the call to sin() which is a retbyref function as it returns a double
    //  so the cp->retvar->ldaIntoRn() below will get the temp arg slot being passed to printsomething()
    //  ...and CalarLocn::getStkOffs() assumes the stack pointer is positioned to point right at the printsomething() call args
    Reg *rareg = nullptr;
    if (retbyref) {
        rareg = new Reg (mf, RP_R05);
        if (cp->retvar != nullptr) {
            cp->retvar->ldaIntoRn (mf, primtoken, rareg, 0);
        } else {
            // caller doesn't want return value, so tell function to put result in the trash
            tsize_t retsize = functype->getRetType ()->getTypeSize (primtoken);
            if (trashsize < retsize) trashsize = retsize;
            mf->append (new LdImmStrMach (__LINE__, primtoken, MO_LDW, rareg, "__trash", nullptr));
        }
    }

    // make room on stack for all arguments
    if (stacksize > 0) {
        bigLdAddrMach (mf, __LINE__, primtoken, mf->spreg, - stacksize, mf->spreg);
    }

    // stack pointer is now offset by that much more
    mf->curspoffs += this->stacksize;

    // set up return-by-reference arg if needed
    // it always is at -SPOFFSET(R6) cuz it is always the first arg passed
    if (retbyref) {
        bigStoreMach (mf, __LINE__, primtoken, MO_STW, rareg, - SPOFFSET, mf->spreg);
    }
}

// output assembly language to call a function
// stack space was already made by CallStartPrim
// and args are filled in by code between CallStartPrim and this prim
void CallPrim::assemprim (MachFile *mf)
{
    Type *enttype = entval->getType ()->stripCVMod ();
    PtrType *ptrtype = enttype->castPtrType ();
    if (ptrtype != nullptr) enttype = ptrtype->getBaseType ()->stripCVMod ();
    FuncType *functype = enttype->castFuncType ();
    if (functype == nullptr) {
        throwerror (primtoken, "calling non-function type '%s'", entval->getType ());
    }

    bool    retbyref  = this->callstartprim->retbyref;
    tsize_t stacksize = this->callstartprim->stacksize;
    tsize_t ssizenova = this->callstartprim->ssizenova;

    // pass 'this' in R2
    Reg *thisreg = nullptr;
    if (this->thisval != nullptr) {
        thisreg = new Reg (mf, RP_R2);
        this->thisval->loadIntoRn (mf, primtoken, thisreg, 0);
    }

    // call the function
    // it pops non-varargs from stack (ssizenova)
    // it writes return value to R0,R1 (if any)
    // it trashes R2,R3
    Reg *rv0reg = nullptr;
    Reg *rv1reg = nullptr;
    if (! retbyref && (retvar != nullptr) && (retvar->getType () != voidtype)) {
        rv0reg = new Reg (mf, RP_R0);
        if (retvar->getType ()->getTypeSize (primtoken) > 2) {
            rv1reg = new Reg (mf, RP_R1);
        }
    }
    LabelMach *retlbl = new LabelMach (__LINE__, primtoken);
    Reg *r3reg = new Reg (mf, RP_R3);
    mf->append (new LdImmLblMach (__LINE__, primtoken, r3reg, retlbl));
    CallMach *mcallop = entval->callTo (mf, primtoken);
    mcallop->stkpop = ssizenova;
    mcallop->regreads.push_back (r3reg);
    if (thisreg != nullptr) mcallop->regreads.push_back (thisreg);
    mf->append (retlbl);
    if (rv0reg != nullptr) retlbl->regwrites.push_back (rv0reg);
    if (rv1reg != nullptr) retlbl->regwrites.push_back (rv1reg);

    // pop varargs from stack
    // function wipes non-varargs args from stack
    if (stacksize > ssizenova) {
        bigLdAddrMach (mf, __LINE__, primtoken, mf->spreg, stacksize - ssizenova, mf->spreg);
    }

    // all args are popped from the stack
    mf->curspoffs -= stacksize;

    // store return value if any
    if (rv0reg != nullptr) retvar->storeFromRn (mf, primtoken, rv0reg, 0);
    if (rv1reg != nullptr) retvar->storeFromRn (mf, primtoken, rv1reg, 2);

    assert ((flonexts.size () == 1) && (flonexts[0] == linext));
}

// output assembly language to convert one numeric type to another numeric type
void CastPrim::assemprim (MachFile *mf)
{
    assert ((flonexts.size () == 1) && (flonexts[0] == linext));

    // aval = input value
    // ovar = output variable
    Type *atype = aval->getType ()->stripCVMod ();
    Type *otype = ovar->getType ()->stripCVMod ();
    tsize_t asize = atype->getTypeSize (primtoken);
    tsize_t osize = otype->getTypeSize (primtoken);
    IntegType *ainttype = atype->castIntegType ();
    IntegType *ointtype = otype->castIntegType ();
    PtrType *aptrtype = atype->castPtrType ();
    PtrType *optrtype = otype->castPtrType ();

    // struct pointer to struct pointer going rootward gets an offset
    if ((aptrtype != nullptr) && (optrtype != nullptr)) {
        StructType *astructtype = aptrtype->getBaseType ()->stripCVMod ()->castStructType ();
        StructType *ostructtype = optrtype->getBaseType ()->stripCVMod ()->castStructType ();
        if ((astructtype != nullptr) && (ostructtype != nullptr)) {
            tsize_t offset;
            if (astructtype->isLeafwardOf (ostructtype, &offset)) {
                Reg *rareg = new Reg (mf, RP_R05);
                aval->loadIntoRn (mf, primtoken, rareg, 0);
                if (offset != 0) {
                    Reg *rbreg = new Reg (mf, RP_R05);
                    mf->append (new LdImmValMach (__LINE__, primtoken, MO_LDW, rbreg, offset));
                    mf->append (new Arith3Mach (__LINE__, primtoken, MO_ADD, rareg, rareg, rbreg));
                }
                ovar->storeFromRn (mf, primtoken, rareg, 0);
                return;
            }
            if (ostructtype->isLeafwardOf (astructtype, &offset)) {
                printerror (primtoken, "use virtual function for leafward cast of '%s *' to '%s *'\n", astructtype->getName (), ostructtype->getName ());
            } else {
                printerror (primtoken, "unrelated cast of '%s *' to '%s *', use intermediate 'void *' to avoid error message", astructtype->getName (), ostructtype->getName ());
            }
        }
    }

    // see if casting pointer from/to pointer or integer of pointer size
    if ((aptrtype != nullptr) && (ointtype != nullptr) && (osize == sizeof (tsize_t))) goto ptrcast;
    if ((ainttype != nullptr) && (asize == sizeof (tsize_t)) && (optrtype != nullptr)) goto ptrcast;
    if ((aptrtype == nullptr) || (optrtype == nullptr)) goto notpcast;
ptrcast:;
    {
        Reg *rareg = new Reg (mf, RP_R05);
        aval->loadIntoRn (mf, primtoken, rareg, 0);
        ovar->storeFromRn (mf, primtoken, rareg, 0);
        return;
    }
notpcast:;

    // see if casting to boolean
    if (otype == booltype) {

        // integer/pointer -> boolean
        if ((ainttype != nullptr) || (aptrtype != nullptr)) {

            // get first 1 or 2 bytes of number into 'rareg'
            Reg *rareg = new Reg (mf, RP_R05);
            aval->loadIntoRn (mf, primtoken, rareg, 0);

            // set up a 'roreg' output register with a zero value to start with
            Reg *roreg = new Reg (mf, RP_R05);
            mf->append (new Arith1Mach (__LINE__, primtoken, MO_CLR, roreg));

            // dispatch based on input integer value
            tsize_t asize = atype->getTypeSize (primtoken);
            switch (asize) {

                // byte,word - ok as is
                case 1:
                case 2: break;

                // quad - or in top two words
                case 8: {
                    Reg *rbreg = new Reg (mf, RP_R05);
                    aval->loadIntoRn (mf, primtoken, rbreg, 6);
                    mf->append (new Arith3Mach (__LINE__, primtoken, MO_OR, rareg, rareg, rbreg));
                    aval->loadIntoRn (mf, primtoken, rbreg, 4);
                    mf->append (new Arith3Mach (__LINE__, primtoken, MO_OR, rareg, rareg, rbreg));
                    // fallthrough
                }

                // long,quad - or in second word
                case 4: {
                    Reg *rbreg = new Reg (mf, RP_R05);
                    aval->loadIntoRn (mf, primtoken, rbreg, 2);
                    mf->append (new Arith3Mach (__LINE__, primtoken, MO_OR, rareg, rareg, rbreg));
                    break;
                }

                default: assert_false ();
            }

            // test composite value and store result
            LabelMach *falselbl = new LabelMach (__LINE__, primtoken);
            mf->append (new TstBrMach (__LINE__, primtoken, MO_BEQ, rareg, falselbl, nullptr));
            mf->append (new Arith2Mach (__LINE__, primtoken, MO_INC, roreg, roreg));
            mf->append (falselbl);
            ovar->storeFromRn (mf, primtoken, roreg, 0);
            return;
        }

        // floatingpoint -> boolean
        FloatType *aflttype = aval->getType ()->stripCVMod ()->castFloatType ();
        if (aflttype != nullptr) {
            Reg *roreg = new Reg (mf, RP_R0);
            Reg *rareg = new Reg (mf, RP_R1);
            aval->ldaIntoRn (mf, primtoken, rareg, 0);
            char *cvtxx = strdup ("__cvt_z?");
            cvtxx[7] = aflttype->sizechar;
            LabelMach *retlbl = new LabelMach (__LINE__, primtoken);
            Reg *r3reg = new Reg (mf, RP_R3);
            mf->append (new LdImmLblMach (__LINE__, primtoken, r3reg, retlbl));
            Mach *callmach = mf->append (new CallImmMach (__LINE__, primtoken, cvtxx));
            callmach->regreads.push_back (r3reg);
            callmach->regreads.push_back (rareg);
            mf->append (retlbl);
            retlbl->regwrites.push_back (roreg);
            ovar->storeFromRn (mf, primtoken, roreg, 0);
            return;
        }

        throwerror (primtoken, "unsupported casting '%s' to '%s'", atype->getName (), otype->getName ());
    }

    // convert one size integer to another
    if ((ainttype != nullptr) && (ointtype != nullptr)) {

        // narrowing integer -> integer
        if (osize <= asize) {
            switch (osize) {
                case 1: {   // byte,word,long,quad -> byte
                    Reg *rareg = new Reg (mf, RP_R05);
                    aval->loadIntoRn  (mf, primtoken, rareg, 0);
                    ovar->storeFromRn (mf, primtoken, rareg, 0);
                    return;
                }
                case 2: {   // word,long,quad -> word
                    Reg *rareg = new Reg (mf, RP_R05);
                    aval->loadIntoRn  (mf, primtoken, rareg, 0);
                    ovar->storeFromRn (mf, primtoken, rareg, 0);
                    return;
                }
                case 4: {   // long,quad -> long
                    Reg *rareg = new Reg (mf, RP_R05);
                    aval->loadIntoRn  (mf, primtoken, rareg, 0);
                    ovar->storeFromRn (mf, primtoken, rareg, 0);
                    aval->loadIntoRn  (mf, primtoken, rareg, 2);
                    ovar->storeFromRn (mf, primtoken, rareg, 2);
                    return;
                }
                case 8: {   // quad -> quad
                    Reg *rareg = new Reg (mf, RP_R05);
                    aval->loadIntoRn  (mf, primtoken, rareg, 0);
                    ovar->storeFromRn (mf, primtoken, rareg, 0);
                    aval->loadIntoRn  (mf, primtoken, rareg, 2);
                    ovar->storeFromRn (mf, primtoken, rareg, 2);
                    aval->loadIntoRn  (mf, primtoken, rareg, 4);
                    ovar->storeFromRn (mf, primtoken, rareg, 4);
                    aval->loadIntoRn  (mf, primtoken, rareg, 6);
                    ovar->storeFromRn (mf, primtoken, rareg, 6);
                    return;
                }
                default: assert_false ();
            }
        }

        // widening integer -> integer
        // if changing signedness, widen using source signedness
        //  eg, widen an int8_t -1 to uint16_t 65535, not uint16_t 255
        bool asined = ainttype->getSign ();

        switch (asize) {

            // byte -> word,long,quad
            // word -> long,quad
            case 1: {
                assert ((osize == 2) || (osize == 4) || (osize == 8));
                Reg *rareg = new Reg (mf, RP_R05);
                aval->loadIntoRn  (mf, primtoken, rareg, 0);
                ovar->storeFromRn (mf, primtoken, rareg, 0);
                Reg *rbreg = nullptr;
                if (osize >= 4) {
                    rbreg = new Reg (mf, RP_R05);
                    if (asined) {
                        mf->append (new Arith3Mach (__LINE__, primtoken, MO_ADD, rbreg, rareg, rareg));
                        mf->append (new Arith3Mach (__LINE__, primtoken, MO_SBB, rbreg, rbreg, rbreg));
                    } else {
                        mf->append (new Arith1Mach (__LINE__, primtoken, MO_CLR, rbreg));
                    }
                    ovar->storeFromRn (mf, primtoken, rbreg, 2);
                }
                if (osize == 8) {
                    ovar->storeFromRn (mf, primtoken, rbreg, 4);
                    ovar->storeFromRn (mf, primtoken, rbreg, 6);
                }
                return;
            }

            case 2: {
                Reg *rareg = new Reg (mf, RP_R05);
                aval->loadIntoRn  (mf, primtoken, rareg, 0);
                ovar->storeFromRn (mf, primtoken, rareg, 0);
                Reg *rbreg = new Reg (mf, RP_R05);
                if (asined) {
                    mf->append (new Arith3Mach (__LINE__, primtoken, MO_ADD, rbreg, rareg, rareg));
                    mf->append (new Arith3Mach (__LINE__, primtoken, MO_SBB, rbreg, rbreg, rbreg));
                } else {
                    mf->append (new Arith1Mach (__LINE__, primtoken, MO_CLR, rbreg));
                }
                switch (osize) {
                    case 4: {
                        ovar->storeFromRn (mf, primtoken, rbreg, 2);
                        return;
                    }
                    case 8: {
                        ovar->storeFromRn (mf, primtoken, rbreg, 2);
                        ovar->storeFromRn (mf, primtoken, rbreg, 4);
                        ovar->storeFromRn (mf, primtoken, rbreg, 6);
                        return;
                    }
                    default: assert_false ();
                }
            }

            // long -> quad
            case 4: {
                assert (osize == 8);
                Reg *rareg = new Reg (mf, RP_R05);
                aval->loadIntoRn  (mf, primtoken, rareg, 0);
                ovar->storeFromRn (mf, primtoken, rareg, 0);
                Reg *rbreg = new Reg (mf, RP_R05);
                aval->loadIntoRn  (mf, primtoken, rbreg, 2);
                ovar->storeFromRn (mf, primtoken, rbreg, 2);
                Reg *rcreg = new Reg (mf, RP_R05);
                if (asined) {
                    mf->append (new Arith3Mach (__LINE__, primtoken, MO_ADD, rcreg, rbreg, rbreg));
                    mf->append (new Arith3Mach (__LINE__, primtoken, MO_SBB, rcreg, rcreg, rcreg));
                } else {
                    mf->append (new Arith1Mach (__LINE__, primtoken, MO_CLR, rcreg));
                }
                ovar->storeFromRn (mf, primtoken, rcreg, 4);
                ovar->storeFromRn (mf, primtoken, rcreg, 6);
                return;
            }

            default: assert_false ();
        }
    }

    // floatingpoint/integer conversion
    NumType *anumtype = atype->castNumType ();
    NumType *onumtype = otype->castNumType ();
    assert ((anumtype != nullptr) && (onumtype != nullptr));

    Reg *r0reg = new Reg (mf, RP_R0);   // result
    if (onumtype->getTypeSize (primtoken) > 2) {
        // result doesn't fit in register, get address in R0
        ovar->ldaIntoRn (mf, primtoken, r0reg, 0);
    } else {
        // result fits in register, set up R0 to capture result
    }

    Reg *r1reg = new Reg (mf, RP_R1);   // left
    if (anumtype->getTypeSize (primtoken) > 2) {
        // left doesn't fit in register, get address in R1
        aval->ldaIntoRn (mf, primtoken, r1reg, 0);
    } else {
        // left fits in register, get value in R1
        aval->loadIntoRn (mf, primtoken, r1reg, 0);
    }

    // make up entrypoint name
    char *cvtxx = strdup ("__cvt_??");
    cvtxx[6] = onumtype->sizechar;
    cvtxx[7] = anumtype->sizechar;

    // output call, and it always reads R1
    LabelMach *retlbl = new LabelMach (__LINE__, primtoken);
    Reg *r3reg = new Reg (mf, RP_R3);
    mf->append (new LdImmLblMach (__LINE__, primtoken, r3reg, retlbl));
    CallImmMach *callmach = new CallImmMach (__LINE__, primtoken, cvtxx);
    mf->append (callmach);
    callmach->regreads.push_back (r1reg);
    callmach->regreads.push_back (r3reg);
    mf->append (retlbl);

    if (onumtype->getTypeSize (primtoken) > 2) {
        // result doesn't fit in register, call reads R0
        callmach->regreads.push_back (r0reg);
        // function stored result via R0
    } else {
        // result fits in register, call writes R0
        callmach->regwrites.push_back (r0reg);
        ovar->storeFromRn (mf, primtoken, r0reg, 0);
    }
}

// convert prim-level compare opcode to machine-level branch opcode
//  input:
//   sined  = true: signed compare; false: unsigned compare
//   opcode = OP_CMP?? compare opcode
//  output:
//   returns MO_B?? branch opcode
static MachOp getbranchmop (bool sined, Opcode opcode)
{
    switch (opcode) {
        case OP_CMPEQ: return MO_BEQ;
        case OP_CMPNE: return MO_BNE;
        case OP_CMPGE: return sined ? MO_BGE : MO_BHIS;
        case OP_CMPGT: return sined ? MO_BGT : MO_BHI;
        case OP_CMPLE: return sined ? MO_BLE : MO_BLOS;
        case OP_CMPLT: return sined ? MO_BLT : MO_BLO;
        default: assert_false ();
    }
}

// catch block entrypoint
//  catlblprim:         << already been written out
//   thrownvar = R0
void CatEntPrim::assemprim (MachFile *mf)
{
    // say that the 'catlblprim' label writes register R0 (containing thrown object pointer) and R1 (containing throw object type id)
    Reg *tvreg = new Reg (mf, RP_R0);
    Reg *ttreg = new Reg (mf, RP_R1);
    Mach *cl = catlblprim->getLabelMach ();
    cl->regwrites.push_back (tvreg);
    cl->regwrites.push_back (ttreg);

    // store R0 in thrownvar
    thrownvar->storeFromRn (mf, primtoken, tvreg, 0);

    // store R1 in ttypevar
    if (ttypevar != nullptr) {
        ttypevar->storeFromRn (mf, primtoken, ttreg, 0);
    }
}

// output assembly language to compare two byte/word/long integers and branch
// quad integers and floatingpoint are handled by a BinopPrim & TestPrim
void CompPrim::assemprim (MachFile *mf)
{
    // upper level code should make sure we are byte/word/long ints and cast to same type
    IntegType *ainttype = aval->getType ()->stripCVMod ()->castIntegType ();
    IntegType *binttype = bval->getType ()->stripCVMod ()->castIntegType ();
    assert ((ainttype != nullptr) && (binttype == ainttype));
    tsize_t asize = ainttype->getTypeSize (primtoken);
    assert ((asize == 1) || (asize == 2) || (asize == 4));

    // get branch assembler opcode
    bool sined = ainttype->getSign ();
    MachOp bop = getbranchmop (sined, opcode);

    // output instructions to compare and branch
    Reg *rareg = new Reg (mf, RP_R05);
    Reg *rbreg = new Reg (mf, RP_R05);

    LabelMach *onelbl = nullptr;
    if (asize == 4) {
        onelbl = new LabelMach (__LINE__, primtoken);
        aval->loadIntoRn (mf, primtoken, rareg, 2);
        bval->loadIntoRn (mf, primtoken, rbreg, 2);
        mf->append (new CmpBrMach (__LINE__, primtoken, MO_BNE, rareg, rbreg, onelbl, nullptr));
    }

    aval->loadIntoRn (mf, primtoken, rareg, 0);
    bval->loadIntoRn (mf, primtoken, rbreg, 0);
    mf->append (new CmpBrMach (__LINE__, primtoken, bop, rareg, rbreg, flonexts[0]->getLabelMach (), onelbl));

    assert ((flonexts.size () == 2) && (flonexts[1] == linext));
}

// generate assembly language for function entry
void EntryPrim::assemprim (MachFile *mf)
{
    mf->curspoffs = 0;

    // generate assembly label and subtract stack size
    Reg *rareg = new Reg (mf, RP_R3);
    Reg *tpreg = (funcdecl->thisptr == nullptr) ? nullptr : new Reg (mf, RP_R2);
    mf->append (new EntMach (__LINE__, primtoken, funcdecl, mf->spreg, rareg, tpreg));

    // save return address in a temp stack var
    bigStoreStkMach (mf, __LINE__, primtoken, MO_STW, rareg, funcdecl->retaddr, 0);

    // save 'this' pointer in a temp stack var
    if (tpreg != nullptr) {
        bigStoreStkMach (mf, __LINE__, primtoken, MO_STW, tpreg, funcdecl->thisptr, 0);
    }

    // if return value is return-by-reference, say the return value is at offset 0 of the call args
    // the funcdecl->retvalue variable should already be set up as a pointer type
    // ...we just set its location to be offset 0 of the call arguments passed to us
    if (funcdecl->getFuncType ()->isRetByRef (primtoken)) {
        RBRPtLocn *rbrptlocn = new RBRPtLocn ();
        rbrptlocn->funcdecl  = funcdecl;
        PtrDecl *ptrdecl = funcdecl->retvalue->castPtrDecl ();
        assert (ptrdecl != nullptr);
        VarDecl *vardecl = ptrdecl->subvaldecl->castVarDecl ();
        assert (vardecl != nullptr);
        vardecl->setVarLocn (rbrptlocn);
    }

    assert ((flonexts.size () == 1) && (flonexts[0] == linext));
}

// call finally block code having it return to the given label
//  R3 = &retlbl;
//  jump finlbl;
void FinCallPrim::assemprim (MachFile *mf)
{
    Reg *rareg = new Reg (mf, RP_R3);
    mf->append (new LdImmLblMach (__LINE__, primtoken, rareg, retlblprim->getLabelMach ()));
    Mach *bm = mf->append (new BranchMach (__LINE__, primtoken, finlblprim->getLabelMach ()));
    bm->regreads.push_back (rareg);
}

// finally block entrypoint
//  finlbl:             << already been written out
//   __trystack = trymarkvar.outer;
//   trymarkvar.outer = R3;
void FinEntPrim::assemprim (MachFile *mf)
{
    // say that the 'finlbl' label writes register R3 (containing where to jump when finally block finishes)
    Reg *rareg = new Reg (mf, RP_R3);
    Mach *fl = finlblprim->getLabelMach ();
    fl->regwrites.push_back (rareg);

    // markptrreg = &trymarkvar
    Reg *markptrreg = new Reg (mf, RP_R05);
    trymarkvar->ldaIntoRn (mf, primtoken, markptrreg, 0);

    // oldstkreg = trymarkvar.outer
    Reg *oldstkreg = new Reg (mf, RP_R05);
    tsize_t outeroffset = trymarktype->getMemberOffset (primtoken, "outer");
    mf->append (new LoadMach (__LINE__, primtoken, MO_LDW, oldstkreg, outeroffset, markptrreg));

    // trystackvar = oldstkreg
    trystackvar->storeFromRn (mf, primtoken, oldstkreg, 0);

    // store R3 in trymarkvar.outer
    mf->append (new StoreMach (__LINE__, primtoken, MO_STW, rareg, outeroffset, markptrreg));
}

// finally block return
//  R0 = &trymarkvar;
//  PC = trymarkvar.outer;
void FinRetPrim::assemprim (MachFile *mf)
{
    Reg *r0reg = new Reg (mf, RP_R0);
    trymarkvar->ldaIntoRn (mf, primtoken, r0reg, 0);
    tsize_t outeroffset = trymarktype->getMemberOffset (primtoken, "outer");
    mf->append (new LoadMach (__LINE__, primtoken, MO_LDW, mf->pcreg, outeroffset, r0reg));
}

void JumpPrim::assemprim (MachFile *mf)
{
    assert (flonexts.size () == 1);

    LabelPrim *lblprim = flonexts[0]->castLabelPrim ();
    assert (lblprim != nullptr);
    mf->append (new BranchMach (__LINE__, primtoken, lblprim->getLabelMach ()));
}

// these are markers for branches
void LabelPrim::assemprim (MachFile *mf)
{
    mf->append (this->getLabelMach ());

    assert ((flonexts.size () == 1) && (flonexts[0] == linext));
}

LabelMach *LabelPrim::getLabelMach ()
{
    if (labelmach == nullptr) {
        labelmach = new LabelMach (__LINE__, primtoken);
    }
    return labelmach;
}

// generate assembly language for function return
void RetPrim::assemprim (MachFile *mf)
{
    Reg *rv0reg = nullptr;
    Reg *rv1reg = nullptr;
    if (rval != nullptr) {
        FuncType *functype = funcdecl->getType ()->stripCVMod ()->castFuncType ();
        Type *rettype = functype->getRetType ();

        // direct return of return value in R0,R1
        if (! functype->isRetByRef (primtoken)) {
            assert (rettype->getTypeSize (primtoken) <= 4);
            rv0reg = new Reg (mf, RP_R0);
            rval->loadIntoRn (mf, primtoken, rv0reg, 0);
            if (rettype->getTypeSize (primtoken) > 2) {
                rv1reg = new Reg (mf, RP_R1);
                rval->loadIntoRn (mf, primtoken, rv1reg, 2);
            }
        }
    }

    // get the stashed return address back in R3
    Reg *rareg = new Reg (mf, RP_R3);
    bigLoadStkMach (mf, __LINE__, primtoken, MO_LDW, rareg, funcdecl->retaddr, 0);

    // output return instruction
    mf->append (new RetMach (__LINE__, primtoken, funcdecl, mf->spreg, rareg, rv0reg, rv1reg));

    assert (flonexts.size () == 0);
}

// generate assembly language for small integer (1, 2 or 4 byte) test and branch
void TestPrim::assemprim (MachFile *mf)
{
    assert (flonexts.size () == 2);
    assert (flonexts[1] == linext);
    assert (flonexts[0]->castLabelPrim () != nullptr);

    // upper level code should make sure we are byte/word/long ints
    IntegType *ainttype = aval->getType ()->stripCVMod ()->castIntegType ();
    assert (ainttype != nullptr);
    tsize_t asize = ainttype->getTypeSize (primtoken);
    assert ((asize == 1) || (asize == 2) || (asize == 4));

    // get branch assembler opcode
    bool sined = ainttype->getSign ();
    MachOp bop = getbranchmop (sined, opcode);

    // output instructions to test and branch
    Reg *rareg = new Reg (mf, RP_R05);

    LabelMach *onelbl = nullptr;
    if (asize == 4) {
        onelbl = new LabelMach (__LINE__, primtoken);
        aval->loadIntoRn (mf, primtoken, rareg, 2);
        mf->append (new TstBrMach (__LINE__, primtoken, MO_BNE, rareg, onelbl, nullptr));
    }

    aval->loadIntoRn (mf, primtoken, rareg, 0);
    mf->append (new TstBrMach (__LINE__, primtoken, bop, rareg, flonexts[0]->getLabelMach (), onelbl));
}

// generate code for a throw
void ThrowPrim::assemprim (MachFile *mf)
{
    if (thrownval == nullptr) {
        mf->append (new CallImmMach (__LINE__, primtoken, "__rethrow"));
    } else {
        Reg *r0reg = new Reg (mf, RP_R0);
        Reg *r1reg = new Reg (mf, RP_R1);

        Type *throwntype = thrownval->getType ();
        thrownval->loadIntoRn (mf, primtoken, r0reg, 0);
        mf->append (new LdImmStrMach (__LINE__, primtoken, MO_LDW, r1reg, throwntype->getThrowID (primtoken), nullptr));

        Mach *cm = mf->append (new CallImmMach (__LINE__, primtoken, "__throw"));
        cm->regreads.push_back (r0reg);
        cm->regreads.push_back (r1reg);
    }
}

// generate code at beginning of try block
//  trymarkvar.outer = __trystack;
//  trymarkvar.descr = &trydesc;
//  __trystack = &trymarkvar;
void TryBegPrim::assemprim (MachFile *mf)
{
    // extern __trymark_t *__trystack;
    if (trystackvar == nullptr) {
        trystackvar = new VarDecl ("__trystack", globalscope, intdeftok, trymarktype->getPtrType (), KW_EXTERN);
        trystackvar->allocstaticvar ();
        trystackvar->outputstaticvar ();
    }

    // make sure all caught typeids have been output to sfile
    for (auto it : catlblprims) {
        Type *cattype = it.first;
        cattype->getThrowID (primtoken);
    }

    // crank out the descriptor table
    //  trydesc:
    //      .word   trymark_spoffset
    //      .word   finalentrypoint
    //      .word   catchpointertype[0]
    //      .word   catchentrypoint[0]
    //      .word   catchpointertype[1]
    //      .word   catchentrypoint[1]
    //          ...
    //      .word   0
    char *trydescname;
    asprintf (&trydescname, "%s.desc", trymarkvar->getName ());
    trymarkvar->outspoffssym = true;
    RefLabelMach *rlm = new RefLabelMach (__LINE__, primtoken);
    fprintf (sfile, "; --------------\n");
    fprintf (sfile, "\t.psect\t.50.text\n");
    fprintf (sfile, "\t.align\t2\n");
    fprintf (sfile, "%s:\n", trydescname);
    fprintf (sfile, "\t.word\t%s.spoffs\n", trymarkvar->getName ());
    fprintf (sfile, "\t.word\t%s ; finally\n", finlblprim->getLabelMach ()->lblstr ());
    rlm->labelvec.push_back (finlblprim->getLabelMach ());
    for (auto it : catlblprims) {
        Type *cattype = it.first;
        LabelPrim *catlabl = it.second;
        fprintf (sfile, "\t.word\t%s,%s ; %s\n", cattype->getThrowID (primtoken), catlabl->getLabelMach ()->lblstr (), cattype->getName ());
        rlm->labelvec.push_back (catlabl->getLabelMach ());
    }
    fprintf (sfile, "\t.word\t0\n");
    fprintf (sfile, "; --------------\n");

    // don't optimize away those handlers unless the try itself gets nuked
    mf->append (rlm);

    // markptrreg = &trymarkvar
    Reg *markptrreg = new Reg (mf, RP_R05);
    trymarkvar->ldaIntoRn (mf, primtoken, markptrreg, 0);

    // stkptrreg = &trystackvar
    Reg *stkptrreg = new Reg (mf, RP_R05);
    trystackvar->ldaIntoRn (mf, primtoken, stkptrreg, 0);

    // oldstkreg = trystackvar
    Reg *oldstkreg = new Reg (mf, RP_R05);
    mf->append (new LoadMach (__LINE__, primtoken, MO_LDW, oldstkreg, 0, stkptrreg));

    // trymarkvar.outer = oldstkreg
    tsize_t outeroffset = trymarktype->getMemberOffset (primtoken, "outer");
    mf->append (new StoreMach (__LINE__, primtoken, MO_STW, oldstkreg, outeroffset, markptrreg));

    // descrreg = &trydesc
    Reg *descrreg = new Reg (mf, RP_R05);
    mf->append (new LdImmStrMach (__LINE__, primtoken, MO_LDW, descrreg, trydescname, trymarkvar));

    // trymarkvar.descr = descrreg
    tsize_t descroffset = trymarktype->getMemberOffset (primtoken, "descr");
    mf->append (new StoreMach (__LINE__, primtoken, MO_STW, descrreg, descroffset, markptrreg));

    // trystackvar = &trymarkvar
    mf->append (new StoreMach (__LINE__, primtoken, MO_STW, markptrreg, 0, stkptrreg));
}

// generate assembly language for unary operator
void UnopPrim::assemprim (MachFile *mf)
{
    switch (opcode) {
        case OP_ADDROF: {
            Reg *rareg = new Reg (mf, RP_R05);
            aval->ldaIntoRn (mf, primtoken, rareg, 0);
            ovar->storeFromRn (mf, primtoken, rareg, 0);
            break;
        }

        case OP_LOGNOT: {
            Reg *rareg = new Reg (mf, RP_R05);
            Reg *roreg = new Reg (mf, RP_R05);
            aval->loadIntoRn (mf, primtoken, rareg, 0);
            if (aval->getType ()->stripCVMod () == booltype) {
                mf->append (new LdImmValMach (__LINE__, primtoken, MO_LDW, roreg, 1));
                mf->append (new Arith3Mach (__LINE__, primtoken, MO_XOR, roreg, roreg, rareg));
            } else {
                Reg *rbreg = new Reg (mf, RP_R05);
                Reg *rcreg = new Reg (mf, RP_R05);
                Reg *rdreg = new Reg (mf, RP_R05);
                mf->append (new LdImmValMach (__LINE__, primtoken, MO_LDW, rbreg, 1));
                mf->append (new Arith3Mach (__LINE__, primtoken, MO_SUB, rcreg, rareg, rbreg));
                mf->append (new Arith3Mach (__LINE__, primtoken, MO_SBB, rdreg, rcreg, rcreg));
                mf->append (new Arith2Mach (__LINE__, primtoken, MO_NEG, roreg, rdreg));
            }
            ovar->storeFromRn (mf, primtoken, roreg, 0);
            break;
        }

        case OP_BITNOT: {
            Type *atype = aval->getType ()->stripCVMod ();
            if (atype == booltype) {
                Reg *rareg = new Reg (mf, RP_R05);
                Reg *roreg = new Reg (mf, RP_R05);
                aval->loadIntoRn (mf, primtoken, rareg, 0);
                mf->append (new LdImmValMach (__LINE__, primtoken, MO_LDW, roreg, 1));
                mf->append (new Arith3Mach (__LINE__, primtoken, MO_XOR, roreg, roreg, rareg));
                ovar->storeFromRn (mf, primtoken, roreg, 0);
            } else {

                IntegType *aintype = atype->castIntegType ();
                assert (aintype != nullptr);

                switch (aintype->getTypeSize (primtoken)) {
                    case 1:
                    case 2: {
                        Reg *rareg = new Reg (mf, RP_R05);
                        Reg *roreg = new Reg (mf, RP_R05);
                        aval->loadIntoRn (mf, primtoken, rareg, 0);
                        mf->append (new Arith2Mach (__LINE__, primtoken, MO_COM, roreg, rareg));
                        ovar->storeFromRn (mf, primtoken, roreg, 0);
                        break;
                    }

                    case 4: {
                        Reg *rareg = new Reg (mf, RP_R05);
                        Reg *rbreg = new Reg (mf, RP_R05);
                        Reg *roreg = new Reg (mf, RP_R05);
                        Reg *rpreg = new Reg (mf, RP_R05);
                        aval->loadIntoRn (mf, primtoken, rareg, 0);
                        aval->loadIntoRn (mf, primtoken, rbreg, 2);
                        mf->append (new Arith2Mach (__LINE__, primtoken, MO_COM, roreg, rareg));
                        mf->append (new Arith2Mach (__LINE__, primtoken, MO_COM, rpreg, rbreg));
                        ovar->storeFromRn (mf, primtoken, roreg, 0);
                        ovar->storeFromRn (mf, primtoken, rpreg, 2);
                        break;
                    }

                    case 8: {
                        Reg *rareg = new Reg (mf, RP_R1);
                        Reg *roreg = new Reg (mf, RP_R0);
                        ovar->ldaIntoRn (mf, primtoken, roreg, 0);
                        aval->ldaIntoRn (mf, primtoken, rareg, 0);
                        LabelMach *retlbl = new LabelMach (__LINE__, primtoken);
                        Reg *r3reg = new Reg (mf, RP_R3);
                        mf->append (new LdImmLblMach (__LINE__, primtoken, r3reg, retlbl));
                        CallImmMach *callmach = new CallImmMach (__LINE__, primtoken, "__com_QQ");
                        mf->append (callmach);
                        callmach->regreads.push_back (r3reg);
                        callmach->regreads.push_back (rareg);
                        callmach->regreads.push_back (roreg);
                        mf->append (retlbl);
                        break;
                    }

                    default: assert_false ();
                }
            }
            break;
        }

        case OP_NEGATE: {
            Type *atype = aval->getType ()->stripCVMod ();
            if (atype == booltype) {
                Reg *rareg = new Reg (mf, RP_R05);
                Reg *roreg = new Reg (mf, RP_R05);
                aval->loadIntoRn (mf, primtoken, rareg, 0);
                mf->append (new LdImmValMach (__LINE__, primtoken, MO_LDW, roreg, 1));
                mf->append (new Arith3Mach (__LINE__, primtoken, MO_XOR, roreg, roreg, rareg));
                ovar->storeFromRn (mf, primtoken, roreg, 0);
            } else {

                IntegType *ainttype = atype->castIntegType ();
                if (ainttype != nullptr) {
                    switch (ainttype->getTypeSize (primtoken)) {
                        case 1:
                        case 2: {
                            Reg *rareg = new Reg (mf, RP_R05);
                            Reg *roreg = new Reg (mf, RP_R05);
                            aval->loadIntoRn (mf, primtoken, rareg, 0);
                            mf->append (new Arith2Mach (__LINE__, primtoken, MO_NEG, roreg, rareg));
                            ovar->storeFromRn (mf, primtoken, roreg, 0);
                            break;
                        }

                        case 4: {
                            Reg *rareg = new Reg (mf, RP_R05);
                            Reg *rbreg = new Reg (mf, RP_R05);
                            Reg *roreg = new Reg (mf, RP_R05);
                            Reg *rpreg = new Reg (mf, RP_R05);
                            Reg *rzreg = new Reg (mf, RP_R05);
                            mf->append (new Arith1Mach (__LINE__, primtoken, MO_CLR, rzreg));
                            aval->loadIntoRn (mf, primtoken, rareg, 0);
                            aval->loadIntoRn (mf, primtoken, rbreg, 2);
                            mf->append (new Arith3Mach (__LINE__, primtoken, MO_SUB, roreg, rzreg, rareg));
                            mf->append (new Arith3Mach (__LINE__, primtoken, MO_SBB, rpreg, rzreg, rbreg));
                            ovar->storeFromRn (mf, primtoken, roreg, 0);
                            ovar->storeFromRn (mf, primtoken, rpreg, 2);
                            break;
                        }

                        case 8: {
                            Reg *rareg = new Reg (mf, RP_R1);
                            Reg *roreg = new Reg (mf, RP_R0);
                            ovar->ldaIntoRn (mf, primtoken, roreg, 0);
                            aval->ldaIntoRn (mf, primtoken, rareg, 0);
                            LabelMach *retlbl = new LabelMach (__LINE__, primtoken);
                            Reg *r3reg = new Reg (mf, RP_R3);
                            mf->append (new LdImmLblMach (__LINE__, primtoken, r3reg, retlbl));
                            CallImmMach *callmach = new CallImmMach (__LINE__, primtoken, "__neg_QQ");
                            mf->append (callmach);
                            callmach->regreads.push_back (r3reg);
                            callmach->regreads.push_back (rareg);
                            callmach->regreads.push_back (roreg);
                            mf->append (retlbl);
                            break;
                        }

                        default: assert_false ();
                    }
                    break;
                }

                FloatType *aflttype = atype->castFloatType ();
                if (aflttype != nullptr) {
                    switch (aflttype->getTypeSize (primtoken)) {
                        case 4: {
#if 0
                            Reg *rareg = new Reg (mf, RP_R05);
                            Reg *rzreg = new Reg (mf, RP_R05);
                            Reg *roreg = new Reg (mf, RP_R05);
                            aval->loadIntoRn  (mf, primtoken, rareg, 2);
                            mf->append (new LdImmValMach (__LINE__, primtoken, MO_LDW, rzreg, 0x8000));
                            mf->append (new Arith3Mach (__LINE__, primtoken, MO_XOR, roreg, rareg, rzreg));
                            ovar->storeFromRn (mf, primtoken, roreg, 2);
                            Reg *rbreg = new Reg (mf, RP_R05);
                            aval->loadIntoRn  (mf, primtoken, rbreg, 0);
                            ovar->storeFromRn (mf, primtoken, rbreg, 0);
#else
                            LabelMach *retlbl = new LabelMach (__LINE__, primtoken);
                            Reg *r0reg = new Reg (mf, RP_R0);
                            Reg *r1reg = new Reg (mf, RP_R1);
                            Reg *r3reg = new Reg (mf, RP_R3);
                            ovar->ldaIntoRn (mf, primtoken, r0reg, 0);
                            aval->ldaIntoRn (mf, primtoken, r1reg, 0);
                            mf->append (new LdImmLblMach (__LINE__, primtoken, r3reg, retlbl));
                            CallImmMach *callmach = new CallImmMach (__LINE__, primtoken, "__neg_ff");
                            mf->append (callmach);
                            callmach->regreads.push_back (r0reg);
                            callmach->regreads.push_back (r1reg);
                            callmach->regreads.push_back (r3reg);
                            mf->append (retlbl);
#endif
                            break;
                        }

                        case 8: {
#if 0
                            Reg *rareg = new Reg (mf, RP_R05);
                            Reg *rzreg = new Reg (mf, RP_R05);
                            Reg *roreg = new Reg (mf, RP_R05);
                            aval->loadIntoRn  (mf, primtoken, rareg, 6);
                            mf->append (new LdImmValMach (__LINE__, primtoken, MO_LDW, rzreg, 0x8000));
                            mf->append (new Arith3Mach (__LINE__, primtoken, MO_XOR, roreg, rareg, rzreg));
                            ovar->storeFromRn (mf, primtoken, roreg, 6);
                            Reg *rbreg = new Reg (mf, RP_R05);
                            aval->loadIntoRn  (mf, primtoken, rbreg, 4);
                            ovar->storeFromRn (mf, primtoken, rbreg, 4);
                            Reg *rcreg = new Reg (mf, RP_R05);
                            aval->loadIntoRn  (mf, primtoken, rcreg, 2);
                            ovar->storeFromRn (mf, primtoken, rcreg, 2);
                            Reg *rdreg = new Reg (mf, RP_R05);
                            aval->loadIntoRn  (mf, primtoken, rdreg, 0);
                            ovar->storeFromRn (mf, primtoken, rdreg, 0);
#else
                            LabelMach *retlbl = new LabelMach (__LINE__, primtoken);
                            Reg *r0reg = new Reg (mf, RP_R0);
                            Reg *r1reg = new Reg (mf, RP_R1);
                            Reg *r3reg = new Reg (mf, RP_R3);
                            ovar->ldaIntoRn (mf, primtoken, r0reg, 0);
                            aval->ldaIntoRn (mf, primtoken, r1reg, 0);
                            mf->append (new LdImmLblMach (__LINE__, primtoken, r3reg, retlbl));
                            CallImmMach *callmach = new CallImmMach (__LINE__, primtoken, "__neg_dd");
                            mf->append (callmach);
                            callmach->regreads.push_back (r0reg);
                            callmach->regreads.push_back (r1reg);
                            callmach->regreads.push_back (r3reg);
                            mf->append (retlbl);
#endif
                            break;
                        }

                        default: assert_false ();
                    }
                    break;
                }

                assert_false ();
            }
            break;
        }

        default: assert_false ();
    }

    assert ((flonexts.size () == 1) && (flonexts[0] == linext));
}

// generate call to internal __memcpy(roreg,rareg,size)
//  input:
//   roreg = points to output
//   rareg = points to input
//   alin  = address alignment of roreg,rareg
//   size  = number of bytes to copy (multiple of alin)
void Prim::gencallmemcpy (MachFile *mf, Reg *roreg, Reg *rareg, tsize_t alin, tsize_t size)
{
    assert ((alin & - alin) == alin);           // alignment must be power-of-two
    assert ((size & (alin - 1)) == 0);          // size must be aligned

    assert (roreg->regplace == RP_R0);          // output address must be in R0
    assert (rareg->regplace == RP_R1);          // input address must be in R1

    Mach *callmach;
    if ((alin > 1) && (size == 8)) {
        LabelMach *retlbl = new LabelMach (__LINE__, primtoken);
        Reg *r3reg = new Reg (mf, RP_R3);
        mf->append (new LdImmLblMach (__LINE__, primtoken, r3reg, retlbl));
        callmach = mf->append (new CallImmMach (__LINE__, primtoken, "__memcpy_4w"));
        callmach->regreads.push_back (roreg);
        callmach->regreads.push_back (rareg);
        callmach->regreads.push_back (r3reg);
        mf->append (retlbl);
        return;
    }

    Reg *rsreg = new Reg (mf, RP_R2);           // size goes in R2

    LabelMach *retlbl = new LabelMach (__LINE__, primtoken);
    Reg *r3reg = new Reg (mf, RP_R3);
    if (alin > 2) {
        mf->append (new LdImmValMach (__LINE__, primtoken, MO_LDW, rsreg, size / 4));
        mf->append (new LdImmLblMach (__LINE__, primtoken, r3reg, retlbl));
        callmach = mf->append (new CallImmMach (__LINE__, primtoken, "__memcpy_ww"));
    } else if (alin > 1) {
        mf->append (new LdImmValMach (__LINE__, primtoken, MO_LDW, rsreg, size / 2));
        mf->append (new LdImmLblMach (__LINE__, primtoken, r3reg, retlbl));
        callmach = mf->append (new CallImmMach (__LINE__, primtoken, "__memcpy_w"));
    } else {
        mf->append (new LdImmValMach (__LINE__, primtoken, MO_LDW, rsreg, size));
        mf->append (new LdImmLblMach (__LINE__, primtoken, r3reg, retlbl));
        callmach = mf->append (new CallImmMach (__LINE__, primtoken, "__memcpy"));
    }

    callmach->regreads.push_back (roreg);
    callmach->regreads.push_back (rareg);
    callmach->regreads.push_back (rsreg);
    callmach->regreads.push_back (r3reg);

    mf->append (retlbl);
}

// load byte/word in register Rd from memory location off(Ra)
// check type valtype supports the load
void Prim::ldbwRdAtRa (MachFile *mf, Type *valtype, Reg *rd, int off, Reg *ra)
{
    valtype = valtype->stripCVMod ();
    if ((valtype->castPtrType () == nullptr) && (valtype->castIntegType () == nullptr)) {
        throwerror (primtoken, "can load register only from integers and pointers not '%s'", valtype->getName ());
    }
    switch (valtype->getTypeSize (primtoken)) {
        case 1: {
            IntegType *itype = valtype->castIntegType ();
            mf->append (new LoadMach (__LINE__, primtoken, itype->getSign () ? MO_LDBS : MO_LDBU, rd, off, ra));
            break;
        }
        case 2: mf->append (new LoadMach (__LINE__, primtoken, MO_LDW, rd, off, ra)); break;
        default: throwerror (primtoken, "unsupported integer size '%s'", valtype->getName ());
    }
}

// store byte/word in register Rd to memory location off(Ra)
// check type valtype supports the store
void Prim::stbwRdAtRa (MachFile *mf, Type *valtype, Reg *rd, int off, Reg *ra)
{
    if (valtype->isConstType ()) {
        throwerror (primtoken, "cannot store register to const type '%s'", valtype->getName ());
    }
    valtype = valtype->stripCVMod ();
    if ((valtype->castPtrType () == nullptr) && (valtype->castIntegType () == nullptr)) {
        throwerror (primtoken, "can store register only into integers and pointers not '%s'", valtype->getName ());
    }
    switch (valtype->getTypeSize (primtoken)) {
        case 1: bigStoreMach (mf, __LINE__, primtoken, MO_STB, rd, off, ra); break;
        case 2: bigStoreMach (mf, __LINE__, primtoken, MO_STW, rd, off, ra); break;
        default: throwerror (primtoken, "unsupported integer size '%s'", valtype->getName ());
    }
}

// get address of this function into a register
//  void *p;
//  p = somefunction;
Mach *FuncDecl::loadIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs)
{
    assert (raoffs == 0);  // addresses fit in one word
    char *buf = isAsmConstExpr ();
    return mf->append (new LdImmStrMach (__LINE__, errtok, MO_LDW, rn, buf, this));
}

// call to the assembly-language label
//  somefunction ();
CallMach *FuncDecl::callTo (MachFile *mf, Token *errtok)
{
    char *buf = isAsmConstExpr ();
    CallMach *cm = new CallImmMach (__LINE__, errtok, buf);
    mf->append (cm);
    return cm;
}

// load address of this numeric constant into a register
Mach *NumDecl::ldaIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs)
{
    if (asmlabel[0] == 0) {
        static tsize_t numfake = 100;

        NumType *type = getType ()->stripCVMod ()->castNumType ();

        sprintf (asmlabel, "num.%u", ++ numfake);
        fprintf (sfile, "; --------------\n");
        fprintf (sfile, "\t.psect\t%s\n", TEXTPSECT);
        fprintf (sfile, "\t.align\t%u\n", type->getTypeAlign (errtok));
        fprintf (sfile, "%s:\n", asmlabel);

        switch (type->sizechar) {
            case 'd': {
                union { double f; uint64_t u; } v;
                v.f = numval.f;
                fprintf (sfile, "\t.long\t0x%08X,0x%08X ; .double %g\n", (uint32_t) v.u, (uint32_t) (v.u >> 32), numval.f);
                break;
            }
            case 'f': {
                union { float f; uint32_t u; } v;
                v.f = numval.f;
                fprintf (sfile, "\t.long\t0x%08X ; .float %g\n", v.u, v.f);
                break;
            }
            case 'b': fprintf (sfile, "\t.byte\t%lld\n", (long long) numval.s); break;
            case 'w': fprintf (sfile, "\t.word\t%lld\n", (long long) numval.s); break;
            case 'l': fprintf (sfile, "\t.long\t%lld\n", (long long) numval.s); break;
            case 'q': fprintf (sfile, "\t.long\t0x%llX,0x%llX\n", (unsigned long long) numval.s & 0xFFFFFFFFU, (unsigned long long) (numval.s >> 32)); break;
            case 'B': fprintf (sfile, "\t.byte\t%llu\n", (unsigned long long) numval.u); break;
            case 'W': fprintf (sfile, "\t.word\t%llu\n", (unsigned long long) numval.u); break;
            case 'L': fprintf (sfile, "\t.long\t%llu\n", (unsigned long long) numval.u); break;
            case 'Q': fprintf (sfile, "\t.long\t0x%llX,0x%llX\n", (unsigned long long) numval.u & 0xFFFFFFFFU, (unsigned long long) (numval.u >> 32)); break;
            default: assert_false ();
        }
    }

    char const *str = asmlabel;
    if (raoffs != 0) {
        char *buf;
        asprintf (&buf, "%s+%u", asmlabel, raoffs);
        str = buf;
    }

    return mf->append (new LdImmStrMach (__LINE__, errtok, MO_LDW, rn, str, this));
}

// load this numeric constant into a register
//  int i;
//  i = 47;
Mach *NumDecl::loadIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs)
{
    PtrType *ptrtype;
    NumType *numtype = getType ()->stripCVMod ()->castNumType ();
    if (numtype != nullptr) {
        MachOp ldop = MO_LDW;
        uint16_t v;
        switch (numtype->sizechar) {
            case 'f': { union { float  f; uint32_t i; } u; u.f = (float) numval.f; v = u.i >> (raoffs * 8); break; }
            case 'd': { union { double d; uint64_t i; } u; u.d =         numval.f; v = u.i >> (raoffs * 8); break; }
            case 'b': { v = (int16_t)  (int8_t)  numval.s; ldop = MO_LDBS; break; }
            case 'B': { v = (uint16_t) (uint8_t) numval.u; ldop = MO_LDBU; break; }
            case 'w': { v = numval.s; break; }
            case 'W': { v = numval.u; break; }
            case 'l': { v = numval.s >> (raoffs * 8); break; }
            case 'L': { v = numval.u >> (raoffs * 8); break; }
            case 'q': { v = numval.s >> (raoffs * 8); break; }
            case 'Q': { v = numval.u >> (raoffs * 8); break; }
            default: assert_false ();
        }

        return mf->append (new LdImmValMach (__LINE__, errtok, ldop, rn, v));
    }

    // *(uint16_t *)0xFFF2U like in getting errno
    ptrtype = getType ()->stripCVMod ()->castPtrType ();
    if (ptrtype != nullptr) {
        assert (! sizetype->getSign ());
        return mf->append (new LdImmValMach (__LINE__, errtok, MO_LDW, rn, numval.u));
    }

    throwerror (errtok, "type '%s' does not fit in an integer register", numtype->getName ());
}

// get address of this string into a register
//  char const *p;
//  p = "somestring";
Mach *StrDecl::loadIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs)
{
    assert (raoffs == 0);  // addresses fit in one word
    char *buf = isAsmConstExpr ();
    return mf->append (new LdImmStrMach (__LINE__, errtok, MO_LDW, rn, buf, this));
}

// call to the assembly-language label
//  somefunction ();
CallMach *AsmDecl::callTo (MachFile *mf, Token *errtok)
{
    char *buf = isAsmConstExpr ();
    CallMach *cm = new CallImmMach (__LINE__, errtok, buf);
    mf->append (cm);
    return cm;
}

// get assembly-language value loaded into register
Mach *AsmDecl::loadIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs)
{
    assert (raoffs == 0);  // assembly-language values fit in one word
    char *buf = isAsmConstExpr ();
    return mf->append (new LdImmStrMach (__LINE__, errtok, MO_LDW, rn, buf, this));
}

// get enum constant value loaded into register
Mach *EnumDecl::loadIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs)
{
    return mf->append (new LdImmValMach (__LINE__, errtok, MO_LDW, rn, value >> (raoffs * 8)));
}

// get address of this variable into a register
//  struct Abc s, *p;
//  p = &s;
Mach *VarDecl::ldaIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs)
{
    switch (locn->locntype) {
        case LT_LABEL: {
            char const *str = locn->castLabelLocn ()->label;
            if (raoffs != 0) {
                char *buf;
                asprintf (&buf, "%s+%u", str, raoffs);
                str = buf;
            }
            return mf->append (new LdImmStrMach (__LINE__, errtok, MO_LDW, rn, str, this));
        }
        case LT_CALAR:
        case LT_PARAM:
        case LT_RBRPT:
        case LT_STACK: {
            return bigLdAddrStkMach (mf, __LINE__, errtok, rn, this);
        }
        default: assert_false ();
    }
}

// load this variable contents into register
//  int i, j;
//  i = j;
// - mf = function the store instructions are for
// - errtok = source file location for error messages
// - raoffs = offset in variable to be loaded from
Mach *VarDecl::loadIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs)
{
    if (isArray ()) {
        return ldaIntoRn (mf, errtok, rn, raoffs);
    }

    MachOp ldop;
    switch (getType ()->getTypeSize (errtok)) {
        case 1: {
            IntegType *itype = getType ()->stripCVMod ()->castIntegType ();
            ldop = itype->getSign () ? MO_LDBS : MO_LDBU;
            break;
        }
        case 2:
        case 4:
        case 8: ldop = MO_LDW; break;
        default: assert_false ();
    }

    assert (locn != nullptr);

    switch (locn->locntype) {
        case LT_LABEL: {
            // code sequence used by MachFile::loadFromValue()
            Reg *rs = new Reg (mf, RP_R05);
            mf->append (new LdImmStrMach (__LINE__, errtok, MO_LDW, rs, locn->castLabelLocn ()->label, this));
            return mf->append (new LoadMach (__LINE__, errtok, ldop, rn, raoffs, rs));
        }
        case LT_CALAR:
        case LT_PARAM:
        case LT_RBRPT:
        case LT_STACK: {
            return bigLoadStkMach (mf, __LINE__, errtok, ldop, rn, this, raoffs);
        }
        default: assert_false ();
    }
}

// store register contents into this variable
// - mf = function the store instructions are for
// - errtok = source file location for error messages
// - raoffs = offset in variable to be stored into
Mach *VarDecl::storeFromRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs)
{
    Type *type = getType ();
    if (type->isConstType ()) {
        throwerror (errtok, "cannot write to const type '%s'", type->getName ());
    }

    assert (! isArray ());  // we checked for const type above

    MachOp stop;
    switch (type->getTypeSize (errtok)) {
        case 1: stop = MO_STB; break;
        case 2:
        case 4:
        case 8: stop = MO_STW; break;
        default: assert_false ();
    }

    switch (locn->locntype) {
        case LT_LABEL: {
            Reg *rs = new Reg (mf, RP_R05);
            mf->append (new LdImmStrMach (__LINE__, errtok, MO_LDW, rs, locn->castLabelLocn ()->label, this));
            return bigStoreMach (mf, __LINE__, errtok, stop, rn, raoffs, rs);
        }
        case LT_CALAR:
        case LT_PARAM:
        case LT_RBRPT:
        case LT_STACK: {
            return bigStoreStkMach (mf, __LINE__, errtok, stop, rn, this, raoffs);
        }
        default: assert_false ();
    }
}

CallMach *PtrDecl::callTo (MachFile *mf, Token *errtok)
{
    Reg *rp = new Reg (mf, RP_R05);
    subvaldecl->loadIntoRn (mf, errtok, rp, 0);
    CallMach *cm = new CallPtrMach (__LINE__, errtok, 0, rp);
    mf->append (cm);
    return cm;
}

Mach *PtrDecl::ldaIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs)
{
    Mach *m = subvaldecl->loadIntoRn (mf, errtok, rn, 0);
    if (raoffs == 0) return m;
    return mf->append (new LdAddrMach (__LINE__, errtok, rn, raoffs, rn));
}

Mach *PtrDecl::loadIntoRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs)
{
    Reg *rp = new Reg (mf, RP_R05);
    subvaldecl->loadIntoRn (mf, errtok, rp, 0);
    MachOp mop = MO_LDW;
    if (getType ()->getTypeSize (errtok) == 1) {
        mop = getType ()->stripCVMod ()->castIntegType ()->getSign () ? MO_LDBS : MO_LDBU;
    }
    return mf->append (new LoadMach (__LINE__, errtok, mop, rn, raoffs, rp));
}

Mach *PtrDecl::storeFromRn (MachFile *mf, Token *errtok, Reg *rn, tsize_t raoffs)
{
    Reg *rp = new Reg (mf, RP_R05);
    subvaldecl->loadIntoRn (mf, errtok, rp, 0);
    MachOp mop = MO_STW;
    if (getType ()->getTypeSize (errtok) == 1) {
        mop = MO_STB;
    }
    return mf->append (new StoreMach (__LINE__, errtok, mop, rn, raoffs, rp));
}

CallMach *ValDecl::callTo (MachFile *mf, Token *errtok)
{
    Reg *rp = new Reg (mf, RP_R05);
    this->ldaIntoRn (mf, errtok, rp, 0);
    CallMach *cm = new CallPtrMach (__LINE__, errtok, 0, rp);
    mf->append (cm);
    return cm;
}

// output lda/load/store instructions with potentially big offsets

//  LDA rd,offs(ra)
// or
//  LDW rtemp,#offs
//  ADD rd,ra,rtemp
static Mach *bigLdAddrMach (MachFile *mf, int line, Token *token, Reg *rd, int offs, Reg *ra)
{
    if ((offs >= LSOFFMIN) && (offs <= LSOFFMAX)) {
        return mf->append (new LdAddrMach (line, token, rd, offs, ra));
    }
    Reg *rtemp = new Reg (mf, RP_R05);
    mf->append (new LdImmValMach (line, token, MO_LDW, rtemp, offs));
    return mf->append (new Arith3Mach (line, token, MO_ADD, rd, ra, rtemp));
}

//  LDA rd,stkoffset(R6)    ...but we don't know offset yet
// so
//  LDW rtemp,#stkoffset
//  ADD rd,R6,rtemp
static Mach *bigLdAddrStkMach (MachFile *mf, int line, Token *token, Reg *rd, VarDecl *stk)
{
    Reg *rtemp = new Reg (mf, RP_R05);
    mf->append (new LdImmStkMach (line, token, rtemp, stk, mf->curspoffs, 0));
    return mf->append (new Arith3Mach (line, token, MO_ADD, rd, mf->spreg, rtemp));
}

/*
//  LD? rd,offs(ra)
// or
//  LDW rtemp,#offs
//  ADD rtemp,ra,rtemp
//  LD? rd,0(rtemp)
static Mach *bigLoadMach (MachFile *mf, int line, Token *token, MachOp mop, Reg *rd, int offs, Reg *ra)
{
    if ((offs >= LSOFFMIN) && (offs <= LSOFFMAX)) {
        return mf->append (new LoadMach (line, token, mop, rd, offs, ra));
    }
    Reg *rtemp1 = new Reg (mf, RP_R05);
    Reg *rtemp2 = new Reg (mf, RP_R05);
    mf->append (new LdImmValMach (line, token, MO_LDW, rtemp1, offs));
    mf->append (new Arith3Mach (line, token, MO_ADD, rtemp2, ra, rtemp1));
    return mf->append (new LoadMach (line, token, mop, rd, 0, rtemp2));
}
*/

//  LD? rd,stkoffset(R6)    ...but we don't know offset yet
// so
//  LDW rtemp,#stkoffset
//  ADD rtemp,R6,rtemp
//  LD? rd,0(rtemp)
static Mach *bigLoadStkMach (MachFile *mf, int line, Token *token, MachOp mop, Reg *rd, VarDecl *stk, tsize_t raoffs)
{
    Reg *rtemp1 = new Reg (mf, RP_R05);
    Reg *rtemp2 = new Reg (mf, RP_R05);
    mf->append (new LdImmStkMach (line, token, rtemp1, stk, mf->curspoffs, raoffs));
    mf->append (new Arith3Mach (line, token, MO_ADD, rtemp2, mf->spreg, rtemp1));
    return mf->append (new LoadMach (line, token, mop, rd, 0, rtemp2));
}

//  ST? rd,offs(ra)
// or
//  LDW rtemp,#offs
//  ADD rtemp,ra,rtemp
//  ST? rd,0(rtemp)
static Mach *bigStoreMach (MachFile *mf, int line, Token *token, MachOp mop, Reg *rd, int offs, Reg *ra)
{
    if ((offs >= LSOFFMIN) && (offs <= LSOFFMAX)) {
        return mf->append (new StoreMach (line, token, mop, rd, offs, ra));
    }
    Reg *rtemp1 = new Reg (mf, RP_R05);
    Reg *rtemp2 = new Reg (mf, RP_R05);
    mf->append (new LdImmValMach (line, token, MO_LDW, rtemp1, offs));
    mf->append (new Arith3Mach (line, token, MO_ADD, rtemp2, ra, rtemp1));
    return mf->append (new StoreMach (line, token, mop, rd, 0, rtemp2));
}

//  ST? rd,stkoffset(R6)    ...but we don't know offset yet
// so
//  LDW rtemp,#stkoffset
//  ADD rtemp,R6,rtemp
//  ST? rd,0(rtemp)
static Mach *bigStoreStkMach (MachFile *mf, int line, Token *token, MachOp mop, Reg *rd, VarDecl *stk, tsize_t raoffs)
{
    Reg *rtemp1 = new Reg (mf, RP_R05);
    Reg *rtemp2 = new Reg (mf, RP_R05);
    mf->append (new LdImmStkMach (line, token, rtemp1, stk, mf->curspoffs, raoffs));
    mf->append (new Arith3Mach (line, token, MO_ADD, rtemp2, mf->spreg, rtemp1));
    return mf->append (new StoreMach (line, token, mop, rd, 0, rtemp2));
}
