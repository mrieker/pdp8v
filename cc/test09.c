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

struct SomeStruct {
    int intmema;
    int intmemb;
    char const strmem[20];
};

SomeStruct *firststruct;
SomeStruct **laststruct = &firststruct;

char const *staticstringpointer = "static string constant";

int anarray[10];
int *apointer = anarray + 7;
int *bpointer = &anarray[3];

int adiff = &anarray[7] - &anarray[4];

int somefunc ()
{
    char stringinit[6] = "abc";

    int arrayinit[10] = { [0] = 99, [5] = 10, 11, [7] = 14, [3] = 6 };

    SomeStruct somestr = { .intmemb = 22, .intmema = 11, .strmem = "struct string" };

    int i, j;
    for (i = 0; i < 10; i ++) {
        j = arrayinit[i];
    }

    return stringinit[0];
}
