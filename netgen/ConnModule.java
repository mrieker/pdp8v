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
 * Built-in Connector module.
 * Provides a connector for sending/receiving signals to/from outside world.
 */

import java.io.PrintStream;
import java.util.LinkedList;

public class ConnModule extends Module implements Module.GetCircuit {
    private Comp.Conn connector;    // corresponding hardware component
    private Config config;          // pin configuration
    private String instvarsuf;      // instantiation name "/instname"

    public ConnModule (String instvarsuf, Config config)
    {
        name = "Conn" + instvarsuf;
        this.instvarsuf = instvarsuf;
        this.config = config;

        if (config == null) {
            params = new OpndLhs[0];
        } else {
            params = config.params;
        }

        // parameters are named {I,O}hwpinname
        //  I params are for 'i' config letters
        //  O params are for 'o' config letters
        // they are indexed by order given in config string,
        //  just including the 'i's and 'o's
        // see getInstConfig()
        for (OpndLhs param : params) {
            if (param instanceof IPinOutput) {
                ((IPinOutput) param).pinmod = this;
            }
            if (param instanceof OPinInput) {
                ((OPinInput) param).pinmod = this;
            }
            variables.put (param.name, param);
        }
    }

    // this gets the connector configuration
    // eg, inst : Conn vviiiooioiiiongg ... [pwrcap] (...);
    //                 ^VCC   ^input ^ground
    //                      ^output ^not-connected
    // one string of letters per row on the connector
    // length of strings gives number of columns in connector
    // all strings must be same length
    //  input pin : signal coming from outside world onto board
    // output pin : signal coming from board going to outside world
    // note:
    //  order of arguments is same as i's and o's in config strings
    //  but parameters are named by the physical pin number, eg
    //  a 2x4 connector config'd with giov viio will be defined:
    //  module Conn giov viio (out I2, in O3, out I6, out I7, in O8) { }
    //        pins: 1234 5678
    @Override
    public Token getInstConfig (Token token, String instvarsuf, Object[] instconfig_r)
            throws Exception
    {
        // configuration string is mandatory
        // also return it as the instantiation parameter
        // one string per row and they must all be of same length
        Config config = new Config ();
        int ncols = 0;
        int nrows = 0;
        String cfgstr = "";
        while (true) {

            // parse pin configuration string
            String sym = token.getSym ();
            if (sym == null) break;
            sym = sym.toLowerCase ();
            switch (sym) {
                case "pwrcap": {
                    config.pwrcap = true;
                    break;
                }
                default: {
                    if (ncols == 0) ncols = sym.length ();
                    else if (sym.length () != ncols) {
                        throw new Exception ("Conn rows must be of equal lengths");
                    }
                    cfgstr += sym;
                    nrows ++;
                    break;
                }
            }
            token = token.next;
        }
        if (cfgstr.equals ("")) throw new Exception ("Conn module instantiation must have configuration");
        if ((ncols < Comp.Conn.MINCOLS) || (ncols > Comp.Conn.MAXCOLS) || (nrows < Comp.Conn.MINROWS) || (nrows > Comp.Conn.MAXROWS)) {
            throw new Exception ("Conn ncols=" + ncols + ", nrows=" + nrows + " out of range, see Comp.Conn");
        }

        config.cfgstr = cfgstr;
        config.nrows  = nrows;
        instconfig_r[0] = config;

        // parse the string and create a wire for each 'i' or 'o'
        int cfglen = cfgstr.length ();
        LinkedList<OpndLhs> params = new LinkedList<> ();
        for (int cfgidx = 0; cfgidx < cfglen; cfgidx ++) {
            String pino = config.pinNumberFromCfgIdx (cfgidx);
            char c = cfgstr.charAt (cfgidx);
            switch (c) {

                // input pin means a pin that the outside world sends a signal to the board on
                // - from the perspective of a little circuit Module plugged into this connector,
                //   it is an output from that little circuit Module, just like there was a
                //   D FlipFlop wire-wrapped to the connector, this pin would be used for the
                //   Q or ~Q output of the flipflop
                case 'i': {
                    params.add (new IPinOutput ("I" + pino + instvarsuf, params.size ()));
                    break;
                }

                // output pin means a pin that the board sends a signal out to the outside world on
                // - from the perspective of a little circuit Module plugged into this connector,
                //   it is an input to that little circuit Module, just like there was a D FlipFlop
                //   wire-wrapped to the connector, this pin would be used for the D or T input to
                //   the flipflop
                case 'o': {
                    params.add (new OPinInput ("O" + pino + instvarsuf));
                    break;
                }

                // ground, not-connected, vcc do not have parameters
                case 'g':
                case 'n':
                case 'v': break;

                // garbazhe
                default: {
                    throw new Exception ("bad character " + c + " in configuration " + cfgstr);
                }
            }
        }

        config.params = params.toArray (new OpndLhs[0]);

        return token;
    }

