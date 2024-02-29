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

// append the formatted string to the C++ string

#include <stdio.h>
#include <stdlib.h>

#include "strprintf.h"

int strprintf (std::string *str, char const *fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);
    int rc = vstrprintf (str, fmt, ap);
    va_end (ap);
    return rc;
}

int vstrprintf (std::string *str, char const *fmt, va_list ap)
{
    char tmp1[1000];
    int rc = vsnprintf (tmp1, sizeof tmp1, fmt, ap);
    if (rc > 0) {
        if (rc < (int) sizeof tmp1) {
            str->append (tmp1, rc);
        } else {
            char tmp2[rc+1];
            int rc2 = vsnprintf (tmp2, sizeof tmp2, fmt, ap);
            if (rc2 != rc) abort ();
            str->append (tmp2, rc);
        }
    }
    return rc;
}
