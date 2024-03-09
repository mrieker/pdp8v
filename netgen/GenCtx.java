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
 * Various things collected while generating circuitry.
 */

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.PrintStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.Stack;
import java.util.TreeMap;
import java.util.TreeSet;

public class GenCtx {
    public final int boardwidth;                // 10th inch
    public final int boardheight;
    public final int maxrowwidth;
    public final int rightside;
    public final int topside;

    public LinkedList<DAOModule>     daomodules;
    public LinkedList<DFFModule>     dffmodules;
    public LinkedList<DLatModule>    dlatmodules;
    public LinkedList<ICombo>        icombos;
    public TreeMap<String,ISequen>   isequens;

    public ArrayList<PreDrawable>    predrawables;
    public HashMap<String,Comp>      comps;     // all components
    public HashMap<String,NetClass>  netclasses;
    public HashMap<String,Network>   nets;      // all networks (interconnections), including wired-and networks and their sub-networks
    public HashMap<String,Network[][]> prewiregrids;
    public HashMap<String,Placeable> placeables;
    public HashMap<String,String>    libparts;  // list of part types (eg, one entry for transistor, one for resistor, etc)
    public HashMap<String,WiredAnd>  wiredands; // all wired ands (combine networks)
    public int topgap;                          // in addition to TOPSIDE
    public LinkedList<Comp>          conncol;
    public LinkedList<StandCell>     standcells;
    public int                       standcellrows;
    public int                       standcellcols;
    public LinkedList<IVerilog>      verilogs;
    public TreeMap<String,IPrintLoc> printlocs;

    public GenCtx (int bw, int bh)
    {
        boardwidth   = bw;
        boardheight  = bh;

        maxrowwidth  = bw - 20;
        rightside    = bw - 10;
        topside      = 10;

        daomodules   = new LinkedList<> ();
        dffmodules   = new LinkedList<> ();
        dlatmodules  = new LinkedList<> ();
        icombos      = new LinkedList<> ();
        isequens     = new TreeMap<> ();
        comps        = new HashMap<> ();
        conncol      = new LinkedList<> ();
        standcells   = new LinkedList<> ();
        nets         = new HashMap<> ();
        libparts     = new HashMap<> ();
        netclasses   = new HashMap<> ();
        placeables   = new HashMap<> ();
        predrawables = new ArrayList<> ();
        prewiregrids = new HashMap<> ();
        printlocs    = new TreeMap<> ();
        wiredands    = new HashMap<> ();
        verilogs     = new LinkedList<> ();

        netclasses.put ("Default", new NetClass ("Default",
            "\"This is the default net class.\"\n" +
            "    (clearance 0.2)\n" +
            "    (trace_width 0.25)\n" +
            "    (via_dia 0.6)\n" +
            "    (via_drill 0.4)\n" +
            "    (uvia_dia 0.3)\n" +
            "    (uvia_drill 0.1)", 0.25));

        netclasses.put ("FILPWR", new NetClass ("FILPWR",
            "\"Filament Power.\"\n" +
            "    (clearance 0.2)\n" +
            "    (trace_width 1.0)\n" +
            "    (via_dia 0.6)\n" +
            "    (via_drill 0.4)\n" +
            "    (uvia_dia 0.3)\n" +
            "    (uvia_drill 0.1)", 1.00));

        netclasses.put ("HVPWR", new NetClass ("HVPWR",
            "\"High Voltage Power.\"\n" +
            "    (clearance 0.2)\n" +
            "    (trace_width 0.5)\n" +
            "    (via_dia 0.6)\n" +
            "    (via_drill 0.4)\n" +
            "    (uvia_dia 0.3)\n" +
            "    (uvia_drill 0.1)", 0.50));

        netclasses.put ("v33wires", new NetClass ("v33wires",
            "\"3.3V from RasPi\"\n" +
            "    (clearance 0.2)\n" +
            "    (trace_width 0.5)\n" +
            "    (via_dia 0.6)\n" +
            "    (via_drill 0.4)\n" +
            "    (uvia_dia 0.3)\n" +
            "    (uvia_drill 0.1)", 0.50));

        prewiregrids.put ("**holes", new Network[boardheight*2][boardwidth*2]);
        prewiregrids.put ("**vias",  new Network[boardheight*2][boardwidth*2]);
    }

