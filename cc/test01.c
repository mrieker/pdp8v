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

void puts (char const *str);
void printdouble (double d);

char buf[80];

double whoratio (int a, int b)
{
    int iyi;
    for (iyi = 0; iyi < 10; iyi ++) {
        buf[0] = (char) iyi + '0';
        buf[1] = '\n';
        buf[2] = 0;
        puts (buf);
    }
    double d;
    d = 0.1;
    puts ("hello world\n");
    return a / (double) b;
}

void callwhore ()
{
    double d = whoratio (5, 9);
    printdouble (d);

    printdouble (whoratio (7, 9));
}
