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
#ifndef _STDIO_H
#define _STDIO_H

#define EOF (-1)
#ifndef NULL
#define NULL ((void *)0)
#endif

#define _IONBF 0
#define _IOLBF 1
#define _IOFBF 2

#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2

#define fgetc(stream) ((stream)->getc ())
#define fgets(buf,size,stream) ((stream)->gets (buf, size))
#define fputc(chr,stream) ((stream)->putc (chr))
#define fputs(buf,stream) ((stream)->puts (buf))
#define fflush(stream) ((stream)->flush ())
#define fseek(stream,offset,whence) ((stream)->seek (offset, whence))
#define ftell(stream) ((stream)->tell ())
#define rewind(stream) ((stream)->_rewind ())

struct FILE {
    FILE ();
    virtual ~FILE ();

    int fclose ();
    int getc ();
    char *gets (char *buf, int size);
    int putc (int chr);
    int puts (char const *buf);
    int fill ();
    int flush ();

    int   wsize;
    int   wused;
    char *wbuff;
    int   wmode;
    int   wchkd;
    int   rsize;
    int   rused;
    int   rnext;
    char *rbuff;

    int maybeflush ();

    virtual int put (char const *buf, int len);

    virtual int close ();
    virtual int read (char *buff, int size);
    virtual int write (char *buff, int size);
};

extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

FILE *fopen (char const *path, char const *mode);
FILE *fdopen (int fd, char const *mode);
int fclose (FILE *stream);

int printf (char const *fmt, ...);
int dprintf (int fd, char const *fmt, ...);
int fprintf (FILE *stream, char const *fmt, ...);
int sprintf (char *buf, char const *fmt, ...);
int snprintf (char *buf, int len, char const *fmt, ...);
int asprintf (char **buf_r, char const *fmt, ...);

#include <stdarg.h>

int vprintf (char const *fmt, va_list ap);
int vdprintf (int fd, char const *fmt, va_list ap);
int vfprintf (FILE *stream, char const *fmt, va_list ap);
int vsprintf (char *buf, char const *fmt, va_list ap);
int vsnprintf (char *buf, int len, char const *fmt, va_list ap);
int vasprintf (char **buf_r, char const *fmt, va_list ap);

#endif
