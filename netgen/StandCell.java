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
 * Standard cell circuit (and-or-invert gate)
 *
                         (VCC)
                           |
                           _
                         orres
                           _                          (plate)----(outnet)
                           |
                           |                          (pwr)
                           |
    (innet)----[|<]----(andnet)----[>|]----(ornet)----(grid)
              andiode             ordiode

   8-tube connector power connections:          Second column same layout, but VF3/VF4 for separate filament power connector:

        GND  VF2  GND  GND  GND  GND  GND  GND      GND  VF4  GND  GND  GND  GND  GND  GND
        GND  VF1  GND  VGG  VCC  GND  VPP  GND      GND  VF3  GND  VGG  VCC  GND  VPP  GND

*/

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.TreeMap;

public class StandCell extends Placeable implements ICombo, IPrintLoc, IVerilog {
    public  final static int CONECHORIZ = 10;   // spacing of connectors on 8-tube boards
    public  final static int CONECVERT  = 15;
    private final static int CELLVGAP   =  0;   // vertical spacing between cells

    private LinkedList<Comp> col1, col2, col3;
    private ArrayList<ArrayList<Network>> inputss;
    private boolean closed;
    private boolean pulsestyle;
    private Comp.Diode[] ordiodes;
    private Comp.Diode[][] andiodess;
    private Comp.Resis bycap;
    private Comp.Resis[] orresiss;
    private Comp.Conn conec;
    private GenCtx genctx;
    private Network ornet;
    private Network outnet;
    private Network[] andnets;
    private Network[] daoins;
    private String tubeloc;
    public String name;

    public StandCell (GenCtx genctx, String name)
    {
        this.genctx = genctx;
        this.name   = name;

        genctx.standcells.add (this);

        inputss = new ArrayList<> ();

        // reserve some room on circuit board in the form of a small 3 column table
        // does not include the bypass capacitor
        //  col1: 'and' diodes
        //  col2: 'or' resistors, 'or' diodes
        //  col3: connector
        col1 = new LinkedList<Comp> ();
        col2 = new LinkedList<Comp> ();
        col3 = new LinkedList<Comp> ();

        // allocate and wire everything up to point of 'ornet' in above diagram
        ornet  = new Network (genctx, "O." + name);
        outnet = new Network (genctx, "Q." + name);

        conec = new Comp.Conn (genctx, col3, "J." + name, 3, 1);

        outnet.addConn (conec, "1");

        // bypass capacitor
        bycap = new Comp.Resis (genctx, null, "C." + name, "C", "0.1u", true, true);
        Network gndnet = genctx.nets.get ("GND");
        Network vccnet = genctx.nets.get ("VCC");
        gndnet.addConn (bycap, Comp.Resis.PIN_A);
        vccnet.addConn (bycap, Comp.Resis.PIN_C);
    }

    // get the output of this cell
    public Network getOutput ()
    {
        return outnet;
    }

    // for PulseModule, puts capacitor on input in place of diode
    public void addPulseInput (Network innet)
    {
        pulsestyle = true;
        addInput (0, innet);
        close ();
    }

    // add an input to this cell
    //  input:
    //   orindex = which input to the NOR gate this is for (starting with 0)
    //   innet = input to the AND gate connected to the NOR input
    //  output:
    //   innet added to cell
    //   returns innet's andindex (-1 if optimized out)
    public int addInput (int orindex, Network innet)
    {
        assert ! closed;

        // make sure there is an array to capture inputs for this branch of the NOR gate
        while (inputss.size () <= orindex) {
            inputss.add (new ArrayList<> ());
        }

        // if an input is hardwired to GND, disable collecting inputs to the AND gate
        if (innet.name.equals ("GND")) {
            inputss.set (orindex, null);
            return -1;
        }

        // get collection of inputs to this AND gate
        // if null, there is an hardwired GND input so don't bother with this new one
        ArrayList<Network> inputs = inputss.get (orindex);
        if (inputs == null) return -1;

        // ignore any inputs hardwired to VCC
        if (innet.name.equals ("VCC")) return -1;

        // AND gate not grounded, add new input
        inputs.add (innet);
        return inputs.size () - 1;
    }

