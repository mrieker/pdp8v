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
 * Electronic components
 *  Transistor, Resistor, Diode, Connector
 * Each component has an unique name, a value and a set of pins
 * The name is derived from the gate/flipflop name
 */

import java.awt.Graphics2D;
import java.io.PrintStream;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.HashMap;
import java.util.LinkedList;

public abstract class Comp extends Placeable {
    public final String name;
    public final String ref;
    public final String tstamp;
    public final String value;

    public abstract void printnet (PrintStream ps);
    public abstract void printres (Graphics2D g2d, int wf, int hf);
    public abstract void printpcb (PrintStream ps, GenCtx genctx);

    public double posx10th;
    public double posy10th;

    protected HashMap<String,Network> netperpin;

    private static int C_refs;
    private static int D_refs;
    private static int J_refs;
    private static int Q_refs;
    private static int R_refs;

    private static HashMap<String,Integer> refcounts;

    // common to all components
    //  input:
    //   genctx = circuit board generation context
    //   column = null: don't enter in any laycout column
    //            else: enter component at bottom of layout column
    //   name   = unique name for the component
    //   refpfx = reference name prefix (eg, "D" for diode, "Q" for transistor, ...)
    //   value  = component value (eg, "150" for 150 ohm resistor, "1N4148" for diode, ...)
    protected Comp (GenCtx genctx, LinkedList<Comp> column, String name, String refpfx, String value)
    {
        this.name  = name;
        this.value = value;

        // timestamp is used by PcbNew to identify the component
        // is supposedly accepts an UUID format number
        // so derive a number from the unique name which should remain the same over other design changes
        try {
            MessageDigest md = MessageDigest.getInstance ("MD5");
            md.update (name.getBytes ());
            byte[] hash = md.digest ();
            tstamp = String.format ("%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                hash[ 0] & 0xFF, hash[ 1] & 0xFF, hash[ 2] & 0xFF, hash[ 3] & 0xFF,
                hash[ 4] & 0xFF, hash[ 5] & 0xFF, hash[ 6] & 0xFF, hash[ 7] & 0xFF, hash[ 8] & 0xFF, hash[ 9] & 0xFF,
                hash[10] & 0xFF, hash[11] & 0xFF, hash[12] & 0xFF, hash[13] & 0xFF, hash[14] & 0xFF, hash[15] & 0xFF);
        } catch (NoSuchAlgorithmException nsae) {
            throw new RuntimeException (nsae);
        }

        // save list of networks connected to each pin of the component
        netperpin = new HashMap<> ();

        // enter the component in list of all components on the pcb
        assert genctx.comps.get (name) == null;
        genctx.comps.put (name, this);

        // maybe enter the component at the bottom of a column
        if (column != null) column.add (this);

        // fill in reference, eg, R235
        if (refcounts == null) refcounts = new HashMap<> ();

        // - use 1 higher than highest so far for the prefix
        Integer refcount = refcounts.get (refpfx);
        if (refcount == null) {
            refcount = 0;
        }
        refcounts.put (refpfx, ++ refcount);

        this.ref = refpfx + refcount;
    }

    @Override  // Placeable
    public String getName ()
    {
        return name;
    }

    @Override  // Placeable
    public Network[] getExtNets ()
    {
        return netperpin.values ().toArray (new Network[netperpin.values().size()]);
    }

    // set what network a pin is connected to
    public void setPinNetwork (String pino, Network net)
    {
        Network oldnet = netperpin.get (pino);
        if ((oldnet != null) && (oldnet != net)) {
            System.err.println ("Comp.setPinNetwork: " + name + " pin " + pino + " already connected to " + oldnet.name + " can't connect to " + net.name);
        }
        netperpin.put (pino, net);
    }

    // get what network a pin is connected to
    public Network getPinNetwork (String pino)
    {
        return netperpin.get (pino);
    }

    // get the network that a pin is connected to
    // return notation used by pcb software
    //  (net code "name")
    protected String pcbNetRef (String pino, GenCtx genctx)
    {
        Network net = netperpin.get (pino);
        if (net == null) {
            throw new NullPointerException ("pin '" + pino + "' of '" + name + "' not connected");
        }
        String wiredandnetname = genctx.getWiredAndNetworkName (net.name);
        net = genctx.nets.get (wiredandnetname);
        if (net.name.equals ("NIL")) return "";
        return "(net " + net.code + " \"" + net.name + "\")";
    }

    // get position of component on pcb
    // return format used by pcb software
    //  xmm ymm
    protected String pcbPosXY ()
    {
        return String.format ("%4.2f %4.2f", posx10th * 2.54, posy10th * 2.54);
    }

    @Override  // Placeable
    public void flipIt ()
    { }

    @Override  // Placeable
    public abstract int getWidth ();
    @Override  // Placeable
    public abstract int getHeight ();
    @Override  // Placeable
    public abstract double getPosX ();
    @Override  // Placeable
    public abstract double getPosY ();
    @Override  // Placeable
    public abstract void setPosXY (double leftx, double topy);

    // get printed circuit board xy in mm of a particular pin
    public final String pcbPosXYPin (String pino)
    {
        return String.format ("%4.2f %4.2f", pcbPosXPin (pino) * 2.54, pcbPosYPin (pino) * 2.54);
    }

    // get printed circuit board xy in 10ths of a particular pin
    public abstract double pcbPosXPin (String pino);
    public abstract double pcbPosYPin (String pino);

    // say that an hole has been drilled in the pcb for the given pin
    // this verifies that the location on the pcb isn't being used for anything else
    protected void holeDrilled (String pino)
    {
        netperpin.get (pino).hashole (pcbPosXPin (pino), pcbPosYPin (pino));
    }

    //// TRANSISTORS ////

    // normal:
    //     C +
    //   B
    //     E +
    // flipped:
    //   + E
    //       B
    //   + C
    public static class Trans extends Comp {
        public final static String PIN_E = "1";
        public final static String PIN_B = "2";
        public final static String PIN_C = "3";

        private boolean flip;
        private double leftx;
        private double topy;

        public Trans (GenCtx genctx, LinkedList<Comp> column, String name, String value)
        {
            super (genctx, column, name, "Q", value);

            if (! genctx.libparts.containsKey ("Q_NPN_EBC")) {
                genctx.libparts.put ("Q_NPN_EBC",
                    "   (libpart (lib device) (part Q_NPN_EBC)\n" +
                    "     (description \"Transistor NPN (general)\")\n" +
                    "     (fields\n" +
                    "       (field (name Reference) Q)\n" +
                    "       (field (name Value) Q_NPN_EBC))\n" +
                    "     (pins\n" +
                    "       (pin (num 1) (name E) (type passive))\n" +
                    "       (pin (num 2) (name B) (type passive))\n" +
                    "       (pin (num 3) (name C) (type passive))))\n");
            }
        }

        @Override  // Placeable
        public void flipIt ()
        {
            flip = ! flip;
        }

        @Override public int getWidth  () { return 4; }
        @Override public int getHeight () { return 3; }
        @Override
        public void setPosXY (double leftx, double topy)
        {
            this.leftx = leftx;
            this.topy  = topy;
        }
        @Override public double getPosX () { return leftx; }
        @Override public double getPosY () { return topy;  }

        @Override
        public void printnet (PrintStream ps)
        {
            ps.println ("    (comp (ref " + ref + ")");
            ps.println ("      (value " + value + ")");
            ps.println ("      (footprint TO_SOT_Packages_THT:TO-92_Molded_Wide)");
            ps.println ("      (libsource (lib device) (part Q_NPN_EBC))");
            ps.println ("      (sheetpath (names /) (tstamps /))");
            ps.println ("      (tstamp " + tstamp + "))");
        }

        @Override
        public void printres (Graphics2D g2d, int wf, int hf)
        {
            double x = (pcbPosXPin (PIN_B) + pcbPosXPin (PIN_C) + pcbPosXPin (PIN_E)) / 3;
            double y = (pcbPosYPin (PIN_B) + pcbPosYPin (PIN_C) + pcbPosYPin (PIN_E)) / 3;
            int ix = (int) Math.round (x * wf);
            int iy = (int) Math.round (y * hf);
            g2d.drawString (ref, ix, iy);
        }

