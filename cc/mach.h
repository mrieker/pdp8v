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
#ifndef _MACH_H
#define _MACH_H

#define SPOFFSET   64   // offset from bottom of stack bytes to stack pointer value

#define LSOFFMIN  -64   // lda/load/store instruction offset range
#define LSOFFMAX   63

#define BROFFMIN -512   // branch instruction offset range
#define BROFFMAX  510

enum MachOp {
    // memory
    MO_LDW,
    MO_LDBU,
    MO_LDBS,
    MO_LDA,
    MO_STW,
    MO_STB,
    // arith
    MO_ADD,
    MO_SUB,
    MO_ADC,
    MO_SBB,
    MO_AND,
    MO_OR,
    MO_XOR,
    MO_COM,
    MO_NEG,
    MO_MOV,
    MO_INC,
    MO_LSR,
    MO_ASR,
    MO_ROR,
    MO_SHL,
    MO_ROL,
    MO_CLR,
    // branch
    MO_BR,
    MO_BEQ,
    MO_BNE,
    MO_BLT,
    MO_BLE,
    MO_BGT,
    MO_BGE,
    MO_BLO,
    MO_BLOS,
    MO_BHI,
    MO_BHIS,
    MO_BVC,
    MO_BVS,
    MO_BMI,
    MO_BPL
};

// register content tracking
enum RegCont {
    RC_CONST,       // holds constant given in regvalue
    RC_CONSTSP,     // holds SP + constant in regvalue
    RC_SEXTBYTE,    // holds a byte with proper sign extension
    RC_ZEXTBYTE,    // holds a byte with proper zero extension
    RC_OTHER        // holds something else
};

enum RegPlace {
    RP_R0 = 0,  // must be in R0
    RP_R1 = 1,  // must be in R1
    RP_R2 = 2,  // must be in R2
    RP_R3 = 3,  // must be in R3
    RP_R4 = 4,  // must be in R4
    RP_R5 = 5,  // must be in R5
    RP_R6 = 6,  // must be in R6
    RP_R7 = 7,  // must be in R7
    RP_R05      // must be in any of R0..R5
};

struct Reg;
struct Mach;
struct Arith1Mach;
struct Arith2Mach;
struct Arith3Mach;
struct BrMach;
struct BranchMach;
struct CallMach;
struct CallImmMach;
struct CallPtrMach;
struct CmpBrMach;
struct EntMach;
struct LabelMach;
struct LdAddrMach;
struct LdAddrStkMach;
struct LdImmLblMach;
struct LdImmStkMach;
struct LdImmStrMach;
struct LdImmValMach;
struct LoadMach;
struct LoadStkMach;
struct RefLabelMach;
struct RetMach;
struct StoreMach;
struct StoreStkMach;
struct TstBrMach;
struct WordMach;
struct MachFile;

#include "decl.h"
#include "mytypes.h"
#include "prim.h"
#include "token.h"

struct Reg {
    Reg (MachFile *mf, RegPlace regplace);
    char const *regstr ();
    bool regFake () { return rfake > 0; }
    bool regeq (Reg *other);

    RegPlace regplace;

    Mach *firstwrite;
    Mach *lastread;

    RegCont regcont;
    uint16_t regvalue;

    bool isread;

private:
    char regstrbuf[8];
    int rfake;
};



struct Mach {
    Mach (int line, Token *token);

    bool readsReg (Reg *r);
    bool writesReg (Reg *r);

    virtual Arith3Mach *castArith3Mach () { return nullptr; }
    virtual BrMach *castBrMach () { return nullptr; }
    virtual CallPtrMach *castCallPtrMach () { return nullptr; }
    virtual LabelMach *castLabelMach () { return nullptr; }
    virtual LdAddrMach *castLdAddrMach () { return nullptr; }
    virtual LdAddrStkMach *castLdAddrStkMach () { return nullptr; }
    virtual LdImmLblMach *castLdImmLblMach () { return nullptr; }
    virtual LdImmStkMach *castLdImmStkMach () { return nullptr; }
    virtual LdImmStrMach *castLdImmStrMach () { return nullptr; }
    virtual LdImmValMach *castLdImmValMach () { return nullptr; }
    virtual LoadMach *castLoadMach () { return nullptr; }
    virtual LoadStkMach *castLoadStkMach () { return nullptr; }
    virtual StoreMach *castStoreMach () { return nullptr; }
    virtual StoreStkMach *castStoreStkMach () { return nullptr; }

