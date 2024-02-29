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

#include <stdlib.h>

#include "mytypes.h"
#include "opcode.h"

char const *opcodestr (Opcode opcode)
{
    switch (opcode) {
        case OP_COMMA:   return ",";
        case OP_MOVE:    return "=";

        case OP_COLON:   return ":";
        case OP_QMARK:   return "?";
        case OP_ASNEQ:   return "=";
        case OP_ASNADD:  return "+=";
        case OP_ASNSUB:  return "-=";
        case OP_ASNMUL:  return "*=";
        case OP_ASNDIV:  return "/=";
        case OP_ASNMOD:  return "%=";
        case OP_ASNSHL:  return "<<=";
        case OP_ASNSHR:  return ">>=";
        case OP_ASNAND:  return "&=";
        case OP_ASNXOR:  return "^=";
        case OP_ASNOR:   return "|=";

        case OP_LOGOR:   return "||";
        case OP_LOGAND:  return "&&";
        case OP_BITOR:   return "|";
        case OP_BITXOR:  return "^";
        case OP_BITAND:  return "&";

        case OP_CMPEQ:   return "==";
        case OP_CMPNE:   return "!=";

        case OP_CMPGE:   return ">=";
        case OP_CMPGT:   return ">";
        case OP_CMPLE:   return "<=";
        case OP_CMPLT:   return "<";

        case OP_SHL:     return "<<";
        case OP_SHR:     return ">>";

        case OP_ADD:     return "+";
        case OP_SUB:     return "-";

        case OP_DIV:     return "/";
        case OP_MUL:     return "*";
        case OP_MOD:     return "%";

        case OP_PREINC:  return "++";
        case OP_PREDEC:  return "--";
        case OP_NEGATE:  return "-";
        case OP_LOGNOT:  return "!";
        case OP_BITNOT:  return "~";
        case OP_DEREF:   return "*";
        case OP_ADDROF:  return "&";
        case OP_SIZEOF:  return "sizeof";
        case OP_ALIGNOF: return "alignof";

        case OP_POSTINC: return "++";
        case OP_POSTDEC: return "--";
        case OP_FUNCALL: return "(...)";
        case OP_SUBSCR:  return "[...]";
        case OP_MEMDOT:  return ".";
        case OP_MEMPTR:  return "->";

        default: assert_false ();
    }
}