    // no more inputs to add, create rest of components and wire them up
    public void close ()
    {
        assert ! closed;
        closed = true;

        // strip out grounded ANDs and count total inputs
        int totalinputs = 0;
        for (int i = inputss.size (); -- i >= 0;) {
            ArrayList<Network> inputs = inputss.get (i);
            if (inputs == null) inputss.remove (i);
            else totalinputs += inputs.size ();
        }

        // save all the input diodes
        andiodess = new Comp.Diode[inputss.size()][];

        // if no inputs, all inputs to OR gate are zero, so tie grid to ground
        //TODO someday optimize whole tube out
        if (inputss.size () == 0) {
            Network gndnet = genctx.nets.get ("GND");
            System.err.println ("StandCell.close: gate " + name + " has no inputs -- grid grounded");
            gndnet.addConn (conec, "3");
            return;
        }

        // all the 'or' cathodes go to the grid
        ornet.addConn  (conec, "3");

        // one end of 'or' resistors gets tied to VCC
        Network vccnet = genctx.nets.get ("VCC");

        // arrays of components
        ordiodes = new Comp.Diode[inputss.size()];
        orresiss = new Comp.Resis[inputss.size()];
        andnets  = new Network[inputss.size()];

        // each and junction can have an external wired-and circuit
        // separate those out here
        daoins = new Network[inputss.size()];

        // special layout for wide NAND layout

        //   col1        col2       col3
        //   andiode1a   orres1     connector
        //   andiode1b   ordiode1
        //   andiode1c   andiode1d
        //   andiode1f   andiode1e
        //   andiode1g   andiode1h

        if (inputss.size () == 1) {

            // get list of inputs to the one-and-only AND section
            ArrayList<Network> inputs = inputss.get (0);

            // get a possible diode-anded-output network
            // it is the anode end of a bunch of diodes
            Network daoin = separateDao (inputs);
            daoins[0] = daoin;

            // set up a network for all the anodes and the resistor
            Network andnet = (daoin != null) ? daoin : new Network (genctx, "A" + "." + name);
            Comp.Resis orres = new Comp.Resis (genctx, col2, "R" + "." + name, Comp.Resis.R_OR, true, true);
            vccnet.addConn (orres, Comp.Resis.PIN_C);

            Comp.Diode ordio = new Comp.Diode (genctx, col2, "D" + "." + name, true, true);
            ornet.addConn (ordio, Comp.Diode.PIN_C);
            andnet.preWire (ordio, Comp.Diode.PIN_A, orres, Comp.Resis.PIN_A);

            andnets[0]  = andnet;
            ordiodes[0] = ordio;
            orresiss[0] = orres;

            // now create, place and connect all the input diodes
            // place them in 1st and 2nd columns
            andiodess[0]   = new Comp.Diode[inputs.size()];
            Comp lastcomp  = orres;
            String lastpin = Comp.Resis.PIN_A;
            int lastcolno  = 1;
            for (int j = 0; j < inputs.size ();) {
                int colno  = lastcolno;
                if (col1.size () < col2.size ()) colno = 1;
                if (col2.size () < col1.size ()) colno = 2;
                LinkedList<Comp> colx = null;
                if (colno == 1) colx = col1;
                if (colno == 2) colx = col2;
                Network input = inputs.get (j ++);
                Comp.Diode andio = new Comp.Diode (genctx, colx, "D" + "_" + j + "." + name, colno == 2, true);
                andiodess[0][j-1] = andio;
                input.addConn (andio, Comp.Diode.PIN_C, 1);
                andnet.preWire (lastcomp, lastpin, andio, Comp.Diode.PIN_A);
                lastcomp  = andio;
                lastpin   = Comp.Diode.PIN_A;
                lastcolno = colno;
            }

            // this tries to draw wire from ordio cathode to connector grid pin
            // but only if they are at the same Y value (ie, line is short horizontal)
            // otherwise, line will have to be routed manually
            genctx.predrawables.add (new PreDrawCG ());
            return;
        }

        // general AND-OR-INVERT layout

        //   col1        col2       col3
        //   andiode1a   orres1     connector
        //   andiode1b   ordiode1
        //   andiode1c
        //   andiode1d
        //   andiode2a   orres2
        //   andiode2b   ordiode2
        //   andiode3a   orres3
        //               ordiode3

        for (int i = 0; i < inputss.size ();) {

            // even up column heights for second and later ordiodes
            if (i > 0) {
                int colht = Math.max (col1.size (), col2.size ());
                while (col1.size () < colht) col1.add (null);
                while (col2.size () < colht) col2.add (null);
            }

            // get list of inputs for this AND partition
            ArrayList<Network> inputs = inputss.get (i);

            // maybe one input to the and gate is the anode end of a bunch of diodes
            // if so, use that as the output network of the and gate
            Network daoin = separateDao (inputs);
            daoins[i++] = daoin;

            // set up a network for all the anodes and the resistor
            Network andnet = (daoin != null) ? daoin : new Network (genctx, "A" + i + "." + name);
            Comp.Resis orres = new Comp.Resis (genctx, col2, "R" + i + "." + name, Comp.Resis.R_OR, true, true);
            vccnet.addConn (orres, Comp.Resis.PIN_C);

            Comp.Diode ordio = new Comp.Diode (genctx, col2, "D" + i + "." + name, true, true);
            ornet.addConn (ordio, Comp.Diode.PIN_C);

            andnet.preWire (ordio, Comp.Diode.PIN_A, orres, Comp.Resis.PIN_A);

            andnets[i-1]  = andnet;
            ordiodes[i-1] = ordio;
            orresiss[i-1] = orres;

            // now create, place and connect all the input diodes
            // place them all in 1st column
            andiodess[i-1] = new Comp.Diode[inputs.size()];
            Comp lastcomp  = orres;
            String lastpin = Comp.Resis.PIN_A;
            for (int j = 0; j < inputs.size ();) {
                Network input = inputs.get (j ++);
                Comp.Diode andio = new Comp.Diode (genctx, col1, "D" + i + "_" + j + "." + name, false, true);
                andiodess[i-1][j-1] = andio;
                input.addConn (andio, Comp.Diode.PIN_C, 1);
                andnet.preWire (lastcomp, lastpin, andio, Comp.Diode.PIN_A);
                lastcomp  = andio;
                lastpin   = Comp.Diode.PIN_A;
            }
        }

        genctx.predrawables.add (new PreDrawAOI ());
        genctx.predrawables.add (new PreDrawCG  ());
    }

