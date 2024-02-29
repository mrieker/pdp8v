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
#include <errno.h>
#include <hode.h>
#include <readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

ReadLine::ReadLine ()
{
    this->fd = -1;
    this->histalloc = 0;
    this->histcount = 0;
    this->histlines = NULL;
}

ReadLine::~ReadLine ()
{
    this->close ();
}

int ReadLine::open (int fd)
{
    int rc = setttyecho (fd, 0);
    if (rc >= 0) this->fd = fd;
    return rc;
}

void ReadLine::close ()
{
    setttyecho (this->fd, 1);
    for (int i = this->histcount; -- i >= 0;) {
        free (this->histlines[i]);
    }
    free (this->histlines);
    this->fd = -1;
    this->histalloc = 0;
    this->histcount = 0;
    this->histlines = NULL;
}

char *ReadLine::read (char const *prompt)
{
    int histindex = this->histcount;

    char *linebuff = malloc (64);
    linebuff[0]  = 0;
    int lineused = 0;
    int lineposn = 0;

    this->writeall (prompt, strlen (prompt));

    while (1) {
        char ch;
        int rc = read (this->fd, &ch, 1);
        if (rc <= 0) break;

        switch (ch) {

            // beginning of line
            case 'A'-'@': {
                this->writebks (lineposn);
                lineposn = 0;
                break;
            }

            // backup one char
            lfarrow:
            case 'B'-'@': {
                if (lineposn > 0) {
                    this->writebks (1);
                    -- lineposn;
                }
                break;
            }

            // return 'end of file'
            case 'D'-'@': {
                this->writebks (lineposn);
                this->writespc (lineposn);
                this->writebks (lineposn);
                goto goteof;
            }

            // end of line
            case 'E'-'@': {
                this->writeall (linebuff + lineposn, lineused - lineposn);
                lineposn = lineused;
                break;
            }

            // forward one char
            rtarrow:
            case 'F'-'@': {
                if (lineposn < lineused) {
                    this->writeall (linebuff + lineposn, 1);
                    lineposn ++;
                }
                break;
            }

            // all done
            case 'J'-'@':
            case 'M'-'@': {
                this->writeall ("\r\n", 2);

                assert (lineused < allocsz (linebuff));
                linebuff[lineused] = 0;
                linebuff = realloc (linebuff, lineused + 1);
                return linebuff;
            }

            // next line in history
            dnarrow:
            case 'N'-'@': {
                if (histindex < this->histcount) {
                    lineposn = lineused = this->newhistline (++ histindex, &linebuff, lineposn, lineused);
                }
                break;
            }

            // previous line in history
            uparrow:
            case 'P'-'@': {
                if (histindex > 0) {
                    lineposn = lineused = this->newhistline (-- histindex, &linebuff, lineposn, lineused);
                }
                break;
            }

            // delete all before cursor
            case 'U'-'@': {
                if (lineposn > 0) {
                    this->writebks (lineposn);
                    this->writeall (linebuff + lineposn, lineused - lineposn);
                    this->writespc (lineposn);
                    this->writebks (lineused);
                    lineused -= lineposn;
                    memmove (linebuff, linebuff + lineposn, lineused);
                    lineposn  = 0;
                }
                break;
            }

            // delete char before cursor
            case 'H'-'@':
            case 127: {
                if (lineposn > 0) {
                    this->writebks (1);
                    this->writeall (linebuff + lineposn, lineused - lineposn);
                    this->writespc (1);
                    this->writebks (lineused - lineposn + 1);
                    memmove (linebuff + lineposn - 1, linebuff + lineposn, lineused - lineposn);
                    -- lineposn;
                    -- lineused;
                }
                break;
            }

            case 033: {
                char es[2];
                rc = read (this->fd, es, 2);
                if ((rc == 1) && (es[0] == '[')) {
                    rc = read (this->fd, es + 1, 1);
                }
                if ((rc > 0) && (es[0] == '[')) {
                    switch (es[1]) {
                        case 'A': goto uparrow;
                        case 'B': goto dnarrow;
                        case 'C': goto rtarrow;
                        case 'D': goto lfarrow;
                    }
                }
                break;
            }

            // insert char at cursor
            default: {
                if (ch >= ' ') {
                    int linesize = allocsz (linebuff);
                    if (lineused + 2 >= linesize) {
                        linesize += linesize / 2;
                        linebuff  = realloc (linebuff, linesize);
                    }
                    memmove (linebuff + lineposn + 1, linebuff + lineposn, lineused - lineposn);
                    lineused ++;
                    linebuff[lineposn] = ch;
                    this->writeall (linebuff + lineposn, lineused - lineposn);
                    lineposn ++;
                    this->writebks (lineused - lineposn);
                }
                break;
            }
        }
    }
goteof:;
    free (linebuff);
    return NULL;
}

void ReadLine::save (char *linebuff)
{
    if (this->histcount >= this->histalloc) {
        this->histalloc += this->histalloc / 2 + 2;
        assert (this->histcount < this->histalloc);
        this->histlines  = realloc (this->histlines, this->histalloc * sizeof *this->histlines);
    }
    this->histlines[this->histcount++] = linebuff;
}

int ReadLine::newhistline (int histindex, char **linebuff_r, int lineposn, int lineused)
{
    int hll = 0;
    if (histindex < this->histcount) {
        char *hlb = this->histlines[histindex];
        hll = strlen (hlb);
        char *linebuff = *linebuff_r;
        int linesize = allocsz (linebuff);
        if (linesize < hll + 1) {
            linesize = hll + 1;
            *linebuff_r = linebuff = realloc (linebuff, linesize);
        }
        memcpy (linebuff, hlb, hll);
        this->writebks (lineposn);
        this->writeall (hlb, hll);
        this->writespc (lineused - hll);
        this->writebks (lineused - hll);
    } else {
        this->writebks (lineposn);
        this->writespc (lineposn);
        this->writebks (lineposn);
    }
    return hll;
}

void ReadLine::writebks (int size)
{
    static char const bsbuff[] = "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b";

    while (size > 0) {
        int rc = write (this->fd, bsbuff, (size > 16) ? (int) 16 : size);
        if (rc <= 0) throw "ReadLine:writebks: error writing";
        size -= rc;
    }
}

void ReadLine::writespc (int size)
{
    static char const spbuff[] = "                ";

    while (size > 0) {
        int rc = write (this->fd, spbuff, (size > 16) ? (int) 16 : size);
        if (rc <= 0) throw "ReadLine:writespc: error writing";
        size -= rc;
    }
}

void ReadLine::writeall (char const *buf, int len)
{
    while (len > 0) {
        int rc = write (this->fd, buf, len);
        if (rc <= 0) throw "ReadLine:writeall: error writing";
        buf += rc;
        len -= rc;
    }
}
