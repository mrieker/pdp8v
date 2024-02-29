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
#ifndef _ERROR_H
#define _ERROR_H

#include "token.h"

struct Error {
    Error (Token *tok, char *buf);
    virtual ~Error ();

    Token *getToken () { return tok; }
    char const *getBuf () { return buf; }

private:
    Token *tok;
    char *buf;
};

void handlerror (Error *err);
void printerror (Token *tok, char const *fmt, ...);
void throwerror (Token *tok, char const *fmt, ...) __attribute__ ((__noreturn__));

#endif