    // given a list of 'and' inputs, remove the possible one-and-only dao and return it
    // the dao net already has diodes and is their anode, therefore does not need a diode
    private Network separateDao (ArrayList<Network> inputs)
    {
        Network daoin = null;
        for (int i = inputs.size (); -- i >= 0;) {
            Network input = inputs.get (i);
            if (input.daotag) {
                if (daoin != null) {
                    throw new RuntimeException ("multiple dao networks on and gate " + daoin.name + "," + input.name);
                }
                daoin = input;
                inputs.remove (i);
            }
        }
        return daoin;
    }

    // predraw AOI wires that weren't done with preWire() calls
    private class PreDrawAOI implements PreDrawable {
        public void preDraw (PrintStream ps)
        {
            double ordio0cx = ordiodes[0].pcbPosXPin (Comp.Diode.PIN_C);
            double ordio0cy = ordiodes[0].pcbPosYPin (Comp.Diode.PIN_C);
            for (int i = 1; i < ordiodes.length; i ++) {
                Comp.Diode ordiode1 = ordiodes[i];
                double ordio1cx = ordiodes[i].pcbPosXPin (Comp.Diode.PIN_C);
                double ordio1cy = ordiodes[i].pcbPosYPin (Comp.Diode.PIN_C);
                ornet.putseg (ps, ordio0cx,       ordio0cy,       ordio0cx - 0.5, ordio0cy + 0.5, "B.Cu");
                ornet.putseg (ps, ordio0cx - 0.5, ordio0cy + 0.5, ordio1cx - 0.5, ordio1cy - 0.5, "B.Cu");
                ornet.putseg (ps, ordio1cx - 0.5, ordio1cy - 0.5, ordio1cx,       ordio1cy,       "B.Cu");
                ordio0cx = ordio1cx;
                ordio0cy = ordio1cy;
            }
        }
    }

    // predraw wire going from top ordio cathode to connector grid pin
    private class PreDrawCG implements PreDrawable {
        public void preDraw (PrintStream ps)
        {
            // draw wire from an ordiode cathode if it is at same Y as the connector's gate pin
            // if there isn't one, it will have to be routed manually, but this handles most cases
            double conec_3_xpos = conec.pcbPosXPin ("3");
            double conec_3_ypos = conec.pcbPosYPin ("3");
            for (Comp.Diode ordio : ordiodes) {
                double ordio_C_xpos = ordio.pcbPosXPin (Comp.Diode.PIN_C);
                double ordio_C_ypos = ordio.pcbPosYPin (Comp.Diode.PIN_C);
                if (ordio_C_ypos == conec_3_ypos) {
                    ornet.putseg (ps, ordio_C_xpos, ordio_C_ypos, conec_3_xpos, conec_3_ypos, "F.Cu");
                    break;
                }
            }
        }
    }

    private Comp[][] compcols;
    private int cellwidth;
    private int cellheight;
    private int[] colwidths;

    @Override  // Placeable
    public String getName ()
    {
        return name;
    }

    @Override  // Placeable
    public void flipIt ()
    {
    }

    @Override  // Placeable
    public int getWidth ()
    {
        sizeUpCell ();
        assert cellwidth <= CONECHORIZ;
        return CONECHORIZ;
    }

    @Override  // Placeable
    public int getHeight ()
    {
        sizeUpCell ();
        return cellheight + CELLVGAP;
    }

    private double saveleftx, savetopy;

    // place cell components on circuit board
    // the 3-pin connector is placed as integral part of the cell, not necessarily on the 1.0 x 1.5 grid yet
    @Override  // Placeable
    public void setPosXY (double leftx, double topy)
    {
        saveleftx = leftx;
        savetopy  = topy;

        bycap.setPosXY (leftx + 4.0, topy - 2.0);

        sizeUpCell ();

        // place parts on board by column
        for (int i = 0; i < compcols.length; i ++) {
            Comp[] compcol = compcols[i];
            double y = topy;
            for (Comp comp : compcol) {
                if (comp != null) {
                    comp.setPosXY (leftx, y);
                    y += comp.getHeight ();
                } else {
                    y += Comp.Resis.HEIGHT;
                }
            }
            leftx += colwidths[i];
        }
    }