    virtual Reg *getRa () { return nullptr; }
    virtual Reg *getRd () { return nullptr; }
    virtual bool getOffs (int16_t *offs_r) { return false; }
    virtual void setOffsRa (int16_t offs, Reg *ra) { assert_false (); }
    virtual VarDecl *getStackRef () { return nullptr; }
    virtual bool endOfBlock () { return false; }
    virtual bool hasSideEffects () { return false; }
    virtual bool fallsThrough () { return true; }
    virtual void markLabelsRefd () { }
    virtual uint32_t trashesHwRegs () { return 0; }
    virtual void stepRegState () { }
    virtual bool isLdImmAny (MachOp *mop_r, char const **str_r, uint16_t *val_r) { return false; }
    virtual bool brokenPCRel () { return false; }
    virtual bool isArithMach () { return false; }
    virtual tsize_t getisize () = 0;
    virtual void dumpmach (FILE *f) = 0;

    void checkRegState ();

    int line;       // assemble.c source line
    Token *token;   // source code location

    MachFile *machfile;
    Mach *nextmach;
    Mach *prevmach;

    std::vector<Mach *> altmachs;
    std::vector<Reg *> regreads;
    std::vector<Reg *> regwrites;

    tsize_t relpc;          // relative PC since beg of function

    uint32_t instnum;       // instruction number, starting at 0
    uint32_t acthregsout;   // bitmask of hardware registers active on output RP_R0..RP_R5
};

// mop rd
//  MO_CLR
struct Arith1Mach : Mach {
    Arith1Mach (int line, Token *token, MachOp mop, Reg *rd);

    virtual Reg *getRd ();
    virtual void stepRegState ();
    virtual bool isArithMach () { return true; }
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    MachOp mop;
};

// mop rd,ra
//  MO_COM, MO_NEG, MO_MOV, MO_INC, MO_LSR, MO_ASR, MO_ROR, MO_SHL, MO_ROL
struct Arith2Mach : Mach {
    Arith2Mach (int line, Token *token, MachOp mop, Reg *rd, Reg *ra);

    virtual Reg *getRa ();
    virtual Reg *getRd ();
    virtual void stepRegState ();
    virtual bool isArithMach () { return true; }
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    MachOp mop;
};

// mop rd,ra,rb
//  MO_ADD, MO_SUB, MO_ADC, MO_SBB, MO_AND, MO_OR, MO_XOR
struct Arith3Mach : Mach {
    Arith3Mach (int line, Token *token, MachOp mop, Reg *rd, Reg *ra, Reg *rb);

    virtual Arith3Mach *castArith3Mach () { return this; }

    Reg *getRb ();

    virtual Reg *getRa ();
    virtual Reg *getRd ();
    virtual void stepRegState ();
    virtual bool isArithMach () { return true; }
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    MachOp mop;
};

// branch abstract class
struct BrMach : Mach {
    BrMach (int line, Token *token);

    virtual BrMach *castBrMach () { return this; }

    virtual void markLabelsRefd ();
    virtual bool brokenPCRel ();

    bool shortbr;
    LabelMach *lbl;
};

// unconditional BR lbl
struct BranchMach : BrMach {
    BranchMach (int line, Token *token, LabelMach *lbl);

    virtual bool endOfBlock () { return true; }
    virtual bool fallsThrough () { return false; }
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);
};

// common call
struct CallMach : Mach {
    CallMach (int line, Token *token);

    virtual bool hasSideEffects () { return true; }
    virtual bool fallsThrough () { return false; }
    virtual uint32_t trashesHwRegs () { return (1U << RP_R0) | (1U << RP_R1) | (1U << RP_R2) | (1U << RP_R3) | (1U << RP_R4) | (1U << RP_R5); }
    virtual void stepRegState ();

