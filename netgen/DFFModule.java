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
 * Built-in D flipflop module.
 */

import java.io.PrintStream;
import java.util.HashSet;
import java.util.TreeMap;

public class DFFModule extends Module {
    private int width;              // number of flipflops
    private int lastgoodclocks;     // bitmask of previous good clock values
    private String instvarsuf;

    private OpndLhs   q;
    private OpndLhs  _q;
    private OpndLhs   d;
    private OpndLhs clk;
    private OpndLhs _pc;
    private OpndLhs _ps;
    private OpndLhs _sc;

    private DFFCell[] dffcells;

    public DFFModule (String instvarsuf, Config config)
    {
        name = "DFF" + instvarsuf;
        this.instvarsuf = instvarsuf;
        this.width = (config == null) ? 1 : config.width;

          q = new  QOutput ( "Q" + instvarsuf, 0);
         _q = new _QOutput ("_Q" + instvarsuf, 1);

        boolean has_sc = (config == null) || config.has_sc;
        if (instvarsuf.equals ("")) {
              d = new IParam (  "D", 2);
            clk = new IParam (  "T", 3);
            _pc = new IParam ("_PC", 4);
            _ps = new IParam ("_PS", 5);
            if (has_sc) _sc = new IParam ("_SC", 6);
        } else {
              d = new OpndLhs (  "D" + instvarsuf);
            clk = new OpndLhs (  "T" + instvarsuf);
            _pc = new OpndLhs ("_PC" + instvarsuf);
            _ps = new OpndLhs ("_PS" + instvarsuf);
            if (has_sc) _sc = new OpndLhs ("_SC" + instvarsuf);
        }

        q.hidim = _q.hidim = d.hidim = clk.hidim = _pc.hidim = _ps.hidim = this.width - 1;
        if (_sc != null) _sc.hidim = d.hidim;

        params = (_sc == null) ? new OpndLhs[] { q, _q, d, clk, _pc, _ps } :
                                 new OpndLhs[] { q, _q, d, clk, _pc, _ps, _sc };

        for (OpndLhs param : params) {
            variables.put (param.name, param);
        }

        // see if simulation should have any stuck-on bits
        if (config != null) {
            astuckon = config.astuckon;
            bstuckon = config.bstuckon;
            cstuckon = config.cstuckon;
            dstuckon = config.dstuckon;
            estuckon = config.estuckon;
            fstuckon = config.fstuckon;
        }
    }

    // this gets the width token after the module name and before the (
    // eg, inst : DFF 16 _SC ( ... );
    //                ^ token is here
    //                       ^ return token here
    // by default, we make a scalar flipflop
    @Override
    public Token getInstConfig (Token token, String instvarsuf, Object[] instconfig_r)
            throws Exception
    {
        Config config = new Config ();
        config.width = 1;   // put 1 for scalar
        while (! token.isDel (Token.DEL.OPRN)) {
            if (token instanceof Token.Int) {
                Token.Int tokint = (Token.Int) token;
                long width = tokint.value;
                if ((width <= 0) || (width > 32)) throw new Exception ("width must be between 1 and 32");
                config.width = (int) width;
                token = token.next;
                continue;
            }
            String tokensym = token.getSym ();
            if (tokensym != null) {
                tokensym = tokensym.toLowerCase ();
                switch (tokensym) {

                    // led is lit when Q=0
                    case "led0": {
                        config.light = -1;
                        break;
                    }

                    // led is lit when Q=1
                    case "led1": {
                        config.light = 1;
                        break;
                    }

                    // has a synchronous clear input
                    case "_sc": {
                        config.has_sc = true;
                        break;
                    }

                    // stuckons
                    case "soa":
                    case "sob":
                    case "soc":
                    case "sod":
                    case "soe":
                    case "sof": {
                        token = token.next;
                        int tokenint = (int) ((Token.Int) token).value;
                        switch (tokensym.charAt (2)) {
                            case 'a': config.astuckon = tokenint; break;
                            case 'b': config.bstuckon = tokenint; break;
                            case 'c': config.cstuckon = tokenint; break;
                            case 'd': config.dstuckon = tokenint; break;
                            case 'e': config.estuckon = tokenint; break;
                            case 'f': config.fstuckon = tokenint; break;
                        }
                        break;
                    }

                    // letters abcdef indicate stuck bits
                    default: {
                        throw new Exception ("bad option " + tokensym);
                    }
                }
                token = token.next;
                continue;
            }
            break;
        }
        instconfig_r[0] = config;
        return token;
    }

