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

#include "fdfile.h"

int dprintf (int fd, char const *fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);
    int rc = vdprintf (fd, fmt, ap);
    va_end (ap);
    return rc;
}

int vdprintf (int fd, char const *fmt, va_list ap)
{
    FDFILE stream;
    stream.fd = fd;
    int rc = vfprintf (&stream, fmt, ap);
    stream.fd = -1;
    return rc;
}
