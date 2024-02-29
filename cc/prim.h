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
#ifndef _PRIM_H
#define _PRIM_H

#include <vector>

struct Prim;
struct AsnopPrim;
struct BinopPrim;
struct CallPrim;
struct CallStartPrim;
struct CastPrim;
struct CatEntPrim;
struct CompPrim;
struct EntryPrim;
struct FinCallPrim;
struct FinEntPrim;
struct FinRetPrim;
struct JumpPrim;
struct LabelPrim;
struct RetPrim;
struct TestPrim;
struct ThrowPrim;
struct TryBegPrim;
struct UnopPrim;

#include "decl.h"
#include "mach.h"
#include "opcode.h"
#include "scope.h"
#include "token.h"

// some instruction
struct Prim {
    virtual ~Prim () { }

    Scope *primscope;
    Token *primtoken;

    Prim *linext;                   // next in linear (source code) sequence
                                    // - null only for last prim in function (should always be a return)

    std::vector<Prim *> flonexts;   // next in flow
                                    // - normally just one entry same as linext
                                    // - empty for returns
                                    // - one entry for goto/break/continue/end-of-loops
                                    //   different than linext
                                    // - two entries for conditional branches
                                    //   flonexts[0] = alternative branch
                                    //   flonexts[1] = same as linext

    Prim (Scope *primscope, Token *primtoken);
    virtual Prim *setLinext (Prim *nextprim);
    virtual void resetLinext (Prim *nextprim);
    virtual bool hasFallthru () { return true; }
    virtual ValDecl *writesTemp () { return nullptr; }
    virtual void replaceReads (ValDecl *oldtemp, ValDecl *newtemp) { }
    virtual void replaceWrites (ValDecl *oldtemp, ValDecl *newtemp) { }
    virtual bool readsOrWritesVar (VarDecl *var) { return false; }
    virtual bool hasSideEffects () { return true; }
    virtual void replaceLabel (LabelPrim *oldlabel, LabelPrim *newlabel);
    void markLabelRefs ();

    virtual AsnopPrim *castAsnopPrim () { return nullptr; }
    virtual BinopPrim *castBinopPrim () { return nullptr; }
    virtual CastPrim  *castCastPrim  () { return nullptr; }
    virtual CompPrim  *castCompPrim  () { return nullptr; }
    virtual JumpPrim  *castJumpPrim  () { return nullptr; }
    virtual LabelPrim *castLabelPrim () { return nullptr; }
    virtual TestPrim  *castTestPrim  () { return nullptr; }
    virtual UnopPrim  *castUnopPrim  () { return nullptr; }

    void dumpprim ();
    virtual char *primstr () = 0;
    virtual void assemprim (MachFile *mf) = 0;
    virtual LabelMach *getLabelMach () { assert_false (); }

    void gencallmemcpy (MachFile *mf, Reg *roreg, Reg *rareg, tsize_t alin, tsize_t size);
    void ldbwRdAtRa (MachFile *mf, Type *valtype, Reg *rd, int off, Reg *ra);
    void stbwRdAtRa (MachFile *mf, Type *valtype, Reg *rd, int off, Reg *ra);
};

// do simple assignment
//  aval = bval
// always falls through to flonexts[0]
struct AsnopPrim : Prim {
    virtual ~AsnopPrim () { }

    ValDecl *aval;      // value of left-hand location
    ValDecl *bval;      // right-hand operand

    AsnopPrim (Scope *primscope, Token *primtoken);

    virtual AsnopPrim *castAsnopPrim () { return this; }

    virtual ValDecl *writesTemp () { return aval; }
    virtual void replaceReads (ValDecl *oldtemp, ValDecl *newtemp);
    virtual void replaceWrites (ValDecl *oldtemp, ValDecl *newtemp);
    virtual bool readsOrWritesVar (VarDecl *var);
    virtual bool hasSideEffects () { return false; }
    virtual char *primstr ();
    virtual void assemprim (MachFile *mf);
};

// do arithmetic: ovar = aval opcode bval
// always falls through to flonexts[0]
struct BinopPrim : Prim {
    virtual ~BinopPrim () { }

    Opcode opcode;
    ValDecl *ovar;
    ValDecl *aval;
    ValDecl *bval;

    BinopPrim (Scope *primscope, Token *primtoken);

    virtual BinopPrim *castBinopPrim () { return this; }

    virtual ValDecl *writesTemp () { return ovar; }
    virtual void replaceReads (ValDecl *oldtemp, ValDecl *newtemp);
    virtual void replaceWrites (ValDecl *oldtemp, ValDecl *newtemp);
    virtual bool readsOrWritesVar (VarDecl *var);
    virtual bool hasSideEffects () { return false; }
    virtual char *primstr ();
    virtual void assemprim (MachFile *mf);