    private static class Config {
        public boolean has_sc;
        public int width;
        public int light;
        public int astuckon;
        public int bstuckon;
        public int cstuckon;
        public int dstuckon;
        public int estuckon;
        public int fstuckon;
    }

    // make a new DFF module instance with wires for parameters
    // the parameters names are suffixed by instvarsuf
    // the bus width is in instconfig[0] as returned by getInstConfig()
    @Override
    public OpndLhs[] getInstParams (UserModule target, String instvarsuf, Object instconfig)
            throws Exception
    {
        Config config = (Config) instconfig;
        DFFModule dffinst = new DFFModule (instvarsuf, config);
        target.variables.putAll (dffinst.variables);
        return dffinst.params;
    }

    // the internal circuitry that writes to the output wires (Q and _Q)

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
            // generate circuit for flipflop
            getCircuit (genctx, rbit);

            // Q output comes from the F gate
            return dffcells[rbit].foutnet;
        }

        @Override
        public void printProp (PropPrinter pp, int rbit)
        {
            pp.println ("DFF", this, rbit);
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
            // generate circuit for flipflop
            getCircuit (genctx, rbit);

            // _Q output comes from the E gate
            return dffcells[rbit].eoutnet;
        }

        @Override
        public void printProp (PropPrinter pp, int rbit)
        {
            pp.println ("DFF", this, rbit);
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

    // generate circuit for the given bitslice of the flipflop array
    //  input:
    //   rbit = relative bitnumber in the array
    //  output:
    //   returns network for the flipflop's Q output
    private void getCircuit (GenCtx genctx, int rbit)
    {
        if (dffcells == null) {
            genctx.dffmodules.add (this);
            dffcells = new DFFCell[width];
        }

        // see if we have already generated circuitry for this bitslice
        if (dffcells[rbit] != null) return;

        // generate circuitry
        DFFCell dffcell = new DFFCell (genctx, rbit);
        dffcells[rbit]  = dffcell;

        // get networks all our inputs are connected to
        // also the D input is start of propagation chain
        genctx.addPropRoot (d, rbit);
        Network   dnet =   d.generate (genctx, rbit %   d.busWidth ());
        Network clknet = clk.generate (genctx, rbit % clk.busWidth ());
        Network _pcnet = _pc.generate (genctx, rbit % _pc.busWidth ());
        Network _psnet = _ps.generate (genctx, rbit % _ps.busWidth ());
        Network _scnet = (_sc == null) ? null : _sc.generate (genctx, rbit % _sc.busWidth ());

        // wire up flipflop inputs
        dffcell.setInputs (genctx, rbit, dnet, clknet, _pcnet, _psnet, _scnet);

        // generate c source if requested
        genctx.isequens.put (name + "." + rbit, dffcell);

        // generate qvartvs verilog if requested
        genctx.verilogs.add (dffcell);
    }

    //// SIMULATION ////

    private int agateout;
    private int bgateout;
    private int cgateout;
    private int dgateout;
    private int egateout;
    private int fgateout;

    private int astuckon;
    private int bstuckon;
    private int cstuckon;
    private int dstuckon;
    private int estuckon;
    private int fstuckon;

    private void stepSimulator (int t)
            throws HaltedException
    {
        // add extra 2 gate delays for 6-transistor configuration
        // ie, the input gates see inputs 2 more gate delays ago
        int t2 = t - 2 * OpndRhs.SIM_PROP;

        // get state all up front in case of recursion
        int   dval = (int) (  d.getSmearedInput (t2, width) >> 32);
        int   tval = (int) (clk.getSmearedInput (t2, width) >> 32);
        int _pcval = (int) (_pc.getSmearedInput (t2, width) >> 32);
        int _psval = (int) (_ps.getSmearedInput (t2, width) >> 32);

        // _SC is just anded with D
        if (_sc != null) {
            int _scval = (int) (_sc.getSmearedInput (t2, width) >> 32);
            dval &= _scval;
        }

        // make new gate outputs
        int mask = (1 << width) - 1;
        int newa = astuckon | mask & ~(_pcval   & dval     & bgateout);
        int newb = bstuckon | mask & ~(agateout & tval     & cgateout);
        int newc = cstuckon | mask & ~(_pcval   & tval     & dgateout);
        int newd = dstuckon | mask & ~(cgateout & agateout & _psval);
        int newe = estuckon | mask & ~(_pcval   & bgateout & fgateout);
        int newf = fstuckon | mask & ~(egateout & cgateout & _psval);

        agateout = newa;
        bgateout = newb;
        cgateout = newc;
        dgateout = newd;
        egateout = newe;
        fgateout = newf;

        // set outputs for this timestep
        long _qval = (((long) egateout) << 32) | (egateout ^ mask);
        long  qval = (((long) fgateout) << 32) | (fgateout ^ mask);
        _q.setSimOutput (t, _qval);
         q.setSimOutput (t,  qval);
    }

    // holds circuitry that makes a single bit D flipflop
    // holds the 6 standard cells that make up a D flipflop
    private class DFFCell implements IPrintLoc, ISequen, IVerilog {
        public int rbit;
        public Network eoutnet;     // _Q output
        public Network foutnet;     //  Q output

        public StandCell anand;     // _PC input, D input, _SC input
        public StandCell bnand;     // clock
        public StandCell cnand;     // clock
        public StandCell dnand;     // _PS input
        public StandCell enand;     // _Q output
        public StandCell fnand;     //  Q output

        private Network clknet;
        private Network datnet;
        private Network _pcnet;
        private Network _psnet;
        private Network _scnet;

        public DFFCell (GenCtx genctx, int rbit)
        {
            this.rbit = rbit;

            // always create output cells before reading inputs to prevent infinite recursion
            enand = new StandCell (genctx, "e." + rbit + "." + name);
            fnand = new StandCell (genctx, "f." + rbit + "." + name);

            genctx.addPlaceable (enand);
            genctx.addPlaceable (fnand);

            eoutnet = enand.getOutput ();
            foutnet = fnand.getOutput ();

            genctx.printlocs.put (rbit + "." + name, this);
        }

        public void setInputs (GenCtx genctx, int rbit, Network dnet, Network clknet, Network _pcnet, Network _psnet, Network _scnet)
        {
            this.clknet = clknet;
            this.datnet =   dnet;
            this._pcnet = _pcnet;
            this._psnet = _psnet;
            this._scnet = _scnet;

            // grounded clock means do an SR flipflop with just enand and fnand
            if (clknet.name.equals ("GND")) {
                enand.addInput (0, _pcnet);
                enand.addInput (0, fnand.getOutput ());
                fnand.addInput (0, enand.getOutput ());
                fnand.addInput (0, _psnet);
                enand.close ();
                fnand.close ();
                return;
            }

            // grounded D or _SC means eliminate anand
            if (dnet.name.equals ("GND") || ((_scnet != null) && _scnet.name.equals ("GND"))) {
                bnand = new StandCell (genctx, "b." + rbit + "." + name);
                cnand = new StandCell (genctx, "c." + rbit + "." + name);
                dnand = new StandCell (genctx, "d." + rbit + "." + name);

                genctx.addPlaceable (bnand);
                genctx.addPlaceable (cnand);
                genctx.addPlaceable (dnand);

                bnand.addInput (0, clknet);
                bnand.addInput (0, cnand.getOutput ());

                cnand.addInput (0, _pcnet);
                cnand.addInput (0, clknet);
                cnand.addInput (0, dnand.getOutput ());

                dnand.addInput (0, cnand.getOutput ());
                dnand.addInput (0, _psnet);

                enand.addInput (0, _pcnet);
                enand.addInput (0, bnand.getOutput ());
                enand.addInput (0, fnand.getOutput ());

                fnand.addInput (0, enand.getOutput ());
                fnand.addInput (0, cnand.getOutput ());
                fnand.addInput (0, _psnet);

                bnand.close ();
                cnand.close ();
                dnand.close ();
                enand.close ();
                fnand.close ();
                return;
            }

            // full D flipflop
            anand = new StandCell (genctx, "a." + rbit + "." + name);
            bnand = new StandCell (genctx, "b." + rbit + "." + name);
            cnand = new StandCell (genctx, "c." + rbit + "." + name);
            dnand = new StandCell (genctx, "d." + rbit + "." + name);

            genctx.addPlaceable (anand);
            genctx.addPlaceable (bnand);
            genctx.addPlaceable (cnand);
            genctx.addPlaceable (dnand);

            if (_scnet != null) anand.addInput (0, _scnet);
            anand.addInput (0, _pcnet);
            anand.addInput (0,   dnet);
            anand.addInput (0, bnand.getOutput ());

            bnand.addInput (0, anand.getOutput ());
            bnand.addInput (0, clknet);
            bnand.addInput (0, cnand.getOutput ());

            cnand.addInput (0, _pcnet);
            cnand.addInput (0, clknet);
            cnand.addInput (0, dnand.getOutput ());

            dnand.addInput (0, cnand.getOutput ());
            dnand.addInput (0, anand.getOutput ());
            dnand.addInput (0, _psnet);

            enand.addInput (0, _pcnet);
            enand.addInput (0, bnand.getOutput ());
            enand.addInput (0, fnand.getOutput ());

            fnand.addInput (0, enand.getOutput ());
            fnand.addInput (0, cnand.getOutput ());
            fnand.addInput (0, _psnet);

            anand.close ();
            bnand.close ();
            cnand.close ();
            dnand.close ();
            enand.close ();
            fnand.close ();
        }

        @Override  // IPrintLoc
        public void printLoc (PrintStream ps, Module genmod)
        {
            fnand.printLocOut (ps, genmod, " f   Q out: ");
            enand.printLocOut (ps, genmod, " e  _Q out: ");
            if (anand != null) anand.printLocIns (ps, genmod, datnet, " a   D  in: ");
            if (bnand != null) bnand.printLocIns (ps, genmod, clknet, " b   T  in: ");
            if (cnand != null) cnand.printLocIns (ps, genmod, clknet, " c   T  in: ");
            if (anand != null) anand.printLocIns (ps, genmod, _pcnet, " a _PC  in: ");
            if (cnand != null) cnand.printLocIns (ps, genmod, _pcnet, " c _PC  in: ");
                               enand.printLocIns (ps, genmod, _pcnet, " e _PC  in: ");
            if (dnand != null) dnand.printLocIns (ps, genmod, _psnet, " d _PS  in: ");
                               fnand.printLocIns (ps, genmod, _psnet, " f _PS  in: ");
        }

        @Override  // ISequen
        public int printISequenDecl (PrintStream ps, int bai)
        {
            String instname = Network.getVeriName (true, name) + "_" + rbit;
            ps.println ("#define " + enand.getOutput ().getVeriName (true) + " boolarray[" + bai + "]");
            enand.getOutput ().boolarrayindex = bai ++;
            ps.println ("#define " + fnand.getOutput ().getVeriName (true) + " boolarray[" + bai + "]");
            fnand.getOutput ().boolarrayindex = bai ++;
            ps.println ("#define " + enand.getOutput ().getVeriName (true) + "_step boolarray[" + bai ++ + "]");
            ps.println ("#define " + fnand.getOutput ().getVeriName (true) + "_step boolarray[" + bai ++ + "]");
            ps.println ("#define " + instname + "_lastt boolarray[" + bai ++ + "]");
            return bai;
        }

        @Override  // ISequen
        public void printISequenUnDecl (PrintStream ps)
        { }

        @Override  // ISequen
        public void printISequenKer (PrintStream ps)
        {
            String instname = Network.getVeriName (true, name) + "_" + rbit;
            String dinput = datnet.getVeriName (true);
            if (_scnet != null) dinput += " & " + _scnet.getVeriName (true);
            ps.println ("    DFFStep (" +
                    dinput + ", " +
                    clknet.getVeriName (true) + ", " +
                    _pcnet.getVeriName (true) + ", " +
                    _psnet.getVeriName (true) + ", " +
                    enand.getOutput ().getVeriName (true) + ", " +
                    fnand.getOutput ().getVeriName (true) + ", &" +
                    enand.getOutput ().getVeriName (true) + "_step, &" +
                    fnand.getOutput ().getVeriName (true) + "_step, &" +
                    instname + "_lastt, \"" + instname + "\");");
        }

        @Override  // ISequen
        public void printISequenChunk (PrintStream ps)
        {
            ps.println ("    " + enand.getOutput ().getVeriName (true) + " = " + enand.getOutput ().getVeriName (true) + "_step;");
            ps.println ("    " + fnand.getOutput ().getVeriName (true) + " = " + fnand.getOutput ().getVeriName (true) + "_step;");
        }

        @Override  // ISequen
        public Network[] getISequenOutNets ()
        {
            return new Network[] { enand.getOutput (), fnand.getOutput () };
        }

        @Override  // ISequen
        public String getISequenName ()
        {
            return rbit + "." + name;
        }

        @Override  // ISequen
        public TreeMap<String,Network> getISequenInNets ()
        {
            TreeMap<String,Network> innets = new TreeMap<> ();
            innets.put ("D", datnet);
            innets.put ("T", clknet);
            innets.put ("_PC", _pcnet);
            innets.put ("_PS", _psnet);
            if (_scnet != null) innets.put ("_SC", _scnet);
            return innets;
        }

        @Override  // IVerilog
        public void getVeriNets (HashSet<Network> verinets)
        {
            verinets.add (eoutnet);
            verinets.add (foutnet);
            verinets.add (clknet);
            verinets.add (datnet);
            verinets.add (_pcnet);
            verinets.add (_psnet);
            verinets.add (_scnet);
        }

        @Override  // IVerilog
        public void printVerilog (PrintStream ps, boolean myverilog)
        {
            String veriname = Network.getVeriName (myverilog, name) + "_" + rbit;
            if (myverilog) {
                ps.print   ("    " + veriname + ": DFF _SC");
                printStuckOn (ps, veriname, 'a');
                printStuckOn (ps, veriname, 'b');
                printStuckOn (ps, veriname, 'c');
                printStuckOn (ps, veriname, 'd');
                printStuckOn (ps, veriname, 'e');
                printStuckOn (ps, veriname, 'f');
                ps.print   (" (");
                ps.print   (  "Q:" + foutnet.getVeriName (myverilog) + ", ");
                ps.print   ( "_Q:" + eoutnet.getVeriName (myverilog) + ", ");
                ps.print   (  "T:" +  clknet.getVeriName (myverilog) + ", ");
                ps.print   (  "D:" +  datnet.getVeriName (myverilog) + ", ");
                ps.print   ("_PC:" +  _pcnet.getVeriName (myverilog) + ", ");
                ps.print   ("_PS:" +  _psnet.getVeriName (myverilog) + ", ");
                ps.println ("_SC:" + ((_scnet == null) ? "1" : _scnet.getVeriName (myverilog)) + ");");
            } else {
                ps.print   ("    DFFModule " + veriname + " (.U(uclk), ");
                ps.print   (  ".Q(" + foutnet.getVeriName (myverilog) + "), ");
                ps.print   ( "._Q(" + eoutnet.getVeriName (myverilog) + "), ");
                ps.print   (  ".T(" +  clknet.getVeriName (myverilog) + "), ");
                ps.print   (  ".D(" +  datnet.getVeriName (myverilog) + "), ");
                ps.print   ("._PC(" +  _pcnet.getVeriName (myverilog) + "), ");
                ps.print   ("._PS(" +  _psnet.getVeriName (myverilog) + "), ");
                ps.println ("._SC(" + ((_scnet == null) ? "1'b1" : _scnet.getVeriName (myverilog)) + "));");
            }
        }

        private void printStuckOn (PrintStream ps, String veriname, char gateid)
        {
            String envnam = "stuckon_" + veriname + "_" + gateid;
            String envstr = System.getenv (envnam);
            if ((envstr != null) && !envstr.equals ("")) {
                ps.print (" so" + gateid + " " + envstr);
                System.out.println ("DFFModule: " + envnam + "=" + envstr);
            }
        }
    }
}