    @Override  // Placeable
    public double getPosX ()
    {
        return saveleftx;
    }

    @Override  // Placeable
    public double getPosY ()
    {
        return savetopy;
    }

    private void sizeUpCell ()
    {
        assert closed;  // col1,2,3 all filled in and won't change

        if (compcols == null) {
            compcols = new Comp[][] {
                col1.toArray (new Comp[col1.size()]),
                col2.toArray (new Comp[col2.size()]),
                col3.toArray (new Comp[col3.size()]) };

            // find widths of each column and highest column height
            colwidths = new int[compcols.length];
            for (int i = 0; i < compcols.length; i ++) {
                Comp[] compcol = compcols[i];
                int width  = 0;
                int height = 0;
                for (int j = compcol.length; -- j >= 0;) {
                    int w, h;
                    Comp comp = compcol[j];
                    if (comp != null) {
                        w = comp.getWidth ();
                        h = comp.getHeight ();
                    } else {
                        w = Comp.Resis.WIDTH;
                        h = Comp.Resis.HEIGHT;
                    }
                    if (width < w) width = w;
                    height += h;
                }
                colwidths[i] = width;
                cellwidth += width;
                if (cellheight < height) cellheight = height;
            }
            /*
                System.out.print ("StandCell.sizeUpCell*: " + name + " w=" + cellwidth);
                for (int i = 0; i < colwidths.length; i ++) {
                    System.out.print (((i == 0) ? "=" : "+") + colwidths[i]);
                }
                System.out.println (" h=" + cellheight);
            */
        }
    }

    @Override  // Placeable
    public Network[] getExtNets ()
    {
        int i = 1;
        for (ArrayList<Network> inputs : inputss) i += inputs.size ();
        Network[] extnets = new Network[i];
        i = 0;
        extnets[i++] = outnet;
        for (ArrayList<Network> inputs : inputss) {
            for (Network input : inputs) {
                extnets[i++] = input;
            }
        }
        return extnets;
    }

    // various things about the cell

    public boolean has3Inputs ()
    {
        return (andiodess.length == 1) && (andiodess[0].length == 3);
    }

    public boolean hasSmLed ()
    {
        return false;
    }

    // print location of output pin of this StandCell
    public void printLocOut (PrintStream ps, Module genmod, String prefix)
    {
        // print location of pin 1 of the 3-pin connector cuz that's where the output always is
        double x = conec.pcbPosXPin ("1") / 10;
        double y = conec.pcbPosYPin ("1") / 10;
        ps.println (String.format ("%s%-4s  %4.1f,%4.1f  %s  =>  %s", prefix, conec.ref, x, y, tubeloc, outnet.name));

        // print names of all the inputs connected to the network
        TreeMap<String,Network.Conn> sorted = new TreeMap<> ();
        for (Network.Conn conn : outnet.connIterable ()) {
            if (conn.comp != conec) {
                sorted.put (conn.comp.name, conn);
            }
        }
        for (Network.Conn conn : sorted.values ()) {
            x = conn.comp.pcbPosXPin (conn.pino) / 10;
            y = conn.comp.pcbPosYPin (conn.pino) / 10;
            ps.println (String.format ("                                     =>  %-4s  %4.1f,%4.1f  %s<%s>", conn.comp.ref, x, y, conn.comp.name, conn.pino));
        }
    }

    // print location of all input pins to this StandCell that are connected to the given inet
    public void printLocIns (PrintStream ps, Module genmod, Network inet, String prefix)
    {
        for (Comp.Diode[] andiodes : andiodess) {
            for (Comp.Diode andiode : andiodes) {
                Network cnet = andiode.getPinNetwork (Comp.Diode.PIN_C);
                if (cnet == inet) {
                    double x = andiode.pcbPosXPin (Comp.Diode.PIN_C) / 10;
                    double y = andiode.pcbPosYPin (Comp.Diode.PIN_C) / 10;
                    ps.println (String.format ("%s%-4s  %4.1f,%4.1f  <=  %s", prefix, andiode.ref, x, y, getInetName (genmod, cnet)));
                }
            }
        }
    }

    // get network name, and if input pin, get associated signal name
    private static String getInetName (Module genmod, Network inet)
    {
        String inetname = inet.name;

        if (inetname.startsWith ("I") && inetname.endsWith ("con")) {
            for (OpndLhs variable : genmod.variables.values ()) {
                for (WField wfield : variable.iteratewriters ()) {
                    if (wfield.operand.name.equals (inetname)) {
                        return inetname + "  " + variable.name + "[" + wfield.hidim + ":" + wfield.lodim + "]";
                    }
                }
            }
        }

        return inetname;
    }

