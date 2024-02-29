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

#include <math.h>

CType CType::make (CData r, CData i)
{
    CType c;
    c.real = r;
    c.imag = i;
    return c;
}

CType CType::add (CType x)
{
    CType y;
    y.real = this->real + x.real;
    y.imag = this->imag + x.imag;
    return y;
}

CType CType::sub (CType x)
{
    CType y;
    y.real = this->real - x.real;
    y.imag = this->imag - x.imag;
    return y;
}

//  (t.real + j * t.imag) * (x.real + j * x.imag) = (t.real * x.real - t.imag * x.imag) + j * (t.real * x.imag + t.imag * x.real)
CType CType::mul (CType x)
{
    CType y;
    y.real = this->real * x.real - this->imag * x.imag;
    y.imag = this->real * x.imag + this->imag * x.real;
    return y;
}

//  t.real + j * t.imag   (t.real + j * t.imag) * (x.real - j * x.imag)
//  ------------------- = ---------------------------------------------
//  x.real + j * x.imag             x.real ** 2 + x.imag ** 2
CType CType::div (CType x)
{
    CData d = x.real * x.real + x.imag * x.imag;
    CType y;
    y.real = (this->real * x.real + this->imag * x.imag) / d;
    y.imag = (this->imag * x.real - this->real * x.imag) / d;
    return y;
}

// 1 / (1 / this + 1 / x))
CType CType::par (CType x)
{
    CType n = this->mul (x);
    CType d = this->add (x);
    return n.div (d);
}

CType CType::mulr (CData x)
{
    CType y;
    y.real = this->real * x;
    y.imag = this->imag * x;
    return y;
}

CType CType::divr (CData x)
{
    CType y;
    y.real = this->real / x;
    y.imag = this->imag / x;
    return y;
}

CType CType::neg ()
{
    CType y;
    y.real = - this->real;
    y.imag = - this->imag;
    return y;
}

// 1.0 / this
CType CType::rec ()
{
    CData d = this->real * this->real + this->imag * this->imag;
    CType y;
    y.real =   this->real / d;
    y.imag = - this->imag / d;
    return y;
}

CData CType::abs ()
{
    if (this->imag == 0) return CMath (fabs) (this->real);
    if (this->real == 0) return CMath (fabs) (this->imag);
    return CMath (sqrt) (this->real * this->real + this->imag * this->imag);
}

CData CType::arg ()
{
    return CMath (atan2) (this->imag, this->real);
}

CType CType::sq ()
{
    CType y;
    y.real = this->real * this->real - this->imag * this->imag;
    y.imag = this->real * this->imag * 2;
    return y;
}

void CType::addeq (CType x)
{
    this->real += x.real;
    this->imag += x.imag;
}

void CType::subeq (CType x)
{
    this->real -= x.real;
    this->imag -= x.imag;
}

//  (t.real + j * t.imag) * (x.real + j * x.imag) = (t.real * x.real - t.imag * x.imag) + j * (t.real * x.imag + t.imag * x.real)
void CType::muleq (CType x)
{
    CData r = this->real * x.real - this->imag * x.imag;
    this->imag = this->real * x.imag + this->imag * x.real;
    this->real = r;
}

//  t.real + j * t.imag   (t.real + j * t.imag) * (x.real - j * x.imag)
//  ------------------- = ---------------------------------------------
//  x.real + j * x.imag             x.real ** 2 + x.imag ** 2
void CType::diveq (CType x)
{
    CData d = x.real * x.real + x.imag * x.imag;
    CData r = (this->real * x.real + this->imag * x.imag) / d;
    this->imag = (this->imag * x.real - this->real * x.imag) / d;
    this->real = r;
}

// (a + ib)**(c + id) = e**((ln r + it)*(c + id))
//  where r = abs a+ib;  t = atan b/a
CType CType::pow (CType x)
{
    if ((x.imag == 0) && (this->imag == 0)) {
        CData r = CMath (pow) (this->real, x.real);
        if (! CMath (isnan) (r)) return CType::make (r, 0);
    }

    CData lnr = CMath (log) (this->abs ());
    CData t   = this->arg ();
    CType expon;
    expon.real = lnr * x.real - t * x.imag;
    expon.imag = t * x.real + lnr * x.imag;
    return expon.exp ();
}

