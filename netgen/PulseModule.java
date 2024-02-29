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
 * Connect input to output with diodes in a 'wired and' fashion.
 */

import java.io.IOException;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.HashSet;

public class PulseModule extends Module implements ICombo, IVerilog {
    private String instvarsuf;

    private Network outnet;
    private Network[] innets;
    private OpndLhs input;
    private OpndLhs output;
    private StandCell standcell;

    public PulseModule (String instvarsuf)
    {
        name = "Pulse" + instvarsuf;
        this.instvarsuf = instvarsuf;

        if (instvarsuf.equals ("")) {
             input = new IParam ("D", 0);
        } else {
             input = new OpndLhs ("D" + instvarsuf);
        }

        output = new PulseOutput ("_Q" + instvarsuf, 1);

        params = new OpndLhs[] { input, output };

        for (OpndLhs param : params) {
            variables.put (param.name, param);
        }
    }

    // make a new Pulse module instance with wires for parameters
    // the parameter names are suffixed by instvarsuf
    @Override
    public OpndLhs[] getInstParams (UserModule target, String instvarsuf, Object instconfig)
            throws Exception
    {
        Module pulseinst = new PulseModule (instvarsuf);

        // make connector's pin variables available to target module
        target.variables.putAll (pulseinst.variables);

        return pulseinst.params;
    }

    // the internal circuitry that writes to the output wires

    private class PulseOutput extends OParam {
        public PulseOutput (String name, int index)
        {
            super (name, index);
        }

        @Override
        public String checkWriters ()
        {
            return null;
        }

        //// GENERATOR ////

        @Override
        public Network generate (GenCtx genctx, int rbit)
        {
            assert rbit == 0;
            getCircuit (genctx);
            return outnet;
        }

        @Override
        public void printProp (PropPrinter pp, int rbit)
        {
            pp.println ("Pulse", this, rbit);
        }

        //// SIMULATOR ////

        @Override
        protected int busWidthWork ()
        {
            return 1;
        }

        @Override
        protected void stepSimWork (int t)
                throws HaltedException
        {
            stepSimulator (t);
        }
    }

    // generate circuitry, ie, capacitor connecting input to the grid
    private void getCircuit (GenCtx genctx)
    {
        if (outnet == null) {
            genctx.icombos.add (this);

            standcell = new StandCell (genctx, name);

            Network innet = input.generate (genctx, 0);
            standcell.addPulseInput (innet);

            innets = new Network[] { innet };

            outnet = standcell.getOutput ();

            // queue for verilog output if requested
            genctx.verilogs.add (this);
        }
    }

    public Network getOutput ()
    {
        return outnet;
    }

    public Network[] getInputs ()
    {
        return innets;
    }

    @Override  // ICombo
    public Network[] getIComboOutputs ()
    {
        return new Network[] { outnet };
    }

    @Override  // ICombo
    public Iterable<Network> getIComboInputs ()
    {
        ArrayList<Network> innetarray = new ArrayList<> (innets.length);
        for (Network innet : innets) innetarray.add (innet);
        return innetarray;
    }

    @Override  // ICombo
    public void printIComboCSource (PrintStream ps)
    {
        printVerilog (ps, true);
    }

    @Override  // IVerilog
    public void getVeriNets (HashSet<Network> verinets)
    {
        verinets.add (outnet);
        verinets.add (innets[0]);
    }

    @Override  // IVerilog
    public void printVerilog (PrintStream ps, boolean myverilog)
    {
        String veriname = Network.getVeriName (myverilog, name);
        if (myverilog) {
            ps.print ("    " + veriname + ": Pulse (");
            ps.print ("_Q:" + outnet.getVeriName (myverilog) + ", ");
            ps.print ( "D:" + innets[0].getVeriName (myverilog) + ")");
        } else {
            ps.print ("    PulseModule " + veriname + " (");
            ps.print ("._Q(" + outnet.getVeriName (myverilog) + "), ");
            ps.print ( ".D(" + innets[0].getVeriName (myverilog) + "))");
        }
        ps.println (";");
    }

    //// SIMULATOR ////

    private final static int PULSEWIDTH = 25;

    private int lastlowtohigh;

    private void stepSimulator (int t)
            throws HaltedException
    {
        long in = input.getSimInput (t);

        // if input now at '1' and previously was not '1', start timer going
        if (t > 0) {
            long lastin = input.getSimInput (t - 1);
            if ((in == 0x100000000L) && (lastin != 0x100000000L)) {
                lastlowtohigh = t;
            }
        }

        // if input now at '1' and recently transitioned from '0', output a '0'
        if ((in == 0x100000000L) && (t - lastlowtohigh <= PULSEWIDTH)) {
            output.setSimOutput (t, 1);
        }

        // otherwise, input is a '0' or transitioned a while ago, output a '1'
        else {
            output.setSimOutput (t, 0x100000000L);
        }
    }
}
