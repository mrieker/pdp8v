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
#include <string.h>

#include "error.h"
#include "scope.h"

Scope::Scope (Scope *outer)
{
    this->outer = outer;
    if (outer != nullptr) {
        outer->inners.push_back (this);
    }
}

StructType *Scope::getStructType (char const *name)
{
    Decl *decl = lookupdecl (name);
    return (decl == nullptr) ? nullptr : decl->castStructType ();
}

// BlockScope

BlockScope::BlockScope (Scope *outer)
    : Scope (outer)
{ }

DeclMap const *BlockScope::getDecls ()
{
    return &decls;
}

Decl *BlockScope::lookupdecl (char const *name)
{
    auto search = decls.find (name);
    if (search != decls.end ()) return search->second;
    return getOuter ()->lookupdecl (name);
}

void BlockScope::enterdecl (Decl *decl)
{
    assert (decl->getScope () == this);
    auto ok = decls.insert ({ decl->getName (), decl });
    if (! ok.second) printerror (decl->getDefTok (), "duplicate definition of '%s'", decl->getName ());
}

void BlockScope::erasedecl (char const *name)
{
    decls.erase (name);
}

LabelStmt *BlockScope::lookuplabel (char const *name)
{
    auto search = labels.find (name);
    if (search != labels.end ()) return search->second;
    return getOuter ()->lookuplabel (name);
}

void BlockScope::enterlabel (LabelStmt *label)
{
    auto ok = labels.insert ({ label->name, label });
    if (! ok.second) printerror (label->getStmtToken (), "duplicate definition of '%s'", label->name);
}

FuncDecl *BlockScope::getOuterFuncDecl ()
{
    return getOuter ()->getOuterFuncDecl ();
}

// GlobalScope

GlobalScope::GlobalScope ()
    : Scope (nullptr)
{ }

DeclMap const *GlobalScope::getDecls ()
{
    return &decls;
}

Decl *GlobalScope::lookupdecl (char const *name)
{
    auto search = decls.find (name);
    return (search != decls.end ()) ? search->second : nullptr;
}

void GlobalScope::enterdecl (Decl *decl)
{
    assert (decl->getScope () == this);
    auto ok = decls.insert ({ decl->getName (), decl });
    if (! ok.second) printerror (decl->getDefTok (), "duplicate definition of '%s'", decl->getName ());
}

void GlobalScope::erasedecl (char const *name)
{
    decls.erase (name);
}

LabelStmt *GlobalScope::lookuplabel (char const *name)
{
    return nullptr;
}

void GlobalScope::enterlabel (LabelStmt *label)
{
    printerror (label->getStmtToken (), "no labels allowed at global scope");
}

FuncDecl *GlobalScope::getOuterFuncDecl ()
{
    return nullptr;
}

// MemberScope

MemberScope::MemberScope (Scope *outer)
    : Scope (outer)
{ }

DeclMap const *MemberScope::getDecls ()
{
    return &decls;
}

Decl *MemberScope::lookupdecl (char const *name)
{
    auto search = decls.find (name);
    if (search != decls.end ()) return search->second;
    return getOuter ()->lookupdecl (name);
}

void MemberScope::enterdecl (Decl *decl)
{
    assert (decl->getScope () == this);
    auto ok = decls.insert ({ decl->getName (), decl });
    if (! ok.second) printerror (decl->getDefTok (), "duplicate definition of '%s'", decl->getName ());
}

void MemberScope::erasedecl (char const *name)
{
    decls.erase (name);
}

LabelStmt *MemberScope::lookuplabel (char const *name)
{
    return nullptr;
}

void MemberScope::enterlabel (LabelStmt *label)
{
    printerror (label->getStmtToken (), "no labels allowed in struct/union");
}

FuncDecl *MemberScope::getOuterFuncDecl ()
{
    return getOuter ()->getOuterFuncDecl ();
}

// ParamScope

ParamScope::ParamScope (Scope *outer)
    : Scope (outer)
{ }

DeclMap const *ParamScope::getDecls ()
{
    return &decls;
}

Decl *ParamScope::lookupdecl (char const *name)
{
    auto search = decls.find (name);
    if (search != decls.end ()) return search->second;
    return getOuter ()->lookupdecl (name);
}

void ParamScope::enterdecl (Decl *decl)
{
    assert (decl->getScope () == this);
    auto ok = decls.insert ({ decl->getName (), decl });
    if (! ok.second) printerror (decl->getDefTok (), "duplicate definition of '%s'", decl->getName ());
}

void ParamScope::erasedecl (char const *name)
{
    decls.erase (name);
}

LabelStmt *ParamScope::lookuplabel (char const *name)
{
    return nullptr;
}

void ParamScope::enterlabel (LabelStmt *label)
{
    printerror (label->getStmtToken (), "no labels allowed in parameters");
}

FuncDecl *ParamScope::getOuterFuncDecl ()
{
    return funcdecl;
}

// TryScope

TryScope::TryScope (Scope *outer)
    : Scope (outer)
{ }

DeclMap const *TryScope::getDecls ()
{
    return &decls;
}

Decl *TryScope::lookupdecl (char const *name)
{
    auto search = decls.find (name);
    if (search != decls.end ()) return search->second;
    return getOuter ()->lookupdecl (name);
}

void TryScope::enterdecl (Decl *decl)
{
    assert (decl->getScope () == this);
    auto ok = decls.insert ({ decl->getName (), decl });
    if (! ok.second) printerror (decl->getDefTok (), "duplicate definition of '%s'", decl->getName ());
}

void TryScope::erasedecl (char const *name)
{
    decls.erase (name);
}

LabelStmt *TryScope::lookuplabel (char const *name)
{
    // if we already have a wrapper set up for this label, use it
    auto search = trylabels.find (name);
    if (search != trylabels.end ()) return search->second;

    // no wrapper, see if we can find label somewheres outside this try
    LabelStmt *outerlabelstmt = getOuter ()->lookuplabel (name);
    if (outerlabelstmt == nullptr) return nullptr;

    // wrap it to execute finally block before jumping to the label
    return wraplabel (outerlabelstmt);
}

TryLabelStmt *TryScope::wraplabel (LabelStmt *outerlabelstmt)
{
    // make a wrapper that calls the finally block then returns to the outerlabel
    TryLabelStmt *trylabel = new TryLabelStmt (outerlabelstmt->getStmtScope (), outerlabelstmt->getStmtToken ());
    trylabel->name = outerlabelstmt->name;
    trylabel->tryscope = this;
    trylabel->outerlabel = outerlabelstmt;

    // insert into list of wrappers for this try statement
    auto ok = trylabels.insert ({ outerlabelstmt->name, trylabel });
    assert (ok.second);

    // return the label wrapper
    return trylabel;
}

void TryScope::enterlabel (LabelStmt *label)
{
    printerror (label->getStmtToken (), "cannot declare in a try statement");
}

FuncDecl *TryScope::getOuterFuncDecl ()
{
    return getOuter ()->getOuterFuncDecl ();
}
