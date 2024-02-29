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

#include <assert.h>
#include <stdio.h>
#include <string>
#include <string.h>

#include "opcode.h"
#include "prim.h"

////////////////////
//  constructors  //
////////////////////

Prim::Prim (Scope *primscope, Token *primtoken)
{
    assert (primscope != nullptr);
    assert (primtoken != nullptr);
    this->primscope = primscope;
    this->primtoken = primtoken;
    this->linext    = nullptr;
}

AsnopPrim::AsnopPrim (Scope *primscope, Token *primtoken)
    : Prim (primscope, primtoken)
{
    aval = nullptr;
    bval = nullptr;
}

BinopPrim::BinopPrim (Scope *primscope, Token *primtoken)
    : Prim (primscope, primtoken)
{
    opcode = OP_END;
    aval   = nullptr;
    bval   = nullptr;
    ovar   = nullptr;
}

CallPrim::CallPrim (Scope *primscope, Token *primtoken)
    : Prim (primscope, primtoken)
{
    this->callstartprim = nullptr;
    this->retvar  = nullptr;
    this->entval  = nullptr;
    this->thisval = nullptr;
}

CallStartPrim::CallStartPrim (Scope *primscope, Token *primtoken)
    : Prim (primscope, primtoken)
{
    this->callprim = nullptr;
}

CastPrim::CastPrim (Scope *primscope, Token *primtoken)
    : Prim (primscope, primtoken)
{
    aval = nullptr;
    ovar = nullptr;
}

CatEntPrim::CatEntPrim (Scope *primscope, Token *primtoken)
    : Prim (primscope, primtoken)
{ }

CompPrim::CompPrim (Scope *primscope, Token *primtoken)
    : Prim (primscope, primtoken)
{
    opcode = OP_END;
    aval = nullptr;
    bval = nullptr;
}

EntryPrim::EntryPrim (Scope *primscope, Token *primtoken)
    : Prim (primscope, primtoken)
{ }

FinCallPrim::FinCallPrim (Scope *primscope, Token *primtoken, LabelPrim *finlblprim, LabelPrim *retlblprim)
    : Prim (primscope, primtoken)
{
    this->finlblprim = finlblprim;
    this->retlblprim = retlblprim;

    flonexts.push_back (finlblprim);
}

FinEntPrim::FinEntPrim (Scope *primscope, Token *primtoken)
    : Prim (primscope, primtoken)
{ }

FinRetPrim::FinRetPrim (Scope *primscope, Token *primtoken)
    : Prim (primscope, primtoken)
{ }

JumpPrim::JumpPrim (Scope *primscope, Token *primtoken, LabelPrim *jumplbl)
    : Prim (primscope, primtoken)
{
    flonexts.push_back (jumplbl);
}

LabelPrim::LabelPrim (Scope *primscope, Token *primtoken, char const *lpname)
    : Prim (primscope, primtoken)
{
    this->lpname = lpname;
    this->labelmach = nullptr;
}

RetPrim::RetPrim (Scope *primscope, Token *primtoken)
    : Prim (primscope, primtoken)
{ }

TestPrim::TestPrim (Scope *primscope, Token *primtoken)
    : Prim (primscope, primtoken)
{ }

ThrowPrim::ThrowPrim (Scope *primscope, Token *primtoken)
    : Prim (primscope, primtoken)
{
    thrownval = nullptr;
}

TryBegPrim::TryBegPrim (Scope *primscope, Token *primtoken)
    : Prim (primscope, primtoken)
{ }

UnopPrim::UnopPrim (Scope *primscope, Token *primtoken)
    : Prim (primscope, primtoken)
{ }

////////////////////////////
//  linear list building  //
////////////////////////////

// by default, each prim flows linearly to the next and nowhere else
Prim *Prim::setLinext (Prim *nextprim)
{
    assert (linext == nullptr);
    assert (flonexts.size () == 0);
    linext = nextprim;
    flonexts.push_back (nextprim);
    return nextprim;
}

// conditional prims (comp, test) have alternative in flonexts[0]
// ...and linear following in flonexts[1]
Prim *CompPrim::setLinext (Prim *nextprim)
{
    assert (linext == nullptr);
    assert (flonexts.size () == 1);
    linext = nextprim;
    flonexts.push_back (nextprim);
    return nextprim;
}

Prim *TestPrim::setLinext (Prim *nextprim)
{
    assert (linext == nullptr);
    assert (flonexts.size () == 1);
    linext = nextprim;
    flonexts.push_back (nextprim);
    return nextprim;
}

// jump flows only to the alternative in flonexts[0]
Prim *JumpPrim::setLinext (Prim *nextprim)
{
    assert (linext == nullptr);
    assert (flonexts.size () == 1);
    linext = nextprim;
    return nextprim;
}

// return does not flow onto anything
Prim *RetPrim::setLinext (Prim *nextprim)
{
    assert (linext == nullptr);
    assert (flonexts.size () == 0);
    linext = nextprim;
    return nextprim;
}

// throw does not flow onto anything
Prim *ThrowPrim::setLinext (Prim *nextprim)
{
    assert (linext == nullptr);
    assert (flonexts.size () == 0);
    linext = nextprim;
    return nextprim;
}

// try flows to the next prim as well as to the finally and all catches
Prim *TryBegPrim::setLinext (Prim *nextprim)
{
    assert (linext == nullptr);
    linext = nextprim;
    flonexts.push_back (nextprim);
    return nextprim;
}

// call finally block(), return to given label
//  flonexts[0] already set to finally block entrypoint
Prim *FinCallPrim::setLinext (Prim *nextprim)
{
    assert (linext == nullptr);
    assert (flonexts.size () == 1);
    assert (flonexts[0] == finlblprim);
    linext = nextprim;
    return nextprim;
}

