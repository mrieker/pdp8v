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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "fdfile.h"

FILE *stdin;
FILE *stdout;
FILE *stderr;

FILE *fdopen (int fd, char const *mode)
{
    FDFILE *stream = new FDFILE;
    stream->fd = fd;
    return stream;
}

FILE *fopen (char const *path, char const *mode)
{
    int flags = 0;
    for (char c; (c = *mode) != 0; mode ++) {
        switch (c) {
            case 'a': flags = O_APPEND | O_CREAT | O_WRONLY; break;
            case 'b': break;
            case 'r': flags = O_RDONLY; break;
            case 'w': flags = O_CREAT | O_TRUNC | O_WRONLY; break;
            case '+': flags = (flags & ~ O_ACCMODE) | O_RDWR; break;
            default: {
                errno = EINVAL;
                return NULL;
            }
        }
    }

    int fd = open (path, flags, 0666);
    if (fd < 0) return NULL;
    return fdopen (fd, mode);
}

int fclose (FILE *stream)
{
    int rc = stream->fclose ();
    delete stream;
    return rc;
}
