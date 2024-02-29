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

void putstr (char const *str);
void putptr (void *ptr);

struct P1 {
    double dbl1;
    int p1;
    virtual void f1 () = 0;
};

struct P2 {
    float flt2;
    int p2;
    virtual void f2 ();
};

struct Ch : P1, P2 {
    Ch ();
    virtual ~Ch ();
    int ch;
    virtual void fc ();
    virtual void f1 ();
    virtual void f2 ();
};

Ch::Ch ()
{
    putstr ("constructing a Ch\n");
    putptr (this);
}

Ch::~Ch ()
{
    putstr ("destructing a Ch\n");
}

void Ch::fc ()
{
    putstr ("this is Ch::fc\n");
}

void Ch::f1 ()
{
    putstr ("this is Ch::f1\n");
}

/*void P1::f1 ()
{
    putstr ("this is P1::f1\n");
}*/

Ch staticch;
//P1 staticp1;

Ch *test24 (bool x)
{
    if (x) {
        Ch stackch[7];
        return &stackch[3];
    }

    Ch single;
    return &single;
}
