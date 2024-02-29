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

#include <stdio.h>
#include <string.h>

#include "mach.h"

static int fakelast = 100;

static char const *mopstring (MachOp mop)
{
    switch (mop) {
        case MO_ADC:    return "adc";
        case MO_ADD:    return "add";
        case MO_AND:    return "and";
        case MO_ASR:    return "asr";
        case MO_BEQ:    return "beq";
        case MO_BGE:    return "bge";
        case MO_BGT:    return "bgt";
        case MO_BHI:    return "bhi";
        case MO_BHIS:   return "bhis";
        case MO_BLO:    return "blo";
        case MO_BLOS:   return "blos";
        case MO_BLE:    return "ble";
        case MO_BLT:    return "blt";
        case MO_BMI:    return "bmi";
        case MO_BNE:    return "bne";
        case MO_BPL:    return "bpl";
        case MO_BR:     return "br";
        case MO_BVC:    return "bvc";
        case MO_BVS:    return "bvs";
        case MO_CLR:    return "clr";
        case MO_COM:    return "com";
        case MO_INC:    return "inc";
        case MO_LDA:    return "lda";
        case MO_LDBS:   return "ldbs";
        case MO_LDBU:   return "ldbu";
        case MO_LDW:    return "ldw";
        case MO_LSR:    return "lsr";
        case MO_MOV:    return "mov";
        case MO_NEG:    return "neg";
        case MO_OR:     return "or";
        case MO_ROL:    return "rol";
        case MO_ROR:    return "ror";
        case MO_SBB:    return "sbb";
        case MO_SHL:    return "shl";
        case MO_STB:    return "stb";
        case MO_STW:    return "stw";
        case MO_SUB:    return "sub";
        case MO_XOR:    return "xor";
        default: assert_false ();
    }
}

static MachOp flipbr (MachOp mop)
{
    switch (mop) {
        case MO_BEQ:    return MO_BNE;
        case MO_BGE:    return MO_BLT;
        case MO_BGT:    return MO_BLE;
        case MO_BHI:    return MO_BLOS;
        case MO_BHIS:   return MO_BLO;
        case MO_BLO:    return MO_BHIS;
        case MO_BLOS:   return MO_BHI;
        case MO_BLE:    return MO_BGT;
        case MO_BLT:    return MO_BGE;
        case MO_BMI:    return MO_BPL;
        case MO_BNE:    return MO_BEQ;
        case MO_BPL:    return MO_BMI;
        case MO_BVC:    return MO_BVS;
        case MO_BVS:    return MO_BVC;
        default: assert_false ();
    }
}

static char const *regstring (RegPlace rp)
{
    switch (rp) {
        case RP_R0: return "%r0";
        case RP_R1: return "%r1";
        case RP_R2: return "%r2";
        case RP_R3: return "%r3";
        case RP_R4: return "%r4";
        case RP_R5: return "%r5";
        case RP_R6: return "%r6";
        case RP_R7: return "%r7";
        default: assert_false ();
    }
}

//////////////////////////////////////
//  whole function of machine code  //
//////////////////////////////////////

MachFile::MachFile (FuncDecl *funcdecl)
{
    this->funcdecl = funcdecl;
    firstmach = nullptr;
    lastmach  = nullptr;
    spreg = new Reg (this, RP_R6);
    pcreg = new Reg (this, RP_R7);
}

// append machine op onto end of function
Mach *MachFile::append (Mach *mach)
{
    assert ((mach->machfile == nullptr) && (mach->nextmach == mach) && (mach->prevmach == mach));

    int oldcount = verifylist ();

    mach->machfile = this;
    mach->nextmach = nullptr;
    mach->prevmach = lastmach;

    if (lastmach == nullptr) {
        assert (firstmach == nullptr);
        firstmach = mach;
    } else {
        lastmach->nextmach = mach;
    }

    lastmach = mach;

    int newcount = verifylist ();
    assert (newcount == oldcount + 1);

    return mach;
}

// insert machine op after given machine op
//  input:
//   mach = new machine op being inserted
//   mark = old machine op being inserted after
void MachFile::insafter (Mach *mach, Mach *mark)
{
    assert ((mach->machfile == nullptr) && (mach->nextmach == mach) && (mach->prevmach == mach));
    assert ((mark->machfile == this)    && (mark->nextmach != mark) && (mark->prevmach != mark));

    int oldcount = verifylist ();

    mach->machfile = this;
    mach->nextmach = mark->nextmach;
    mach->prevmach = mark;

    if (mach->nextmach == nullptr) {
        this->lastmach = mach;
    } else {
        mach->nextmach->prevmach = mach;
    }

    mark->nextmach = mach;

    int newcount = verifylist ();
    assert (newcount == oldcount + 1);
}

// insert machine op before given machine op
void MachFile::insbefore (Mach *mach, Mach *mark)
{
    assert ((mach->machfile == nullptr) && (mach->nextmach == mach) && (mach->prevmach == mach));
    assert ((mark->machfile == this)    && (mark->nextmach != mark) && (mark->prevmach != mark));

    int oldcount = verifylist ();

    mach->machfile = this;
    mach->nextmach = mark;
    mach->prevmach = mark->prevmach;

    if (mach->prevmach == nullptr) {
        this->firstmach = mach;
    } else {
        mach->prevmach->nextmach = mach;
    }

    mark->prevmach = mach;

    int newcount = verifylist ();
    assert (newcount == oldcount + 1);
}

// replace the given instruction in the function
Mach *MachFile::replace (Mach *oldmach, Mach *newmach)
{
    assert ((oldmach->machfile == this)    && (oldmach->nextmach != oldmach) && (oldmach->prevmach != oldmach));
    assert ((newmach->machfile == nullptr) && (newmach->nextmach == newmach) && (newmach->prevmach == newmach));

    int oldcount = verifylist ();

    newmach->machfile = this;
    newmach->nextmach = oldmach->nextmach;
    newmach->prevmach = oldmach->prevmach;

    if (newmach->nextmach == nullptr) {
        this->lastmach = newmach;
    } else {
        newmach->nextmach->prevmach = newmach;
    }

    if (newmach->prevmach == nullptr) {
        this->firstmach = newmach;
    } else {
        newmach->prevmach->nextmach = newmach;
    }

    oldmach->machfile = nullptr;
    oldmach->nextmach = oldmach;
    oldmach->prevmach = oldmach;

    int newcount = verifylist ();
    assert (newcount == oldcount);

    return newmach;
}

// remove the given instruction from the function
void MachFile::remove (Mach *mach)
{
    assert ((mach->machfile == this) && (mach->nextmach != mach) && (mach->prevmach != mach));

    int oldcount = verifylist ();

    if (mach->prevmach == nullptr) {
        firstmach = mach->nextmach;
    } else {
        mach->prevmach->nextmach = mach->nextmach;
    }
    if (mach->nextmach == nullptr) {
        lastmach = mach->prevmach;
    } else {
        mach->nextmach->prevmach = mach->prevmach;
    }

    mach->machfile = nullptr;
    mach->nextmach = mach;
    mach->prevmach = mach;

    int newcount = verifylist ();
    assert (newcount == oldcount - 1);
}