        @Override
        public void printpcb (PrintStream ps, GenCtx genctx)
        {
            // where emitter goes
            posx10th = leftx + 2;
            posy10th = topy  + (flip ? 0 : 3);

            String path   = name;
            String tedit  = String.format ("%08X", name.hashCode ());
            String posxy  = pcbPosXY ();
            String pin1   = pcbNetRef ("1", genctx);
            String pin2   = pcbNetRef ("2", genctx);
            String pin3   = pcbNetRef ("3", genctx);

            ps.println (
                "  (module TO_SOT_Packages_THT:TO-92_Molded_Wide (layer F.Cu) (tedit " + tedit + ") (tstamp " + tstamp + ")\n" +
                "    (at " + posxy + " " + (flip ? "270" : "90") + ")\n" +
                "    (descr \"TO-92 leads molded, wide, drill 0.8mm (see NXP sot054_po.pdf)\")\n" +
                "    (tags \"to-92 sc-43 sc-43a sot54 PA33 transistor\")\n" +
                "    (path /" + path + ")\n" +
                "    (fp_text reference " + ref + " (at 2.54 -4.191 270) (layer F.SilkS)\n" +
                "      (effects (font (size 1 1) (thickness 0.15)))\n" +
                "    )\n" +
                "    (fp_text value " + value + " (at 2.54 2.794 90) (layer F.Fab)\n" +
                "      (effects (font (size 1 1) (thickness 0.15)))\n" +
                "    )\n" +
                "    (fp_line (start -1.1 -3.7) (end 6.1 -3.7) (layer F.CrtYd) (width 0.05))\n" +
                "    (fp_line (start 6.1 -3.7) (end 6.1 2.3) (layer F.CrtYd) (width 0.05))\n" +
                "    (fp_line (start 6.1 2.3) (end -1.1 2.3) (layer F.CrtYd) (width 0.05))\n" +
                "    (fp_line (start -1.1 2.3) (end -1.1 -3.7) (layer F.CrtYd) (width 0.05))\n" +
                "    (fp_line (start 0.74 1.85) (end 4.34 1.85) (layer F.SilkS) (width 0.12))\n" +
                "    (fp_line (start 0.8 1.75) (end 4.3 1.75) (layer F.Fab) (width 0.1))\n" +
                "    (fp_arc (start 2.54 0) (end 0.74 1.85) (angle 20) (layer F.SilkS) (width 0.12))\n" +
                "    (fp_arc (start 2.54 0) (end 2.54 -2.6) (angle -65) (layer F.SilkS) (width 0.12))\n" +
                "    (fp_arc (start 2.54 0) (end 2.54 -2.6) (angle 65) (layer F.SilkS) (width 0.12))\n" +
                "    (fp_arc (start 2.54 0) (end 2.54 -2.48) (angle 135) (layer F.Fab) (width 0.1))\n" +
                "    (fp_arc (start 2.54 0) (end 2.54 -2.48) (angle -135) (layer F.Fab) (width 0.1))\n" +
                "    (fp_arc (start 2.54 0) (end 4.34 1.85) (angle -20) (layer F.SilkS) (width 0.12))\n" +
                "    (pad 1 thru_hole rect   (at  0     0   180) (size 1.6 1.6) (drill 0.8) (layers *.Cu *.Mask) " + pin1 + ")\n" +
                "    (pad 2 thru_hole circle (at 2.54 -2.54 180) (size 1.6 1.6) (drill 0.8) (layers *.Cu *.Mask) " + pin2 + ")\n" +
                "    (pad 3 thru_hole circle (at 5.08   0   180) (size 1.6 1.6) (drill 0.8) (layers *.Cu *.Mask) " + pin3 + ")\n" +
                "    (model TO_SOT_Packages_THT.3dshapes/TO-92_Molded_Wide.wrl\n" +
                "      (at (xyz 0.1 0 0))\n" +
                "      (scale (xyz 1 1 1))\n" +
                "      (rotate (xyz 0 0 " + (flip ? "-270" : "-90") + "))\n" +
                "    )\n" +
                "  )");

            holeDrilled (PIN_E);
            holeDrilled (PIN_B);
            holeDrilled (PIN_C);
        }

        // get location of the given pin on the circuit board
        @Override
        public double pcbPosXPin (String pino)
        {
            double x;
            switch (pino) {
                case PIN_E: x = leftx + 2; break;
                case PIN_B: x = leftx + (flip ? 3 : 1); break;
                case PIN_C: x = leftx + 2; break;
                default: throw new IllegalArgumentException ("bad pin " + pino);
            }
            return x;
        }
        @Override
        public double pcbPosYPin (String pino)
        {
            double y;
            switch (pino) {
                case PIN_E: y = topy + (flip ? 0 : 3); break;
                case PIN_B: y = topy + (flip ? 1 : 2); break;
                case PIN_C: y = topy + (flip ? 2 : 1); break;
                default: throw new IllegalArgumentException ("bad pin " + pino);
            }
            return y;
        }
    }

    //// BYPASS CAPACITOR for TRANSISTOR ////
    // position in same place as transistor
    // transistor takes up all the board space
    // pins are offset just to right of transistor
    // the bypass capacitor size is 0x0 so it doesn't collide with transistor
    public static class ByCap extends Comp {

        // bypass capacitor pins
        public final static String PIN_Y = "4";     // normally vcc
        public final static String PIN_Z = "5";     // normally gnd

        private boolean flip;
        private boolean horiz;
        private boolean positioned;
        private double leftx;
        private double topy;

        public ByCap (GenCtx genctx, LinkedList<Comp> column, String name)
        {
            this (genctx, column, name, false);
        }

        public ByCap (GenCtx genctx, LinkedList<Comp> column, String name, boolean horiz)
        {
            super (genctx, column, name, "C", "0.1uF");
            this.horiz = horiz;
        }

        @Override  // Placeable
        public void flipIt ()
        {
            flip = ! flip;
        }

        @Override public int getWidth  () { return 0; }
        @Override public int getHeight () { return 0; }
        @Override
        public void setPosXY (double leftx, double topy)
        {
            this.leftx = leftx;
            this.topy  = topy;
            positioned = true;
        }
        @Override public double getPosX () { return leftx; }
        @Override public double getPosY () { return topy;  }

        @Override
        public void printnet (PrintStream ps)
        {
            ps.println ("    (comp (ref " + ref + ")");
            ps.println ("      (value " + value + ")");
            ps.println ("      (footprint Capacitors_THT:C_Axial_5.00mm)");
            ps.println ("      (libsource (lib device) (part CP))");     //??
            ps.println ("      (sheetpath (names /) (tstamps /))");
            ps.println ("      (tstamp " + tstamp + "))");
        }

        @Override
        public void printres (Graphics2D g2d, int wf, int hf)
        { }

        @Override
        public void printpcb (PrintStream ps, GenCtx genctx)
        {
            // where emitter goes
            posx10th = leftx + 2;
            posy10th = topy  + (flip ? 0 : 3);

            String path   = name;
            String tedit  = String.format ("%08X", name.hashCode ());
            String posxy  = pcbPosXY ();
            String pin4   = pcbNetRef ("4", genctx);    // normally vcc
            String pin5   = pcbNetRef ("5", genctx);    // normally gnd

            int rot = (horiz ? 0 : 90) + (flip ? 180 : 0);
            ps.println (
                "  (module Capacitors_THT:C_Axial_5.00mm (layer F.Cu) (tedit " + tedit + ") (tstamp " + tstamp + "C)\n" +
                "    (at " + posxy + " " + rot + ")\n" +
                "    (descr \"axial cap drill 0.8mm\")\n" +
                "    (tags \"C Axial series pin pitch 5.00mm diameter 2.41mm capacitor\")\n" +
                "    (path /" + path + ")\n" +
                "    (fp_text reference " + ref + " (at 2.54 -4.191 270) (layer F.SilkS)\n" +
                "      (effects (font (size 0 0) (thickness 0)))\n" +
                "    )\n" +
                //"    (fp_text value " + value + " (at 2.54 2.794 90) (layer F.Fab)\n" +
                //"      (effects (font (size 1 1) (thickness 0.15)))\n" +
                //"    )\n" +
                "    (pad 1 thru_hole circle (at 5.08  2.54 180) (size 1.6 1.6) (drill 0.8) (layers *.Cu *.Mask) " + pin4 + ")\n" +
                "    (pad 2 thru_hole circle (at  0    2.54 180) (size 1.6 1.6) (drill 0.8) (layers *.Cu *.Mask) " + pin5 + ")\n" +
                "  )");

            if (positioned) {
                holeDrilled (PIN_Y);
                holeDrilled (PIN_Z);
            }
        }

        // get location of the given pin on the circuit board
        @Override
        public double pcbPosXPin (String pino)
        {
            double x;
            switch (pino) {
                case PIN_Y: x = leftx + (flip ? 1 : 3); break;
                case PIN_Z: x = leftx + (flip ? 1 : 3); break;
                default: throw new IllegalArgumentException ("bad pin " + pino);
            }
            return x;
        }
        @Override
        public double pcbPosYPin (String pino)
        {
            double y;
            switch (pino) {
                case PIN_Z: y = topy + (flip ? 0 : 3); break;
                case PIN_Y: y = topy + (flip ? 2 : 1); break;
                default: throw new IllegalArgumentException ("bad pin " + pino);
            }
            return y;
        }
    }

    //// CAPACITORS ////

    public static class Capac extends Comp {
        public final static String PIN_P = "1";     // square pad / positive
        public final static String PIN_N = "2";

        private boolean positioned;
        private char pospin;

        public Capac (GenCtx genctx, LinkedList<Comp> column, String name, String value)
        {
            super (genctx, column, name, "C", value);

            pospin = 'U';

            if (! genctx.libparts.containsKey ("CP")) {
                genctx.libparts.put ("CP",
                    "    (libpart (lib device) (part CP)\n" +
                    "      (description \"Polarised capacitor\")\n" +
                    "      (footprints\n" +
                    "        (fp CP_*))\n" +
                    "      (fields\n" +
                    "        (field (name Reference) C)\n" +
                    "        (field (name Value) CP))\n" +
                    "      (pins\n" +
                    "        (pin (num 1) (name ~) (type passive))\n" +
                    "        (pin (num 2) (name ~) (type passive))))\n");
            }
        }

