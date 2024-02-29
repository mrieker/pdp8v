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
 * Built-in buffered LED module.
 */

import java.io.PrintStream;

public class BufLEDModule extends Module {
    private boolean inverted;       // light when zero
    private int width;              // number of LEDs
    private String instvarsuf;

    private OpndLhs d;
    private QOutput qoutput;

    private Network[] innets;
    private Network[] outnets;

    public BufLEDModule (String instvarsuf, int width, boolean inverted)
    {
        name = "BufLED" + instvarsuf;
        this.instvarsuf = instvarsuf;
        this.inverted   = inverted;
        this.width = width;

        if (instvarsuf.equals ("")) {
            d = new IParam ("D", 0);
        } else {
            d = new OpndLhs ("D" + instvarsuf);
        }
        qoutput = new QOutput ("Q" + instvarsuf, 1);

        qoutput.hidim = d.hidim = this.width - 1;

        params = new OpndLhs[] { d };

        variables.put (d.name, d);
    }

    // this gets the width token after the module name and before the (
    // eg, inst : BufLED 16 [inv] ( ... );
    //                   ^ token is here
    //                            ^ return token here
    // by default, we make a scalar buffered LED
    @Override
    public Token getInstConfig (Token token, String instvarsuf, Object[] instconfig_r)
            throws Exception
    {
        Config config = new Config ();
        config.width = 1;   // put 1 for scalar
        while (! "(".equals (token.getDel ())) {
            if (token instanceof Token.Int) {
                Token.Int tokint = (Token.Int) token;
                long width = tokint.value;
                if ((width <= 0) || (width > 32)) throw new Exception ("width must be between 1 and 32");
                config.width = (int) width;
                token = token.next;
                continue;
            }
            if ("inv".equals (token.getSym ())) {
                config.inv = true;
                token = token.next;
                continue;
            }
            break;
        }
        instconfig_r[0] = config;
        return token;
    }

    private static class Config {
        public boolean inv;
        public int width;
    }

    // make a new BufLED module instance with wires for parameters
    // the parameters names are suffixed by instvarsuf
    // the bus width is in instconfig[0] as returned by getInstConfig()
    @Override
    public OpndLhs[] getInstParams (UserModule target, String instvarsuf, Object instconfig)
            throws Exception
    {
        // instantiate to create a wire named D/instvarsuf
        Config config = (Config) instconfig;
        BufLEDModule blinst = new BufLEDModule (instvarsuf, config.width, config.inv);
        target.variables.putAll (blinst.variables);

        // get a dummy oparam so we get called to generate circuitry
        target.generators.add (blinst.qoutput);

        return blinst.params;
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
            if (outnets == null) {
                innets  = new Network[width];
                outnets = new Network[width];
            }

            Network outnet = outnets[rbit];
            if (outnet == null) {
                String label = instvarsuf;
                if (label.startsWith ("/")) label = label.substring (1);
                if (width > 10) label += String.format ("%02d", rbit);
                else if (width > 1) label += String.format ("%d", rbit);

                BufLEDPlaceable placeable = new BufLEDPlaceable (rbit);

                placeable.rc = new Comp.Resis (genctx, null, "rc." + rbit + "." + name, "390",  true, true);
                placeable.rb = new Comp.Resis (genctx, null, "rb." + rbit + "." + name, "10K",  true, true);
                placeable.zd = new Comp.Diode (genctx, null, "zd." + rbit + "." + name, "1N5239", true, true);  // 9.1v, anything over 5V will do
                placeable.sl = new Comp.SmLed (genctx, null, "sl." + rbit + "." + name, label,  true);
                assert ! inverted;
                placeable.qt = new Comp.Trans (genctx, null, "q."  + rbit + "." + name, "2N7000"); //?? inverted ? "2N3906" : "2N3904");

                Network gndnet = genctx.nets.get ("GND");
                Network vccnet = genctx.nets.get ("VCC");

                outnets[rbit] = outnet = new Network (genctx, rbit + "." + name);
                Network basenet = new Network (genctx, "b." + rbit + "." + name);
                Network collnet = new Network (genctx, "c." + rbit + "." + name);
                Network litenet = new Network (genctx, "l." + rbit + "." + name);

                Network innet = d.generate (genctx, rbit % d.busWidth ());
                innets[rbit] = innet;

                gndnet.addConn  (placeable.zd, Comp.Diode.PIN_A);
                gndnet.addConn  (placeable.qt, Comp.Trans.PIN_E);
                vccnet.addConn  (placeable.rc, Comp.Resis.PIN_A);
                innet.addConn   (placeable.rb, Comp.Resis.PIN_A);

                litenet.preWire (placeable.sl, Comp.SmLed.PIN_A, placeable.rc, Comp.Resis.PIN_C);
                collnet.preWire (placeable.sl, Comp.SmLed.PIN_C, placeable.qt, Comp.Trans.PIN_C);
                basenet.preWire (placeable.zd, Comp.Diode.PIN_C, placeable.rb, Comp.Resis.PIN_C);
                basenet.preWire (placeable.qt, Comp.Trans.PIN_B, placeable.rb, Comp.Resis.PIN_C);

                genctx.addPlaceable (placeable);
            }
            return outnet;
        }

        @Override
        public void printProp (PropPrinter pp, int rbit)
        { }

        @Override
        protected int busWidthWork ()
        {
            return width;
        }

        @Override
        protected void stepSimWork (int t)
                throws HaltedException
        {
            setSimOutput (t, ((1L << width) - 1) << 32);
        }
    }

    // places the LED, transistor and resistors as a group on the circuit board
    private class BufLEDPlaceable extends Placeable {
        public double leftx;
        public double topy;

        public int rbit;
        public Comp.Resis rc;
        public Comp.Resis rb;
        public Comp.Diode zd;
        public Comp.SmLed sl;
        public Comp.Trans qt;

        public BufLEDPlaceable (int rbit)
        {
            this.rbit = rbit;
        }

        public String getName () { return rbit + "." + name; }
        public void flipIt    () { System.err.println ("flipping BufLED not supported"); }
        public int getHeight  () { return 6; }
        public int getWidth   () { return 8; }
        public double getPosX () { return leftx; }
        public double getPosY () { return topy;  }

        public void setPosXY (double leftx, double topy)
        {
            this.leftx = leftx;
            this.topy  = topy;

            rc.setPosXY (leftx + 0, topy + 0);
            rb.setPosXY (leftx + 0, topy + 3);
            zd.setPosXY (leftx + 0, topy + 5);
            sl.setPosXY (leftx + 5, topy + 0);
            qt.setPosXY (leftx + 4, topy + 2);
        }

        public Network[] getExtNets ()
        {
            return innets;
        }
    }
}
