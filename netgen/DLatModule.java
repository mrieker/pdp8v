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
 * Built-in latch module.
 */

import java.io.PrintStream;
import java.util.ArrayList;
import java.util.HashSet;

public class DLatModule extends Module {
    private int width;              // number of latches
    private String instvarsuf;

    private OpndLhs   q;
    private OpndLhs  _q;
    private OpndLhs   d;
    private OpndLhs   g;
    private OpndLhs _pc;
    private OpndLhs _ps;

    private DLatCell[] dlatcells;

    public DLatModule (String instvarsuf, Config config)
    {
        name = "DLat" + instvarsuf;
        this.instvarsuf = instvarsuf;
        this.width = (config == null) ? 1 : config.width;

         q = new  QOutput ( "Q" + instvarsuf, 0);
        _q = new _QOutput ("_Q" + instvarsuf, 1);

        if (instvarsuf.equals ("")) {
              d = new IParam (  "D", 2);
              g = new IParam (  "G", 3);
            _pc = new IParam ("_PC", 4);
            _ps = new IParam ("_PS", 5);
        } else {
              d = new OpndLhs (  "D" + instvarsuf);
              g = new OpndLhs (  "G" + instvarsuf);
            _pc = new OpndLhs ("_PC" + instvarsuf);
            _ps = new OpndLhs ("_PS" + instvarsuf);
        }

        q.hidim = _q.hidim = d.hidim = this.width - 1;

        params = new OpndLhs[] { q, _q, d, g, _pc, _ps };

        for (OpndLhs param : params) {
            variables.put (param.name, param);
        }

        // see if simulation should have any stuck-on bits
        if (config != null) {
            astuckon = config.astuckon;
            bstuckon = config.bstuckon;
            cstuckon = config.cstuckon;
            dstuckon = config.dstuckon;
        }
    }

    // this gets the width token after the module name and before the (
    // eg, inst : DLat 16 ( ... );
    //                 ^ token is here
    //                    ^ return token here
    // by default, we make a scalar latch
    @Override
    public Token getInstConfig (Token token, String instvarsuf, Object[] instconfig_r)
            throws Exception
    {
        Config config = new Config ();
        config.width = 1;  // put 1 for scalar
        if (token instanceof Token.Int) {
            Token.Int tokint = (Token.Int) token;
            long width = tokint.value;
            if ((width <= 0) || (width > 32)) throw new Exception ("width must be between 1 and 32");
            config.width = (int) width;
            token = token.next;
        }
        instconfig_r[0] = config;
        return token;
    }

    private static class Config {
        public int width;
        public int astuckon;
        public int bstuckon;
        public int cstuckon;
        public int dstuckon;
    }

    // make a new DLat module instance with wires for parameters
    // the parameters names are suffixed by instvarsuf
    // the bus width is in instconfig[0] as returned by getInstConfig()
    @Override
    public OpndLhs[] getInstParams (UserModule target, String instvarsuf, Object instconfig)
            throws Exception
    {
        Config config = (Config) instconfig;
        DLatModule dlatinst = new DLatModule (instvarsuf, config);
        target.variables.putAll (dlatinst.variables);
        return dlatinst.params;
    }

    // the internal circuitry that writes to the output wires

    private class QOutput extends OParam {
        public QOutput (String name, int index)
        {
            super (name, index);
        }

        @Override
        public String checkWriters ()
        {
            return null;
        }

        @Override
        public Network generate (GenCtx genctx, int rbit)
        {
            // generate circuit for latch
            getCircuit (genctx, rbit);

            // Q output comes from the C gate
            return dlatcells[rbit].cnand.getOutput ();
        }

        @Override
        public void printProp (PropPrinter pp, int rbit)
        {
            pp.println ("DLat", this, rbit);
            pp.inclevel ();
            d.printProp (pp, rbit);
            pp.declevel ();
        }

        @Override
        protected int busWidthWork ()
        {
            return width;
        }

        @Override
        protected void stepSimWork (int t)
                throws HaltedException
        {
            stepSimulator (t);
        }
    }

    private class _QOutput extends OParam {
        public _QOutput (String name, int index)
        {
            super (name, index);
        }

        @Override
        public String checkWriters ()
        {
            return null;
        }

        @Override
        public Network generate (GenCtx genctx, int rbit)
        {
            // generate circuit for latch
            getCircuit (genctx, rbit);

            // _Q output comes from the D gate
            return dlatcells[rbit].dnand.getOutput ();
        }

        @Override
        public void printProp (PropPrinter pp, int rbit)
        {
            pp.println ("DLat", this, rbit);
            pp.inclevel ();
            d.printProp (pp, rbit);
            pp.declevel ();
        }

        @Override
        protected int busWidthWork ()
        {
            return width;
        }