        @Override
        public void parseManual (String[] words, int i)
                throws Exception
        {
            for (; i < words.length; i ++) {
                switch (words[i].toUpperCase ()) {
                    case "D": pospin = 'D'; break;
                    case "L": pospin = 'L'; break;
                    case "R": pospin = 'R'; break;
                    case "U": pospin = 'U'; break;
                    default: throw new Exception ("unknown orientation " + words[i]);
                }
            }
        }

        @Override public int getWidth  () { return 3; }
        @Override public int getHeight () { return 3; }
        @Override
        public void setPosXY (double leftx, double topy)
        {
            posx10th = leftx + 1;
            posy10th = topy  + 1;
            positioned = true;
        }
        @Override public double getPosX () { return posx10th - 1; }
        @Override public double getPosY () { return posy10th - 1; }

        @Override
        public void printnet (PrintStream ps)
        {
            ps.println ("    (comp (ref " + ref + ")");
            ps.println ("      (value " + value + ")");
            ps.println ("      (footprint Capacitors_THT:C_Disc_D10.0mm_W2.5mm_P5.00mm)");
            ps.println ("      (libsource (lib device) (part CP))");
            ps.println ("      (sheetpath (names /) (tstamps /))");
            ps.println ("      (tstamp " + tstamp + "))");
        }

        @Override
        public void printres (Graphics2D g2d, int wf, int hf)
        { }

        @Override
        public void printpcb (PrintStream ps, GenCtx genctx)
        {
            String path   = name;
            String tedit  = String.format ("%08X", name.hashCode ());
            String posxy  = pcbPosXY ();
            String pin1   = pcbNetRef ("1", genctx);
            String pin2   = pcbNetRef ("2", genctx);

            String rot = "";
            switch (pospin) {
                case 'D': rot =  " 90"; break;
                case 'L': rot =   " 0"; break;
                case 'R': rot = " 180"; break;
                case 'U': rot = " 270"; break;
            }

            ps.println (
                "  (module Capacitors_THT:C_Disc_D10.0mm_W2.5mm_P5.00mm (layer F.Cu) (tedit " + tedit + ") (tstamp " + tstamp + ")\n" +
                "    (at " + posxy + rot + ")\n" +
                "    (descr \"C, Disc series, Radial, pin pitch=5.00mm, , diameter*width=10*2.5mm^2, Capacitor, http://cdn-reichelt.de/documents/datenblatt/B300/DS_KERKO_TC.pdf\")\n" +
                "    (tags \"C Disc series Radial pin pitch 5.00mm  diameter 10mm width 2.5mm Capacitor\")\n" +
                "    (path /" + path + ")\n" +
                "    (fp_text reference " + ref + " (at 2.5 -2.31) (layer F.SilkS)\n" +
                "      (effects (font (size 1 1) (thickness 0.15)))\n" +
                "    )\n" +
                "    (fp_text value " + value + " (at 2.5 2.31) (layer F.Fab)\n" +
                "      (effects (font (size 1 1) (thickness 0.15)))\n" +
                "    )\n" +
                "    (fp_line (start -2.5 -1.25) (end -2.5 1.25) (layer F.Fab) (width 0.1))\n" +
                "    (fp_line (start -2.5 1.25) (end 7.5 1.25) (layer F.Fab) (width 0.1))\n" +
                "    (fp_line (start 7.5 1.25) (end 7.5 -1.25) (layer F.Fab) (width 0.1))\n" +
                "    (fp_line (start 7.5 -1.25) (end -2.5 -1.25) (layer F.Fab) (width 0.1))\n" +
                "    (fp_line (start -2.56 -1.31) (end 7.56 -1.31) (layer F.SilkS) (width 0.12))\n" +
                "    (fp_line (start -2.56 1.31) (end 7.56 1.31) (layer F.SilkS) (width 0.12))\n" +
                "    (fp_line (start -2.56 -1.31) (end -2.56 1.31) (layer F.SilkS) (width 0.12))\n" +
                "    (fp_line (start 7.56 -1.31) (end 7.56 1.31) (layer F.SilkS) (width 0.12))\n" +
                "    (fp_line (start -2.85 -1.6) (end -2.85 1.6) (layer F.CrtYd) (width 0.05))\n" +
                "    (fp_line (start -2.85 1.6) (end 7.85 1.6) (layer F.CrtYd) (width 0.05))\n" +
                "    (fp_line (start 7.85 1.6) (end 7.85 -1.6) (layer F.CrtYd) (width 0.05))\n" +
                "    (fp_line (start 7.85 -1.6) (end -2.85 -1.6) (layer F.CrtYd) (width 0.05))\n" +
                "    (pad 1 thru_hole rect (at 0 0) (size 1.6 1.6) (drill 0.8) (layers *.Cu *.Mask)\n" +
                "      " + pin1 + ")\n" +
                "    (pad 2 thru_hole circle (at 5 0) (size 1.6 1.6) (drill 0.8) (layers *.Cu *.Mask)\n" +
                "      " + pin2 + ")\n" +
                "    (model Capacitors_THT.3dshapes/C_Disc_D10.0mm_W2.5mm_P5.00mm.wrl\n" +
                "      (at (xyz 0 0 0))\n" +
                "      (scale (xyz 0.393701 0.393701 0.393701))\n" +
                "      (rotate (xyz 0 0 " + rot + "))\n" +
                "    )\n" +
                "  )");

            if (positioned) {
                holeDrilled (PIN_P);
                holeDrilled (PIN_N);
            }
        }

        // get location of the given pin on the circuit board
        @Override
        public double pcbPosXPin (String pino)
        {
            switch (pino) {
                case PIN_P: return posx10th;
                case PIN_N: return posx10th + 1;
                default: throw new IllegalArgumentException ("bad pin " + pino);
            }
        }
        @Override
        public double pcbPosYPin (String pino)
        {
            return posy10th;
        }
    }

    //// DIODES ////

    public static class Diode extends Comp {
        public final static String PIN_C = "1";     // banded end
        public final static String PIN_A = "2";

        private boolean flat;   // false: stand up on circuit board
                                //  true: lay flat on circuit board
        private boolean flip;   // false: PIN_C on left; PIN_A on right
                                //  true: PIN_A on left; PIN_C on right

        // normal:   cathode ---|<--- anode
        // flipped:  anode --->|--- cathode
        public Diode (GenCtx genctx, LinkedList<Comp> column, String name, boolean flip, boolean flat)
        {
            this (genctx, column, name, "1N4148", flip, flat);
        }

        public Diode (GenCtx genctx, LinkedList<Comp> column, String name, String value, boolean flip, boolean flat)
        {
            super (genctx, column, name, "D", value);
            this.flip = flip;
            this.flat = flat;

            if (! genctx.libparts.containsKey ("D_ALT")) {
                genctx.libparts.put ("D_ALT",
                    "   (libpart (lib device) (part D_ALT)\n" +
                    "     (description \"Diode, alternativ symbol\")\n" +
                    "     (footprints\n" +
                    "       (fp TO-???*)\n" +
                    "       (fp *SingleDiode)\n" +
                    "       (fp *_Diode_*)\n" +
                    "       (fp *SingleDiode*)\n" +
                    "       (fp D_*))\n" +
                    "     (fields\n" +
                    "       (field (name Reference) D)\n" +
                    "       (field (name Value) D_ALT))\n" +
                    "     (pins\n" +
                    "       (pin (num 1) (name K) (type passive))\n" +
                    "       (pin (num 2) (name A) (type passive))))\n");
            }
        }

        @Override  // Placeable
        public void flipIt ()
        {
            flip = ! flip;
        }

        @Override public int getWidth  () { return 4; }
        @Override public int getHeight () { return 2; }
        @Override
        public void setPosXY (double leftx, double topy)
        {
            posx10th = leftx + (flip ? (flat ? 3 : 2) : 1);
            posy10th = topy  + 1;
        }
        @Override public double getPosX () { return posx10th - (flip ? (flat ? 3 : 2) : 1); }
        @Override public double getPosY () { return posy10th - 1; }

        @Override
        public void printnet (PrintStream ps)
        {
            if (flat) {
                ps.println ("    (comp (ref " + ref + ")");
                ps.println ("      (value " + value + ")");
                ps.println ("      (footprint Diodes_THT:D_T-1_P5.08mm_Horizontal)");
                ps.println ("      (libsource (lib device) (part D_ALT))");
                ps.println ("      (sheetpath (names /) (tstamps /))");
                ps.println ("      (tstamp " + tstamp + "))");
            } else {
                ps.println ("    (comp (ref " + ref + ")");
                ps.println ("      (value " + value + ")");
                ps.println ("      (footprint Diodes_THT:D_DO-35_SOD27_P2.54mm_Vertical_KathodeUp)");
                ps.println ("      (libsource (lib device) (part D_ALT))");
                ps.println ("      (sheetpath (names /) (tstamps /))");
                ps.println ("      (tstamp " + tstamp + "))");
            }
        }

        @Override
        public void printres (Graphics2D g2d, int wf, int hf)
        { }