// replace all references to the old register to the new register in all instructions of the function
void MachFile::replacereg (Reg *oldreg, Reg *newreg)
{
    Mach *pm = nullptr;
    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {
        assert (mach->prevmach == pm);
        for (int i = mach->regreads.size (); -- i >= 0;) {
            if (mach->regreads[i] == oldreg) {
                mach->regreads[i] = newreg;
            }
        }
        for (int i = mach->regwrites.size (); -- i >= 0;) {
            if (mach->regwrites[i] == oldreg) {
                mach->regwrites[i] = newreg;
            }
        }
        pm = mach;
    }
    assert (lastmach == pm);
}

int MachFile::verifylist ()
{
    int n = 0;
    Mach *last = nullptr;
    for (Mach *mach = firstmach; mach != nullptr; mach = mach->nextmach) {
        assert (mach->machfile == this);
        assert (mach->prevmach == last);
        last = mach;
        n ++;
    }
    assert (lastmach == last);
    return n;
}



////////////////////////////////////////
//  base of all machine instructions  //
////////////////////////////////////////

Mach::Mach (int line, Token *token)
{
    this->line     = line;
    this->token    = token;
    this->machfile = nullptr;
    this->nextmach = this;
    this->prevmach = this;
}

bool Mach::readsReg (Reg *r)
{
    for (int i = regreads.size (); -- i >= 0;) {
        if (regreads[i]->regeq (r)) return true;
    }
    return false;
}

bool Mach::writesReg (Reg *r)
{
    for (int i = regwrites.size (); -- i >= 0;) {
        if (regwrites[i]->regeq (r)) return true;
    }
    return false;
}

void Mach::checkRegState ()
{
    assert (machfile->spreg->regplace == RP_R6);
    assert (machfile->spreg->regcont == RC_CONSTSP);
    assert (machfile->spreg->regvalue == 0);
    for (Reg *r : regreads) {
        assert ((r->regplace != RP_R6) || (r == machfile->spreg));
    }
    for (Reg *r : regwrites) {
        assert ((r->regplace != RP_R6) || (r == machfile->spreg));
    }
}

///////////////////////////////
//  arithmetic instructions  //
///////////////////////////////

Arith1Mach::Arith1Mach (int line, Token *token, MachOp mop, Reg *rd)
    : Mach (line, token)
{
    this->mop = mop;

    assert (mop == MO_CLR);
    assert (rd->regplace != RP_R7);
    this->regwrites.push_back (rd);
}

Reg *Arith1Mach::getRd ()
{
    return regwrites[0];
}

void Arith1Mach::stepRegState ()
{
    checkRegState ();

    Reg *rd = regwrites[0];

    assert (mop == MO_CLR);             // all we do is clear registers
    assert (rd->regplace != RP_R6);     // should never try to clear stack pointer

    rd->regcont = RC_CONST;
    rd->regvalue = 0;

    checkRegState ();
}

tsize_t Arith1Mach::getisize ()
{
    return 2;
}

void Arith1Mach::dumpmach (FILE *f)
{
    fprintf (f, "\t%s\t%s\n", mopstring (mop), getRd ()->regstr ());
}

Arith2Mach::Arith2Mach (int line, Token *token, MachOp mop, Reg *rd, Reg *ra)
    : Mach (line, token)
{
    this->mop = mop;

    assert ((mop == MO_COM) || (mop == MO_NEG) || (mop == MO_MOV) || (mop == MO_INC) || (mop == MO_LSR) || (mop == MO_ASR) || (mop == MO_ROR) || (mop == MO_SHL) || (mop == MO_ROL));
    assert (rd->regplace != RP_R7);
    this->regwrites.push_back (rd);
    this->regreads.push_back (ra);
}

Reg *Arith2Mach::getRa ()
{
    return regreads[0];
}

Reg *Arith2Mach::getRd ()
{
    return regwrites[0];
}

void Arith2Mach::stepRegState ()
{
    checkRegState ();

    Reg *ra = regreads[0];
    Reg *rd = regwrites[0];

    assert (rd->regplace != RP_R6);

    rd->regcont = ra->regcont;
    switch (ra->regcont) {
        case RC_OTHER: break;

        case RC_CONST: {
            switch (mop) {
                case MO_COM: rd->regvalue = ~ ra->regvalue; break;
                case MO_NEG: rd->regvalue = - ra->regvalue; break;
                case MO_MOV: rd->regvalue =   ra->regvalue; break;
                case MO_INC: rd->regvalue = ra->regvalue +  1; break;
                case MO_LSR: rd->regvalue = ra->regvalue >> 1; break;
                case MO_ASR: rd->regvalue = (uint16_t) (((int16_t) ra->regvalue) >> 1); break;
                case MO_SHL: rd->regvalue = ra->regvalue << 1; break;
                default:     rd->regcont  = RC_OTHER; break;
            }
            break;
        }

        case RC_CONSTSP: {
            switch (mop) {
                case MO_MOV: rd->regvalue = ra->regvalue;     break;
                case MO_INC: rd->regvalue = ra->regvalue + 1; break;
                default:     rd->regcont  = RC_OTHER; break;
            }
            break;
        }

        case RC_SEXTBYTE: {
            switch (mop) {
                case MO_COM:
                case MO_MOV:
                case MO_ASR: break;
                default:     rd->regcont = RC_OTHER; break;
            }
            break;
        }

        case RC_ZEXTBYTE: {
            switch (mop) {
                case MO_MOV:
                case MO_LSR: break;
                default:     rd->regcont = RC_OTHER; break;
            }
            break;
        }

        default: assert_false ();
    }

    checkRegState ();
}

tsize_t Arith2Mach::getisize ()
{
    return 2;
}

void Arith2Mach::dumpmach (FILE *f)
{
    fprintf (f, "\t%s\t%s,%s\n", mopstring (mop), getRd ()->regstr (), getRa ()->regstr ());
}

Arith3Mach::Arith3Mach (int line, Token *token, MachOp mop, Reg *rd, Reg *ra, Reg *rb)
    : Mach (line, token)
{
    this->mop = mop;

    assert ((mop == MO_ADD) || (mop == MO_SUB) || (mop == MO_ADC) || (mop == MO_SBB) || (mop == MO_AND) || (mop == MO_OR) || (mop == MO_XOR));
    assert (rd->regplace != RP_R7);
    this->regwrites.push_back (rd);
    this->regreads.push_back (ra);
    this->regreads.push_back (rb);
}

Reg *Arith3Mach::getRd ()
{
    return regwrites[0];
}