    public double colPcbX () { return conec.pcbPosXPin ("1"); }
    public double colPcbY () { return conec.pcbPosYPin ("1"); }

    public double ledPcbX () { return conec.pcbPosXPin ("1"); }
    public double ledPcbY () { return conec.pcbPosYPin ("1"); }

    public double resPcbX () { return conec.pcbPosXPin ("1"); }
    public double resPcbY () { return conec.pcbPosYPin ("1"); }

    public double inPcbX (int i) { return andiodess[0][i].pcbPosXPin (Comp.Diode.PIN_C); }
    public double inPcbY (int i) { return andiodess[0][i].pcbPosYPin (Comp.Diode.PIN_C); }

    // fill in connector's power pin

    private final static String[][] powernets = {
        { "GND",  "GND",  "GND",  "VF1",  "VF2",  "GND",  "GND",  "GND",   "GND",  "GND",  "GND",  "VF3",  "VF4",  "GND",  "GND",  "GND"  },
        { "GND",  "NIL",  "NIL",  "VCC",  "VCC",  "VGG",  "VPP",  "GND",   "GND",  "VPP",  "VGG",  "VCC",  "VCC",  "NIL",  "NIL",  "GND"  }
    };

    private static void connectPowerPin (GenCtx genctx, Comp.Conn conn, int row, int col)
    {
        String powername = powernets[row%2][col%16];
        Network powernet = genctx.nets.get (powername);
        powernet.addConn (conn, "2");
    }

    // on entry, the 3-pin connectors are integral to the cells
    // this function moves them to a 1.0 x 1.5 grid so they fit the 8-tube plugin boards
    public static void place8TubeConns (GenCtx genctx)
    {
        // put the standcells in columns in order left-to-right, top-to-bottom
        // if they don't fit in the columns, don't bother, map file will have to be edited
        boolean error = false;
        ArrayList<TreeMap<Double,StandCell>> columns = new ArrayList<> ();
        for (StandCell standcell : genctx.standcells) {
            double x = standcell.getPosX ();
            double y = standcell.getPosY ();
            int colnum = (int) Math.floor (x - MapEdit.BOARDL10TH) / CONECHORIZ;
            if ((colnum < 0) || (colnum * CONECHORIZ + MapEdit.BOARDL10TH != x) || (y < MapEdit.CONSH10TH)) {
                System.err.println ("StandCell.place8TubeConns: " + standcell.name + " at " + x + "," + y + " not in place");
                error = true;
            }
            while (columns.size () <= colnum) columns.add (new TreeMap<> ());
            columns.get (colnum).put (y, standcell);
        }
        if (error) return;

        // count number of rows then make it even for the 8-tube boards
        int numrows = 0;
        for (TreeMap<Double,StandCell> column : columns) {
            int n = column.size ();
            if (numrows < n) numrows = n;
        }
        numrows = (numrows + 1) & -2;

        // go through each column and assign connectors sequentially
        // assume the standcells are packed in tightly so we can simply count starting at 0
        int colno = 0;
        for (TreeMap<Double,StandCell> column : columns) {
            int rowno = 0;
            for (StandCell sc : column.values ()) {
                connectPowerPin (genctx, sc.conec, rowno, colno);
                sc.conec.setPosXY (MapEdit.BOARDL10TH + (colno + 1) * CONECHORIZ - sc.conec.getWidth (),
                                   MapEdit.CONSH10TH + rowno * CONECVERT);
                sc.tubeloc = Integer.toString ((rowno / 2) + 1) +                       // which board row, starting with 1 at top
                             ((colno >= 8) ? "L-U" : "R-U") +                           // left or right board column, looking at tube side
                             Integer.toString ((rowno % 2 + 1) * 4 - colno % 8 / 2) +   // which tube number, U1..U8
                             (char) ('B' - colno % 2);                                  // which tube section, A or B
                rowno ++;
            }
            while (rowno < numrows) {
                fillPowerConnector (genctx, rowno, colno);
                rowno ++;
            }
            colno ++;
        }

        // pad out columns on right for 8-tube panels
        while ((colno & 7) != 0) {
            for (int rowno = 0; rowno < numrows; rowno ++) {
                fillPowerConnector (genctx, rowno, colno);
            }
            colno ++;
        }

        genctx.standcellrows = numrows;
        genctx.standcellcols = colno;
    }