        @Override
        public void printpcb (PrintStream ps, GenCtx genctx)
        {
            String path   = name;
            String tedit  = String.format ("%08X", name.hashCode ());
            String posxy  = pcbPosXY ();
            String pin1   = pcbNetRef ("1", genctx);
            String pin2   = pcbNetRef ("2", genctx);

            if (flat) {
                ps.println (
                    "  (module Diodes_THT:D_T-1_P5.08mm_Horizontal (layer F.Cu) (tedit " + tedit + ") (tstamp " + tstamp + ")\n" +
                    "    (at " + posxy + (flip ? " 180" : "") + ")\n" +
                    "    (descr \"D, T-1 series, Axial, Horizontal, pin pitch=5.08mm, , length*diameter=3.2*2.6mm^2, , http://www.diodes.com/_files/packages/T-1.pdf\")\n" +
                    "    (tags \"D T-1 series Axial Horizontal pin pitch 5.08mm  length 3.2mm diameter 2.6mm\")\n" +
                    "    (path /" + path + ")\n" +
                    "    (fp_text reference " + ref + " (at 2.54 " + (flip ? "1.86" : "-1.86") + ") (layer F.SilkS)\n" +
                    "      (effects (font (size 1 1) (thickness 0.15)))\n" +
                    "    )\n" +
                    "    (fp_text value " + value + " (at 2.54 " + (flip ? "-1.86" : "1.86") + ") (layer F.Fab)\n" +
                    "      (effects (font (size 1 1) (thickness 0.15)))\n" +
                    "    )\n" +
                    "    (fp_line (start 1.5 -0.8) (end 1.5 0.8) (layer F.Fab) (width 0.1))\n" +
                    "    (fp_line (start 0.74 -0.8) (end 0.74 0.8) (layer F.Fab) (width 0.1))\n" +
                    "    (fp_line (start 0.74 0.8) (end 4.34 0.8) (layer F.Fab) (width 0.1))\n" +
                    "    (fp_line (start 4.34 0.8) (end 4.34 -0.8) (layer F.Fab) (width 0.1))\n" +
                    "    (fp_line (start 4.34 -0.8) (end 0.74 -0.8) (layer F.Fab) (width 0.1))\n" +
                    "    (fp_line (start 0 0) (end 0.74 0) (layer F.Fab) (width 0.1))\n" +
                    "    (fp_line (start 5.08 0) (end 4.34 0) (layer F.Fab) (width 0.1))\n" +
                    "    (fp_line (start 1.5 -0.86) (end 1.5 0.86) (layer F.SilkS) (width 0.12))\n" +
                    "    (fp_line (start 0.68 -0.86) (end 4.4 -0.86) (layer F.SilkS) (width 0.12))\n" +
                    "    (fp_line (start 0.68 0.86) (end 4.4 0.86) (layer F.SilkS) (width 0.12))\n" +
                    "    (fp_line (start -0.95 -1.15) (end -0.95 1.15) (layer F.CrtYd) (width 0.05))\n" +
                    "    (fp_line (start -0.95 1.15) (end 6.05 1.15) (layer F.CrtYd) (width 0.05))\n" +
                    "    (fp_line (start 6.05 1.15) (end 6.05 -1.15) (layer F.CrtYd) (width 0.05))\n" +
                    "    (fp_line (start 6.05 -1.15) (end -0.95 -1.15) (layer F.CrtYd) (width 0.05))\n" +
                    "    (pad 1 thru_hole rect (at 0 0) (size 1.6 1.6) (drill 0.8) (layers *.Cu *.Mask)\n" +
                    "      " + pin1 + ")\n" +
                    "    (pad 2 thru_hole oval (at 5.08 0) (size 1.6 1.6) (drill 0.8) (layers *.Cu *.Mask)\n" +
                    "      " + pin2 + ")\n" +
                    "    (model Diodes_THT.3dshapes/D_T-1_P5.08mm_Horizontal.wrl\n" +
                    "      (at (xyz 0 0 0))\n" +
                    "      (scale (xyz 0.393701 0.393701 0.393701))\n" +
                    "      (rotate (xyz 0 0 " + (flip ? "180" : "0") + "))\n" +
                    "    )\n" +
                    "  )");
            } else {
                ps.println (
                    "  (module Diodes_THT:D_DO-35_SOD27_P2.54mm_Vertical_KathodeUp (layer F.Cu) (tedit " + tedit + ") (tstamp " + tstamp + ")\n" +
                    "    (at " + posxy + (flip ? " 180" : "") + ")\n" +
                    "    (descr \"D, DO-35_SOD27 series, Axial, Vertical, pin pitch=2.54mm, , length*diameter=4*2mm^2, , http://www.diodes.com/_files/packages/DO-35.pdf\")\n" +
                    "    (tags \"D DO-35_SOD27 series Axial Vertical pin pitch 2.54mm  length 4mm diameter 2mm\")\n" +
                    "    (path /" + path + ")\n" +
                    "    (fp_text reference " + ref + " (at 1.27 " + (flip ? "2.266371" : "-2.266371") + ") (layer F.SilkS)\n" +
                    "      (effects (font (size 1 1) (thickness 0.15)))\n" +
                    "    )\n" +
                    "    (fp_text value " + value + " (at 1.27 " + (flip ? "-2.266371" : "2.266371") + ") (layer F.Fab)\n" +
                    "      (effects (font (size 1 1) (thickness 0.15)))\n" +
                    "    )\n" +
                    "    (fp_circle (center 2.54 0) (end 3.54 0) (layer F.Fab) (width 0.1))\n" +
                    "    (fp_circle (center 2.54 0) (end 3.806371 0) (layer F.SilkS) (width 0.12))\n" +
                    "    (fp_line (start 0 0) (end 2.54 0) (layer F.Fab) (width 0.1))\n" +
                    "    (fp_line (start 1.266371 0) (end 1.44 0) (layer F.SilkS) (width 0.12))\n" +
                    "    (fp_line (start 1.397 0.98) (end 1.397 1.869) (layer F.SilkS) (width 0.12))\n" +
                    "    (fp_line (start 1.397 1.4245) (end 1.989667 0.98) (layer F.SilkS) (width 0.12))\n" +
                    "    (fp_line (start 1.989667 0.98) (end 1.989667 1.869) (layer F.SilkS) (width 0.12))\n" +
                    "    (fp_line (start 1.989667 1.869) (end 1.397 1.4245) (layer F.SilkS) (width 0.12))\n" +
                    "    (fp_line (start -1.15 -1.55) (end -1.15 1.55) (layer F.CrtYd) (width 0.05))\n" +
                    "    (fp_line (start -1.15 1.55) (end 3.85 1.55) (layer F.CrtYd) (width 0.05))\n" +
                    "    (fp_line (start 3.85 1.55) (end 3.85 -1.55) (layer F.CrtYd) (width 0.05))\n" +
                    "    (fp_line (start 3.85 -1.55) (end -1.15 -1.55) (layer F.CrtYd) (width 0.05))\n" +
                    "    (fp_text user K (at -1.5 0) (layer F.Fab)\n" +
                    "      (effects (font (size 1 1) (thickness 0.15)))\n" +
                    "    )\n" +
                    "    (pad 1 thru_hole rect (at 0 0) (size 1.6 1.6) (drill 0.8) (layers *.Cu *.Mask)\n" +
                    "      " + pin1 + ")\n" +
                    "    (pad 2 thru_hole oval (at 2.54 0) (size 1.6 1.6) (drill 0.8) (layers *.Cu *.Mask)\n" +
                    "      " + pin2 + ")\n" +
                    "    (model Diodes_THT.3dshapes/D_DO-35_SOD27_P2.54mm_Vertical_KathodeUp.wrl\n" +
                    "      (at (xyz 0 0 0))\n" +
                    "      (scale (xyz 0.393701 0.393701 0.393701))\n" +
                    "      (rotate (xyz 0 0 " + (flip ? "180" : "0") + "))\n" +
                    "    )\n" +
                    "  )");
            }

            holeDrilled (PIN_C);
            holeDrilled (PIN_A);
        }

        // get location of the given pin on the circuit board
        @Override
        public double pcbPosXPin (String pino)
        {
            switch (pino) {
                case PIN_C: return posx10th;
                case PIN_A: return posx10th + (flip ? -1 : 1) * (flat ? 2 : 1);
                default: throw new IllegalArgumentException ("bad pin " + pino);
            }
        }
        @Override
        public double pcbPosYPin (String pino)
        {
            return posy10th;
        }
    }

    //// LABELS ////

    public static class Label extends Comp {
        public final int size;      // tenths of an inch
        public final String value;

        public Label (GenCtx genctx, LinkedList<Comp> column, String name, String value, int size)
        {
            super (genctx, column, name, "X", "");
            this.value = value;
            this.size  = size;
        }

        @Override  // Placeable
        public void flipIt ()
        { }

        @Override public int getWidth  () { return size * value.length () / 2; }
        @Override public int getHeight () { return size; }
        @Override
        public void setPosXY (double leftx, double topy)
        {
            posx10th = leftx;
            posy10th = topy;
        }
        @Override public double getPosX () { return posx10th; }
        @Override public double getPosY () { return posy10th; }

        @Override
        public void printnet (PrintStream ps)
        { }

        @Override
        public void printres (Graphics2D g2d, int wf, int hf)
        { }