Reg *Arith3Mach::getRa ()
{
    return regreads[0];
}

Reg *Arith3Mach::getRb ()
{
    return regreads[1];
}

void Arith3Mach::stepRegState ()
{
    checkRegState ();

    Reg *ra = getRa ();
    Reg *rb = getRb ();
    Reg *rd = getRd ();

    // if both operands are constants, compute resultant constant
    // can't compute for ADC,SBB cuz we don't know carry bit
    if ((ra->regcont == RC_CONST) && (rb->regcont == RC_CONST)) {
        switch (mop) {
            case MO_ADD: rd->regcont = RC_CONST; rd->regvalue = ra->regvalue + rb->regvalue; break;
            case MO_SUB: rd->regcont = RC_CONST; rd->regvalue = ra->regvalue - rb->regvalue; break;
            case MO_AND: rd->regcont = RC_CONST; rd->regvalue = ra->regvalue & rb->regvalue; break;
            case MO_OR:  rd->regcont = RC_CONST; rd->regvalue = ra->regvalue | rb->regvalue; break;
            case MO_XOR: rd->regcont = RC_CONST; rd->regvalue = ra->regvalue ^ rb->regvalue; break;
            default:     rd->regcont = RC_OTHER; break;
        }
    }

    // if adding Rn + constant, where Rn is SP + constant, say output register is SP + constant
    //  ADD Rd,Ra=constsp,Rb=const
    else if ((mop == MO_ADD) && (ra->regcont == RC_CONSTSP) && (rb->regcont == RC_CONST)) {

        if (rd->regplace == RP_R6) {

            // there should be only one R6 register
            assert (rd == machfile->spreg);
            assert (rd->regcont == RC_CONSTSP);
            assert (rd->regvalue == 0);

            // ADD R6,Ra=constsp,Rb=const
            // stack pointer being incremented by stack offset in Ra + value in Rb
            uint16_t spinc = ra->regvalue + rb->regvalue;

            // subtract that increment from all registers that have stack offset
            for (Reg *r : machfile->allregs) {
                if (r->regcont == RC_CONSTSP) r->regvalue -= spinc;
            }

            // but make the stack pointer have offset 0, cuz that's its offset from itself
            rd->regcont  = RC_CONSTSP;
            rd->regvalue = 0;
        }

        // some register other than stack pointer being written, set its offset from stack pointer
        else {
            rd->regcont  = RC_CONSTSP;
            rd->regvalue = ra->regvalue + rb->regvalue;
        }
    }

    // anding with byte-sized constant gives zero-extended byte
    else if ((mop == MO_AND) && (((ra->regcont == RC_CONST) && (ra->regvalue <= 0xFF)) || ((rb->regcont == RC_CONST) && (rb->regvalue <= 0xFF)))) {
        rd->regcont = RC_ZEXTBYTE;
    }

    // and/or/xoring two same type byte sized values results in same type byte sized value
    else if (((mop == MO_AND) || (mop == MO_OR) || (mop == MO_XOR)) && ((ra->regcont == RC_SEXTBYTE) || (ra->regcont == RC_ZEXTBYTE)) && (ra->regcont == rb->regcont)) {
        rd->regcont = rb->regcont;
    }

    // don't know what we got, mark contents as OTHER
    else {
        assert (rd->regplace != RP_R6);
        rd->regcont = RC_OTHER;
    }

    checkRegState ();
}

tsize_t Arith3Mach::getisize ()
{
    return 2;
}

void Arith3Mach::dumpmach (FILE *f)
{
    fprintf (f, "\t%s\t%s,%s,%s\n", mopstring (mop), regwrites[0]->regstr (), regreads[0]->regstr (), regreads[1]->regstr ());
}

///////////////////////////////////
//  Abstract class for branches  //
///////////////////////////////////

BrMach::BrMach (int line, Token *token)
    : Mach (line, token)
{
    shortbr = false;
}

// mark label as referenced
void BrMach::markLabelsRefd ()
{
    lbl->labelmachrefd = true;
}

// see if the branch is out of range
bool BrMach::brokenPCRel ()
{
    // long branches cannot be out of range
    if (! shortbr) return false;

    // look for label within BROFFMAX in forward direction
    int16_t fwdoffs = 0;
    for (Mach *m = this; (m = m->nextmach) != nullptr;) {
        if (m == lbl) return false;
        fwdoffs += m->getisize ();
        if (fwdoffs > BROFFMAX) break;
    }

    // look for label within BROFFMIN in backward direction
    int16_t bwdoffs = 0;
    for (Mach *m = this; m != nullptr; m = m->prevmach) {
        bwdoffs -= m->getisize ();
        if (bwdoffs < BROFFMIN) break;
        if (m == lbl) return false;
    }

    // not found within range, so it's broken
    return true;
}

////////////////////////////////////////////
//  unconditional branch within function  //
////////////////////////////////////////////

BranchMach::BranchMach (int line, Token *token, LabelMach *lbl)
    : BrMach (line, token)
{
    this->lbl = lbl;
}

tsize_t BranchMach::getisize ()
{
    return shortbr ? 2 : 4;
}

void BranchMach::dumpmach (FILE *f)
{
    if (shortbr) {
        fprintf (f, "\tbr\t%s\n", lbl->lblstr ());
    } else {
        fprintf (f, "\tldw\t%%r7,#%s\n", lbl->lblstr ());
    }
}

///////////////////////
//  call a function  //
///////////////////////

CallMach::CallMach (int line, Token *token)
    : Mach (line, token)
{
    this->stkpop = 0;
}

void CallMach::stepRegState ()
{
    checkRegState ();

    // invalidate all registers except R4,R5,R6
    for (Reg *r : machfile->allregs) {
        if ((r->regplace != RP_R4) && (r->regplace != RP_R5) && (r->regplace != RP_R6)) r->regcont = RC_OTHER;
    }

    // stack pointer was incremented by stkpop by the function
    // update any stack-relative registers by that much
    for (Reg *r : machfile->allregs) {
        if ((r->regplace != RP_R6) && (r->regcont == RC_CONSTSP)) r->regvalue -= stkpop;
    }

    checkRegState ();
}

CallImmMach::CallImmMach (int line, Token *token, char const *ent)
    : CallMach (line, token)
{
    this->ent = ent;
}

tsize_t CallImmMach::getisize ()
{
    return 4;
}

void CallImmMach::dumpmach (FILE *f)
{
    fprintf (f, "\tldw\t%%r7,#%s\n", ent);
}

CallPtrMach::CallPtrMach (int line, Token *token, int16_t offs, Reg *ra)
    : CallMach (line, token)
{
    this->regreads.push_back (ra);
    this->offs = offs;
}

Reg *CallPtrMach::getRa ()
{
    return regreads[0];
}

tsize_t CallPtrMach::getisize ()
{
    return 2;
}

