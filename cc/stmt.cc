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

#include "stmt.h"

////////////////////
//  CONSTRUCTORS  //
////////////////////

Stmt::Stmt (Scope *stmtscope, Token *stmttoken)
{
    this->stmtscope = stmtscope;
    this->stmttoken = stmttoken;
}

BlockStmt::BlockStmt (Scope *stmtscope, Token *stmttoken)
    : Stmt (stmtscope, stmttoken)
{ }

CaseStmt::CaseStmt (Scope *stmtscope, Token *stmttoken)
    : Stmt (stmtscope, stmttoken)
{
    this->lont = nullptr;
    this->hint = nullptr;
    this->lonc = NC_NONE;
    this->hinc = NC_NONE;
    this->labelprim = nullptr;
}

DeclStmt::DeclStmt ()
    : Stmt (nullptr, nullptr)
{ }

ExprStmt::ExprStmt ()
    : Stmt (nullptr, nullptr)
{ }

GotoStmt::GotoStmt (Scope *stmtscope, Token *stmttoken)
    : Stmt (stmtscope, stmttoken)
{ }

IfStmt::IfStmt (Scope *stmtscope, Token *stmttoken)
    : Stmt (stmtscope, stmttoken)
{
    this->condrev  = false;
    this->condexpr = nullptr;
    this->gotostmt = nullptr;
}

LabelStmt::LabelStmt (Scope *stmtscope, Token *stmttoken)
    : Stmt (stmtscope, stmttoken)
{
    this->name = nullptr;
    this->labelprim = nullptr;
}

LabelPrim *LabelStmt::getLabelPrim ()
{
    if (labelprim == nullptr) {
        labelprim = new LabelPrim (this->getStmtScope (), this->getStmtToken (), name);
    }
    return labelprim;
}

TryLabelStmt::TryLabelStmt (Scope *stmtscope, Token *stmttoken)
    : LabelStmt (stmtscope, stmttoken)
{
    this->tryscope = nullptr;
    this->outerlabel = nullptr;
}

NullStmt::NullStmt (Scope *stmtscope, Token *stmttoken)
    : Stmt (stmtscope, stmttoken)
{ }

SwitchStmt::SwitchStmt (Scope *stmtscope, Token *stmttoken)
    : Stmt (stmtscope, stmttoken)
{ }

ThrowStmt::ThrowStmt (Scope *stmtscope, Token *stmttoken)
    : Stmt (stmtscope, stmttoken)
{
    thrownexpr = nullptr;
}

TryStmt::TryStmt (Scope *stmtscope, Token *stmttoken)
    : Stmt (stmtscope, stmttoken)
{ }

////////////////////
//  dumpstmt()s  //
///////////////////

#define II 4

void BlockStmt::dumpstmt (int indent)
{
    if (stmts.empty ()) {
        printf ("{ }\n");
    } else {
        printf ("{\n");
        for (Stmt *stmt : stmts) {
            printf ("%*s", indent + II, "");
            stmt->dumpstmt (indent + II);
        }
        printf ("%*s}\n", indent, "");
    }
}

void CaseStmt::dumpstmt (int indent)
{
    if (lonc == NC_NONE) {
        printf ("default:\n");
    } else {
        NumToken lont ("", 0, 0, lonc, lonv);
        char const *lons = lont.getName ();
        printf ("case %s", lons);

        if (hinc != NC_NONE) {
            NumToken hint ("", 0, 0, hinc, hinv);
            char const *hins = hint.getName ();
            printf (" ... %s", hins);
        }
        printf (":\n");
    }
}

void DeclStmt::dumpstmt (int indent)
{
    for (Decl *decl : alldecls) {
        decl->dumpdecl ();
        printf (";\n");
    }
}

void ExprStmt::dumpstmt (int indent)
{
    expr->dumpexpr ();
    printf (";\n");
}

void GotoStmt::dumpstmt (int indent)
{
    printf ("goto %s;\n", name);
}

void IfStmt::dumpstmt (int indent)
{
    printf ("if (");
    printf (condrev ? "if (!(" : "if (");
    condexpr->dumpexpr ();
    printf (condrev ? ")) " : ") ");
    gotostmt->dumpstmt (indent);
}

void LabelStmt::dumpstmt (int indent)
{
    printf ("%s:\n", name);
}

void NullStmt::dumpstmt (int indent)
{
    printf (";\n");
}

void SwitchStmt::dumpstmt (int indent)
{
    printf ("switch (");
    valuexpr->dumpexpr ();
    printf (") {\n");
    for (Stmt *stmt : bodystmts) {
        printf ("%*s", indent + II, "");
        stmt->dumpstmt (indent + II);
    }
    printf ("%*s}\n", indent, "");
}

void ThrowStmt::dumpstmt (int indent)
{
    printf ("throw");
    if (thrownexpr != nullptr) {
        printf (" ");
        thrownexpr->dumpexpr ();
    }
    printf (";\n");
}

void TryStmt::dumpstmt (int indent)
{
    printf ("try ");
    bodystmt->dumpstmt (indent);
    for (std::pair<std::pair<VarDecl *,VarDecl *>,Stmt *> cc : catches) {
        printf ("%*scatch (", indent, "");
        cc.first.first->dumpdecl ();
        if (cc.first.second != nullptr) {
            printf (", ");
            cc.first.second->dumpdecl ();
        }
        printf (") ");
        cc.second->dumpstmt (indent);
    }
    if (finstmt != nullptr) {
        printf ("%*sfinally ", indent, "");
        finstmt->dumpstmt (indent + II);
    }
}

void TryLabelStmt::dumpstmt (int indent)
{
    printf ("%s(trylabelstmt):\n", name);
}