        @Override
        public void printpcb (PrintStream ps, GenCtx genctx)
        {
            String posxy = String.format ("%4.2f %4.2f", posx10th * 2.54, (posy10th + size / 2.0) * 2.54);

            String fs = String.format ("%3.1f", 1.5 * size);
            String th = String.format ("%3.1f", 0.3 * size);

            ps.println ("  (gr_text \"" + value + "\" (at " + posxy + ") (layer F.SilkS)");
            ps.println ("    (effects (font (size " + fs + " " + fs + ") (thickness " + th + ")) (justify left))");
            ps.println ("  )");
        }

        // get location of the given pin on the circuit board
        @Override
        public double pcbPosXPin (String pino)
        {
            return posx10th;
        }
        @Override
        public double pcbPosYPin (String pino)
        {
            return posy10th;
        }
    }

    //// SMALL LEDS ////

    public static class SmLed extends Comp {
        public final static String PIN_C = "1";
        public final static String PIN_A = "2";

        private boolean flip;

        public SmLed (GenCtx genctx, LinkedList<Comp> column, String name, String value, boolean flip)
        {
            super (genctx, column, name, "D", value);
            this.flip = flip;

            if (! genctx.libparts.containsKey ("LED_ALT")) {
                genctx.libparts.put ("LED_ALT",
                    "   (libpart (lib device) (part LED_ALT)\n" +
                    "     (description \"LED generic, alternativ symbol\")\n" +
                    "     (footprints\n" +
                    "       (fp LED*))\n" +
                    "     (fields\n" +
                    "       (field (name Reference) D)\n" +
                    "       (field (name Value) LED_ALT))\n" +
                    "     (pins\n" +
                    "       (pin (num 1) (name K) (type passive))\n" +
                    "       (pin (num 2) (name A) (type passive))))\n");
            }
        }

        @Override  // Placeable
        public void flipIt ()
        {
            flip = ! flip;
        }

        @Override public int getWidth  () { return 3; }
        @Override public int getHeight () { return 2; }
        @Override
        public void setPosXY (double leftx, double topy)
        {
            posx10th = leftx + (flip ? 1 : 0);
            posy10th = topy  + 1;
        }
        @Override public double getPosX () { return posx10th - (flip ? 2 : 1); }
        @Override public double getPosY () { return posy10th - 1; }

        @Override
        public void printnet (PrintStream ps)
        {
            ps.println ("    (comp (ref " + ref + ")");
            ps.println ("      (value " + value + ")");
            ps.println ("      (footprint LEDs:LED_D3.0mm)");
            ps.println ("      (libsource (lib device) (part LED_ALT))");
            ps.println ("      (sheetpath (names /) (tstamps /))");
            ps.println ("      (tstamp " + tstamp + "))");
        }

        @Override
        public void printres (Graphics2D g2d, int wf, int hf)
        { }

        @Override
        public void printpcb (PrintStream ps, GenCtx genctx)
        {
            String path   = name;
            String tedit  = String.format ("%08X", name.hashCode ());
            String posxy  = pcbPosXY ();
            String pin1   = pcbNetRef ("1", genctx);
            String pin2   = pcbNetRef ("2", genctx);

            ps.println (
                "  (module LEDs:LED_D3.0mm (layer F.Cu) (tedit " + tedit + ") (tstamp " + tstamp + ")\n" +
                "    (at " + posxy + (flip ? " 180" : "") + ")\n" +
                "    (descr \"LED, diameter 3.0mm, 2 pins\")\n" +
                "    (tags \"LED diameter 3.0mm 2 pins\")\n" +
                "    (path /" + path + ")\n" +
                "    (fp_text reference " + value.toUpperCase () + " (at 1.27 " + (flip ? "2.96" : "-2.96") + ") (layer F.SilkS)\n" +
                "      (effects (font (size 1 1) (thickness 0.15)))\n" +
                "    )\n" +
                "    (fp_text value SmLED (at 1.27 " + (flip ? "-2.96" : "2.96") + ") (layer F.Fab)\n" +
                "      (effects (font (size 1 1) (thickness 0.15)))\n" +
                "    )\n" +
                "    (fp_arc (start 1.27 0) (end -0.23 -1.16619) (angle 284.3) (layer F.Fab) (width 0.1))\n" +
                "    (fp_arc (start 1.27 0) (end -0.29 -1.235516) (angle 108.8) (layer F.SilkS) (width 0.12))\n" +
                "    (fp_arc (start 1.27 0) (end -0.29 1.235516) (angle -108.8) (layer F.SilkS) (width 0.12))\n" +
                "    (fp_arc (start 1.27 0) (end 0.229039 -1.08) (angle 87.9) (layer F.SilkS) (width 0.12))\n" +
                "    (fp_arc (start 1.27 0) (end 0.229039 1.08) (angle -87.9) (layer F.SilkS) (width 0.12))\n" +
                "    (fp_circle (center 1.27 0) (end 2.77 0) (layer F.Fab) (width 0.1))\n" +
                "    (fp_line (start -0.23 -1.16619) (end -0.23 1.16619) (layer F.Fab) (width 0.1))\n" +
                "    (fp_line (start -0.29 -1.236) (end -0.29 -1.08) (layer F.SilkS) (width 0.12))\n" +
                "    (fp_line (start -0.29 1.08) (end -0.29 1.236) (layer F.SilkS) (width 0.12))\n" +
                "    (fp_line (start -1.15 -2.25) (end -1.15 2.25) (layer F.CrtYd) (width 0.05))\n" +
                "    (fp_line (start -1.15 2.25) (end 3.7 2.25) (layer F.CrtYd) (width 0.05))\n" +
                "    (fp_line (start 3.7 2.25) (end 3.7 -2.25) (layer F.CrtYd) (width 0.05))\n" +
                "    (fp_line (start 3.7 -2.25) (end -1.15 -2.25) (layer F.CrtYd) (width 0.05))\n" +
                "    (pad 1 thru_hole rect (at 0 0) (size 1.8 1.8) (drill 0.9) (layers *.Cu *.Mask)\n" +
                "      " + pin1 + ")\n" +
                "    (pad 2 thru_hole circle (at 2.54 0) (size 1.8 1.8) (drill 0.9) (layers *.Cu *.Mask)\n" +
                "      " + pin2 + ")\n" +
                "    (model LEDs.3dshapes/LED_D3.0mm.wrl\n" +
                "      (at (xyz 0 0 0))\n" +
                "      (scale (xyz 0.393701 0.393701 0.393701))\n" +
                "      (rotate (xyz 0 0 " + (flip ? "180" : "0") + "))\n" +
                "    )\n" +
                "  )");

            holeDrilled (PIN_C);
            holeDrilled (PIN_A);
        }

        // get location of the given pin on the circuit board
        @Override
        public double pcbPosXPin (String pino)
        {
            switch (pino) {
                case PIN_C: return posx10th;
                case PIN_A: return posx10th + (flip ? -1 : 1);
                default: throw new IllegalArgumentException ("bad pin " + pino);
            }
        }
        @Override
        public double pcbPosYPin (String pino)
        {
            return posy10th;
        }
    }

    //// RESISTORS ////

    public static class Resis extends Comp {
        public final static String PIN_C = "1";     // banded end
        public final static String PIN_A = "2";
        public final static int WIDTH  = 4;
        public final static int HEIGHT = 2;

        public final static String R_OR  = "47K";   // input circuit pullup

        private boolean flat;   // false: stand up on circuit board
                                //  true: lay flat on circuit board
        private boolean flip;   // false: PIN_C on left; PIN_A on right
                                //  true: PIN_A on left; PIN_C on right
        private boolean positioned;

        public Resis (GenCtx genctx, LinkedList<Comp> column, String name, String value, boolean flip, boolean flat)
        {
            this (genctx, column, name, "R", value, flip, flat);
        }

        public Resis (GenCtx genctx, LinkedList<Comp> column, String name, String refpfx, String value, boolean flip, boolean flat)
        {
            super (genctx, column, name, refpfx, value);
            this.flat = flat;
            this.flip = flip;

            if (! genctx.libparts.containsKey ("R")) {
                genctx.libparts.put ("R",
                    "   (libpart (lib device) (part R)\n" +
                    "     (description Resistor)\n" +
                    "     (footprints\n" +
                    "       (fp R_*)\n" +
                    "       (fp R_*))\n" +
                    "     (fields\n" +
                    "       (field (name Reference) R)\n" +
                    "       (field (name Value) R))\n" +
                    "     (pins\n" +
                    "       (pin (num 1) (name ~) (type passive))\n" +
                    "       (pin (num 2) (name ~) (type passive))))\n");
            }
        }

        @Override  // Placeable
        public void flipIt ()
        {
            flip = ! flip;
        }

        @Override public int getWidth  () { return WIDTH;  }
        @Override public int getHeight () { return HEIGHT; }
        @Override
        public void setPosXY (double leftx, double topy)
        {
            posx10th = leftx + (flip ? (flat ? 3 : 2) : 1);
            posy10th = topy  + 1;
            positioned = true;
        }
        @Override public double getPosX () { return posx10th - (flip ? (flat ? 3 : 2) : 1); }
        @Override public double getPosY () { return posy10th - 1; }

