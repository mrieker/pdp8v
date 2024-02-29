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
#ifndef _OPCODE_H
#define _OPCODE_H

#define PRECED(op) (op & 0xFF00)
#define PRRTOL(op) ((op & 0x0100) != 0)

enum Opcode {
    // https://en.cppreference.com/w/cpp/language/operator_precedence
    OP_END     = 0x0000,

    OP_COMMA   = 0x0201,    // comma

    OP_ASNEQ   = 0x0301,    // =
    OP_ASNADD  = 0x0302,    // +=
    OP_ASNSUB  = 0x0303,    // -=
    OP_ASNMUL  = 0x0304,    // *=
    OP_ASNDIV  = 0x0305,    // /=
    OP_ASNMOD  = 0x0306,    // %=
    OP_ASNSHL  = 0x0307,    // <<=
    OP_ASNSHR  = 0x0308,    // >>=
    OP_ASNAND  = 0x0309,    // &=
    OP_ASNXOR  = 0x030A,    // ^=
    OP_ASNOR   = 0x030B,    // |=

    OP_COLON   = 0x0501,    // colon
    OP_QMARK   = 0x0502,    // question mark

    OP_LOGOR   = 0x0601,    // ||
    OP_LOGAND  = 0x0801,    // &&
    OP_BITOR   = 0x0A01,    // |
    OP_BITXOR  = 0x0C01,    // ^
    OP_BITAND  = 0x0E01,    // &

    OP_CMPEQ   = 0x1001,    // ==
    OP_CMPNE   = 0x1002,    // !=

    OP_CMPGE   = 0x1201,    // >=
    OP_CMPGT   = 0x1202,    // >
    OP_CMPLE   = 0x1203,    // <=
    OP_CMPLT   = 0x1204,    // <

    OP_SHL     = 0x1401,    // <<
    OP_SHR     = 0x1402,    // >>

    OP_ADD     = 0x1601,    // +
    OP_SUB     = 0x1602,    // -

    OP_DIV     = 0x1801,    // /
    OP_MUL     = 0x1802,    // *
    OP_MOD     = 0x1803,    // %

    OP_PREINC  = 0x1901,    // ++
    OP_PREDEC  = 0x1902,    // --
    OP_MOVE    = 0x1903,    // +
    OP_NEGATE  = 0x1904,    // -
    OP_LOGNOT  = 0x1905,    // !
    OP_BITNOT  = 0x1906,    // ~
    OP_CCAST   = 0x1907,    // (cast)
    OP_DEREF   = 0x1908,    // *
    OP_ADDROF  = 0x1909,    // &
    OP_SIZEOF  = 0x190A,    // sizeof
    OP_ALIGNOF = 0x190B,    // alignof

    OP_POSTINC = 0x1A01,    // ++
    OP_POSTDEC = 0x1A02,    // --
    OP_FUNCALL = 0x1A03,    // ( function call parameters )
    OP_SUBSCR  = 0x1A04,    // [...]
    OP_MEMDOT  = 0x1A05,    // .
    OP_MEMPTR  = 0x1A06     // ->
};

char const *opcodestr (Opcode opcode);

#endif
