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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct MALFILE : FILE {
    virtual int put (char const *buf, int len);
};

MALFILE::MALFILE ();

int MALFILE::put (char const *buf, int len)
{
    if (len > 0) {
        if (this->wused + len >= this->wsize) {
            do this->wsize += this->wsize / 2;
            while (this->wused + len >= this->wsize);
            this->wbuff = realloc (this->wbuff, this->wsize);
        }
        memcpy (this->wbuff + this->wused, buf, len);
        this->wused += len;
    }
    return len;
}

int asprintf (char **buf_r, char const *fmt, ...)
{
    va_list ap;
    va_start (ap, fmt);
    int rc = vasprintf (buf_r, fmt, ap);
    va_end (ap);
    return rc;
}

int vasprintf (char **buf_r, char const *fmt, va_list ap)
{
    MALFILE stream;
    stream.wbuff = malloc (64);
    stream.wsize = 64;
    int rc = vfprintf (&stream, fmt, ap);
    if (rc < 0) {
        free (stream.wbuff);
        *buf_r = NULL;
        return rc;
    }
    assert (rc < stream.wsize);
    stream.wbuff[rc] = 0;
    *buf_r = realloc (stream.wbuff, rc + 1);
    return rc;
}
