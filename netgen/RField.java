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
 * Access a read-only view of sub-field of an operand.
 */

import java.io.PrintStream;

public class RField extends OpndRhs {
    public OpndLhs refdopnd;    // operand being accessed
    public int lodim;           // bits within that operand
    public int hidim;

    //// GENERATION ////

    @Override
    public Network generate (GenCtx genctx, int rbit)
    {
        return refdopnd.generate (genctx, lodim - refdopnd.lodim + rbit);
    }

    @Override
    public void printProp (PropPrinter pp, int rbit)
    {
        refdopnd.printProp (pp, lodim - refdopnd.lodim + rbit);
    }

    //// SIMULATION ////

    @Override
    protected int busWidthWork ()
    {
        assert hidim >= lodim;
        return hidim - lodim + 1;
    }

    @Override
    protected void stepSimWork (int t)
            throws HaltedException
    {
        assert lodim >= refdopnd.lodim;
        long in = refdopnd.getSimOutput (t);
        setSimOutput (t, in >>> (lodim - refdopnd.lodim));
    }
}