    private static void fillPowerConnector (GenCtx genctx, int rowno, int colno)
    {
        String conname = "J.etfill." + colno + "." + rowno;
        Comp.Conn conn = new Comp.Conn (genctx, null, conname, 3, 1);

        // place connector on main circuit board
        conn.setPosXY (MapEdit.BOARDL10TH + (colno + 1) * CONECHORIZ - conn.getWidth (),
                       MapEdit.CONSH10TH + rowno * CONECVERT);

        // connect its power pin
        connectPowerPin (genctx, conn, rowno, colno);

        // dummy networks for the other pins so we get holes drilled
        Network nilnet = genctx.nets.get ("NIL");
        nilnet.addConn (conn, "1");
        nilnet.addConn (conn, "3");
    }

    // print power traces to the 8-tube panel connectors
    // also draw an outline for 8-tube panel on back of board
    public static void printPowerTraces (PrintStream ps, GenCtx genctx)
    {
        if (genctx.standcellcols > 0) {

            // find power connectors on panel, they should be in standard places
            // - depends on cons.inc
            Comp.Conn pwr1 = (Comp.Conn) genctx.placeables.get ("Conn/pwr1");   // VF1 VF1 GND GND VF2 VF2
            Comp.Conn pwr2 = (Comp.Conn) genctx.placeables.get ("Conn/pwr2");   // VPP GND VGG GND VCC VCC
            Comp.Conn pwr3 = (Comp.Conn) genctx.placeables.get ("Conn/pwr3");   // VF3 VF3 GND GND VF4 VF4
            double ypwr = 2;
            assert pwr1.pcbPosYPin ("1") == ypwr;
            assert pwr2.pcbPosYPin ("1") == ypwr;
            double xpwr1 = pwr1.pcbPosXPin ("1");
            double xpwr2 = pwr2.pcbPosXPin ("1");
            double xpwr3 = (pwr3 == null) ? Double.NaN : pwr3.pcbPosXPin ("1");

            // get power networks we are going to wire up
            Network vf1net = genctx.nets.get ("VF1");
            Network vf2net = genctx.nets.get ("VF2");
            Network vf3net = genctx.nets.get ("VF3");
            Network vf4net = genctx.nets.get ("VF4");
            Network vggnet = genctx.nets.get ("VGG");
            Network vppnet = genctx.nets.get ("VPP");

            // do left column of panels then right column of panels
            for (int leftidx = 0; leftidx < genctx.standcellcols; leftidx += 8) {

                double vf1ovry = 6;                                                     // VF1 over row
                double vf2ovry = 4;                                                     // VF2 over row
                double vppovry = 6;                                                     // VPP over row
                double vggovry = 4;                                                     // VGG over row

                double leftxpos = MapEdit.BOARDL10TH + leftidx * CONECHORIZ;            // X-position of left edge of 8-tube board
                double vf1colx, vf2colx, vggcolx, vppcolx;
                if (leftidx == 0) {
                    vf1colx = leftxpos + 4 * CONECHORIZ - 2;                            // VF1 column X (just to left of connector)
                    vf2colx = leftxpos + 5 * CONECHORIZ - 2;                            // VF2 column X (just to left of connector)
                    vggcolx = leftxpos + 6 * CONECHORIZ;                                // VGG column X (just to right of connector)
                    vppcolx = leftxpos + 7 * CONECHORIZ;                                // VPP column X (just to left of connector)
                } else {
                    vppcolx = leftxpos + 2 * CONECHORIZ - 2;                            // VPP column X (just to left of connector)
                    vggcolx = leftxpos + 3 * CONECHORIZ - 2;                            // VGG column X (just to right of connector)
                    vf1colx = leftxpos + 4 * CONECHORIZ;                                // VF1 column X (just to left of connector)
                    vf2colx = leftxpos + 5 * CONECHORIZ;                                // VF2 column X (just to left of connector)
                }

                vf1net.putseg (ps, xpwr1 + 0, ypwr,   xpwr1 + 1, ypwr, "In1.Cu");       // pwr1.1 - pwr1.2 : yellow
                vf1net.putseg (ps, xpwr1 + 0, ypwr,   xpwr1 + 0, vf1ovry, "In1.Cu");    // pwr1.1 - down
                vf1net.putseg (ps, xpwr1 + 1, ypwr,   xpwr1 + 1, vf1ovry, "In1.Cu");    // pwr1.2 - down
                if (vf1colx > xpwr1 + 0) {
                    vf1net.putseg (ps, xpwr1 + 0, vf1ovry, vf1colx, vf1ovry, "In1.Cu"); // over to just left of VF1 connector columns
                } else {
                    vf1net.putseg (ps, xpwr1 + 1, vf1ovry, vf1colx, vf1ovry, "In1.Cu"); // over to just left of VF1 connector columns
                }

                vf2net.putseg (ps, xpwr1 + 4, ypwr,   xpwr1 + 5, ypwr, "In1.Cu");       // pwr1.5 - pwr1.6 : yellow
                vf2net.putseg (ps, xpwr1 + 4, ypwr,   xpwr1 + 4, vf2ovry, "In1.Cu");    // pwr1.5 - down
                vf2net.putseg (ps, xpwr1 + 5, ypwr,   xpwr1 + 5, vf2ovry, "In1.Cu");    // pwr1.2 - down
                vf2net.putseg (ps, xpwr1 + 4, vf2ovry, vf2colx, vf2ovry, "In1.Cu");     // over to just left of VF2 connector columns

                if (leftidx == 0) {
                    vggnet.putseg (ps, xpwr2 + 2, ypwr, xpwr2 + 2, ypwr - 1, "In1.Cu");         // pwr2.3 - up
                    vggnet.putseg (ps, xpwr2 + 2, ypwr - 1, xpwr2 - 1.5, ypwr - 1, "In1.Cu");   // over to left
                    vggnet.putseg (ps, xpwr2 - 1.5, ypwr - 1, xpwr2 - 1.5, vggovry, "In1.Cu");  // down
                    vggnet.putseg (ps, xpwr2 - 1.5, vggovry, vggcolx, vggovry, "In1.Cu");       // over to column

                    vppnet.putseg (ps, xpwr2 + 0, ypwr, xpwr2 + 0, vppovry, "In1.Cu");          // pwr2.1 - down
                    vppnet.putseg (ps, xpwr2 + 0, vppovry, vppcolx, vppovry, "In1.Cu");         // over to column
                } else {
                    assert ! Double.isNaN (xpwr3);

                    vggnet.putseg (ps, xpwr2 + 2, ypwr, xpwr2 + 2, vggovry, "In1.Cu");
                    vggnet.putseg (ps, xpwr2 + 2, vggovry, vggcolx, vggovry, "In1.Cu");

                    vppnet.putseg (ps, xpwr2 + 0, vppovry, vppcolx, vppovry, "In1.Cu");
                }

                // draw columns to connect various powers to the panels
                double lastvf1piny = vf1ovry;
                double lastvf2piny = vf2ovry;
                double lastvggpiny = vggovry;
                double lastvpppiny = vppovry;
                int nip = (leftidx == 0) ? 1 : -1;
                for (int topidx = 0; topidx < genctx.standcellrows; topidx += 2) {

                    // top edge of 8-tube board
                    double topypos = MapEdit.CONSH10TH + topidx * CONECVERT;

                    double vf1piny = topypos + 2;
                    vf1net.putseg (ps, vf1colx, lastvf1piny, vf1colx, vf1piny, "In1.Cu");
                    vf1net.putseg (ps, vf1colx, vf1piny, vf1colx + nip, vf1piny, "In1.Cu");
                    lastvf1piny = vf1piny;

                    double vf2piny = topypos + 2;
                    vf2net.putseg (ps, vf2colx, lastvf2piny, vf2colx, vf2piny, "In1.Cu");
                    vf2net.putseg (ps, vf2colx, vf2piny, vf2colx + nip, vf2piny, "In1.Cu");
                    lastvf2piny = vf2piny;

                    double vggpiny = topypos + 2 + CONECVERT;
                    vggnet.putseg (ps, vggcolx, lastvggpiny, vggcolx, vggpiny, "In1.Cu");
                    vggnet.putseg (ps, vggcolx, vggpiny, vggcolx - nip, vggpiny, "In1.Cu");
                    lastvggpiny = vggpiny;

                    double vpppiny = topypos + 2 + CONECVERT;
                    vppnet.putseg (ps, vppcolx, lastvpppiny, vppcolx, vpppiny, "In1.Cu");
                    vppnet.putseg (ps, vppcolx, vpppiny, vppcolx - nip, vpppiny, "In1.Cu");
                    lastvpppiny = vpppiny;

                    // draw box on back side of board
                    double lx = (leftxpos + 1.5) * 2.54;
                    double rx = lx + 7.9 * 25.4;
                    double ty = (topypos - 1.5) * 2.54;
                    double by = ty + 2.9 * 25.4;
                    ps.println (String.format ("  (gr_line (start %4.2f %4.2f) (end %4.2f %4.2f) (angle 90) (layer B.SilkS) (width 0.2))", lx, ty, rx, ty));
                    ps.println (String.format ("  (gr_line (start %4.2f %4.2f) (end %4.2f %4.2f) (angle 90) (layer B.SilkS) (width 0.2))", rx, ty, rx, by));
                    ps.println (String.format ("  (gr_line (start %4.2f %4.2f) (end %4.2f %4.2f) (angle 90) (layer B.SilkS) (width 0.2))", rx, by, lx, by));
                    ps.println (String.format ("  (gr_line (start %4.2f %4.2f) (end %4.2f %4.2f) (angle 90) (layer B.SilkS) (width 0.2))", lx, by, lx, ty));
                }

                // set up to do second column
                vf1net = vf3net;
                vf2net = vf4net;
                xpwr1  = xpwr3;
            }
        }
    }