        @Override
        public void printnet (PrintStream ps)
        {
            if (flat) {
                ps.println ("    (comp (ref " + ref + ")");
                ps.println ("      (value " + value + ")");
                ps.println ("      (footprint Resistors_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P5.08mm_Horizontal)");
                ps.println ("      (libsource (lib device) (part R))");
                ps.println ("      (sheetpath (names /) (tstamps /))");
                ps.println ("      (tstamp " + tstamp + "))");
            } else {
                ps.println ("    (comp (ref " + ref + ")");
                ps.println ("      (value " + value + ")");
                ps.println ("      (footprint Resistors_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P2.54mm_Vertical)");
                ps.println ("      (libsource (lib device) (part R))");
                ps.println ("      (sheetpath (names /) (tstamps /))");
                ps.println ("      (tstamp " + tstamp + "))");
            }
        }

        @Override
        public void printres (Graphics2D g2d, int wf, int hf)
        {
            double x = (pcbPosXPin (PIN_C) + pcbPosXPin (PIN_A)) / 2;
            double y = (pcbPosYPin (PIN_C) + pcbPosYPin (PIN_A)) / 2;
            int ix = (int) Math.round (x * wf);
            int iy = (int) Math.round (y * hf);
            g2d.drawString (ref, ix, iy);
            g2d.drawString (value, ix, iy + hf);
        }

        @Override
        public void printpcb (PrintStream ps, GenCtx genctx)
        {
            String path   = name;
            String tedit  = String.format ("%08X", name.hashCode ());
            String posxy  = pcbPosXY ();
            String pin1   = pcbNetRef ("1", genctx);
            String pin2   = pcbNetRef ("2", genctx);

            if (flat) {
                ps.println (
                    "  (module Resistors_THT:R_Axial_DIN0204_L3.6mm_D1.6mm_P5.08mm_Horizontal (layer F.Cu) (tedit " + tedit + ") (tstamp " + tstamp + ")\n" +
                    "    (at " + posxy + (flip ? " 180" : "") + ")\n" +
                    "    (descr \"Resistor, Axial_DIN0204 series, Axial, Horizontal, pin pitch=5.08mm, 0.125W = 1/8W, length*diameter=3.6*1.6mm^2, http://cdn-reichelt.de/documents/datenblatt/B400/1_4W%23YAG.pdf\")\n" +
                    "    (tags \"Resistor Axial_DIN0204 series Axial Horizontal pin pitch 5.08mm 0.125W = 1/8W length 3.6mm diameter 1.6mm\")\n" +
                    "    (path /" + path + ")\n" +
                    "    (fp_text reference " + ref + " (at 2.54 " + (flip ? "1.86" : "-1.86") + ") (layer F.SilkS)\n" +
                    "      (effects (font (size 1 1) (thickness 0.15)))\n" +
                    "    )\n" +
                    "    (fp_text value " + value + " (at 2.54 " + (flip ? "-1.86" : "1.86") + ") (layer F.Fab)\n" +
                    "      (effects (font (size 1 1) (thickness 0.15)))\n" +
                    "    )\n" +
                    "    (fp_line (start 0.74 -0.8) (end 0.74 0.8) (layer F.Fab) (width 0.1))\n" +
                    "    (fp_line (start 0.74 0.8) (end 4.34 0.8) (layer F.Fab) (width 0.1))\n" +
                    "    (fp_line (start 4.34 0.8) (end 4.34 -0.8) (layer F.Fab) (width 0.1))\n" +
                    "    (fp_line (start 4.34 -0.8) (end 0.74 -0.8) (layer F.Fab) (width 0.1))\n" +
                    "    (fp_line (start 0 0) (end 0.74 0) (layer F.Fab) (width 0.1))\n" +
                    "    (fp_line (start 5.08 0) (end 4.34 0) (layer F.Fab) (width 0.1))\n" +
                    "    (fp_line (start 0.68 -0.86) (end 4.4 -0.86) (layer F.SilkS) (width 0.12))\n" +
                    "    (fp_line (start 0.68 0.86) (end 4.4 0.86) (layer F.SilkS) (width 0.12))\n" +
                    "    (fp_line (start -0.95 -1.15) (end -0.95 1.15) (layer F.CrtYd) (width 0.05))\n" +
                    "    (fp_line (start -0.95 1.15) (end 6.05 1.15) (layer F.CrtYd) (width 0.05))\n" +
                    "    (fp_line (start 6.05 1.15) (end 6.05 -1.15) (layer F.CrtYd) (width 0.05))\n" +
                    "    (fp_line (start 6.05 -1.15) (end -0.95 -1.15) (layer F.CrtYd) (width 0.05))\n" +
                    "    (pad 1 thru_hole circle (at 0 0) (size 1.6 1.6) (drill 0.8) (layers *.Cu *.Mask)\n" +
                    "      " + pin1 + ")\n" +
                    "    (pad 2 thru_hole oval (at 5.08 0) (size 1.6 1.6) (drill 0.8) (layers *.Cu *.Mask)\n" +
                    "      " + pin2 + ")\n" +
                    "    (model Resistors_THT.3dshapes/R_Axial_DIN0204_L3.6mm_D1.6mm_P5.08mm_Horizontal.wrl\n" +
                    "      (at (xyz 0 0 0))\n" +
                    "      (scale (xyz 0.393701 0.393701 0.393701))\n" +
                    "      (rotate (xyz 0 0 " + (flip ? "180" : "0") + "))\n" +
                    "    )\n" +
                    "  )");
            } else {
                ps.println (
                    "  (module Resistors_THT:R_Axial_DIN0207_L6.3mm_D2.5mm_P2.54mm_Vertical (layer F.Cu) (tedit " + tedit + ") (tstamp " + tstamp + ")\n" +
                    "    (at " + posxy + (flip ? " 180" : "") + ")\n" +
                    "    (descr \"Resistor, Axial_DIN0207 series, Axial, Vertical, pin pitch=2.54mm, 0.125W = 1/8W, length*diameter=6.3*2.5mm^2\")\n" +
                    "    (tags \"Resistor Axial_DIN0207 series Axial Vertical pin pitch 2.54mm 0.125W = 1/8W length 6.3mm diameter 2.5mm\")\n" +
                    "    (path /" + path + ")\n" +
                    "    (fp_text reference " + ref + " (at 1.27 " + (flip ? "2.31" : "-2.31") + ") (layer F.SilkS)\n" +
                    "      (effects (font (size 1 1) (thickness 0.15)))\n" +
                    "    )\n" +
                    "    (fp_text value " + value + " (at 1.27 " + (flip ? "-2.31" : "2.31") + ") (layer F.Fab)\n" +
                    "      (effects (font (size 1 1) (thickness 0.15)))\n" +
                    "    )\n" +
                    "    (fp_circle (center 0 0) (end 1.25 0) (layer F.Fab) (width 0.1))\n" +
                    "    (fp_circle (center 0 0) (end 1.31 0) (layer F.SilkS) (width 0.12))\n" +
                    "    (fp_line (start 0 0) (end 2.54 0) (layer F.Fab) (width 0.1))\n" +
                    "    (fp_line (start 1.31 0) (end 1.44 0) (layer F.SilkS) (width 0.12))\n" +
                    "    (fp_line (start -1.6 -1.6) (end -1.6 1.6) (layer F.CrtYd) (width 0.05))\n" +
                    "    (fp_line (start -1.6 1.6) (end 3.65 1.6) (layer F.CrtYd) (width 0.05))\n" +
                    "    (fp_line (start 3.65 1.6) (end 3.65 -1.6) (layer F.CrtYd) (width 0.05))\n" +
                    "    (fp_line (start 3.65 -1.6) (end -1.6 -1.6) (layer F.CrtYd) (width 0.05))\n" +
                    "    (pad 1 thru_hole circle (at 0 0) (size 1.6 1.6) (drill 0.8) (layers *.Cu *.Mask)\n" +
                    "      " + pin1 + ")\n" +
                    "    (pad 2 thru_hole oval (at 2.54 0) (size 1.6 1.6) (drill 0.8) (layers *.Cu *.Mask)\n" +
                    "      " + pin2 + ")\n" +
                    "    (model Resistors_THT.3dshapes/R_Axial_DIN0207_L6.3mm_D2.5mm_P2.54mm_Vertical.wrl\n" +
                    "      (at (xyz 0 0 0))\n" +
                    "      (scale (xyz 0.393701 0.393701 0.393701))\n" +
                    "      (rotate (xyz 0 0 " + (flip ? "180" : "0") + "))\n" +
                    "    )\n" +
                    "  )");
            }

            if (positioned) {
                holeDrilled (PIN_C);
                holeDrilled (PIN_A);
            }
        }

        // get location of the given pin on the circuit board
        @Override
        public double pcbPosXPin (String pino)
        {
            double x;
            switch (pino) {
                case PIN_C: x = posx10th; break;
                case PIN_A: x = posx10th + (flip ? -1 : 1) * (flat ? 2 : 1); break;
                default: throw new IllegalArgumentException ("bad pin " + pino);
            }
            return x;
        }
        @Override
        public double pcbPosYPin (String pino)
        {
            return posy10th;
        }
    }

    //// CONNECTORS ////