void CallPtrMach::dumpmach (FILE *f)
{
    fprintf (f, "\tldw\t%%r7,%d(%s)\n", offs, getRa ()->regstr ());
}

////////////////////////////////////////////////////
//  compare registers and branch within function  //
////////////////////////////////////////////////////

CmpBrMach::CmpBrMach (int line, Token *token, MachOp mop, Reg *ra, Reg *rb, LabelMach *lbl, LabelMach *brlbl)
    : BrMach (line, token)
{
    this->mop = mop;
    this->lbl = lbl;
    this->brlbl = brlbl;

    this->cmpfake = ++ fakelast;

    assert ((mop == MO_BEQ) || (mop == MO_BNE) || (mop == MO_BLT) || (mop == MO_BLE) || (mop == MO_BGT) || (mop == MO_BGE) || (mop == MO_BLO) || (mop == MO_BLOS) || (mop == MO_BHI) || (mop == MO_BHIS) || (mop == MO_BVC) || (mop == MO_BVS) || (mop == MO_BMI) || (mop == MO_BPL));
    assert (lbl != nullptr);

    this->regreads.push_back (ra);
    this->regreads.push_back (rb);
}

Reg *CmpBrMach::getRa ()
{
    return regreads[0];
}

tsize_t CmpBrMach::getisize ()
{
    tsize_t s = shortbr ? 4 : 8;
    if (brlbl != nullptr) s += brlbl->getisize ();
    return s;
}

void CmpBrMach::dumpmach (FILE *f)
{
    fprintf (f, "\tcmp\t%s,%s\n", regreads[0]->regstr (), regreads[1]->regstr ());
    if (brlbl != nullptr) brlbl->dumpmach (f);
    if (shortbr) {
        fprintf (f, "\t%s\t%s\n", mopstring (mop), lbl->lblstr ());
    } else {
        fprintf (f, "\t%s\tcmp.%d\n", mopstring (flipbr (mop)), cmpfake);
        fprintf (f, "\tldw\t%%r7,#%s\n", lbl->lblstr ());
        fprintf (f, "cmp.%d:\n", cmpfake);
    }
}

///////////////////////////
//  function entrypoint  //
///////////////////////////

EntMach::EntMach (int line, Token *token, FuncDecl *funcdecl, Reg *sp, Reg *ra, Reg *tp)
    : Mach (line, token)
{
    this->funcdecl = funcdecl;
    assert (sp->regplace == RP_R6);
    assert (ra->regplace == RP_R3);
    this->regwrites.push_back (sp);
    this->regwrites.push_back (ra);
    if (tp != nullptr) {
        assert (tp->regplace == RP_R2);
        this->regwrites.push_back (tp);
    }
}

void EntMach::stepRegState ()
{
    // invalidate all registers cuz none are filled in on entry to function
    for (Reg *r : machfile->allregs) {
        r->regcont  = RC_OTHER;
        r->regvalue = 0;
    }

    // stack pointer is always relative 0 offset to itself
    machfile->spreg->regcont  = RC_CONSTSP;
    machfile->spreg->regvalue = 0;

    checkRegState ();
}

tsize_t EntMach::getisize ()
{
    tsize_t isize = 0;

    if (funcdecl->getStorClass () == KW_VIRTUAL) {
        StructType *structtype = funcdecl->getEncType ();
        for (int i = structtype->exttypes.size (); -- i >= 0;) {
            StructType *exttype = structtype->exttypes[i];
            if (exttype->hasVirtFunc (funcdecl->getName ())) {
                int16_t extoffset = - structtype->getExtOffset (i);
                for (int j = i; -- j >= 0;) {
                    if (structtype->exttypes[j]->hasVirtFunc (funcdecl->getName ())) {
                        extoffset += structtype->getExtOffset (j);
                        break;
                    }
                }
                if (extoffset != 0) isize += (extoffset < LSOFFMIN) ? 6 : 2;
            }
        }
    }

    int offset = - funcdecl->stacksize;
    if (offset != 0) isize += (offset < LSOFFMIN) ? 6 : 2;
    return isize;
}

void EntMach::dumpmach (FILE *f)
{
    // for each time this function overrides a parent function,
    // output an entrypoint that casts its 'this' pointer from
    // the parent object to the child object
    if (funcdecl->getStorClass () == KW_VIRTUAL) {
        StructType *structtype = funcdecl->getEncType ();
        for (int i = structtype->exttypes.size (); -- i >= 0;) {
            StructType *exttype = structtype->exttypes[i];
            if (exttype->hasVirtFunc (funcdecl->getName ())) {
                fprintf (f, "\t.global\t%s::%s__%s\n", exttype->getName (), structtype->getName (), funcdecl->getName ());
            }
        }
    }

    fprintf (f, "\t.global\t%s\n", funcdecl->getEncName ());

    if (funcdecl->getStorClass () == KW_VIRTUAL) {
        StructType *structtype = funcdecl->getEncType ();
        for (int i = structtype->exttypes.size (); -- i >= 0;) {
            StructType *exttype = structtype->exttypes[i];
            if (exttype->hasVirtFunc (funcdecl->getName ())) {
                fprintf (f, "%s::%s__%s:\n", exttype->getName (), structtype->getName (), funcdecl->getName ());
                int16_t extoffset = - structtype->getExtOffset (i);
                for (int j = i; -- j >= 0;) {
                    if (structtype->exttypes[j]->hasVirtFunc (funcdecl->getName ())) {
                        extoffset += structtype->getExtOffset (j);
                        break;
                    }
                }
                if (extoffset < LSOFFMIN) {
                    fprintf (f, "\tldw\t%%r2,#%d\n", extoffset);
                    fprintf (f, "\tadd\t%%r2,%%r2,%%r0\n");
                } else if (extoffset < 0) {
                    fprintf (f, "\tlda\t%%r2,%d(%%r2)\n", extoffset);
                } else {
                    assert (extoffset == 0);
                }
            }
        }
    }

    // main function entrypoint
    fprintf (f, "%s:\n", funcdecl->getEncName ());

    // preamble code to make room on stack
    int offset = - funcdecl->stacksize;
    if (offset < LSOFFMIN) {
        fprintf (f, "\tldw\t%%r0,#%d\n", offset);
        fprintf (f, "\tadd\t%%r6,%%r6,%%r0\n");
    } else if (offset < 0) {
        fprintf (f, "\tlda\t%%r6,%d(%%r6)\n", offset);
    } else {
        assert (offset == 0);
    }
}

////////////////////////
//  branchable label  //
////////////////////////

LabelMach::LabelMach (int line, Token *token)
    : Mach (line, token)
{
    this->lfake = ++ fakelast;
    snprintf (lblstrbuf, sizeof lblstrbuf, "lbl.%d", lfake);
}

char const *LabelMach::lblstr ()
{
    return lblstrbuf;
}

