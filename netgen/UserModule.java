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
 * One of these per module in source code.
 * Also gets extended by BIModule each built-in module.
 */

import java.io.PrintStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.TreeMap;

public class UserModule extends Module {
    public Token tokstart;  // starting '{' in source code

    // make an array of wires corresponding to instantiated module and insert into target module
    // caller can then use them in assignment-like constructs to match with arguments
    @Override
    public OpndLhs[] getInstParams (UserModule target, String instvarsuf, Object instconfig)
            throws Exception
    {
        OpndLhs[] paramwires = new OpndLhs[params.length];
        for (int i = 0; i < params.length; i ++) {
            OpndLhs oldparam  = params[i];
            OpndLhs paramwire = new OpndLhs ();
            paramwire.name    = oldparam.name + instvarsuf;
            paramwire.lodim   = oldparam.lodim;
            paramwire.hidim   = oldparam.hidim;
            paramwires[i]     = paramwire;
            assert target.variables.get (paramwire.name) == null;
            target.variables.put (paramwire.name, paramwire);
        }
        return paramwires;
    }
}