    // get all input networks
    public LinkedList<Network> getInputs ()
    {
        LinkedList<Network> innets = new LinkedList<> ();
        for (ArrayList<Network> inputs : inputss) {
            for (Network input : inputs) {
                innets.add (input);
            }
        }
        if (daoins != null) {
            for (Network daoin : daoins) {
                if (daoin != null) innets.add (daoin);
            }
        }
        return innets;
    }

    // get output network
    @Override  // ICombo
    public Network[] getIComboOutputs ()
    {
        return new Network[] { outnet };
    }

    // get input networks
    @Override  // ICombo
    public LinkedList<Network> getIComboInputs ()
    {
        return getInputs ();
    }

    // print CSource statement for boolean assignment
    @Override  // ICombo
    public int printIComboCSource (PrintStream ps)
    {
        String outvname = outnet.getVeriName (true);
        String envnam = "stuckon_" + outvname;
        String envstr = System.getenv (envnam);
        if ((envstr != null) && !envstr.equals ("")) {
            ps.println ("    nto += " + outvname + " = " + envstr + ";");
            System.out.println ("StandCell: " + envnam + "=" + envstr);
        } else if (inputss.size () == 0) {
            ps.println ("    nto += " + outvname + " = true;");
        } else {
            String ssvname = Network.getVeriName (true, name);
            ps.print ("    nto += " + outvname + " = !(");
            String sep1 = "(";
            for (int i = 0; i < inputss.size (); i ++) {
                ArrayList<Network> inputs = inputss.get (i);
                ps.print (sep1);
                String sep2 = "";
                for (Network input : inputs) {
                    ps.print (sep2);
                    ps.print (input.getVeriName (true));
                    sep2 = " & ";
                }
                Network daoin = daoins[i];
                if (daoin != null) {
                    ps.print (sep2);
                    ps.print (daoin.getVeriName (true));
                    sep2 = " & ";
                }
                if (sep2.equals ("")) ps.print ("true");
                sep1 = ") | (";
            }
            ps.println ("));");
        }
        return 1;
    }