    // get names of merged networks
    // networks get merged when they are wire-anded
    public TreeSet<String> getMergedNetNames ()
    {
        TreeSet<String> netnames = new TreeSet<> ();
        for (String netname : nets.keySet ()) {
            netnames.add (getWiredAndNetworkName (netname));
        }
        return netnames;
    }

    // if a network has been wire-anded with other networks,
    // get the one network name used to represent them all
    private HashMap<String,String> wanamemap;
    public String getWiredAndNetworkName (String netname)
    {
        // make a map of subnet names -> wired-and net name
        if (wanamemap == null) {
            wanamemap = new HashMap<> ();
            for (WiredAnd wiredand : wiredands.values ()) {
                for (Network subnet : wiredand.subnets) {
                    wanamemap.put (subnet.name, wiredand.name);
                }
            }
        }

        // see if given name, possibly a subnet name, translates to a wired-and net name
        for (String waname; (waname = wanamemap.get (netname)) != null;) {
            if (waname.equals (netname)) break;
            netname = waname;
        }

        return netname;
    }

    // add placeable component to circuit board
    public void addPlaceable (Placeable p)
    {
        String name = p.getName ();
        if (placeables.get (name) != null) {
            throw new RuntimeException ("duplicate placeable " + name);
        }
        placeables.put (name, p);
    }

    // place components on circuit board
    public void placeComponents (String mapname)
            throws Exception
    {
        if (mapname == null) {
            autoPlace ();
        } else {
            mappedPlace (mapname);
        }
    }

    // - doing automatic placement
    //   starts in top right corner and just fills right-to-left, top-to-bottom, starting with first output pin
    private void autoPlace ()
    {
        int topy = topside + topgap;
        Placeable[] placeablearray = placeables.values ().toArray (new Placeable[0]);
        int nextplaceable = 0;
        while (nextplaceable < placeablearray.length) {

            // make a row of placeable columns with one placeable per column to start with
            ArrayList<ArrayList<Placeable>> columns = new ArrayList<> ();
            int rowheight = 0;
            do {
                Placeable placeable = placeablearray[nextplaceable];
                int rowwidth = wholeRowWidth (columns);
                if (rowwidth + placeable.getWidth () > maxrowwidth) break;
                ArrayList<Placeable> column = new ArrayList<> ();
                column.add (placeable);
                columns.add (column);
                int h = placeable.getHeight ();
                if (rowheight < h) rowheight = h;
            } while (++ nextplaceable < placeablearray.length);

            // combine short columns as long as combination is no taller than tallest column in row
            for (int colindex = 0; colindex < columns.size () - 1; colindex ++) {
                do {
                    ArrayList<Placeable> thiscolumn = columns.get (colindex);
                    ArrayList<Placeable> nextcolumn = columns.get (colindex + 1);
                    if (columnHeight (thiscolumn) + columnHeight (nextcolumn) > rowheight) break;
                    thiscolumn.addAll (nextcolumn);
                    columns.remove (colindex + 1);
                } while (colindex < columns.size () - 1);
            }

            // try to fill out row with some more placeables if possible
            while (nextplaceable < placeablearray.length) {
                Placeable unseenplaceable = placeablearray[nextplaceable];
                int unseenheight = unseenplaceable.getHeight ();
                if (unseenheight > rowheight) break;
                ArrayList<Placeable> column = columns.get (columns.size () - 1);
                if (columnHeight (column) + unseenheight > rowheight) {
                    int rowwidth = wholeRowWidth (columns);
                    if (rowwidth + unseenplaceable.getWidth () > maxrowwidth) break;
                    column = new ArrayList<> ();
                    columns.add (column);
                }
                int colwidth = columnWidth (column);
                int w = unseenplaceable.getWidth ();
                int newcolwidth = colwidth > w ? colwidth : w;
                int rowwidth = wholeRowWidth (columns);
                if (rowwidth - colwidth + newcolwidth > maxrowwidth) break;
                column.add (unseenplaceable);
                nextplaceable ++;
            }

            assert wholeRowWidth (columns) <= maxrowwidth;

            // place components in the row
            int rightx = rightside;
            for (ArrayList<Placeable> column : columns) {
                int colwidth = columnWidth (column);
                rightx -= colwidth;
                int y = topy;
                for (Placeable placeable : column) {
                    placeable.setPosXY (rightx + placeable.offsetx, y + placeable.offsety);
                    y += placeable.getHeight ();
                }
            }
            topy += rowheight;
        }
    }