    // make a new connector module instance with wires for parameters
    // the parameters names are suffixed by instvarsuf
    // the pin configuration is in instconfig as returned by getInstConfig()
    @Override
    public OpndLhs[] getInstParams (UserModule target, String instvarsuf, Object instconfig)
            throws Exception
    {
        ConnModule conninst = new ConnModule (instvarsuf, (Config) instconfig);

        // make connector's pin variables available to target module
        target.variables.putAll (conninst.variables);

        // make sure connector gets placed on pcb (maybe it just has power pins)
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
        Config config = (Config) instconfig;
        OpndLhs param = config.params[prmn];

        assert param.name.startsWith ("I") || param.name.startsWith ("O");

        // pin configured as 'i' (name starting with 'I')
        //  - signal going from connector to board
        //    so it is an output from perspective of external circuit connected to board

        // pin configured as 'o' (name starting with 'O')
        //  - signal going from board to connector
        //    so it is an input from perspective of external circuit connected to board

        return param.name.startsWith ("O");
    }

    // OUTPUT PIN
    // - these route signals from the board out to the outside world
    //   from the perspective of some circuit plugged into the connector,
    //   the circuit reads from the pin, like the D input of a flipflop

    // just like OpndLhs (ie, a plain wire), except it causes the connector
    // to be placed on the circuit board when then output pin is generated
    public static class OPinInput extends OpndLhs {
        public ConnModule pinmod;

        public OPinInput (String name)
        {
            super (name);
        }

        // being called to generate on-board circuitry that drives this pin
        // make sure the connector gets put on the board and the on-board
        //  circuit that drives the pin gets connected to the pin
        @Override
        public Network generate (GenCtx genctx, int rbit)
        {
            pinmod.getCircuit (genctx);
            return super.generate (genctx, rbit);
        }
    }

    // INPUT PIN
    // - these route signals from the outside world onto the board
    //   from the perspective of some circuit plugged into the connector,
    //   the circuit writes to the pin, like the Q output of a flipflop

    public static class IPinOutput extends OParam {
        public ConnModule pinmod;
        public Network pinnet;
        public OpndRhs[] jumpers;

        public IPinOutput (String name, int index)
        {
            super (name, index);
        }

        // don't allow writing to an input pin
        @Override
        public String addWriter (WField wfield)
        {
            return "cannot write to a connector input pin";
        }

        // input pins are good without any writers
        @Override
        public String checkWriters ()
        {
            return null;
        }

        //// GENERATOR ////

        // some circuit on the board is trying to read the signal from the pin,
        // so make a network, connect the network to the pin and return that network
        //  input:
        //   rbit = should always be 0 cuz these pins are scalar
        //  output:
        //   returns network for the pin
        @Override
        public Network generate (GenCtx genctx, int rbit)
        {
            assert rbit == 0;

            // generate circuit for the connector
            pinmod.getCircuit (genctx);

            // return network that has the pin's signal
            // presumably driven by some external circuitry
            return pinnet;
        }

        @Override
        public void printProp (PropPrinter pp, int rbit)
        {
            pp.println ("IPIN", this, rbit);
        }

        //// SIMULATOR ////

        @Override
        public int busWidthWork ()
        {
            return 1;
        }

        @Override
        public boolean forceable ()
        {
            return true;
        }

        // values must be provided via force command or jumper statement
        // if none given, use 'X', ie, undefined signal coming into board on this pin
        @Override
        public void stepSimWork (int t)
                throws HaltedException
        {
            if (jumpers != null) {
                long ival = 0x100000000L;
                for (OpndRhs jumper : jumpers) {
                    long oval = jumper.getSimOutput (t);
                    if (oval == 1) {
                        ival = 1;
                        break;
                    }
                    assert (oval & ~0x100000000L) == 0;
                    ival &= oval;
                }
                setSimOutput (t, ival);
            }
        }
    }