    // print input and output pin locations for the StandCell as a gate
    @Override  // IPrintLoc
    public void printLoc (PrintStream ps, Module genmod)
    {
        printLocOut (ps, genmod, "       out: ");
        String pfx = "        in: ";
        for (Comp.Diode[] andiodes : andiodess) {
            for (Comp.Diode andiode : andiodes) {
                Network cnet = andiode.getPinNetwork (Comp.Diode.PIN_C);
                double x = andiode.pcbPosXPin (Comp.Diode.PIN_C) / 10;
                double y = andiode.pcbPosYPin (Comp.Diode.PIN_C) / 10;
                ps.println (String.format ("%s%-4s  %4.1f,%4.1f  <=  %s", pfx, andiode.ref, x, y, getInetName (genmod, cnet)));
                pfx = "      and   ";
            }
            pfx = "     or     ";
        }
    }

    // get verilog networks
    @Override  // IVerilog
    public void getVeriNets (HashSet<Network> verinets)
    {
        verinets.add (outnet);
        for (ArrayList<Network> inputs : inputss) {
            for (Network input : inputs) {
                verinets.add (input);
            }
        }
        if (daoins != null) {
            for (Network daoin : daoins) {
                if (daoin != null) verinets.add (daoin);
            }
        }
    }

    // print verilog for this cell
    @Override  // IVerilog
    public void printVerilog (PrintStream ps, boolean myverilog)
    {
        String outvname = outnet.getVeriName (myverilog);
        if (myverilog) {
            String envnam = "stuckon_" + outvname;
            String envstr = System.getenv (envnam);
            if ((envstr != null) && !envstr.equals ("")) {
                ps.println ("    " + outvname + " = " + envstr + ";");
                System.out.println ("StandCell: " + envnam + "=" + envstr);
                return;
            }
        }
        if (inputss.size () == 0) {
            if (myverilog) ps.println ("    " + outvname + " = 1;");
            else ps.println ("    assign " + outvname + " = 1'b1;");
        } else {
            String ssvname = Network.getVeriName (myverilog, name);
            if (myverilog) {
                ps.print ("    " + outvname + " = ~(");
            } else {
                ps.print ("    StandCell " + ssvname + " (.U(uclk), ._Q(" + outvname + "), .D(");
            }
            String sep1 = "(";
            for (int i = 0; i < inputss.size (); i ++) {
                ArrayList<Network> inputs = inputss.get (i);
                ps.print (sep1);
                ps.print (myverilog ? "1" : "1'b1");
                for (Network input : inputs) {
                    ps.print (" & ");
                    ps.print (input.getVeriName (myverilog));
                }
                Network daoin = daoins[i];
                if (daoin != null) {
                    ps.print (" & ");
                    ps.print (daoin.getVeriName (myverilog));
                }
                sep1 = ") | (";
            }
            ps.println (myverilog ? "));" : ")));");
        }
    }
}