    private static int wholeRowWidth (ArrayList<ArrayList<Placeable>> columns)
    {
        int width = 0;
        for (ArrayList<Placeable> column : columns) width += columnWidth (column);
        return width;
    }

    private static int columnWidth (ArrayList<Placeable> column)
    {
        int width = 0;
        for (Placeable placeable : column) {
            int w = placeable.getWidth ();
            if (width < w) width = w;
        }
        return width;
    }

    private static int columnHeight (ArrayList<Placeable> column)
    {
        int height = 0;
        for (Placeable placeable : column) height += placeable.getHeight ();
        return height;
    }

    // - doing a mapped placement, use placement given in data file
    private void mappedPlace (String mapname)
            throws Exception
    {
        // read and parse map file
        HashMap<String,MappedPart> partsbyname = new HashMap<> ();
        MappedRow toprow = new MappedRow ();
        toprow.columns = new ArrayList<> ();
        Object opened = toprow;

        boolean partsmismatch = false;
        BufferedReader mapfile = new BufferedReader (new FileReader (mapname));
        for (String line; (line = mapfile.readLine ()) != null;) {
            int j = line.indexOf ('#');
            if (j >= 0) line = line.substring (0, j);
            line = line.trim ().replaceAll (",", " ").replaceAll ("\\s+", " ");
            String[] words = line.split (" ");

            // starting a table within a column
            if ((words.length >= 2) && words[0].equals ("tab") && words[words.length-1].equals ("{")) {
                if (! (opened instanceof MappedColumn)) {
                    throw new Exception ("tab { must be inside a col {");
                }
                MappedColumn mc = (MappedColumn) opened;
                MappedTable mt = new MappedTable ();
                mt.rows = new ArrayList<> ();
                mt.outercol = mc;
                mc.contents.add (mt);
                opened = mt;
            }

            // starting a row within a table
            if ((words.length >= 2) && words[0].equals ("row") && words[words.length-1].equals ("{")) {
                if (! (opened instanceof MappedTable)) {
                    throw new Exception ("row { must be inside a tab {");
                }
                MappedTable mt = (MappedTable) opened;
                MappedRow mr = new MappedRow ();
                mr.columns = new ArrayList<> ();
                mr.outertab = mt;
                mt.rows.add (mr);
                opened = mr;
            }

            // starting a column within a row
            if ((words.length >= 2) && words[0].equals ("col") && words[words.length-1].equals ("{")) {
                if (! (opened instanceof MappedRow)) {
                    throw new Exception ("col { must be inside a row {");
                }
                MappedRow mr = (MappedRow) opened;
                MappedColumn mc = new MappedColumn ();
                mc.contents = new ArrayList<> ();
                mc.outerrow = mr;
                for (int i = 0; ++ i < words.length - 1;) {
                    switch (words[i]) {
                        case "cellheight": {
                            mc.cellheight = Math.round (Float.parseFloat (words[++i]) * 10);
                            break;
                        }
                        case "width": {
                            mc.minwidth = Math.round (Float.parseFloat (words[++i]) * 10);
                            break;
                        }
                    }
                }
                mr.columns.add (mc);
                opened = mc;
            }

            // manual placement
            //   manual <name> [x,y] [flip]
            if ((words.length > 3) && words[0].equals ("manual")) {
                String name = words[1];
                Placeable placeable = placeables.get (name);
                if (placeable == null) {
                    System.err.println ("cannot find -pcbmap manual placeable " + name + " in module");
                    partsmismatch = true;
                } else {
                    if (placeable.placed) {
                        throw new Exception ("manual placeable already placed " + name);
                    }
                    String coords = words[2] + "," + words[3];
                    double x, y;
                    try {
                        if (! coords.startsWith ("[") || ! coords.endsWith ("]")) throw new Exception ("missing [ or ]");
                        int i = coords.indexOf (',');
                        x = Double.parseDouble (coords.substring (1, i));
                        y = Double.parseDouble (coords.substring (++ i, coords.length () - 1));
                    } catch (Exception e) {
                        throw new Exception ("bad -pcbmap manual " + name + " placement coord " + coords, e);
                    }
                    placeable.parseManual (words, 4);
                    placeable.setPosXY (x + placeable.offsetx, y + placeable.offsety);
                    placeable.placed = true;
                }
            }

            // add vertical padding somewhere within a column
            if ((words.length == 2) && words[0].equals ("vpad")) {
                if (! (opened instanceof MappedColumn)) {
                    throw new Exception ("vpad must be inside a col {");
                }
                MappedColumn mc = (MappedColumn) opened;
                MappedVPad vpad = new MappedVPad ();
                vpad.height = Math.round (Float.parseFloat (words[1]) * 10);
                mc.contents.add (vpad);
            }

            // adding a part to a column
            if ((words.length >= 7) && words[1].equals ("x") && words[3].equals ("at")) {
                if (! (opened instanceof MappedColumn)) {
                    throw new Exception ("part must be inside a col {");
                }
                MappedColumn mc = (MappedColumn) opened;
                MappedPart part = new MappedPart ();
                part.name       = words[6];
                part.placeable  = placeables.get (part.name);
                if (part.placeable == null) {
                    System.err.println ("placeable " + part.name + " listed in -pcbmap file not found in module");
                    partsmismatch = true;
                } else if (part.placeable.placed) {
                    System.err.println ("placeable " + part.name + " listed in -pcbmap file already placed");
                    partsmismatch = true;
                } else {
                    part.placeable.parseManual (words, 7);
                    part.column = mc;
                    mc.contents.add (part);
                    partsbyname.put (part.name, part);
                    part.placeable.placed = true;
                }
            }

            // closing table/row/column
            if ((words.length == 1) && words[0].equals ("}")) {
                if (opened == toprow) throw new Exception ("pcbmap } without matching {");
                if (opened instanceof MappedTable) opened = ((MappedTable) opened).outercol;
                else if (opened instanceof MappedColumn) opened = ((MappedColumn) opened).outerrow;
                else if (opened instanceof MappedRow) opened = ((MappedRow) opened).outertab;
            }
        }
        if (opened != toprow) throw new Exception ("pcbmap missing } for some {");
        mapfile.close ();

        // make sure all the placeables defined in module got placed on circuit board
        for (String name : placeables.keySet ()) {
            if (! placeables.get (name).placed) {
                System.err.println ("placeable " + name + " in module not listed in -pcbmap file");
                partsmismatch = true;
            }
        }
        if (partsmismatch) throw new Exception ("placeables in -pcbmap file mismatch placeables in module");

        // place the parts centered on the circuit board
        int toprowwidth = toprow.getWidth ();
        int leftx = (boardwidth - toprowwidth) / 2;
        System.out.println ("boardwidth=" + boardwidth + " totalwidth=" + toprowwidth + " leftmargin=" + leftx);
        int toprowheight = toprow.getHeight ();
        int topy = (boardheight - toprowheight) / 2;
        System.out.println ("boardheight=" + boardheight + " totalheight=" + toprowheight + " topmargin=" + topy);
        toprow.placeContents (leftx, topy);
    }

