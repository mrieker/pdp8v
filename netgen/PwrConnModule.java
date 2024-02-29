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
 * Built-in Power Connector module.
 * Provides a connector that connects to power networks.
 * Power networks are created when components are placed that connect to them,
 * eg, StandCell places a resistor that connects to VCC and GND networks, so it
 * creates the needed power networks.
 */

import java.util.ArrayList;

public class PwrConnModule extends Module implements Module.GetCircuit {
    private Comp.Conn connector;    // corresponding hardware component
    private String[] netnames;      // names of the power networks
    private String instvarsuf;      // instantiation name "/instname"

    public PwrConnModule (String instvarsuf, String[] netnames)
    {
        name = "PwrConn" + instvarsuf;
        this.instvarsuf = instvarsuf;
        this.netnames = netnames;

        // no parameters for this module
        params = new OpndLhs[0];
    }

    // this gets the connector configuration
    // eg, inst : PwrConn netnames ... ();
    @Override
    public Token getInstConfig (Token token, String instvarsuf, Object[] instconfig_r)
            throws Exception
    {
        ArrayList<String> netnames = new ArrayList<> ();
        for (String sym; (sym = token.getSym ()) != null; token = token.next) {
            netnames.add (sym);
        }
        if (netnames.size () == 0) throw new Exception ("PwrConn module instantiation must have net names");
        instconfig_r[0] = netnames.toArray (new String[0]);
        return token;
    }

    // make a new power connector module instance
    // the network names are in instconfig as built by getInstConfig()
    @Override
    public OpndLhs[] getInstParams (UserModule target, String instvarsuf, Object instconfig)
            throws Exception
    {
        String[] netnames = (String[]) instconfig;
        PwrConnModule conninst = new PwrConnModule (instvarsuf, netnames);

        // make connector's pin variables available to target module
        target.variables.putAll (conninst.variables);

        // make sure connector gets placed on pcb (as it probably just has power pins)
        target.connectors.add (conninst);

        // add output pins to list of top-level things that need to have circuits generated
        // this is how we guarantee circuits that drive these pins will be generated
        for (OpndLhs param : conninst.params) {
            if (param.name.startsWith ("O")) {
                target.generators.add (param);
            }
        }

        return conninst.params;
    }

    // get whether module parameter is an 'input' or an 'output' parameter
    //  input:
    //   instconfig = as returned by getInstConfig()
    //   prmn = parameter number
    //  output:
    //   returns true: IN
    //          false: OC,OUT
    @Override
    public boolean isInstParamIn (Object instconfig, int prmn)
    {
        return false;
    }

    //// GENERATION ////

    // generate circuit for the connector
    //  output:
    //   returns with connector placed on board, pins wired up, pinnets filled in
    public void getCircuit (GenCtx genctx)
    {
        if (connector != null) return;

        int npins = netnames.length;
        connector = new Comp.Conn (genctx, genctx.conncol, "Conn" + instvarsuf, 1, npins);
        genctx.addPlaceable (connector);

        for (int i = 0; i < npins; i ++) {
            String netname = netnames[i];
            Network network = genctx.nets.get (netname);
            if (network == null) {
                network = new Network (genctx, netname);
                genctx.nets.put (netname, network);
            }
            network.addConn (connector, Integer.toString (i + 1));
        }
    }
}