// http://scipp.ucsc.edu/~haber/archives/physics116A10/arc_10.pdf
//  acos z = 1 / i * ln (z + i * sqrt (abs (1 - z**2)) * (e ** i/2*arg(1-z**2)))
//         =   - i * ln (z + i * sqrt (abs (1 - z**2)) * (e ** i/2*arg(1-z**2)))
CType CType::acos ()
{
    if (this->imag == 0) {
        return CType::make (CMath (acos) (this->real), 0);
    }

    CType epow, expon, isqrtepow, one_zsq, zsq;
    zsq.real = this->real * this->real - this->imag * this->imag;
    zsq.imag = this->real * this->imag * 2;
    one_zsq.real = 1 - zsq.real;
    one_zsq.imag =   - zsq.imag;
    CData sqrt_abs_one_zsq = CMath (sqrt) (one_zsq.abs ());
    CData arg_one_zsq = one_zsq.arg ();
    expon.real = 0;
    expon.imag = arg_one_zsq / 2;
    epow = expon.exp ();
    isqrtepow.real = - sqrt_abs_one_zsq * epow.imag;
    isqrtepow.imag =   sqrt_abs_one_zsq * epow.real;
    CType ln = this->add (isqrtepow).log ();
    return CType::make (ln.imag, - ln.real);
}

// http://scipp.ucsc.edu/~haber/archives/physics116A10/arc_10.pdf
//  asin z = pi/2 - acos z
CType CType::asin ()
{
    if (this->imag == 0) {
        return CType::make (CMath (asin) (this->real), 0);
    }

    CType acosz = this->acos ();
    CType asinz;
    asinz.real = ((CData)M_PI_2) - acosz.real;
    asinz.imag =                 - acosz.imag;
    return asinz;
}

// http://scipp.ucsc.edu/~haber/archives/physics116A10/arc_10.pdf
//  atan z = 1 / 2i * ln ((i - z) / (i + z))
//         = -0.5 i * ln ((i - z) / (i + z))
CType CType::atan ()
{
    if (this->imag == 0) {
        return CType::make (CMath (atan) (this->real), 0);
    }

    CType num, den, log;
    num.real = 0 - this->real;
    num.imag = 1 - this->imag;
    den.real = 0 + this->real;
    den.imag = 1 + this->imag;
    log = num.div (den).log ();
    return CType::make (0.5 * log.imag, -0.5 * log.real);
}

// http://mathonline.wikidot.com/the-complex-cosine-and-sine-functions
//  cos z = (e**iz + e**-iz) / 2
CType CType::cos ()
{
    if (this->imag == 0) {
        return CType::make (CMath (cos) (this->real), 0);
    }

    CType iz     = CType::make (- this->imag, this->real);
    CType e__iz  = iz.exp ();
    CType e___iz = e__iz.rec ();
    CType sum    = e__iz.add (e___iz);
    return CType::make (sum.real * 0.5, sum.imag * 0.5);
}

// http://mathonline.wikidot.com/the-complex-exponential-function
//  e**z = e**(x+yi) = e**x * (cos y + i sin y)
CType CType::exp ()
{
    CData e__x = CMath (exp) (this->real);
    if (this->imag == 0) {
        return CType::make (e__x, 0);
    }
    return CType::make (e__x * CMath (cos) (this->imag), e__x * CMath (sin) (this->imag));
}

// http://mathonline.wikidot.com/the-complex-natural-logarithm-function
//  log z = log |z| + i arg z
CType CType::log ()
{
    return CType::make (CMath (log) (this->abs ()), this->arg ());
}

// http://mathonline.wikidot.com/the-complex-cosine-and-sine-functions
//  sin z = (e**iz - e**-iz) / 2i = -0.5 i (e**iz - e**-iz)
CType CType::sin ()
{
    if (this->imag == 0) {
        return CType::make (CMath (sin) (this->real), 0);
    }

    CType iz     = CType::make (- this->imag, this->real);
    CType e__iz  = iz.exp ();
    CType e___iz = e__iz.rec ();
    CType diff   = e__iz.sub (e___iz);
    return CType::make (diff.imag * 0.5, diff.real * -0.5);
}

CType CType::sqrt ()
{
    if (this->imag == 0) {
        CData sqr = CMath (sqrt) (CMath (fabs) (this->real));
        return (this->real < 0) ? CType::make (0, sqr) : CType::make (sqr, 0);
    }

    CData mag = CMath (sqrt) (this->abs ());
    CData arg = this->arg () / 2;
    return CType::make (mag * CMath (cos) (arg), mag * CMath (sin) (arg));
}

// tan z = sin z / cos z = -0.5 i (e**iz - e**-iz) / [ (e**iz + e**-iz) / 2 ]
//                       = -i (e**iz - e**-iz) / (e**iz + e**-iz)
CType CType::tan ()
{
    if (this->imag == 0) {
        return CType::make (CMath (tan) (this->real), 0);
    }

    CType iz     = CType::make (- this->imag, this->real);
    CType e__iz  = iz.exp ();
    CType e___iz = e__iz.rec ();
    CType quot   = (e__iz.sub (e___iz)).div (e__iz.add (e___iz));
    return CType::make (quot.imag, - quot.real);
}