// returning out bottom of finally block followed by retlbls of all FinCallPrims
//  flonexts = already filled in with retlbls of all FinCallPrims
//             should always at least have the one from bottom of try block
Prim *FinRetPrim::setLinext (Prim *nextprim)
{
    assert (linext == nullptr);
    assert (flonexts.size () > 0);
    linext = nextprim;
    return nextprim;
}

////////////
//  dump  //
////////////

void Prim::dumpprim ()
{
    char *buf = primstr ();
    printf ("%p: %s\n", this, buf);
    free (buf);
}

char *AsnopPrim::primstr ()
{
    assert ((flonexts.size () == 1) && (flonexts[0] == linext));

    char *buf;
    asprintf (&buf, "ASNOP   %s = %s", aval->getName (), bval->getName ());
    return buf;
}

char *BinopPrim::primstr ()
{
    assert ((flonexts.size () == 1) && (flonexts[0] == linext));

    assert (ovar != nullptr);
    char *buf;
    asprintf (&buf, "BINOP   %s = %s %s %s", ovar->getName (), aval->getName (), opcodestr (opcode), bval->getName ());
    return buf;
}

char *CallPrim::primstr ()
{
    assert ((flonexts.size () == 1) && (flonexts[0] == linext));

    std::string str;
    str.append ("CALL    ");
    if (retvar != nullptr) {
        str.append (retvar->getName ());
        str.append (" = ");
    }
    if (thisval != nullptr) {
        str.append (thisval->getName ());
        str.append ("->");
    }
    str.append (entval->getEncName ());
    str.append (" (");

    bool first = true;
    for (VarDecl *argvar : argvars) {
        if (! first) str.append (", ");
        str.append (argvar->getName ());
        first = false;
    }
    str.append (")");

    return strdup (str.c_str ());
}

char *CallStartPrim::primstr ()
{
    assert ((flonexts.size () == 1) && (flonexts[0] == linext));
    return strdup ("CALLSTART");
}

char *CastPrim::primstr ()
{
    assert ((flonexts.size () == 1) && (flonexts[0] == linext));

    assert (ovar != nullptr);
    char *buf;
    asprintf (&buf, "CAST    %s = (%s) %s",
            ovar->getName (),
            ovar->getType ()->getName (),
            aval->getName ());
    return buf;
}

char *CatEntPrim::primstr ()
{
    assert (flonexts.size () == 1);

    char *buf;
    asprintf (&buf, "CATENT  %s: %s %s", catlblprim->lpname, thrownvar->getType ()->getName (), thrownvar->getName ());
    return buf;
}

char *CompPrim::primstr ()
{
    assert ((flonexts.size () == 2) && (flonexts[1] == linext));

    char *buf;
    asprintf (&buf, "COMP    if (%s %s %s) goto %p", aval->getName (), opcodestr (opcode), bval->getName (), flonexts[0]);
    return buf;
}

char *EntryPrim::primstr ()
{
    assert ((flonexts.size () == 1) && (flonexts[0] == linext));

    char *buf;
    asprintf (&buf, "ENTRY   %s", funcdecl->getEncName ());
    return buf;
}

char *FinCallPrim::primstr ()
{
    assert (flonexts.size () == 1);

    char *buf;
    asprintf (&buf, "FINCALL %p,%p : %s,%s", finlblprim, retlblprim, finlblprim->lpname, retlblprim->lpname);
    return buf;
}

char *FinEntPrim::primstr ()
{
    assert (flonexts.size () == 1);

    char *buf;
    asprintf (&buf, "FINENT  %p : %s", finlblprim, finlblprim->lpname);
    return buf;
}

char *FinRetPrim::primstr ()
{
    char *buf;
    asprintf (&buf, "FINRET  %s", trymarkvar->getName ());
    return buf;
}

char *JumpPrim::primstr ()
{
    assert (flonexts.size () == 1);

    LabelPrim *lp = flonexts[0]->castLabelPrim ();

    char *buf;
    asprintf (&buf, "JUMP    %p : %s", lp, lp->lpname);
    return buf;
}

char *LabelPrim::primstr ()
{
    assert ((flonexts.size () == 1) && (flonexts[0] == linext));

    char *buf;
    asprintf (&buf, "LABEL   %s", lpname);
    return buf;
}

char *RetPrim::primstr ()
{
    assert (flonexts.size () == 0);

    char *buf;

    if (rval != nullptr) {
        asprintf (&buf, "RET     %s", rval->getName ());
    } else {
        asprintf (&buf, "RET");
    }

    return buf;
}

char *TestPrim::primstr ()
{
    assert ((flonexts.size () == 2) && (flonexts[1] == linext));

    char *buf;
    asprintf (&buf, "TEST    if (%s %s 0) goto %p", aval->getName (), opcodestr (opcode), flonexts[0]);
    return buf;
}

char *ThrowPrim::primstr ()
{
    if (thrownval == nullptr) {
        return strdup ("RETHROW");
    }
    char *buf;
    asprintf (&buf, "THROW   %s", thrownval->getName ());
    return buf;
}

char *TryBegPrim::primstr ()
{
    char *buf;
    asprintf (&buf, "TRYBEG  %s", trymarkvar->getName ());
    return buf;
}

char *UnopPrim::primstr ()
{
    assert ((flonexts.size () == 1) && (flonexts[0] == linext));

    assert (ovar != nullptr);
    char *buf;
    asprintf (&buf, "UNOP    %s = %s %s", ovar->getName (), opcodestr (opcode), aval->getName ());
    return buf;
}
