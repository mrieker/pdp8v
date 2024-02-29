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

#ifndef _READLINE_H
#define _READLINE_H

struct ReadLine {
    ReadLine ();
    virtual ~ReadLine ();
    int open (int fd);
    void close ();
    char *read (char const *prompt);
    void save (char *linebuff);

    char **histlines;
    int fd;
    int histalloc;
    int histcount;

    int newhistline (int histline, char **linebuff_r, int lineposn, int lineused);
    void writeall (char const *buf, int len);
    void writebks (int size);
    void writespc (int size);
};

#endif