    tsize_t stkpop;
};

// call the assembly-label function 'ent'
struct CallImmMach : CallMach {
    CallImmMach (int line, Token *token, char const *ent);

    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    char const *ent;
};

// call the function pointer is in 'offs(ra)'
struct CallPtrMach : CallMach {
    CallPtrMach (int line, Token *token, int16_t offs, Reg *ra);

    CallPtrMach *castCallPtrMach () { return this; }

    virtual Reg *getRa ();
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    int16_t offs;
};

//  CMP ra,rb
// brlbl:
//  mop lbl
//   MO_BEQ, MO_BNE, MO_BLT, MO_BLE, MO_BGT, MO_BGE, MO_BLO,
//   MO_BLOS, MO_BHI, MO_BHIS, MO_BVC, MO_BVS, MO_BMI, MO_BPL
struct CmpBrMach : BrMach {
    CmpBrMach (int line, Token *token, MachOp mop, Reg *ra, Reg *rb, LabelMach *lbl, LabelMach *brlbl);

    Reg *getRb ();

    virtual bool endOfBlock () { return true; }
    virtual Reg *getRa ();
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    MachOp mop;
    LabelMach *brlbl;
    int cmpfake;
};

// label and function entry code and make stack room
struct EntMach : Mach {
    EntMach (int line, Token *token, FuncDecl *funcdecl, Reg *sp, Reg *ra, Reg *tp);

    virtual bool hasSideEffects () { return true; }
    virtual void stepRegState ();
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    FuncDecl *funcdecl;
};

// lbl: - places label in assembly source
struct LabelMach : Mach {
    LabelMach (int line, Token *token);

    virtual LabelMach *castLabelMach () { return this; }

    virtual bool endOfBlock () { return true; }
    virtual bool hasSideEffects () { return true; }
    virtual void stepRegState ();
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    char const *lblstr ();

    char labelmachrefd;

private:
    char lblstrbuf[12];
    int lfake;
};

// LDA rd,offs(ra)
struct LdAddrMach : Mach {
    LdAddrMach (int line, Token *token, Reg *rd, int16_t offs, Reg *ra);

    virtual LdAddrMach *castLdAddrMach () { return this; }

    virtual Reg *getRa ();
    virtual Reg *getRd ();
    virtual bool getOffs (int16_t *offs_r);
    virtual void setOffsRa (int16_t offs, Reg *ra);
    virtual void stepRegState ();
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    int16_t offs;
};

// LDA rd,stk - stack variable
struct LdAddrStkMach : Mach {
    LdAddrStkMach (int line, Token *token, Reg *rd, VarDecl *stk, tsize_t spoffs, tsize_t raoffs);

    virtual LdAddrStkMach *castLdAddrStkMach () { return this; }

    virtual Reg *getRd ();
    virtual VarDecl *getStackRef () { return stk; }
    virtual void stepRegState ();
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    int16_t getStkOffs ();

    VarDecl *stk;
    tsize_t spoffs;
    tsize_t raoffs;
};

// LDW rd,#lbl - assembly language label, eg, lbl.234
struct LdImmLblMach : Mach {
    LdImmLblMach (int line, Token *token, Reg *rd, LabelMach *lbl);

    virtual LdImmLblMach *castLdImmLblMach () { return this; }

    virtual Reg *getRd ();
    virtual void stepRegState ();
    virtual void markLabelsRefd () { lbl->labelmachrefd = true; }
    virtual bool isLdImmAny (MachOp *mop_r, char const **str_r, uint16_t *val_r);
    virtual bool brokenPCRel ();
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    LabelMach *lbl;
    bool madeshort;

private:
    bool testshort ();
};

// LDW rd,#stkoffs - offset of a stack variable on the stack
struct LdImmStkMach : Mach {
    LdImmStkMach (int line, Token *token, Reg *rd, VarDecl *stk, tsize_t spoffs, tsize_t raoffs);

    virtual LdImmStkMach *castLdImmStkMach () { return this; }

    int16_t getStkOffs ();

