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
/**
 * Numeric constants in source code.
 */

import java.io.PrintStream;

public class Const extends OpndRhs {
    public Token.Int token;

    //// GENERATION ////

    @Override
    public Network generate (GenCtx genctx, int rbit)
    {
        return genctx.nets.get ((((token.value >> rbit) & 1) == 0) ? "GND" : "VCC");
    }

    @Override
    public void printProp (PropPrinter pp, int rbit)
    {
        pp.println ("Const", this, rbit);
    }

    //// SIMULATION ////

    @Override
    protected int busWidthWork ()
    {
        if (token.width > 0) return token.width;
        for (int w = 1; w < 32; w ++) {
            if (token.value >>> w == 0) return w;
        }
        return 32;
    }

    @Override
    protected void stepSimWork (int t)
    {
        setSimOutput (t, (token.value << 32) | ((~token.value) & 0xFFFFFFFFL));
    }
}
