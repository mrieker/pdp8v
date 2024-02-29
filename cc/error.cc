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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "error.h"

void handlerror (Error *err)
{
    printerror (err->getToken (), "%s", err->getBuf ());
    delete err;
    int braces = 0;
    while (true) {
        Token *tok = gettoken ();
        switch (tok->getKw ()) {
            case KW_OBRACE: {
                braces ++;
                break;
            }
            case KW_CBRACE: {
                if (-- braces <= 0) {
                    if (braces < 0) pushtoken (tok);
                    return;
                }
                break;
            }
            case KW_EOF: return;
            case KW_SEMI: {
                if (braces <= 0) return;
                break;
            }
            default: break;
        }
    }
}

void printerror (Token *tok, char const *fmt, ...)
{
    va_list ap;
    char *buf;
    va_start (ap, fmt);
    vasprintf (&buf, fmt, ap);
    va_end (ap);
    fprintf (stderr, "%s:%d.%d: %s\n", tok->getFile (), tok->getLine (), tok->getColn (), buf);
    free (buf);
    errorflag = true;
}

void throwerror (Token *tok, char const *fmt, ...)
{
    char *buf;
    va_list ap;
    va_start (ap, fmt);
    vasprintf (&buf, fmt, ap);
    va_end (ap);
    throw new Error (tok, buf);
}

Error::Error (Token *tok, char *buf)
{
    this->tok = tok;
    this->buf = buf;
}

Error::~Error ()
{
    free (buf);
    buf = nullptr;
}