    void addSubAndXorOr (MachFile *mf, MachOp mop, char const *mnestr);
    void shifts (MachFile *mf, MachOp mop, char const *mnestr);
    void mulDivMod (MachFile *mf, char const *mnestr);
    void compares (MachFile *mf, MachOp notbop);
};

// do function call: retvar = entval ( argvars )
// always falls through to flonexts[0]
struct CallPrim : Prim {
    CallPrim (Scope *primscope, Token *primtoken);
    virtual ~CallPrim () { }

    CallStartPrim *callstartprim;
    ValDecl *retvar;
    ValDecl *entval;
    ValDecl *thisval;
    std::vector<VarDecl *> argvars;

    virtual ValDecl *writesTemp () { return retvar; }
    virtual void replaceReads (ValDecl *oldtemp, ValDecl *newtemp);
    virtual void replaceWrites (ValDecl *oldtemp, ValDecl *newtemp);
    virtual bool readsOrWritesVar (VarDecl *var);
    virtual char *primstr ();
    virtual void assemprim (MachFile *mf);

    Prim *pushargval (Prim *prevprim, ValDecl *argval);
};

struct CallStartPrim : Prim {
    CallStartPrim (Scope *primscope, Token *primtoken);
    virtual ~CallStartPrim () { }

    CallPrim *callprim;

    bool retbyref;      // return value passed-by-ref as first argument
    tsize_t stacksize;  // stack size needed for all arguments
    tsize_t ssizenova;  // stack size needed for non-varargs arguments

    virtual void assemprim (MachFile *mf);
    virtual char *primstr ();
};

// cast value to different type: ovar = (casttype) aval
// always falls through to flonexts[0]
struct CastPrim : Prim {
    virtual ~CastPrim () { }

    ValDecl *ovar;
    ValDecl *aval;

    CastPrim (Scope *primscope, Token *primtoken);

    virtual CastPrim *castCastPrim () { return this; }

    virtual ValDecl *writesTemp () { return ovar; }
    virtual void replaceReads (ValDecl *oldtemp, ValDecl *newtemp);
    virtual void replaceWrites (ValDecl *oldtemp, ValDecl *newtemp);
    virtual bool readsOrWritesVar (VarDecl *var);
    virtual bool hasSideEffects () { return false; }
    virtual char *primstr ();
    virtual void assemprim (MachFile *mf);
};

// entry to catch block
struct CatEntPrim : Prim {
    CatEntPrim (Scope *primscope, Token *primtoken);
    virtual ~CatEntPrim () { }

    LabelPrim *catlblprim;  // label at start of catch block
    VarDecl *thrownvar;     // var that holds thrown pointer
    VarDecl *ttypevar;      // var that holds thrown type id

    virtual char *primstr ();
    virtual void assemprim (MachFile *mf);
};

// do comparison: goto flonexts [ (aval opcode bval) ? 0 : 1 ]
//  flonexts[0] = conditional branch target
//  flonexts[1] = fallthrough (same as linext)
struct CompPrim : Prim {
    virtual ~CompPrim () { }

    Opcode opcode;
    ValDecl *aval;
    ValDecl *bval;

    CompPrim (Scope *primscope, Token *primtoken);
    virtual CompPrim *castCompPrim () { return this; }
    virtual Prim *setLinext (Prim *nextprim);

    virtual bool readsOrWritesVar (VarDecl *var);
    virtual void replaceReads (ValDecl *oldtemp, ValDecl *newtemp);
    virtual char *primstr ();
    virtual void assemprim (MachFile *mf);
};

// function entrypoint
// always falls through to flonexts[0]
struct EntryPrim : Prim {
    virtual ~EntryPrim () { }

    FuncDecl *funcdecl;

    EntryPrim (Scope *primscope, Token *primtoken);

    virtual char *primstr ();
    virtual void assemprim (MachFile *mf);
};

// call finally block, returning to given spot
struct FinCallPrim : Prim {
    FinCallPrim (Scope *primscope, Token *primtoken, LabelPrim *finlblprim, LabelPrim *retlblprim);
    virtual ~FinCallPrim () { }

    LabelPrim *finlblprim;  // start of finally block
    LabelPrim *retlblprim;  // where finally block jumps when it completes

    virtual Prim *setLinext (Prim *nextprim);
    virtual bool hasFallthru () { return false; }
    virtual void replaceLabel (LabelPrim *oldlabel, LabelPrim *newlabel);
    virtual char *primstr ();
    virtual void assemprim (MachFile *mf);
};

// entry to finally block
struct FinEntPrim : Prim {
    FinEntPrim (Scope *primscope, Token *primtoken);
    virtual ~FinEntPrim () { }