void LabelMach::stepRegState ()
{
    checkRegState ();

    // invalidate all registers cuz they may be different depending on how we got here
    // except leave the stack pointer as RC_CONSTSP+0
    for (Reg *r : machfile->allregs) {
        if (r->regplace != RP_R6) r->regcont = RC_OTHER;
    }

    checkRegState ();
}

tsize_t LabelMach::getisize ()
{
    return 0;
}

void LabelMach::dumpmach (FILE *f)
{
    fprintf (f, "%s:\n", this->lblstr ());
}

//////////////////////////////////
//  load address into register  //
//////////////////////////////////

LdAddrMach::LdAddrMach (int line, Token *token, Reg *rd, int16_t offs, Reg *ra)
    : Mach (line, token)
{
    this->offs = offs;

    assert (rd->regplace != RP_R7);
    this->regwrites.push_back (rd);
    this->regreads.push_back (ra);
}

Reg *LdAddrMach::getRd ()
{
    return regwrites[0];
}

Reg *LdAddrMach::getRa ()
{
    return regreads[0];
}

bool LdAddrMach::getOffs (int16_t *offs_r)
{
    *offs_r = offs;
    return true;
}

void LdAddrMach::setOffsRa (int16_t offs, Reg *ra)
{
    this->offs = offs;
    regreads[0] = ra;
}

void LdAddrMach::stepRegState ()
{
    checkRegState ();

    Reg *rd = getRd ();
    Reg *ra = getRa ();

    // if doing something like
    //  LDA SP,offs(Ra=constsp)
    // leave SP as holding SP+0
    //  and modify all other RC_CONSTSP registers with the offset
    if (rd->regplace == RP_R6) {

        // there should be only one R6 register
        assert (rd == machfile->spreg);
        assert (rd->regcont == RC_CONSTSP);
        assert (rd->regvalue == 0);

        // only handle modifying SP relative to itself
        assert (ra->regcont == RC_CONSTSP);

        // compute how much stack pointer is being incremented by
        uint16_t inc = ra->regvalue + offs;

        // modify all other stack relative registers with increment
        for (Reg *r : machfile->allregs) {
            if ((r->regcont == RC_CONSTSP) && (r->regplace != RP_R6)) {
                r->regvalue -= inc;
            }
        }
    }

    // some other register, it is offset plus what was in input register
    else {
        switch (ra->regcont) {
            case RC_CONST:
            case RC_CONSTSP: {
                rd->regcont  = ra->regcont;
                rd->regvalue = ra->regvalue + offs;
                break;
            }
            default: {
                rd->regcont = RC_OTHER;
                break;
            }
        }
    }

    checkRegState ();
}

tsize_t LdAddrMach::getisize ()
{
    return 2;
}

void LdAddrMach::dumpmach (FILE *f)
{
    fprintf (f, "\tlda\t%s,%d(%s)\n", getRd ()->regstr (), offs, getRa ()->regstr ());
}

// load address of stack variable into a register
//  line   = line in assemble.cc
//  token  = source code token for error messages
//  rd     = register being loaded into
//  stk    = stack variable to take the address of
//  spoffs = temporary offset amount of stack pointer for such as call parameters
//  raoffs = offset within variable to get address of
LdAddrStkMach::LdAddrStkMach (int line, Token *token, Reg *rd, VarDecl *stk, tsize_t spoffs, tsize_t raoffs)
    : Mach (line, token)
{
    this->stk = stk;
    this->spoffs = spoffs;
    this->raoffs = raoffs;

    assert (rd->regplace != RP_R7);
    this->regwrites.push_back (rd);
}

Reg *LdAddrStkMach::getRd ()
{
    return regwrites[0];
}

int16_t LdAddrStkMach::getStkOffs ()
{
    int16_t stkoffs;
    bool ok = stk->getVarLocn ()->getStkOffs (&stkoffs, spoffs);
    assert (ok);
    return stkoffs + raoffs;
}

void LdAddrStkMach::stepRegState ()
{
    checkRegState ();

    Reg *rd = getRd ();

    // should never try to point stack pointer at a stack variable
    assert (rd->regplace != RP_R6);

    // address of the parameter/variable is an offset from stack pointer
    int16_t stkoffs;
    if (stk->getVarLocn ()->getStkOffs (&stkoffs, spoffs)) {
        rd->regcont  = RC_CONSTSP;
        rd->regvalue = stkoffs + raoffs;
    } else {
        rd->regcont  = RC_OTHER;
    }

    checkRegState ();
}

tsize_t LdAddrStkMach::getisize ()
{
    return 2;
}

void LdAddrStkMach::dumpmach (FILE *f)
{
    int16_t stkoffs;
    if (stk->getVarLocn ()->getStkOffs (&stkoffs, spoffs)) {
        int16_t offset = stkoffs + raoffs;
        fprintf (f, "\tlda\t%s,%d(%%r6) ; stk:%s\n", getRd ()->regstr (), offset, stk->getName ());
    } else {
        fprintf (f, "\tlda\t%s,stk:%s+%u\n", getRd ()->regstr (), stk->getName (), raoffs);
    }
}

////////////////////////////////////
//  load immediate into register  //
////////////////////////////////////

// load assembly language label into register
LdImmLblMach::LdImmLblMach (int line, Token *token, Reg *rd, LabelMach *lbl)
    : Mach (line, token)
{
    assert (lbl != nullptr);
    this->lbl = lbl;
    this->madeshort = false;

    assert (rd->regplace != RP_R7);
    this->regwrites.push_back (rd);
}

Reg *LdImmLblMach::getRd ()
{
    return regwrites[0];
}

void LdImmLblMach::stepRegState ()
{
    checkRegState ();

    Reg *rd = getRd ();
    assert (rd->regplace != RP_R6);
    rd->regcont = RC_OTHER;

    checkRegState ();
}

// allows optimizing:
//      LDW rd,#lbl
// to:
//  otherlbl: .word lbl
//       ...
//      LDW rd,otherlbl
bool LdImmLblMach::isLdImmAny (MachOp *mop_r, char const **lbl_r, uint16_t *val_r)
{
    // if we can do an LDA rd,lbl don't bother optimizing the other way
    if (testshort ()) return false;

    // can't do LDA rd,lbl, maybe optimize the other way
    *mop_r = MO_LDW;
    *lbl_r = lbl->lblstr ();
    *val_r = 0;
    return true;
}

bool LdImmLblMach::brokenPCRel ()
{
    // if haven't said we are short, then we aren't broken
    if (! madeshort) return false;

    // we have previously reported short, don't allow to go long
    return ! testshort ();
}

tsize_t LdImmLblMach::getisize ()
{
    return madeshort ? 2 : 4;
}

void LdImmLblMach::dumpmach (FILE *f)
{
    if (testshort ()) {
        fprintf (f, "\tlda\t%s,%s\n", getRd ()->regstr (), lbl->lblstr ());
    } else {
        fprintf (f, "\tldw\t%s,#%s\n", getRd ()->regstr (), lbl->lblstr ());
    }
}

