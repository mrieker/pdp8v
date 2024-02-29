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

void puts (char const *msg);

struct Blob {
    __int32_t someint;
    char      somechar;
    __int16_t someshorts[12];
    Blob     *nextinlist;
};

int whoratio (int a, int b)
{
    puts ("hello world\n");

    Blob blobvar;

    blobvar.somechar = (char) a;
    blobvar.someshorts[1] = b;

    Blob *blobptr;

    blobptr = &blobvar;

    __int32_t  a = blobvar.someint;
    __int8_t   b = blobvar.somechar;
    __int16_t *c = blobvar.someshorts;
    __int16_t  d = blobvar.someshorts[3];
    Blob      *e = blobvar.nextinlist;

    __int32_t  f = blobptr->someint;
    __int8_t   g = blobptr->somechar;
    __int16_t *h = blobptr->someshorts;
    __int16_t  i = blobptr->someshorts[3];
    Blob      *j = blobptr->nextinlist;

    return blobvar.somechar;
}
