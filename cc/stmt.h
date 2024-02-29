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
#ifndef _STMT_H
#define _STMT_H

#include <map>
#include <vector>

struct Stmt;
struct BlockStmt;
struct CaseStmt;
struct DeclStmt;
struct DoStmt;
struct ExprStmt;
struct ForStmt;
struct GotoStmt;
struct IfStmt;
struct LabelStmt;
struct TryLabelStmt;
struct SwitchStmt;
struct ThrowStmt;
struct TryStmt;
struct WhileStmt;

#include "cmp_str.h"
#include "decl.h"
#include "expr.h"
#include "prim.h"
#include "scope.h"
#include "token.h"

struct Stmt {
    Stmt (Scope *stmtscope, Token *stmttoken);

    Scope *getStmtScope () { return stmtscope; }
    Token *getStmtToken () { return stmttoken; }

    virtual ExprStmt *castExprStmt () { return nullptr; }
    virtual GotoStmt *castGotoStmt () { return nullptr; }

    virtual void dumpstmt (int indent) = 0;
    virtual Prim *generstmt (Prim *prevprim) = 0;

    static Prim *ifgoto (Prim *prevprim, Expr *condexpr, bool sense, LabelPrim *gotolbl);
    static Opcode revcmpop (Opcode cmpop);

private:
    Scope *stmtscope;
    Token *stmttoken;
};

struct BlockStmt : Stmt {
    BlockStmt (Scope *stmtscope, Token *stmttoken);

    std::vector<Stmt *> stmts;

    virtual void dumpstmt (int indent);
    virtual Prim *generstmt (Prim *prevprim);
};

struct CaseStmt : Stmt {
    CaseStmt (Scope *stmtscope, Token *stmttoken);

    NumType *lont, *hint;
    NumCat lonc, hinc;
    NumValue lonv, hinv;
    LabelPrim *labelprim;

    virtual void dumpstmt (int indent);
    virtual Prim *generstmt (Prim *prevprim);
};

struct DeclStmt : Stmt {
    DeclStmt ();

    DeclVec alldecls;

    virtual void dumpstmt (int indent);
    virtual Prim *generstmt (Prim *prevprim);
};

struct ExprStmt : Stmt {
    ExprStmt ();

    Expr *expr;

    virtual ExprStmt *castExprStmt () { return this; }

    virtual void dumpstmt (int indent);
    virtual Prim *generstmt (Prim *prevprim);
};

struct GotoStmt : Stmt {
    GotoStmt (Scope *stmtscope, Token *stmttoken);

    virtual GotoStmt *castGotoStmt () { return this; }

    char const *name;
    LabelStmt *label;

    virtual void dumpstmt (int indent);
    virtual Prim *generstmt (Prim *prevprim);
};

struct IfStmt : Stmt {
    IfStmt (Scope *stmtscope, Token *stmttoken);

    bool      condrev;
    Expr     *condexpr;
    GotoStmt *gotostmt;

    virtual void dumpstmt (int indent);
    virtual Prim *generstmt (Prim *prevprim);
};

struct LabelStmt : Stmt {
    LabelStmt (Scope *stmtscope, Token *stmttoken);

    char const *name;

    LabelPrim *getLabelPrim ();

    virtual void dumpstmt (int indent);
    virtual Prim *generstmt (Prim *prevprim);

private:
    LabelPrim *labelprim;
};

//      try {
//          ...
//          goto a;     // wrapper calls finally block then jumps to 'a'
//          ...
//      } finally {
//          ...
//      }
//      ...
//  a:
//      ...
struct TryLabelStmt : LabelStmt {
    TryLabelStmt (Scope *stmtscope, Token *stmttoken);

    TryScope *tryscope;
    LabelStmt *outerlabel;

    virtual void dumpstmt (int indent);
    virtual Prim *generstmt (Prim *prevprim);
};

struct NullStmt : Stmt {
    NullStmt (Scope *stmtscope, Token *stmttoken);

    virtual void dumpstmt (int indent);
    virtual Prim *generstmt (Prim *prevprim);
};

struct SwitchStmt : Stmt {
    SwitchStmt (Scope *stmtscope, Token *stmttoken);

    Expr *valuexpr;
    CaseStmt *defstmt;
    LabelStmt *breaklbl;
    std::vector<Stmt *> bodystmts;
    std::vector<CaseStmt *> casestmts;

    virtual void dumpstmt (int indent);
    virtual Prim *generstmt (Prim *prevprim);
};

struct ThrowStmt : Stmt {
    ThrowStmt (Scope *stmtscope, Token *stmttoken);

    Expr *thrownexpr;

    virtual void dumpstmt (int indent);
    virtual Prim *generstmt (Prim *prevprim);
};

struct TryStmt : Stmt {
    TryStmt (Scope *stmtscope, Token *stmttoken);

    Stmt *bodystmt;
    std::vector<std::pair<std::pair<VarDecl *,VarDecl *>,Stmt *>> catches;
    Token *fintoken;    // 'finally' token
    Stmt *finstmt;      // might be nullptr

    virtual void dumpstmt (int indent);
    virtual Prim *generstmt (Prim *prevprim);
};

#endif
