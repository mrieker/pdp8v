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

#include <complex.h>
#include <math.h>
#include <stdio.h>

int main ()
{
    double w_minus = 2 * M_PI * 2.8e6;
    double w_plus  = 2 * M_PI * 7.0e6;
    double C  = 150e-12;
    double C1 = 5e-12;
    double L  = 3.45e-6;
    double R  = 1;

    Complex y_w_plus = Complex::make (R * C / L, 0.0);

    printf ("y_w_plus = %lg + j %lg = %lg ang %lg deg\n", y_w_plus.real, y_w_plus.imag, y_w_plus.abs (), y_w_plus.ang () * 180.0 / M_PI);

    Complex y_w_minus_numer = Complex::make (1 - w_minus * w_minus, w_minus * R * C);
    Complex y_w_minus_denom = Complex::make (R, w_minus * L);
    Complex y_w_minus = y_w_minus_numer.div (y_w_minus_denom);

    printf ("y_w_minus = %lg + j %lg = %lg ang %lg deg\n", y_w_minus.real, y_w_minus.imag, y_w_minus.abs (), y_w_minus.ang () * 180.0 / M_PI);

    //Complex numer = Complex::make (1, 0).sub (Complex::make (0, 1).mul (y_w_plus).div  (Complex::make (w_plus * C1,  0)));
    //Complex denom = Complex::make (1, 0).sub (Complex::make (0, 1).mul (y_w_minus).div (Complex::make (w_minus * C1, 0)));

    Complex numer = Complex::make (1, 0).sub (Complex::make (0, 1.0 / w_plus  / C1).mul (y_w_plus));
    Complex denom = Complex::make (1, 0).sub (Complex::make (0, 1.0 / w_minus / C1).mul (y_w_minus));

    printf ("numer = %lg + j %lg  denom = %lg + j %lg\n", numer.real, numer.imag, denom.real, denom.imag);

    Complex ratio = numer.div (denom);

    printf ("ratio = %lg + j %lg = %lg ang %lg deg\n", ratio.real, ratio.imag, ratio.abs (), ratio.ang () * 180.0 / M_PI);

    printf ("  abs = %lg\n", ratio.abs ());

    return 0;
}
