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

struct Blob {
    __int32_t someint;
    char      somechar;
    __int16_t someshorts[12];
    Blob     *nextinlist;
};

void putc (char chr);
void puts (char const *str);

char buf[80];
int frac = 123 / 4;

int whoratio (int a, int b)
{
    puts ("hello world\n");
    int iyi;
    iyi = sizeof 1145;
    iyi = sizeof (__uint64_t);
    iyi = sizeof buf;
    iyi = sizeof "size incl null";
    iyi = sizeof (Blob);
    for (iyi = 0; iyi < 10U; iyi ++) {
        buf[0] = (char) iyi + '0';
        buf[1] = '\n';
        buf[2] = 0;
        puts (buf);
    }
    for (iyi = 0; iyi < 3; iyi ++) {
        char c;
        c = buf[iyi];
        putc (c);

        static short accum = 34;
        if (! (iyi & 1)) {
            accum += accum;
        }

        switch (iyi) {
            default: puts ("default");
            case 0: puts ("zero\n");
            case 2: puts ("two\n"); break;
            case 1: puts ("one\n");
            case 3: puts ("three\n"); break;
        }

        puts ((iyi == 1) ? "singular\n" : "plural\n");

        puts ("end of loop\n");
    }
    return a / b;
}
