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
#include <hode.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "fdfile.h"

#define BUFSIZ 512

FILE::FILE ()
{
    this->wsize = 0;
    this->wused = 0;
    this->wbuff = NULL;
    this->wmode = _IOLBF;
    this->wchkd = 0;
    this->rsize = 0;
    this->rused = 0;
    this->rnext = 0;
    this->rbuff = NULL;
}

FILE::~FILE ()
{
    this->fclose ();
}

int FILE::fclose ()
{
    if (this->rbuff != NULL) {
        free (this->rbuff);
        this->rbuff = NULL;
    }
    if (this->wbuff != NULL) {
        this->flush ();
        free (this->wbuff);
        this->wbuff = NULL;
    }
    return this->close ();
}

int FILE::getc ()
{
    if (this->rnext >= this->rused) {
        int rc = this->fill ();
        if (rc <= 0) return -1;
    }
    char ch = this->rbuff[this->rnext++];
    return (unsigned int) (__uint8_t) ch;
}

char *FILE::gets (char *buf, int size)
{
    if (-- size < 0) return NULL;

    char *ptr = buf;
    while (-- size >= 0) {
        if (this->rnext >= this->rused) {
            int rc = this->fill ();
            if (rc <= 0) {
                if (ptr > buf) break;
                return NULL;
            }
        }
        char ch = this->rbuff[this->rnext++];
        *(ptr ++) = ch;
        if (ch == '\n') break;
    }
    *ptr = 0;

    return buf;
}

int FILE::putc (int chr)
{
    if (this->wused >= this->wsize) {
        int rc = this->flush ();
        if (rc < 0) return -1;
    }
    this->wbuff[this->wused++] = (char) chr;
    return this->maybeflush ();
}

int FILE::puts (char const *buf)
{
    return this->put (buf, strlen (buf));
}

int FILE::put (char const *buf, int len)
{
    while (len > 0) {
        int room = this->wsize - this->wused;
        if (room <= 0) {
            int rc = this->flush ();
            if (rc < 0) return -1;
            room = this->wsize - this->wused;
        }
        if (room > len) room = len;
        memcpy (this->wbuff + this->wused, buf, room);
        this->wused += room;
        len -= room;
    }
    return this->maybeflush ();
}

int FILE::close ()
{
    return 0;
}

int FILE::read (char *buf, int len)
{
    return 0;
}

int FILE::write (char *buf, int len)
{
    return 0;
}

int FILE::maybeflush ()
{
    switch (this->wmode) {
    case _IOFBF:
        return 0;
    case _IOLBF:
        if (memchr (this->wbuff + this->wchkd, '\n', this->wused - this->wchkd) == NULL) {
            this->wchkd = this->wused;
            return 0;
        }
    }
    return this->flush ();
}

// read more into read buffer (rbuff)
// returns number of characters read (or 0/-1 for eof/error)
int FILE::fill ()
{
    if (this->rbuff == NULL) {
        this->rsize = BUFSIZ;
        this->rbuff = malloc (BUFSIZ);
    }
    if (this->rnext >= this->rused) {
        this->rused  = 0;
    } else {
        this->rused -= this->rnext;
        memmove (this->rbuff, this->rbuff + this->rnext, this->rused);
    }
    this->rnext = 0;
    int rc = this->read (this->rbuff + this->rused, this->rsize - this->rused);
    if (rc > 0) {
        this->rused += rc;
    }
    return rc;
}

int FILE::flush ()
{
    if (this->wbuff == NULL) {
        this->wsize = BUFSIZ;
        this->wbuff = malloc (BUFSIZ);
    }
    if (this->wused > 0) {
        int rc;
        for (int i = 0; i < this->wused; i += rc) {
            rc = this->write (this->wbuff + i, this->wused - i);
            if (rc <= 0) {
                return -1;
            }
        }
    }
    this->wused = 0;
    this->wchkd = 0;
    return 0;
}

FDFILE::FDFILE ()
{
    this->fd = -1;
}

int FDFILE::close ()
{
    int rc = close (this->fd);
    this->fd = -1;
    return rc;
}

int FDFILE::read (char *buff, int size)
{
    return read (this->fd, buff, size);
}

int FDFILE::write (char *buff, int size)
{
    return write (this->fd, buff, size);
}