// see if label is close enough to allow the LDA rd,lbl form
bool LdImmLblMach::testshort ()
{
    // look for label within LSOFFMAX in forward direction
    int16_t fwdoffs = 0;
    for (Mach *m = this; (m = m->nextmach) != nullptr;) {
        if (m == lbl) {
            madeshort = true;
            return true;
        }
        fwdoffs += m->getisize ();
        if (fwdoffs > LSOFFMAX) break;
    }

    // look for label within LSOFFMIN in backward direction
    int16_t bwdoffs = 0;
    for (Mach *m = this; m != nullptr; m = m->prevmach) {
        bwdoffs -= m->getisize ();
        if (bwdoffs < LSOFFMIN) break;
        if (m == lbl) {
            madeshort = true;
            return true;
        }
    }

    // not found within range, so long form needed
    return false;
}

// load offset of function parameter or variable/temporary on stack into register
LdImmStkMach::LdImmStkMach (int line, Token *token, Reg *rd, VarDecl *stk, tsize_t spoffs, tsize_t raoffs)
    : Mach (line, token)
{
    this->stk = stk;
    this->spoffs = spoffs;
    this->raoffs = raoffs;

    assert (rd->regplace != RP_R7);
    this->regwrites.push_back (rd);
}

Reg *LdImmStkMach::getRd ()
{
    return regwrites[0];
}

int16_t LdImmStkMach::getStkOffs ()
{
    int16_t stkoffs;
    bool ok = stk->getVarLocn ()->getStkOffs (&stkoffs, spoffs);
    assert (ok);
    return stkoffs + raoffs;
}

void LdImmStkMach::stepRegState ()
{
    checkRegState ();

    Reg *rd = getRd ();

    // should never try to set stack pointer to a constant
    assert (rd->regplace != RP_R6);

    // if stack location known, return that the register points to something on the stack
    // otherwise, register contents are unknown
    int16_t stkoffs;
    if (stk->getVarLocn ()->getStkOffs (&stkoffs, spoffs)) {
        rd->regcont  = RC_CONST;
        rd->regvalue = stkoffs + raoffs;
    } else {
        rd->regcont  = RC_OTHER;
    }

    checkRegState ();
}

bool LdImmStkMach::isLdImmAny (MachOp *mop_r, char const **str_r, uint16_t *val_r)
{
    int16_t stkoffs;
    if (! stk->getVarLocn ()->getStkOffs (&stkoffs, spoffs)) return false;

    *mop_r = MO_LDW;
    *str_r = nullptr;
    *val_r = stkoffs + raoffs;
    return true;
}

tsize_t LdImmStkMach::getisize ()
{
    return 4;
}

void LdImmStkMach::dumpmach (FILE *f)
{
    int16_t stkoffs;
    if (stk->getVarLocn ()->getStkOffs (&stkoffs, spoffs)) {
        int16_t offset = stkoffs + raoffs;
        fprintf (f, "\tldw\t%s,#%d ; stk:%s+%u\n", getRd ()->regstr (), offset, stk->getName (), raoffs);
    } else {
        fprintf (f, "\tldw\t%s,#stk:%s+%u\n", getRd ()->regstr (), stk->getName (), raoffs);
    }
}

// load assembly language label into register
LdImmStrMach::LdImmStrMach (int line, Token *token, MachOp mop, Reg *rd, char const *str, ValDecl *val)
    : Mach (line, token)
{
    this->mop = mop;
    this->str = str;
    this->val = val;

    assert (rd->regplace != RP_R7);
    this->regwrites.push_back (rd);
}

bool LdImmStrMach::isZero ()
{
    int npar = 0;
    while (str[npar] == '(') npar ++;
    if (str[npar] != '0') return false;
    for (int i = 0; i < npar; i ++) {
        if (str[npar+i+1] != ')') return false;
    }
    return str[npar*2+1] == 0;
}

Reg *LdImmStrMach::getRd ()
{
    return regwrites[0];
}

void LdImmStrMach::stepRegState ()
{
    checkRegState ();

    Reg *rd = getRd ();
    assert (rd->regplace != RP_R6);
    rd->regcont = RC_OTHER;

    checkRegState ();
}

bool LdImmStrMach::isLdImmAny (MachOp *mop_r, char const **str_r, uint16_t *val_r)
{
    *mop_r = mop;
    *str_r = isZero () ? nullptr : str;
    *val_r = 0;
    return true;
}

tsize_t LdImmStrMach::getisize ()
{
    return isZero () ? 2 : 4;
}

void LdImmStrMach::dumpmach (FILE *f)
{
    if (isZero ()) {
        fprintf (f, "\tclr\t%s\n", getRd ()->regstr ());
    } else {
        fprintf (f, "\t%s\t%s,#%s\n", mopstring (mop), getRd ()->regstr (), str);
    }
}

// load immediate integer value into register
LdImmValMach::LdImmValMach (int line, Token *token, MachOp mop, Reg *rd, tsize_t val)
    : Mach (line, token)
{
    this->mop = mop;
    this->val = val;

    assert (rd->regplace != RP_R7);
    this->regwrites.push_back (rd);
}

Reg *LdImmValMach::getRd ()
{
    return regwrites[0];
}

void LdImmValMach::stepRegState ()
{
    checkRegState ();

    Reg *rd = getRd ();

    assert (rd->regplace != RP_R6);

    rd->regcont  = RC_CONST;
    rd->regvalue = val;

    checkRegState ();
}

bool LdImmValMach::isLdImmAny (MachOp *mop_r, char const **str_r, uint16_t *val_r)
{
    *mop_r = mop;
    *str_r = nullptr;
    *val_r = val;
    return true;
}

tsize_t LdImmValMach::getisize ()
{
    return (val == 0) ? 2 : 4;
}

void LdImmValMach::dumpmach (FILE *f)
{
    if (val == 0) {
        fprintf (f, "\tclr\t%s\n", getRd ()->regstr ());
    } else {
        fprintf (f, "\t%s\t%s,#%u\n", mopstring (mop), getRd ()->regstr (), val);
    }
}

/////////////////////////////////////////
//  load value or address pc relative  //
/////////////////////////////////////////

// line  = line in assemble.cc
// token = source line token
// mop   = OP_LDA,OP_LDBS,OP_LDBU,OP_LDW
// rd    = register being loaded into
// label = assembly-language label being loaded from
LdPCRelMach::LdPCRelMach (int line, Token *token, MachOp mop, Reg *rd, LabelMach *lbl)
    : Mach (line, token)
{
    this->mop = mop;
    this->lbl = lbl;
    regwrites.push_back (rd);
}

Reg *LdPCRelMach::getRd ()
{
    return regwrites[0];
}