    //// GENERATION ////

    // generate circuit for the connector
    //  output:
    //   returns with connector placed on board, pins wired up, pinnets filled in
    public void getCircuit (GenCtx genctx)
    {
        if (connector != null) return;

        int npins = config.cfgstr.length ();
        int nrows = config.nrows;
        assert npins % nrows == 0;
        connector = new Comp.Conn (genctx, genctx.conncol, "Conn" + instvarsuf, nrows, npins / nrows);
        genctx.addPlaceable (connector);

        if (config.pwrcap) {
            Comp.Capac capacitor = new Comp.Capac (genctx, genctx.conncol, "Cap" + instvarsuf, "50");
            genctx.addPlaceable (capacitor);
            genctx.nets.get ("VCC").addConn (capacitor, Comp.Capac.PIN_P);
            genctx.nets.get ("GND").addConn (capacitor, Comp.Capac.PIN_N);
        }

        int prmidx = 0;
        for (int cfgidx = 0; cfgidx < npins; cfgidx ++) {
            String pino = config.pinNumberFromCfgIdx (cfgidx);
            char c = config.cfgstr.charAt (cfgidx);
            switch (c) {

                // GND pin - connect to ground plane
                case 'g': {
                    genctx.nets.get ("GND").addConn (connector, pino);
                    break;
                }

                // Input pin - create a network connected to that pin
                //  this network is driven by the external circuit connected to the pin
                case 'i': {

                    // the pin is represented by this verilog-style wire object
                    IPinOutput ipo = (IPinOutput) params[prmidx++];

                    // start a new kicad-style network starting at that pin
                    // any onboard circuitry that wants to read the pin will get connected to this network
                    assert ipo.pinnet == null;
                    ipo.pinnet = new Network (genctx, ipo.name);
                    ipo.pinnet.addConn (connector, pino);
                    break;
                }

                // Output pin - skip for now, see below
                case 'o': {
                    prmidx ++;
                    break;
                }

                // VCC pin - connect to power plane
                case 'v': {
                    genctx.nets.get ("VCC").addConn (connector, pino);
                    break;
                }
            }
        }

        // now that all pinnets are filled in, it is safe to connect the 'o' pins to the circuits that drive them
        // the calls to opin.generate() below might recurse back to one of our IPinOutputs so the pinnets must be all filled in

        prmidx = 0;
        for (int cfgidx = 0; cfgidx < npins; cfgidx ++) {
            String pino = config.pinNumberFromCfgIdx (cfgidx);
            char c = config.cfgstr.charAt (cfgidx);
            switch (c) {

                // Input pin - already done above
                case 'i': {
                    prmidx ++;
                    break;
                }

                // Output pin - connect pin to network from on-board circuitry that drives the pin
                case 'o': {

                    // the pin is represented by this verilog-style wire object
                    OpndLhs opin = params[prmidx++];

                    // get a kicad-style network coming from whatever onboard circuitry is supposed to drive that pin
                    Network onet = opin.generate (genctx, 0);

                    // tell kicad to connect that network trace to the connector pin
                    onet.addConn (connector, pino);
                    break;
                }
            }
        }
    }

    // holds per-connector configuration information
    private static class Config {
        public boolean pwrcap;
        public int nrows;
        public OpndLhs[] params;
        public String cfgstr;

        // cfgstr for a 2x4 connector has 4 chars for top row indexed 0..3
        // and 4 chars for bottom row indexed 4..7:
        //   *1 2 3 4
        //    5 6 7 8
        // so convert the index to pin
        //  input:
        //   cfgidx = which letter (zero based) from all config strings tacked together
        //  output:
        //   returns hardware pin number understood by kicad
        public String pinNumberFromCfgIdx (int cfgidx)
        {
            assert (cfgidx >= 0) && (cfgidx < cfgstr.length ());
            return Integer.toString (cfgidx + 1);
        }

        // parameters are named after the pin numbers
        //  input:
        //   prmidx = which parameter (zero based)
        //  output:
        //   returns hardware pin number understood by kicad
        public String pinNumberFromPrmIdx (int prmidx)
        {
            String paramname = params[prmidx].name;
            assert paramname.startsWith ("I") || paramname.startsWith ("O");
            int i = paramname.indexOf ('/');
            return (i < 0) ? paramname.substring (i) : paramname.substring (1, i);
        }
    }
}