        @Override
        protected void stepSimWork (int t)
                throws HaltedException
        {
            stepSimulator (t);
        }
    }

    //// GENERATION ////

    // generate circuit for the given bitslice of the latch array
    //  input:
    //   rbit = relative bitnumber in the array
    //  output:
    //   returns network for the latch's Q output
    private void getCircuit (GenCtx genctx, int rbit)
    {
        if (dlatcells == null) {
            genctx.dlatmodules.add (this);
            dlatcells = new DLatCell[width];
        }

        // see if we have already generated circuitry for this bitslice
        if (dlatcells[rbit] == null) {

            // make 4 cells for this bitslice of the latch
            DLatCell dlatcell = new DLatCell ();
            dlatcell.rbit  = rbit;
            dlatcell.anand = new StandCell (genctx, "a." + rbit + "." + name);
            dlatcell.bnand = new StandCell (genctx, "b." + rbit + "." + name);
            dlatcell.cnand = new StandCell (genctx, "c." + rbit + "." + name);
            dlatcell.dnand = new StandCell (genctx, "d." + rbit + "." + name);
            dlatcells[rbit] = dlatcell;

            genctx.addPlaceable (dlatcell.anand);
            genctx.addPlaceable (dlatcell.bnand);
            genctx.addPlaceable (dlatcell.cnand);
            genctx.addPlaceable (dlatcell.dnand);
            genctx.printlocs.put (rbit + "." + name, dlatcell);

            // get networks all our inputs are connected to
            dlatcell.dnet   =   d.generate (genctx, rbit %   d.busWidth ());
            dlatcell.gnet   =   g.generate (genctx, rbit %   g.busWidth ());
            dlatcell._pcnet = _pc.generate (genctx, rbit % _pc.busWidth ());
            dlatcell._psnet = _ps.generate (genctx, rbit % _ps.busWidth ());

            // connect the cells and inputs into a latch arrangement
            //   data -> a.d1   a -> c.d1   c -> q
            //   gate -> a.d2   d -> c.d2
            //                _ps -> c.d3
            //   gate -> b.d1   b -> d.d1   d -> _q
            //      a -> b.d2   c -> d.d2
            //                _pc -> d.d3
            dlatcell.anand.addInput (0, dlatcell.dnet);
            dlatcell.anand.addInput (0, dlatcell.gnet);
            dlatcell.bnand.addInput (0, dlatcell.gnet);
            dlatcell.bnand.addInput (0, dlatcell.anand.getOutput ());
            dlatcell.cnand.addInput (0, dlatcell.anand.getOutput ());
            dlatcell.cnand.addInput (0, dlatcell.dnand.getOutput ());
            dlatcell.cnand.addInput (0, dlatcell._psnet);
            dlatcell.dnand.addInput (0, dlatcell.bnand.getOutput ());
            dlatcell.dnand.addInput (0, dlatcell.cnand.getOutput ());
            dlatcell.dnand.addInput (0, dlatcell._pcnet);

            dlatcell.anand.close ();
            dlatcell.bnand.close ();
            dlatcell.cnand.close ();
            dlatcell.dnand.close ();

            // generate c source if requested
            genctx.icombos.add (dlatcell);

            // generate qvartvs verilog if requested
            genctx.verilogs.add (dlatcell);
        }
    }

    //// SIMULATION ////

    private int agateout;
    private int bgateout;
    private int cgateout;
    private int dgateout;

    private int astuckon;
    private int bstuckon;
    private int cstuckon;
    private int dstuckon;

    private void stepSimulator (int t)
            throws HaltedException
    {
        // add extra gate delay for 4-transistor configuration
        // ie, the input gates see inputs 1 more gate delay ago
        int t1 = t - OpndRhs.SIM_PROP;

        // get state all up front in case of recursion
        int   dval = (int) (  d.getSmearedInput (t1, width) >> 32);
        int   gval = (int) (  g.getSmearedInput (t1, width) >> 32);
        int _pcval = (int) (_pc.getSmearedInput (t1, width) >> 32);
        int _psval = (int) (_ps.getSmearedInput (t1, width) >> 32);

        // make new gate outputs
        int mask = (1 << width) - 1;
        int newa = astuckon | mask & ~(gval & dval);
        int newb = bstuckon | mask & ~(gval & agateout);
        int newc = cstuckon | mask & ~(agateout & dgateout & _psval);
        int newd = dstuckon | mask & ~(bgateout & cgateout & _pcval);

        agateout = newa;
        bgateout = newb;
        cgateout = newc;
        dgateout = newd;

        // set outputs for this timestep
        long _qval = (((long) dgateout) << 32) | (dgateout ^ mask);
        long  qval = (((long) cgateout) << 32) | (cgateout ^ mask);
        _q.setSimOutput (t, _qval);
         q.setSimOutput (t,  qval);
    }