bool LdPCRelMach::brokenPCRel ()
{
    // look for label within LSOFFMAX in forward direction
    int16_t fwdoffs = 0;
    for (Mach *m = this; (m = m->nextmach) != nullptr;) {
        if (m == lbl) return false;
        fwdoffs += m->getisize ();
        if (fwdoffs > LSOFFMAX) break;
    }

    // look for label within LSOFFMIN in backward direction
    int16_t bwdoffs = 0;
    for (Mach *m = this; m != nullptr; m = m->prevmach) {
        bwdoffs -= m->getisize ();
        if (bwdoffs < LSOFFMIN) break;
        if (m == lbl) return false;
    }

    // not found within range, so it's broken
    return true;
}

tsize_t LdPCRelMach::getisize ()
{
    return 2;
}

void LdPCRelMach::dumpmach (FILE *f)
{
    fprintf (f, "\t%s\t%s,%s\n", mopstring (mop), getRd ()->regstr (), lbl->lblstr ());
}

//////////////////////////////////////
//  load into register from memory  //
//////////////////////////////////////

LoadMach::LoadMach (int line, Token *token, MachOp mop, Reg *rd, int16_t offs, Reg *ra)
    : Mach (line, token)
{
    assert ((mop == MO_LDBS) || (mop == MO_LDBU) || (mop == MO_LDW));
    assert (rd != nullptr);
    assert (ra != nullptr);
    this->mop  = mop;
    this->offs = offs;

    this->regwrites.push_back (rd);
    this->regreads.push_back (ra);
}

Reg *LoadMach::getRd ()
{
    return regwrites[0];
}

Reg *LoadMach::getRa ()
{
    return regreads[0];
}

bool LoadMach::hasSideEffects ()
{
    return getRd ()->regplace == RP_R7;
}

bool LoadMach::fallsThrough ()
{
    return getRd ()->regplace != RP_R7;
}

bool LoadMach::getOffs (int16_t *offs_r)
{
    *offs_r = offs;
    return true;
}

void LoadMach::setOffsRa (int16_t offs, Reg *ra)
{
    this->offs = offs;
    regreads[0] = ra;
}

void LoadMach::stepRegState ()
{
    checkRegState ();

    Reg *rd = getRd ();
    assert (rd->regplace != RP_R6);
    switch (mop) {
        case MO_LDW:  rd->regcont = RC_OTHER;    break;
        case MO_LDBS: rd->regcont = RC_SEXTBYTE; break;
        case MO_LDBU: rd->regcont = RC_ZEXTBYTE; break;
        default: assert_false ();
    }

    checkRegState ();
}

tsize_t LoadMach::getisize ()
{
    return 2;
}

void LoadMach::dumpmach (FILE *f)
{
    fprintf (f, "\t%s\t%s,%d(%s)\n", mopstring (mop), getRd ()->regstr (), offs, getRa ()->regstr ());
}

LoadStkMach::LoadStkMach (int line, Token *token, MachOp mop, Reg *rd, VarDecl *stk, tsize_t spoffs, tsize_t raoffs)
    : Mach (line, token)
{
    assert (rd != nullptr);
    this->mop = mop;
    this->stk = stk;
    this->spoffs = spoffs;
    this->raoffs = raoffs;

    this->regwrites.push_back (rd);
}

Reg *LoadStkMach::getRd ()
{
    return regwrites[0];
}

bool LoadStkMach::hasSideEffects ()
{
    return getRd ()->regplace == RP_R7;
}

bool LoadStkMach::fallsThrough ()
{
    return getRd ()->regplace != RP_R7;
}

void LoadStkMach::stepRegState ()
{
    checkRegState ();

    Reg *rd = getRd ();
    assert (rd->regplace != RP_R6);
    switch (mop) {
        case MO_LDW:  rd->regcont = RC_OTHER;    break;
        case MO_LDBS: rd->regcont = RC_SEXTBYTE; break;
        case MO_LDBU: rd->regcont = RC_ZEXTBYTE; break;
        default: assert_false ();
    }

    checkRegState ();
}

tsize_t LoadStkMach::getisize ()
{
    return 2;
}

void LoadStkMach::dumpmach (FILE *f)
{
    int16_t stkoffs;
    if (stk->getVarLocn ()->getStkOffs (&stkoffs, spoffs)) {
        int16_t offset = stkoffs + raoffs;
        fprintf (f, "\t%s\t%s,%d(%%r6) ; stk:%s+%u\n", mopstring (mop), getRd ()->regstr (), offset, stk->getName (), raoffs);
    } else {
        fprintf (f, "\t%s\t%s,stk:%s+%u\n", mopstring (mop), getRd ()->regstr (), stk->getName (), raoffs);
    }
}

////////////////////////////////////////////////////////////
//  bunch of labels that need to be marked as referenced  //
////////////////////////////////////////////////////////////

RefLabelMach::RefLabelMach (int line, Token *token)
    : Mach (line, token)
{ }

void RefLabelMach::markLabelsRefd ()
{
    for (auto it : labelvec) {
        it->labelmachrefd = true;
    }
}

tsize_t RefLabelMach::getisize ()
{
    return 0;
}

void RefLabelMach::dumpmach (FILE *f)
{ }

///////////////////////
//  function return  //
///////////////////////

RetMach::RetMach (int line, Token *token, FuncDecl *funcdecl, Reg *sp, Reg *ra, Reg *rv0, Reg *rv1)
    : Mach (line, token)
{
    this->funcdecl = funcdecl;
    assert (sp->regplace == RP_R6);
    assert (ra->regplace == RP_R3);
    this->regreads.push_back (sp);
    this->regreads.push_back (ra);
    if (rv0 != nullptr) this->regreads.push_back (rv0);
    if (rv1 != nullptr) this->regreads.push_back (rv1);

    FuncType *functype = funcdecl->getFuncType ();
    prmsize = functype->isRetByRef (functype->getDefTok ()) ? 2 : 0;
    tsize_t nprms = functype->getNumParams ();
    for (tsize_t i = 0; i < nprms; i ++) {
        Type *prmtype = functype->getParamType (i);
        tsize_t alin  = prmtype->getTypeAlign (token);
        tsize_t size  = prmtype->getTypeSize (token);
        prmsize  = (prmsize + alin - 1) & - alin;
        prmsize += size;
    }
    prmsize = (prmsize + MAXALIGN - 1) & - MAXALIGN;
}

tsize_t RetMach::getisize ()
{
    int offset = prmsize + funcdecl->stacksize;
    return (offset > LSOFFMAX) ? 8 : 4;
}

void RetMach::dumpmach (FILE *f)
{
    int offset = prmsize + funcdecl->stacksize;
    if (offset > LSOFFMAX) {
        fprintf (f, "\tldw\t%%r2,#%d\n", offset);
        fprintf (f, "\tadd\t%%r6,%%r6,%%r2\n");
    } else {
        fprintf (f, "\tlda\t%%r6,%d(%%r6)\n", offset);
    }
    fprintf (f, "\tlda\t%%r7,0(%s)\n", regreads[1]->regstr ());
}