    private static class MappedColumn {
        public ArrayList<Object> contents;  // MappedPart, MappedTable, MappedVPad
        public int cellheight;
        public int minwidth;
        public MappedRow outerrow;

        // place parts and sub-columns on circuit board
        public void placeContents (int leftx, int topy)
        {
            int y = topy;
            int columnwidth = getWidth ();
            for (Object content : contents) {
                if (content instanceof MappedPart) {
                    MappedPart mp = (MappedPart) content;
                    Placeable pl = mp.placeable;
                    double plx = leftx + columnwidth - pl.getWidth () + pl.offsetx;
                    double ply = y + pl.offsety;
                    pl.setPosXY (plx, ply);
                    y += (cellheight > 0) ? cellheight : mp.placeable.getHeight ();
                }
                if (content instanceof MappedTable) {
                    MappedTable mt = (MappedTable) content;
                    int h = mt.getHeight ();
                    mt.placeContents (leftx, y);
                    y += h;
                }
                if (content instanceof MappedVPad) {
                    MappedVPad mv = (MappedVPad) content;
                    y += mv.height;
                }
            }
        }

        // get width used by column
        public int getWidth ()
        {
            int widest = minwidth;
            for (Object content : contents) {
                if (content instanceof MappedPart) {
                    MappedPart mp = (MappedPart) content;
                    int w = mp.placeable.getWidth ();
                    if (widest < w) widest = w;
                }
                if (content instanceof MappedTable) {
                    MappedTable mt = (MappedTable) content;
                    int w = mt.getWidth ();
                    if (widest < w) widest = w;
                }
            }
            return widest;
        }