    virtual Reg *getRd ();
    virtual void stepRegState ();
    virtual bool isLdImmAny (MachOp *mop_r, char const **str_r, uint16_t *val_r);
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    VarDecl *stk;
    tsize_t spoffs;
    tsize_t raoffs;
};

// LDx rd,#str - assembly language label, eg, someintarray+6
struct LdImmStrMach : Mach {
    LdImmStrMach (int line, Token *token, MachOp mop, Reg *rd, char const *str, ValDecl *val);

    virtual LdImmStrMach *castLdImmStrMach () { return this; }

    bool isZero ();

    virtual Reg *getRd ();
    virtual void stepRegState ();
    virtual bool isLdImmAny (MachOp *mop_r, char const **str_r, uint16_t *val_r);
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    MachOp mop;
    char const *str;
    ValDecl *val;
};

// LDx rd,#val - integer value
struct LdImmValMach : Mach {
    LdImmValMach (int line, Token *token, MachOp mop, Reg *rd, tsize_t val);

    virtual LdImmValMach *castLdImmValMach () { return this; }

    virtual Reg *getRd ();
    virtual void stepRegState ();
    virtual bool isLdImmAny (MachOp *mop_r, char const **str_r, uint16_t *val_r);
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    MachOp mop;
    tsize_t val;
};

// LDx rd,x(PC) ; x(PC) derived from label
struct LdPCRelMach : Mach {
    LdPCRelMach (int line, Token *token, MachOp mop, Reg *rd, LabelMach *lbl);

    virtual Reg *getRd ();
    virtual bool brokenPCRel ();
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    MachOp mop;
    LabelMach *lbl;
};

// LDx rd,offs(ra)
struct LoadMach : Mach {
    LoadMach (int line, Token *token, MachOp mop, Reg *rd, int16_t offs, Reg *ra);

    virtual LoadMach *castLoadMach () { return this; }

    virtual Reg *getRa ();
    virtual Reg *getRd ();
    virtual bool hasSideEffects ();
    virtual bool fallsThrough ();
    virtual bool getOffs (int16_t *offs_r);
    virtual void setOffsRa (int16_t offs, Reg *ra);
    virtual void stepRegState ();
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    MachOp mop;
    int16_t offs;
};

// LDx rd,stk - stack temporary
struct LoadStkMach : Mach {
    LoadStkMach (int line, Token *token, MachOp mop, Reg *rd, VarDecl *stk, tsize_t spoffs, tsize_t raoffs);

    virtual LoadStkMach *castLoadStkMach () { return this; }

    virtual Reg *getRd ();
    virtual bool hasSideEffects ();
    virtual bool fallsThrough ();
    virtual VarDecl *getStackRef () { return stk; }
    virtual void stepRegState ();
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    MachOp mop;
    VarDecl *stk;
    tsize_t spoffs;
    tsize_t raoffs;
};

struct RefLabelMach : Mach {
    RefLabelMach (int line, Token *token);

    std::vector<LabelMach *> labelvec;

    virtual void markLabelsRefd ();
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);
};

// wipes stack
// jumps to return address
struct RetMach : Mach {
    RetMach (int line, Token *token, FuncDecl *funcdecl, Reg *sp, Reg *ra, Reg *rv0, Reg *rv1);

    virtual bool endOfBlock () { return true; }
    virtual bool hasSideEffects () { return true; }
    virtual bool fallsThrough () { return false; }
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    FuncDecl *funcdecl;
    tsize_t prmsize;
};

// STx rd,offs(ra)
struct StoreMach : Mach {
    StoreMach (int line, Token *token, MachOp mop, Reg *rd, int16_t offs, Reg *ra);

    virtual StoreMach *castStoreMach () { return this; }

    virtual Reg *getRa ();
    virtual Reg *getRd ();
    virtual bool getOffs (int16_t *offs_r);
    virtual void setOffsRa (int16_t offs, Reg *ra);
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    MachOp mop;
    int16_t offs;
};