    // pins for a 4x2 (4 rows, 2 columns) NORM:
    //       *1 2
    //        3 4
    //        5 6
    //        7 8
    // same 4x2 CW:
    //      7 5 3 1
    //      8 6 4 2
    // same 4x2 CCW:
    //      2 4 6 8
    //      1 3 5 7
    // same 4x2 FLIP:
    //        8 7
    //        6 5
    //        4 3
    //        2 1
    // same 4x2 OVER:
    //        2 1*
    //        4 3
    //        6 5
    //        8 7
    // same 4x2 OCW:
    //      8 6 4 2
    //      7 5 3 1
    // same 4x2 OCCW:
    //      1 3 5 7
    //      2 4 6 8
    // same 4x2 OFLIP:
    //        7 8
    //        5 6
    //        3 4
    //        1 2
    public static class Conn extends Comp {
        public final static int HPAD = 0;
        public final static int MINCOLS =  1;
        public final static int MAXCOLS = 40;
        public final static int MINROWS =  1;
        public final static int MAXROWS = 40;
        public static enum Orien { NORM, CW, FLIP, CCW, OVER, OCW, OFLIP, OCCW };

        private int ncols;
        private int nrows;
        private int xcols;
        private int yrows;
        public  Orien orien;
        private String ncspelt;

        public Conn (GenCtx genctx, LinkedList<Comp> column, String name, int nrows, int ncols)
        {
            super (genctx, column, name, "J", String.format ("CONN_%02dX%02d", nrows, ncols));
            this.ncols = ncols;
            this.nrows = nrows;
            this.orien = Orien.NORM;

            ncspelt = ncols == 1 ? "single" : ncols == 2 ? "double" : "multiple";

            // note: kicad seems to have terms 'column' and 'row' reversed
            //       it has either single or double row connectors
            //       ...but are always drawn long edge vertical

            if (! genctx.libparts.containsKey (value)) {
                StringBuilder sb = new StringBuilder ();
                sb.append ("    (libpart (lib conn) (part "); sb.append (value); sb.append (")\n");
                sb.append ("      (description \"Connector, ");
                sb.append (ncspelt);
                sb.append (" row, ");
                sb.append (value.substring (5));
                sb.append (", pin header\")\n");
                sb.append ("      (footprints\n");
                sb.append ("        (fp Pin_Header_Straight_"); sb.append (ncols); sb.append ("X*)\n");
                sb.append ("        (fp Pin_Header_Angled_"); sb.append (ncols); sb.append ("X*)\n");
                sb.append ("        (fp Socket_Strip_Straight_"); sb.append (ncols); sb.append ("X*)\n");
                sb.append ("        (fp Socket_Strip_Angled_"); sb.append (ncols); sb.append ("X*))\n");
                sb.append ("      (fields\n");
                sb.append ("        (field (name Reference) J)\n");
                sb.append ("        (field (name Value) "); sb.append (value); sb.append ("))\n");
                sb.append ("      (pins");
                for (int n = 0; ++ n <= ncols * nrows;) {
                    sb.append ("\n        (pin (num "); sb.append (n); sb.append (") (name P"); sb.append (n); sb.append (") (type passive))");
                }
                sb.append ("))\n");
                genctx.libparts.put (value, sb.toString ());
            }
        }

        @Override
        public void parseManual (String[] words, int i)
                throws Exception
        {
            for (; i < words.length; i ++) {
                switch (words[i].toUpperCase ()) {
                    case  "NORM": orien = Orien.NORM;  break;
                    case  "FLIP": orien = Orien.FLIP;  break;
                    case  "OVER": orien = Orien.OVER;  break;
                    case "OFLIP": orien = Orien.OFLIP; break;
                    case    "CW": orien = Orien.CW;    break;
                    case   "CCW": orien = Orien.CCW;   break;
                    case   "OCW": orien = Orien.OCW;   break;
                    case  "OCCW": orien = Orien.OCCW;  break;
                    default: throw new Exception ("unknown orientation " + words[i]);
                }
            }
        }

        @Override
        public int getWidth  ()
        {
            switch (orien) {
                case  NORM:
                case  FLIP:
                case  OVER:
                case OFLIP: return ncols + 1 + HPAD;
                case    CW:
                case   CCW:
                case   OCW:
                case  OCCW: return nrows + 1 + HPAD;
                default: throw new RuntimeException ();
            }
        }

        @Override
        public int getHeight ()
        {
            switch (orien) {
                case  NORM:
                case  FLIP:
                case  OVER:
                case OFLIP: return nrows + 3;
                case    CW:
                case   CCW:
                case   OCW:
                case  OCCW: return ncols + 3;
                default: throw new RuntimeException ();
            }
        }

        @Override
        public void setPosXY (double leftx, double topy)
        {
            // where upper left pin goes
            posx10th = leftx + 1;
            posy10th = topy  + 1;
        }
        @Override public double getPosX () { return posx10th - 1; }
        @Override public double getPosY () { return posy10th - 1; }

        @Override
        public void printnet (PrintStream ps)
        {
            ps.println ("    (comp (ref " + ref + ")");
            ps.println (String.format ("      (footprint Pin_Headers:Pin_Header_Straight_%dx%02d_Pitch2.54mm)", ncols, nrows));
            ps.println ("      (libsource (lib conn) (part " + value + "))");
            ps.println ("      (sheetpath (names /) (tstamps /))");
            ps.println ("      (tstamp " + tstamp + "))");
        }

        @Override
        public void printres (Graphics2D g2d, int wf, int hf)
        { }

        @Override
        public void printpcb (PrintStream ps, GenCtx genctx)
        {
            String path   = name;
            String tedit  = String.format ("%08X", name.hashCode ());
            String posxy  = pcbPosXY ();

            computeXColsYRows ();

            String ncxnr  = xcols + "x" + yrows;                            // eg 2x4

            String valuex = String.format ("%4.2f", 1.27 + 1.27 * (xcols - 2));
            String iybx   = String.format ("%4.2f", 3.81 + 2.54 * (xcols - 2));
            String bbbx   = String.format ("%4.2f", 3.93 + 2.54 * (xcols - 2));
            String ogbx   = String.format ("%4.2f", 4.10 + 2.54 * (xcols - 2));

            String valuey = String.format ("%4.2f", 50.65 + 2.54 * (yrows - 20));
            String iyby   = String.format ("%4.2f", 49.53 + 2.54 * (yrows - 20));
            String bbby   = String.format ("%4.2f", 49.65 + 2.54 * (yrows - 20));
            String ogby   = String.format ("%4.2f", 49.80 + 2.54 * (yrows - 20));

            ps.println (part1.replaceAll ("%tedit%", tedit).replaceAll ("%tstamp%", tstamp).
                    replaceAll ("%posxy%", posxy).replaceAll ("%path%", path).
                    replaceAll ("%ref%", ref).replaceAll ("%ncxnr%", ncxnr).
                    replaceAll ("%value%", value).replaceAll ("%valuey%", valuey).
                    replaceAll ("%iyby%", iyby).replaceAll ("%bbby%", bbby).
                    replaceAll ("%ogby%", ogby).replaceAll ("%iybx%", iybx).
                    replaceAll ("%bbbx%", bbbx).replaceAll ("%ogbx%", ogbx).
                    replaceAll ("%valuex%", valuex).replaceAll ("%ncspelt%", ncspelt));

            int npins = ncols * nrows;
            for (int pin = 0; pin < npins; pin ++) {
                String pinstr = Integer.toString (getPinNum (pin));
                if (netperpin.get (pinstr) != null) {
                    int ix = pin % xcols;
                    int iy = pin / xcols;
                    double xmm = ix * 2.54;
                    double ymm = iy * 2.54;
                    StringBuilder sb = new StringBuilder ();
                    sb.append ("    (pad ");
                    sb.append (pinstr);
                    sb.append (" thru_hole ");
                    sb.append (pinstr.equals ("1") ? "rect" : "oval");
                    sb.append (String.format (" (at %4.2f %4.2f)", xmm, ymm));
                    sb.append (" (size 1.7 1.7) (drill 1) (layers *.Cu *.Mask) ");
                    sb.append (pcbNetRef (pinstr, genctx));
                    sb.append (')');
                    if (pinstr.equals ("1")) {
                        // blue outline for pin 1
                        String xl = String.format ("%4.2f", xmm - 1.27);
                        String xr = String.format ("%4.2f", xmm + 1.27);
                        String yt = String.format ("%4.2f", ymm - 1.27);
                        String yb = String.format ("%4.2f", ymm + 1.27);
                        sb.append ("    (fp_line (start " + xl + " " + yt + ") (end " + xr + " " + yt + ") (layer F.SilkS) (width 0.12))");
                        sb.append ("    (fp_line (start " + xr + " " + yt + ") (end " + xr + " " + yb + ") (layer F.SilkS) (width 0.12))");
                        sb.append ("    (fp_line (start " + xr + " " + yb + ") (end " + xl + " " + yb + ") (layer F.SilkS) (width 0.12))");
                        sb.append ("    (fp_line (start " + xl + " " + yb + ") (end " + xl + " " + yt + ") (layer F.SilkS) (width 0.12))");
                    }
                    ps.println (sb);

                    netperpin.get (pinstr).hashole (posx10th + ix, posy10th + iy);
                }
            }

            ps.println (part2.replaceAll ("%ncxnr%", ncxnr));
        }

