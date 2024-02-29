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


typedef __uint16_t LineNum;

void ass_putint (int i);

struct Value { };

struct IntValue : Value {
    int value;
};

struct StrValue : Value {
    char const *value;
};

struct Expr {
    LineNum explinum;
    virtual Value *compute ();
};

struct Label {
    LineNum lablinum;
};

struct Stmt {
    LineNum linenum;
    virtual Stmt *execute ();
};

struct IfStmt : Stmt {
    Expr *condexpr;
    Label *thenlabel;
};


void test22 ()
{
    IfStmt ifstmt;

    ass_putint ((__size_t) &ifstmt.linenum   - (__size_t) &ifstmt);
    ass_putint ((__size_t) &ifstmt.condexpr  - (__size_t) &ifstmt);
    ass_putint ((__size_t) &ifstmt.thenlabel - (__size_t) &ifstmt);
}