// STx rd,stk - stack temporary
// - line   = assemble.cc line that created it
// - token  = source line token for error messages
// - mop    = MO_STW,MO_STB
// - rd     = data register
// - stk    = stack variable
// - spoffs = offset amount of stack pointer for such as call arguments being built
// - raoffs = offset within variable to access a word within a multi-word variable
struct StoreStkMach : Mach {
    StoreStkMach (int line, Token *token, MachOp mop, Reg *rd, VarDecl *stk, tsize_t spoffs, tsize_t raoffs);

    virtual StoreStkMach *castStoreStkMach () { return this; }

    virtual Reg *getRd ();
    virtual VarDecl *getStackRef () { return stk; }
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    MachOp mop;
    VarDecl *stk;
    tsize_t spoffs;
    tsize_t raoffs;
};

//  TST ra
// brlbl:
//  mop lbl
//   MO_BEQ, MO_BNE, MO_BLT, MO_BLE, MO_BGT, MO_BGE, MO_BLO,
//   MO_BLOS, MO_BHI, MO_BHIS, MO_BVC, MO_BVS, MO_BMI, MO_BPL
struct TstBrMach : BrMach {
    TstBrMach (int line, Token *token, MachOp mop, Reg *ra, LabelMach *lbl, LabelMach *brlbl);

    virtual Reg *getRa ();
    virtual bool endOfBlock () { return true; }
    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    MachOp mop;
    LabelMach *brlbl;
    int tstfake;
};

// .WORD value
struct WordMach : Mach {
    WordMach (int line, Token *token, const char *str, uint16_t val);

    virtual tsize_t getisize ();
    virtual void dumpmach (FILE *f);

    char const *str;
    uint16_t val;
};



// all the machine instructions for a function
struct MachFile {
    MachFile (FuncDecl *funcdecl);

    FuncDecl *funcdecl;

    Mach *firstmach;                // first machine instruction
    Mach *lastmach;                 // last machine instruction

    Reg *pcreg;                     // the one-and-only R7 for function
    Reg *spreg;                     // the one-and-only R6 for function

    std::vector<Reg *> allregs;     // all registers in fuction

    tsize_t curspoffs;              // current stack-pointer offset for call arguments

    MachFile ();

    Mach *append (Mach *mach);
    void optmachs ();
    void insafter (Mach *mach, Mach *mark);
    void insbefore (Mach *mach, Mach *mark);
    void remove (Mach *mach);
    Mach *replace (Mach *oldmach, Mach *newmach);
    void replacereg (Reg *oldreg, Reg *newreg);

private:
    void opt_dump (int passno);
    bool opt_01 (bool didsomething);
    bool opt_02 (bool didsomething);
    bool opt_03 (bool didsomething);
    bool opt_04 (bool didsomething);
    bool opt_05 (bool didsomething);
    bool opt_06 (bool didsomething);
    bool opt_20 (bool didsomething);
    bool opt_21 (bool didsomething);
    bool opt_22 (bool didsomething);
    bool opt_22_b (bool didsomething, Mach *mach, Reg *rt, int16_t ldaoffs, Reg *ra);
    void opt_40 ();
    void opt_41 ();
    void opt_42 ();
    bool opt_50 (bool didsomething);
    void opt_51 ();
    void opt_asnstack ();
    void opt_asnhregs ();

    bool loadFromValue (Mach *mach, MachOp *loadopcode, Reg **loadregister, ValDecl **loadvalue, tsize_t *loadvalueoff, Mach **loadsubsequentmach);
    bool isModifyValue (Mach *mach, ValDecl *loadvalue, tsize_t loadvalueoff);
    bool storeOntoStack (Mach *mach, MachOp *storeopcode, Reg **storeregister, VarDecl **storestackvar, tsize_t *storestackvaroff, Mach **storesubsequentmach);
    bool loadFromStack (Mach *mach, MachOp *loadopcode, Reg **loadregister, VarDecl **loadstackvar, tsize_t *loadstackvaroff, Mach **loadsubsequentmach);
    bool addrOfStack (Mach *mach, Reg **addrregister, VarDecl **addrstackvar, tsize_t *addrstackvaroff, Mach **addrsubsequentmach);

    int verifylist ();
};

#endif