        // convert visual pin number to circuit pin number
        //  input:
        //   pins numbered (zero-based) left-to-right then top-to-bottom as connector appears on circuit board
        //  output:
        //   pins numbered (one-based) as in circuit
        private int getPinNum (int pin)
        {
            switch (orien) {
                case NORM:
                case OVER: {

                    // pins for a 4x2 (4 rows, 2 columns)
                    // NORM:
                    //       0=>0  1=>1
                    //       2=>2  3=>3
                    //       4=>4  5=>5
                    //       6=>6  7=>7

                    break;
                }

                case FLIP:
                case OFLIP: {

                    // pins for a 4x2 (4 rows, 2 columns)
                    // FLIP:
                    //       0=>7  1=>6
                    //       2=>5  3=>4
                    //       4=>3  5=>2
                    //       6=>1  7=>0

                    pin = ncols * nrows - pin - 1;
                    break;
                }

                case CW:
                case OCW: {

                    // pins for a 4x2 (4 rows, 2 columns)
                    // CW:
                    //      0=>6  1=>4  2=>2  3=>0   iy=0
                    //      4=>7  5=>5  6=>3  7=>1   iy=1
                    //      ix=0  ix=1  ix=2  ix=3

                    int ix = pin % nrows;
                    int iy = pin / nrows;
                    pin = ncols * nrows - ncols - ix * ncols + iy;
                    break;
                }

                case CCW:
                case OCCW: {

                    // pins for a 4x2 (4 rows, 2 columns)
                    // CCW:
                    //      0=>1  1=>3  2=>5  3=>7   iy=0
                    //      4=>0  5=>2  6=>4  7=>6   iy=1
                    //      ix=0  ix=1  ix=2  ix=3

                    int ix = pin % nrows;
                    int iy = pin / nrows;
                    pin = ix * ncols + ncols - iy - 1;
                    break;
                }

                default: throw new RuntimeException ();
            }

            // if turned over, exchange 0<->1, 2<->3, 4<->5, 6<->7 in all the above
            switch (orien) {
                case OVER:
                case OFLIP:
                case OCW:
                case OCCW: {
                    int ix = pin % ncols;
                    int iy = pin / ncols;
                    pin = (ncols - ix - 1) + iy * ncols;
                }
            }

            // result is 1-based
            return pin + 1;
        }

        private final static String part1 =
            "  (module Pin_Headers:Pin_Header_Straight_%ncxnr%_Pitch2.54mm (layer F.Cu) (tedit %tedit%) (tstamp %tstamp%)\n" +
            "    (at %posxy%)\n" +
            "    (descr \"Through hole straight pin header, %ncxnr%, 2.54mm pitch, %ncspelt% rows\")\n" +
            "    (tags \"Through hole pin header THT %ncxnr% 2.54mm %ncspelt% row\")\n" +
            "    (path /%path%)\n" +
            "    (fp_text reference %ref% (at %valuex% -2.39) (layer F.SilkS)\n" +
            "      (effects (font (size 1 1) (thickness 0.15)))\n" +
            "    )\n" +
            "    (fp_line (start -1.27 -1.27) (end -1.27 %iyby%) (layer F.Fab) (width 0.1))\n" +   // inner yellow box
            "    (fp_line (start -1.27 %iyby%) (end %iybx% %iyby%) (layer F.Fab) (width 0.1))\n" +
            "    (fp_line (start %iybx% %iyby%) (end %iybx% -1.27) (layer F.Fab) (width 0.1))\n" +
            "    (fp_line (start %iybx% -1.27) (end -1.27 -1.27) (layer F.Fab) (width 0.1))\n" +

            "    (fp_line (start -1.39 -1.39) (end %bbbx% -1.39) (layer F.SilkS) (width 0.12))\n" +  // blue box (no cutout)
            "    (fp_line (start %bbbx% -1.39) (end %bbbx% %bbby%) (layer F.SilkS) (width 0.12))\n" +
            "    (fp_line (start %bbbx% %bbby%) (end -1.39 %bbby%) (layer F.SilkS) (width 0.12))\n" +
            "    (fp_line (start -1.39 %bbby%) (end -1.39 -1.39) (layer F.SilkS) (width 0.12))\n" +

            "    (fp_line (start -1.6 -1.6) (end -1.6 %ogby%) (layer F.CrtYd) (width 0.05))\n" +  // outer gray box
            "    (fp_line (start -1.6 %ogby%) (end %ogbx% %ogby%) (layer F.CrtYd) (width 0.05))\n" +
            "    (fp_line (start %ogbx% %ogby%) (end %ogbx% -1.6) (layer F.CrtYd) (width 0.05))\n" +
            "    (fp_line (start %ogbx% -1.6) (end -1.6 -1.6) (layer F.CrtYd) (width 0.05))";

        private final static String part2 =
            "    (model Pin_Headers.3dshapes/Pin_Header_Straight_%ncxnr%_Pitch2.54mm.wrl\n" +
            "      (at (xyz 0.05 -0.95 0))\n" +
            "      (scale (xyz 1 1 1))\n" +
            "      (rotate (xyz 0 0 90))\n" +
            "    )\n" +
            "  )";

        // get location of the given pin on the circuit board
        @Override
        public double pcbPosXPin (String pino)
        {
            computeXColsYRows ();
            int npins = ncols * nrows;
            for (int pin = 0; pin < npins; pin ++) {
                String pinstr = Integer.toString (getPinNum (pin));
                if (pinstr.equals (pino)) {
                    return posx10th + pin % xcols;
                }
            }
            throw new RuntimeException ("bad pin " + pino);
        }
        @Override
        public double pcbPosYPin (String pino)
        {
            computeXColsYRows ();
            int npins = ncols * nrows;
            for (int pin = 0; pin < npins; pin ++) {
                String pinstr = Integer.toString (getPinNum (pin));
                if (pinstr.equals (pino)) {
                    return posy10th + pin / xcols;
                }
            }
            throw new RuntimeException ("bad pin " + pino);
        }

        // compute number of columns and number of rows as it appears on circuit board
        private void computeXColsYRows ()
        {
            switch (orien) {
                case  NORM:
                case  FLIP:
                case  OVER:
                case OFLIP: {
                    xcols = ncols;
                    yrows = nrows;
                    break;
                }
                case    CW:
                case   CCW:
                case   OCW:
                case  OCCW: {
                    xcols = nrows;
                    yrows = ncols;
                    break;
                }
                default: throw new RuntimeException ();
            }
        }
    }

    //// HOLE ////

    public static class Hole extends Comp {
        private String ncspelt;

        public Hole (GenCtx genctx, LinkedList<Comp> column, String name)
        {
            super (genctx, column, name, "J", "1pin");
        }

        @Override public int getWidth  () { return 2; }
        @Override public int getHeight () { return 2; }
        @Override
        public void setPosXY (double leftx, double topy)
        {
            // where center goes
            posx10th = leftx + 1;
            posy10th = topy  + 1;
        }
        @Override public double getPosX () { return posx10th - 1; }
        @Override public double getPosY () { return posy10th - 1; }

        @Override
        public void printnet (PrintStream ps)
        {
            ps.println ("    (comp (ref " + ref + ")");
            ps.println ("      (footprint Connectors:1pin)");
            ps.println ("      (libsource (lib conn) (part " + value + "))");
            ps.println ("      (sheetpath (names /) (tstamps /))");
            ps.println ("      (tstamp " + tstamp + "))");
        }

        @Override
        public void printres (Graphics2D g2d, int wf, int hf)
        { }

        @Override
        public void printpcb (PrintStream ps, GenCtx genctx)
        {
            String path   = name;
            String tedit  = String.format ("%08X", name.hashCode ());
            String posxy  = pcbPosXY ();

            ps.println ("  (module Connectors:1pin (layer F.Cu) (tedit " + tedit + ") (tstamp " + tstamp + ")");
            ps.println ("    (at " + posxy + ")");
            ps.println ("    (descr \"module 1 pin (ou trou mecanique de percage)\")");
            ps.println ("    (tags DEV)");
            ps.println ("    (path /" + path + ")");
            ps.println ("    (fp_text reference " + ref + " (at 0 0 0) (layer F.SilkS)");
            ps.println ("      (effects (font (size 0 0) (thickness 0)))");
            ps.println ("    )");
            ps.println ("    (fp_circle (center 0 0) (end 2 0.8) (layer F.Fab) (width 0.1))");
            ps.println ("    (fp_circle (center 0 0) (end 2.6 0) (layer F.CrtYd) (width 0.05))");
            ps.println ("    (fp_circle (center 0 0) (end 0 -2.286) (layer F.SilkS) (width 0.12))");
            ps.println ("    (pad 1 thru_hole circle (at 0 0) (size 4.064 4.064) (drill 3.048) (layers *.Cu *.Mask)))");
        }

        // get location of the given pin on the circuit board
        @Override
        public double pcbPosXPin (String pino)
        {
            throw new IllegalArgumentException ("sorry no prewiring holes");
        }
        @Override
        public double pcbPosYPin (String pino)
        {
            throw new IllegalArgumentException ("sorry no prewiring holes");
        }
    }
}