///////////////////////////////////////
//  store from register into memory  //
///////////////////////////////////////

StoreMach::StoreMach (int line, Token *token, MachOp mop, Reg *rd, int16_t offs, Reg *ra)
    : Mach (line, token)
{
    this->mop  = mop;
    this->offs = offs;

    this->regreads.push_back (rd);
    this->regreads.push_back (ra);
}

Reg *StoreMach::getRd ()
{
    return regreads[0];
}

Reg *StoreMach::getRa ()
{
    return regreads[1];
}

bool StoreMach::getOffs (int16_t *offs_r)
{
    *offs_r = offs;
    return true;
}

void StoreMach::setOffsRa (int16_t offs, Reg *ra)
{
    this->offs = offs;
    regreads[1] = ra;
}

tsize_t StoreMach::getisize ()
{
    return 2;
}

void StoreMach::dumpmach (FILE *f)
{
    fprintf (f, "\t%s\t%s,%d(%s)\n", mopstring (mop), getRd ()->regstr (), offs, getRa ()->regstr ());
}

// - line   = line within assemble.cc
// - token  = token for error messages
// - mop    = OP_STB,OP_STW
// - rd     = data register being stored
// - stk    = stack variable being stored into
// - spoffs = stack pointer offset for such as call parameters
// - raoffs = offset within variable to store to
StoreStkMach::StoreStkMach (int line, Token *token, MachOp mop, Reg *rd, VarDecl *stk, tsize_t spoffs, tsize_t raoffs)
    : Mach (line, token)
{
    this->mop = mop;
    this->stk = stk;
    this->spoffs = spoffs;
    this->raoffs = raoffs;

    this->regreads.push_back (rd);
}

Reg *StoreStkMach::getRd ()
{
    return regreads[0];
}

tsize_t StoreStkMach::getisize ()
{
    return 2;
}

void StoreStkMach::dumpmach (FILE *f)
{
    int16_t stkoffs;
    if (stk->getVarLocn ()->getStkOffs (&stkoffs, spoffs)) {
        stkoffs += raoffs;
        fprintf (f, "\t%s\t%s,%d(%%r6) ; stk:%s+%u\n", mopstring (mop), getRd ()->regstr (), stkoffs, stk->getName (), raoffs);
    } else {
        fprintf (f, "\t%s\t%s,stk:%s+%u\n", mopstring (mop), getRd ()->regstr (), stk->getName (), raoffs);
    }
}

// get offset of call argument on stack about to be passed to a function
// we assume the stack pointer is pointed at the call arg list, ie, no accessing outer call args while building inner call args
bool CalarLocn::getStkOffs (int16_t *stkoffs_r, tsize_t spoffs)
{
    *stkoffs_r = offset - SPOFFSET;
    return true;
}

// get offset of function parameter from the stack pointer
bool ParamLocn::getStkOffs (int16_t *stkoffs_r, tsize_t spoffs)
{
    int16_t offs = this->funcdecl->stacksize + this->offset - SPOFFSET;
    if (this->funcdecl->getFuncType ()->isRetByRef (nullptr)) offs += 2;
    *stkoffs_r = offs + spoffs;
    return true;
}

// get offset of return-by-reference pointer from the stack pointer
bool RBRPtLocn::getStkOffs (int16_t *stkoffs_r, tsize_t spoffs)
{
    *stkoffs_r = this->funcdecl->stacksize - SPOFFSET + spoffs;
    return true;
}

// get offset of stack-based variable or temporary from the stack pointer
bool StackLocn::getStkOffs (int16_t *stkoffs_r, tsize_t spoffs)
{
    if (! this->assigned) return false;
    *stkoffs_r = this->offset - SPOFFSET + spoffs;
    return true;
}

////////////////////////////////////////////////
//  test register and branch within function  //
////////////////////////////////////////////////

TstBrMach::TstBrMach (int line, Token *token, MachOp mop, Reg *ra, LabelMach *lbl, LabelMach *brlbl)
    : BrMach (line, token)
{
    this->mop = mop;
    this->lbl = lbl;
    this->brlbl = brlbl;

    this->tstfake = ++ fakelast;

    this->regreads.push_back (ra);
}

Reg *TstBrMach::getRa ()
{
    return regreads[0];
}

tsize_t TstBrMach::getisize ()
{
    tsize_t isize = shortbr ? 4 : 8;
    if (brlbl != nullptr) isize += brlbl->getisize ();
    if (! this->prevmach->isArithMach () || ! this->prevmach->getRd ()->regeq (getRa ())) {
        isize += 2;
    }
    return isize;
}

void TstBrMach::dumpmach (FILE *f)
{
    if (! this->prevmach->isArithMach () || ! this->prevmach->getRd ()->regeq (getRa ())) {
        fprintf (f, "\ttst\t%s\n", regreads[0]->regstr ());
    }
    if (brlbl != nullptr) brlbl->dumpmach (f);
    if (shortbr) {
        fprintf (f, "\t%s\t%s\n", mopstring (mop), lbl->lblstr ());
    } else {
        fprintf (f, "\t%s\ttst.%d\n", mopstring (flipbr (mop)), tstfake);
        fprintf (f, "\tldw\t%%r7,#%s\n", lbl->lblstr ());
        fprintf (f, "tst.%d:\n", tstfake);
    }
}

////////////////////////////
//  16-bit constant word  //
////////////////////////////

WordMach::WordMach (int line, Token *token, char const *str, uint16_t val)
    : Mach (line, token)
{
    this->str = str;
    this->val = val;
}

tsize_t WordMach::getisize ()
{
    return 2;
}

void WordMach::dumpmach (FILE *f)
{
    if (str != nullptr) {
        fprintf (f, "\t.word\t%s\n", str);
    } else {
        fprintf (f, "\t.word\t%u\n", val);
    }
}



Reg::Reg (MachFile *mf, RegPlace regplace)
{
    this->regplace = regplace;
    this->regcont  = RC_OTHER;
    this->regvalue = 0;

    this->rfake = 0;
    if (regplace == RP_R05) {
        this->rfake = ++ fakelast;
    }

    this->regstrbuf[0] = 0;
    this->firstwrite = nullptr;
    this->lastread   = nullptr;

    mf->allregs.push_back (this);
}

bool Reg::regeq (Reg *other)
{
    if (regFake ()) return this == other;
    return this->regplace == other->regplace;
}

char const *Reg::regstr ()
{
    if (regplace == RP_R05) {
        if (regstrbuf[0] == 0) {
            snprintf (regstrbuf, sizeof regstrbuf, "%%r%d", rfake);
        }
        return regstrbuf;
    }
    return regstring (regplace);
}
