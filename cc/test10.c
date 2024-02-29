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

typedef int IntToInt (int x);

int negate (int x)
{
    return -x;
}

int complement (int x)
{
    return ~x;
}

IntToInt increment
{
    return x + 1;
}

IntToInt decrement
{
    return x - 1;
}

IntToInt *funcpointer = complement;

int intarray[10];

int main ()
{
    int total = 0;
    int i;
    int *p = &intarray[3];
    for (i = 3; i < 10; i += 1) {
        total += *(p ++);
    }
    return 0;
}