    LabelPrim *finlblprim;  // label at start of finally block
    VarDecl *trymarkvar;    // temp var that holds try block context on stack

    virtual char *primstr ();
    virtual void assemprim (MachFile *mf);
};

// exit from finally block
struct FinRetPrim : Prim {
    FinRetPrim (Scope *primscope, Token *primtoken);
    virtual ~FinRetPrim () { }

    VarDecl *trymarkvar;    // temp var that holds try block context on stack

    virtual Prim *setLinext (Prim *nextprim);
    virtual bool hasFallthru () { return false; }
    virtual char *primstr ();
    virtual void assemprim (MachFile *mf);
};

// unconditional branch to flonexts[0]
// never falls through
struct JumpPrim : Prim {
    virtual ~JumpPrim () { }

    JumpPrim (Scope *primscope, Token *primtoken, LabelPrim *jumplbl);
    virtual JumpPrim *castJumpPrim () { return this; }

    virtual Prim *setLinext (Prim *nextprim);
    virtual bool hasFallthru () { return false; }

    virtual char *primstr ();
    virtual void assemprim (MachFile *mf);
};

// nop filler for target to jump to
// always falls through to flonexts[0]
struct LabelPrim : Prim {
    virtual ~LabelPrim () { }

    char const *lpname;
    bool lblrefd;

    LabelPrim (Scope *primscope, Token *primtoken, char const *lpname);

    virtual LabelPrim *castLabelPrim () { return this; }

    virtual char *primstr ();
    virtual void assemprim (MachFile *mf);
    virtual LabelMach *getLabelMach ();

private:
    LabelMach *labelmach;
};

// return from function
//  rval = return value (or nullptr if void)
struct RetPrim : Prim {
    virtual ~RetPrim () { }

    FuncDecl *funcdecl;
    ValDecl *rval;

    RetPrim (Scope *primscope, Token *primtoken);
    virtual RetPrim *castRetPrim () { return this; }
    virtual Prim *setLinext (Prim *nextprim);
    virtual bool hasFallthru () { return false; }

    virtual bool readsOrWritesVar (VarDecl *var);
    virtual void replaceReads (ValDecl *oldtemp, ValDecl *newtemp);
    virtual char *primstr ();
    virtual void assemprim (MachFile *mf);
};

// do test: goto flonexts [ (aval opcode 0) ? 0 : 1 ]
//  flonexts[0] = conditional branch target
//  flonexts[1] = fallthrough (same as linext)
struct TestPrim : Prim {
    TestPrim (Scope *primscope, Token *primtoken);

    virtual ~TestPrim () { }

    virtual TestPrim *castTestPrim () { return this; }

    virtual Prim *setLinext (Prim *nextprim);

    virtual bool readsOrWritesVar (VarDecl *var);
    virtual void replaceReads (ValDecl *oldtemp, ValDecl *newtemp);
    virtual char *primstr ();
    virtual void assemprim (MachFile *mf);

    Opcode opcode;
    ValDecl *aval;
};

struct ThrowPrim : Prim {
    ThrowPrim (Scope *primscope, Token *primtoken);
    virtual ~ThrowPrim () { }

    virtual Prim *setLinext (Prim *nextprim);
    virtual bool readsOrWritesVar (VarDecl *var);
    virtual void replaceReads (ValDecl *oldtemp, ValDecl *newtemp);
    virtual bool hasFallthru () { return false; }
    virtual char *primstr ();
    virtual void assemprim (MachFile *mf);

    ValDecl *thrownval;
};

// beginning of try block
struct TryBegPrim : Prim {
    TryBegPrim (Scope *primscope, Token *primtoken);
    virtual ~TryBegPrim () { }

    VarDecl *trymarkvar;
    std::vector<std::pair<Type *,LabelPrim *>> catlblprims;
    LabelPrim *finlblprim;

    virtual Prim *setLinext (Prim *nextprim);
    virtual char *primstr ();
    virtual void assemprim (MachFile *mf);
};

// do arithmetic: ovar = opcode aval
// always falls through to flonexts[0]
struct UnopPrim : Prim {
    virtual ~UnopPrim () { }

    Opcode opcode;
    ValDecl *ovar;
    ValDecl *aval;

    UnopPrim (Scope *primscope, Token *primtoken);

    virtual UnopPrim *castUnopPrim () { return this; }

    virtual ValDecl *writesTemp () { return ovar; }
    virtual void replaceReads (ValDecl *oldtemp, ValDecl *newtemp);
    virtual void replaceWrites (ValDecl *oldtemp, ValDecl *newtemp);
    virtual bool readsOrWritesVar (VarDecl *var);
    virtual bool hasSideEffects () { return false; }
    virtual char *primstr ();
    virtual void assemprim (MachFile *mf);
};

#endif