    // holds the 4 StandCells that make up a single-bit D latch
    private class DLatCell implements ICombo, IPrintLoc, IVerilog {
        public int rbit;
        public StandCell anand;     // D,G input
        public StandCell bnand;     //  G input
        public StandCell cnand;     //  Q output
        public StandCell dnand;     // _Q output

        public Network dnet;        // what drives the latch data input
        public Network gnet;        // what drives the latch gate input
        public Network _pcnet;      // what drives the latch preclear input (or null)
        public Network _psnet;      // what drives the latch preset input (or null)

        @Override  // ICombo
        public Network[] getIComboOutputs ()
        {
            return new Network[] { cnand.getOutput (), dnand.getOutput () };
        }

        @Override  // ICombo
        public Iterable<Network> getIComboInputs ()
        {
            ArrayList<Network> al = new ArrayList<> (4);
            al.add (dnet);
            al.add (gnet);
            al.add (_pcnet);
            al.add (_psnet);
            return al;
        }

        @Override  // IPrintLoc
        public void printLoc (PrintStream ps, Module genmod)
        {
            cnand.printLocOut (ps, genmod, " c   Q out: ");
            dnand.printLocOut (ps, genmod, " d  _Q out: ");
            anand.printLocIns (ps, genmod,   dnet, " a   D  in: ");
            anand.printLocIns (ps, genmod,   gnet, " a   G  in: ");
            bnand.printLocIns (ps, genmod,   gnet, " b   G  in: ");
            dnand.printLocIns (ps, genmod, _pcnet, " d _PC  in: ");
            cnand.printLocIns (ps, genmod, _psnet, " c _PS  in: ");
        }

        @Override  // ICombo
        public int printIComboCSource (PrintStream ps)
        {
            String instname = Network.getVeriName (true, name) + "_" + rbit;
            ps.println ("    DLatStep (" +
                    dnet.getVeriName (true) + ", " +
                    gnet.getVeriName (true) + ", " +
                    _pcnet.getVeriName (true) + ", " +
                    _psnet.getVeriName (true) + ", &" +
                    cnand.getOutput ().getVeriName (true) + ", &" +
                    dnand.getOutput ().getVeriName (true) + ", \"" +
                    instname + "\");");
            return 4;
        }

        @Override  // IVerilog
        public void getVeriNets (HashSet<Network> verinets)
        {
            verinets.add (cnand.getOutput ());
            verinets.add (dnand.getOutput ());
            verinets.add (dnet);
            verinets.add (gnet);
            verinets.add (_pcnet);
            verinets.add (_psnet);
        }

        @Override  // IVerilog
        public void printVerilog (PrintStream ps, boolean myverilog)
        {
            String veriname = Network.getVeriName (myverilog, name) + "_" + rbit;
            if (myverilog) {
                ps.print   ("    " + veriname + ": DLat");
                printStuckOn (ps, veriname, 'a');
                printStuckOn (ps, veriname, 'b');
                printStuckOn (ps, veriname, 'c');
                printStuckOn (ps, veriname, 'd');
                ps.print   (" (");
                ps.print   (  "Q:" + cnand.getOutput ().getVeriName (myverilog) + ", ");
                ps.print   ( "_Q:" + dnand.getOutput ().getVeriName (myverilog) + ", ");
                ps.print   (  "D:" + dnet.getVeriName (myverilog) + ", ");
                ps.print   (  "G:" + gnet.getVeriName (myverilog) + ", ");
                ps.print   ("_PC:" + _pcnet.getVeriName (myverilog) + ", ");
                ps.println ("_PS:" + _psnet.getVeriName (myverilog) + ");");
            } else {
                ps.print   ("    DLatModule " + veriname + " (.U(uclk), ");
                ps.print   (  ".Q(" + cnand.getOutput ().getVeriName (myverilog) + "), ");
                ps.print   ( "._Q(" + dnand.getOutput ().getVeriName (myverilog) + "), ");
                ps.print   (  ".D(" + dnet.getVeriName (myverilog) + "), ");
                ps.print   (  ".G(" + gnet.getVeriName (myverilog) + "), ");
                ps.print   ("._PC(" + _pcnet.getVeriName (myverilog) + "), ");
                ps.println ("._PS(" + _psnet.getVeriName (myverilog) + "));");
            }
        }

        private void printStuckOn (PrintStream ps, String veriname, char gateid)
        {
            String envnam = "stuckon_" + veriname + "_" + gateid;
            String envstr = System.getenv (envnam);
            if ((envstr != null) && !envstr.equals ("")) {
                ps.print (" so" + gateid + " " + envstr);
                System.out.println ("DLatModule: " + envnam + "=" + envstr);
            }
        }
    }
}
