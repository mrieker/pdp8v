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

public class DAOModule extends Module implements ICombo, IVerilog {
    private int nbits;
    private String instvarsuf;

    private Network outnet;
    private Network[] innets;
    private OpndLhs input;
    private OpndLhs output;

    public DAOModule (String instvarsuf)
    {
        this (instvarsuf, 1);
    }

    public DAOModule (String instvarsuf, int nbits)
    {
        name = "DAO" + instvarsuf;
        this.instvarsuf = instvarsuf;
        this.nbits = nbits;

        if (instvarsuf.equals ("")) {
             input = new IParam ("IN", 0);
        } else {
             input = new OpndLhs ("IN" + instvarsuf);
        }

        output = new DAOOutput ("OUT" + instvarsuf, 1);

        input.hidim = nbits - 1;

        params = new OpndLhs[] { input, output };

        for (OpndLhs param : params) {
            variables.put (param.name, param);
        }
    }

    // this gets the nbits token after the module name and before the (
    // eg, inst : DAO 12 ( ... );
    //                ^ token is here
    //                   ^ return token here
    // by default, use 1
    @Override
    public Token getInstConfig (Token token, String instvarsuf, Object[] instconfig_r)
            throws Exception
    {
        int nbits = 1;
        while (! token.isDel (Token.DEL.OPRN)) {
            if (token instanceof Token.Int) {
                Token.Int tokint = (Token.Int) token;
                nbits = (int) tokint.value;
                if ((nbits <= 0) || (nbits > 32)) {
                    throw new Exception ("nbits " + nbits + " must be 1..32");
                }
                token = token.next;
                continue;
            }
            throw new Exception ("bad dao config " + token.toString ());
        }
        instconfig_r[0] = nbits;
        return token;
    }

    // make a new DAO module instance with wires for parameters
    // the parameter names are suffixed by instvarsuf
    @Override
    public OpndLhs[] getInstParams (UserModule target, String instvarsuf, Object instconfig)
            throws Exception
    {
        DAOModule daoinst = new DAOModule (instvarsuf, (Integer) instconfig);

        // make connector's pin variables available to target module
        target.variables.putAll (daoinst.variables);

        return daoinst.params;
    }

    // the internal circuitry that writes to the output wires

    private class DAOOutput extends OParam {
        public DAOOutput (String name, int index)
        {
            super (name, index);
        }

        public DAOModule getDAO ()
        {
            return DAOModule.this;
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
            pp.println ("DAO", this, rbit);
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

    // generate circuitry, ie, diodes connecting all inputs to single output
    private void getCircuit (GenCtx genctx)
    {
        if (outnet == null) {
            genctx.daomodules.add (this);
            genctx.icombos.add (this);

            // set up network (circuit board trace) that all the anodes connect to
            // mark it as 'dao' so a diode won't be used to connect it to the input of a gate
            outnet = new Network (genctx, "out." + name);
            outnet.daotag = true;

            // get the networks for all the cathodes
            innets = new Network[nbits];
            for (int i = 0; i < nbits; i ++) {

                // generate circuitry that drives the input line if not already generated
                Network innet = input.generate (genctx, i);
                innets[i] = innet;

                // don't bother with VCC
                if (! innet.name.equals ("VCC")) {

                    // allocate a diode
                    Comp.Diode diode = new Comp.Diode (genctx, null, "D" + i + "." + name, false, true);

                    // queue it to be placed on circuit board
                    genctx.addPlaceable (diode);

                    // connect the two ends
                    innet.addConn (diode, Comp.Diode.PIN_C, 1);
                    outnet.addConn (diode, Comp.Diode.PIN_A);
                }
            }

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
    public int printIComboCSource (PrintStream ps)
    {
        printVerilog (ps, true);
        return 0;
    }

    @Override  // IVerilog
    public void getVeriNets (HashSet<Network> verinets)
    {
        verinets.add (outnet);
        for (int i = 0; i < nbits; i ++) {
            verinets.add (innets[i]);
        }
    }

    @Override  // IVerilog
    public void printVerilog (PrintStream ps, boolean myverilog)
    {
        ps.print ("    ");
        if (! myverilog) ps.print ("assign ");
        ps.print (outnet.getVeriName (myverilog));
        ps.print (" = ");
        for (int i = 0; i < nbits; i ++) {
            if (i > 0) ps.print (" & ");
            ps.print (innets[i].getVeriName (myverilog));
        }
        ps.println (";");
    }

    //// SIMULATOR ////

    private void stepSimulator (int t)
            throws HaltedException
    {
        // diodes are wired as an 'and' gate

        long anyzeroes = 0;
        long allones = 0x100000000L;

        long in = input.getSimInput (t);
        for (int i = 0; i < nbits; i ++) {
            anyzeroes |= in & 1;
            allones   &= in;
            in >>>= 1;
        }

        output.setSimOutput (t, allones | anyzeroes);
    }
}