        // get total height used by column including its parts and sub-tables
        public int getHeight ()
        {
            int height = 0;
            for (Object content : contents) {
                if (content instanceof MappedPart) {
                    MappedPart mp = (MappedPart) content;
                    height += (cellheight > 0) ? cellheight : mp.placeable.getHeight ();
                }
                if (content instanceof MappedTable) {
                    MappedTable mt = (MappedTable) content;
                    height += mt.getHeight ();
                }
                if (content instanceof MappedVPad) {
                    MappedVPad mv = (MappedVPad) content;
                    height += mv.height;
                }
            }
            return height;
        }
    }

    private static class MappedPart {
        public MappedColumn column;
        public Placeable placeable;
        public String name;
    }

    private static class MappedTable {
        public ArrayList<MappedRow> rows;
        public MappedColumn outercol;

        public void placeContents (int leftx, int topy)
        {
            for (MappedRow row : rows) {
                row.placeContents (leftx, topy);
                topy += row.getHeight ();
            }
        }

        public int getWidth ()
        {
            int widest = 0;
            for (MappedRow row : rows) {
                int rw = row.getWidth ();
                if (widest < rw) widest = rw;
            }
            return widest;
        }

        public int getHeight ()
        {
            int height = 0;
            for (MappedRow row : rows) {
                height += row.getHeight ();
            }
            return height;
        }
    }

    private static class MappedRow {
        public ArrayList<MappedColumn> columns;
        public MappedTable outertab;

        public void placeContents (int leftx, int topy)
        {
            for (MappedColumn column : columns) {
                column.placeContents (leftx, topy);
                leftx += column.getWidth ();
            }
        }

        public int getWidth ()
        {
            int width = 0;
            for (MappedColumn column : columns) {
                width += column.getWidth ();
            }
            return width;
        }

        public int getHeight ()
        {
            int height = 0;
            for (MappedColumn column : columns) {
                int h = column.getHeight ();
                if (height < h) height = h;
            }
            return height;
        }
    }

    private static class MappedVPad {
        public int height;
    }

    // assign network numbers
    public void assignNetNums ()
    {
        int newcode = 0;
        TreeSet<String> netnames = getMergedNetNames ();
        for (String name : netnames) {
            if (! name.equals ("NIL")) {
                Network net = nets.get (name);
                net.code = ++ newcode;
            }
        }
    }

    public LinkedList<PropRoot> proproots;

    public void addPropRoot (OpndRhs propopnd, int proprbit)
    {
        if (proproots == null) {
            proproots = new LinkedList<> ();
        }
        PropRoot root = new PropRoot ();
        root.propopnd = propopnd;
        root.proprbit = proprbit;
        proproots.add (root);
    }
}
