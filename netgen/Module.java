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
 * One of these per module, builtin or user defined.
 */

import java.io.PrintStream;
import java.util.LinkedList;
import java.util.TreeMap;

public abstract class Module {
    public interface GetCircuit {
        void getCircuit (GenCtx genctx);
    }
    public LinkedList<GetCircuit> connectors;   // connectors in this module (make sure they get placed on pcb)
    public OpndLhs[] params;                    // parameters
    public TreeMap<String,OpndLhs> variables;   // variables (parameters, wires)
    public LinkedList<OpndLhs> generators;      // also generate these elements as well as output parameters
    public String name;                         // module name

    public Module ()
    {
        generators = new LinkedList<> ();
        variables  = new TreeMap<> ();
        connectors = new LinkedList<> ();
    }

    // this can get tokens after the module name and before the (
    // eg, inst : DFF 16 ( ... );
    //                ^ token is here
    //                   ^ return token here
    // source-defined modules do not have any config tokens
    public Token getInstConfig (Token token, String instvarsuf, Object[] instconfig_r)
            throws Exception
    {
        return token;
    }

    // make an array of wires corresponding to instantiated module and insert into target module
    // caller can then use them in assignment-like constructs to match with arguments
    public abstract OpndLhs[] getInstParams (UserModule target, String instvarsuf, Object instconfig)
            throws Exception;

    // get whether module parameter is an 'input' or an 'output' parameter
    //  input:
    //   instconfig = as returned by getInstConfig()
    //   prmn = parameter number
    //  output:
    //   returns true: IN
    //          false: OC,OUT
    public boolean isInstParamIn (Object instconfig, int prmn)
    {
        OpndLhs param = params[prmn];
        assert param instanceof IParam || param instanceof OParam;
        return param instanceof IParam;
    }
}
