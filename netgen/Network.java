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
 * A network is a circuit board trace connecting particular pins of particular components.
 * There is also a network called "GND" and a network called "VCC" for ground and power.
 */

import java.io.PrintStream;
import java.util.HashSet;
import java.util.Iterator;
import java.util.LinkedList;

public class Network {
    public boolean daotag;      // diode-anded (this net is anode end of some diodes)
    public int code;            // sequential numbering starting with 1 (assigned when printing)
    public HashSet<Placeable> placeables;  // placeables connected to network;
    public final String name;

    public final static boolean KEEPGOING = true; // false;

    protected int loads;        // total reader load for standard logic nets

    private double cachednetlen = Double.NaN;
    private GenCtx genctx;
    private LinkedList<PreWire> prewires;
    private NetClass netclass;
    private String veriname;

    public ICombo icombooutput; // is the output of this ICombo logic
    public HashSet<Network> dependson;
    public int boolarrayindex;
    public int shortestlevel;

    // components and their pins that this network connects together
    protected LinkedList<Conn> connections;

    private static int lastcode = 0;

    // create a new network
    //  input:
    //   nets = list of all networks
    //   name = unique name for network
    public Network (GenCtx genctx, String name)
    {
        this (genctx, name, genctx.netclasses.get ("Default"));
    }
    public Network (GenCtx genctx, String name, NetClass netclass)
    {
        this.genctx = genctx;
        this.name = name;
        connections = new LinkedList<> ();
        prewires = new LinkedList<> ();
        if (genctx.nets.get (name) != null) {
            throw new RuntimeException ("duplicate network name " + name);
        }
        genctx.nets.put (name, this);
        netclass.networks.add (this);
        this.netclass = netclass;
    }

    // get verilog-compatible name
    public String getVeriName (boolean myverilog)
    {
        if (veriname == null) {
            veriname = getVeriName (myverilog, name);
        }
        return veriname;
    }

    public static String getVeriName (boolean myverilog, String name)
    {
        if (name.equals ("GND")) return myverilog ? "0" : "1'b0";
        if (name.equals ("VCC")) return myverilog ? "1" : "1'b1";
        int j = name.length ();
        StringBuilder sb = new StringBuilder (j + 2);
        for (int i = 0; i < j; i ++) {
            char c = name.charAt (i);
            if ((i == 0) && (c >= '0') && (c <= '9')) sb.append ("__");
            if ("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789".indexOf (c) < 0) c = '_';
            sb.append (c);
        }
        return sb.toString ();
    }

    // add a connection to the network
    //  input:
    //   comp = component to be connected
    //   pino = pin number of that component to connect
    //   load = component loads the network by this much
    //          standard logic lines only
    public void addConn (Comp comp, String pino)
    {
        addConn (comp, pino, 0);
    }
    public void addConn (Comp comp, String pino, int load)
    {
        Conn conn = new Conn (comp, pino);
        connections.add (conn);
        comp.setPinNetwork (pino, this);
        loads += load;
    }

    // just like calling addConn for the two pins
    // ...except draws a direct trace between the two pins
    // does not count any load on the network
    //  input:
    //   a = one component and pin
    //   b = other component and pin
    public void preWire (Comp acomp, String apino, Comp bcomp, String bpino)
    {
        Conn a = new Conn (acomp, apino);
        Conn b = new Conn (bcomp, bpino);
        connections.add (a);
        connections.add (b);
        acomp.setPinNetwork (apino, this);
        bcomp.setPinNetwork (bpino, this);
        PreWire pw = new PreWire ();
        pw.a = a;
        pw.b = b;
        prewires.add (pw);
    }

    // print all pre-wired connections on this network
    public void printPreWires (PrintStream ps)
    {
        for (PreWire pw : prewires) {
            double ax10th = pw.a.comp.pcbPosXPin (pw.a.pino);
            double ay10th = pw.a.comp.pcbPosYPin (pw.a.pino);
            double bx10th = pw.b.comp.pcbPosXPin (pw.b.pino);
            double by10th = pw.b.comp.pcbPosYPin (pw.b.pino);

            try {
                if (ax10th == bx10th) {
                    // vertical - use Back layer (Green)
                    putseg (ps, ax10th, ay10th, bx10th, by10th, "B.Cu");
                } else if (ay10th == by10th) {
                    // horizontal - use Front layer (Red)
                    putseg (ps, ax10th, ay10th, bx10th, by10th, "F.Cu");
                } else {
                    // at least 1 dot in both directions, use both layers and a via
                    putseg (ps, ax10th, ay10th, ax10th, by10th, "B.Cu");    // vertical on back (green)
                    putvia (ps, ax10th, by10th, "F.Cu", "B.Cu");            // via at intersection
                    putseg (ps, ax10th, by10th, bx10th, by10th, "F.Cu");    // horizontal on front (red)
                }
            } catch (Exception e) {
                System.err.println ("exception prewiring " + pw.a.comp.name + " pin " + pw.a.pino + " at " + ax10th + "," + ay10th +
                        " to " + pw.b.comp.name + " pin " + pw.b.pino + " at " + bx10th + "," + by10th);
                throw e;
            }
        }
    }

    // print a segment (straight-line portion of wiring trace) on the circuit board
    //  input:
    //   ps = circuit board file being written
    //   ax10th,ay10th = starting point (in 0.5 increments)
    //   bx10th,by10th = ending point (in 0.5 increments)
    //   layer = layer name "F.Cu" or "B.Cu"
    //  output:
    //   line written to ps
    public void putseg (PrintStream ps, double ax10th, double ay10th, double bx10th, double by10th, String layer)
    {
        // make sure we have a prewiregrid array for the given layer
        Network[][] prewiregrid = genctx.prewiregrids.get (layer);
        if (prewiregrid == null) {
            prewiregrid = new Network[genctx.boardheight*2][genctx.boardwidth*2];
            genctx.prewiregrids.put (layer, prewiregrid);
        }

        // see if there is already a trace going on the specific layer at any point along the segment
        // ...or if there is an hole or a via at any point along the segment
        Network[][] holes = genctx.prewiregrids.get ("**holes");
        Network[][] vias  = genctx.prewiregrids.get ("**vias");
        int ax20th = (int) Math.round (ax10th * 2.0);
        int ay20th = (int) Math.round (ay10th * 2.0);
        int bx20th = (int) Math.round (bx10th * 2.0);
        int by20th = (int) Math.round (by10th * 2.0);
        if (ax20th == bx20th) {
            int miny20th = Math.min (ay20th, by20th);
            int maxy20th = Math.max (ay20th, by20th);
            for (int iy20th = miny20th; iy20th <= maxy20th; iy20th ++) {
                checkTracePoint (ax20th, iy20th, layer);
            }
        } else if (ay20th == by20th) {
            int minx20th = Math.min (ax20th, bx20th);
            int maxx20th = Math.max (ax20th, bx20th);
            for (int ix20th = minx20th; ix20th <= maxx20th; ix20th ++) {
                checkTracePoint (ix20th, by20th, layer);
            }
        } else if (Math.abs (ax20th - bx20th) == Math.abs (ay20th - by20th)) {
            for (int i = Math.abs (ax20th - bx20th); i >= 0; -- i) {
                int ix20th = (ax20th - bx20th) * i / Math.abs (ay20th - by20th) + bx20th;
                int iy20th = (ay20th - by20th) * i / Math.abs (ax20th - bx20th) + by20th;
                checkTracePoint (ix20th, iy20th, layer);
            }
        } else {
            preWireError ("invalid prewire segment " + ax10th + "," + ay10th + " to " + bx10th + "," + by10th + " on " + layer);
        }

        // put it on the circuit board
        ps.println (String.format (
            "  (segment (start %4.2f %4.2f) (end %4.2f %4.2f) (width %4.2f) (layer %s) (net %d) (status C00000))",
                ax10th * 2.54, ay10th * 2.54, bx10th * 2.54, by10th * 2.54, netclass.widthmm, layer, code));
    }

    private void checkTracePoint (int ix20th, int iy20th, String layer)
    {
        Network[][] prewiregrid = genctx.prewiregrids.get (layer);
        Network[][] holes = genctx.prewiregrids.get ("**holes");
        Network[][] vias  = genctx.prewiregrids.get ("**vias");

        if (KEEPGOING) {
            if (ix20th < 0) return;
            if (iy20th < 0) return;
            if (ix20th >= genctx.boardwidth * 2)  return;
            if (iy20th >= genctx.boardheight * 2) return;
        }

        Network tn = prewiregrid[iy20th][ix20th];   // trace network at this location
        Network hn = holes[iy20th][ix20th];         // hole network at this location
        Network vn = vias[iy20th][ix20th];          // via network at this location
        if (tn != null && tn != this) {
            preWireError ("trace point for network <" + this.name + "> already occupied by trace <" + tn.name + "> at " + (ix20th / 2.0) + "," + (iy20th / 2.0));
        }
        if (hn != null && hn != this) {
            preWireError ("trace point for network <" + this.name + "> already occupied by hole <" + hn.name + "> at " + (ix20th / 2.0) + "," + (iy20th / 2.0));
        }
        if (vn != null && vn != this) {
            preWireError ("trace point for network <" + this.name + "> already occupied by via <" + vn.name + "> at " + (ix20th / 2.0) + "," + (iy20th / 2.0));
        }

        prewiregrid[iy20th][ix20th] = this;
    }

    // drill and fill a via on the circuit board
    //  input:
    //   ps = circuit board file being written
    //   vx10th,vy10th = via location (in 0.5 increments)
    //   layera,layerb = layer names "F.Cu" or "B.Cu"
    //  output:
    //   via written to ps
    public void putvia (PrintStream ps, double vx10th, double vy10th, String layera, String layerb)
    {
        int ix20th = (int) Math.round (vx10th * 2.0);
        int iy20th = (int) Math.round (vy10th * 2.0);

        if (KEEPGOING) {
            if (ix20th < 0) return;
            if (iy20th < 0) return;
            if (ix20th >= genctx.boardwidth * 2)  return;
            if (iy20th >= genctx.boardheight * 2) return;
        }

        // cannot be adjacent to an hole
        Network[][] holes = genctx.prewiregrids.get ("**holes");
        for (int i = -1; i <= 1; i ++) {
            int ix = ix20th - i;
            int iy = iy20th - i;
            if ((ix >= 0) && (ix < genctx.boardwidth * 2)  && (holes[iy20th][ix] != null)) {
                preWireError ("via for <" + this.name + "> adjacent to hole for <" + holes[iy20th][ix].name + "> at " + (ix20th / 2.0) + "," + (iy20th / 2.0));
            }
            if ((iy >= 0) && (iy < genctx.boardheight * 2) && (holes[iy][ix20th] != null)) {
                preWireError ("via for <" + this.name + "> adjacent to hole for <" + holes[iy][ix20th].name + "> at " + (ix20th / 2.0) + "," + (iy20th / 2.0));
            }
        }

        // cannot be at exact spot as a trace or another via unless it is same network
        for (Network[][] layergrid : genctx.prewiregrids.values ()) {
            Network n = layergrid[iy20th][ix20th];
            if ((n != null) && (n != this)) {
                preWireError ("via point already occupied");
            }
        }

        // remember we have a via here now
        Network[][] vias = genctx.prewiregrids.get ("**vias");
        vias[iy20th][ix20th] = this;

        // put via on pcb
        ps.println (String.format (
            "  (via (at %4.2f %4.2f) (size 0.6) (drill 0.4) (layers %s %s) (net %d))",
                vx10th * 2.54, vy10th * 2.54, layera, layerb, code));
    }

    // an hole is being drilled in the pcb for a component lead
    // nothing can be in same spot
    // solder pads are big enough to prohibit holes or vias on adjacent grid point
    // adjacent traces are ok
    public void hashole (double hx10th, double hy10th)
    {
        int ix20th = (int) Math.round (hx10th * 2.0);
        int iy20th = (int) Math.round (hy10th * 2.0);

        if (KEEPGOING) {
            if (ix20th < 0) return;
            if (iy20th < 0) return;
            if (ix20th >= genctx.boardwidth * 2)  return;
            if (iy20th >= genctx.boardheight * 2) return;
        }

        // make sure there isn't an hole or via in the same or adjacent spot already
        Network[][] holes = genctx.prewiregrids.get ("**holes");
        Network[][] vias  = genctx.prewiregrids.get ("**vias");
        for (int i = -1; i <= 1; i ++) {
            int ix = ix20th - i;
            int iy = iy20th - i;
            if (((ix >= 0) && (ix < genctx.boardwidth * 2)  && (holes[iy20th][ix] != null || vias[iy20th][ix] != null)) ||
                ((iy >= 0) && (iy < genctx.boardheight * 2) && (holes[iy][ix20th] != null || vias[iy][ix20th] != null))) {
                preWireError ("hole adjacent to hole or via");
            }
        }

        // make sure there isn't a trace in any layer on the exact hole spot
        for (Network[][] prewirenet : genctx.prewiregrids.values ()) {
            Network holenet = prewirenet[iy20th][ix20th];
            if (holenet != null) {
                preWireError ("hole already occupied by net " + holenet.name);
            }
        }

        // remember we have an hole here
        holes[iy20th][ix20th] = this;
    }

    private static void preWireError (String msg)
    {
        System.err.println (msg);
        if (! KEEPGOING) throw new RuntimeException ("prewire error");
    }

    // get an iterator to get components connected to this network
    public Iterable<Conn> connIterable ()
    {
        return connections;
    }

    // get total loads
    public int getLoads ()
    {
        return loads;
    }

    // get trace length for the network
    public double getNetLen ()
    {
        if (Double.isNaN (cachednetlen)) {
            cachednetlen = 0.0;
            int ncons = connections.size ();
            if (! name.equals ("GND") && ! name.equals ("VCC") && (ncons > 1)) {

                // find connection closest to upper left corner
                Conn[] sorted = new Conn[ncons];
                Conn[] random = connections.toArray (new Conn[ncons]);
                int closest = -1;
                double clodist = 999999999.0;
                for (int r = 0; r < ncons; r ++) {
                    Conn randconn = random[r];
                    double randx  = randconn.comp.pcbPosXPin (randconn.pino);
                    double randy  = randconn.comp.pcbPosYPin (randconn.pino);
                    double dist   = Math.hypot (randx, randy);
                    if (clodist > dist) {
                        clodist = dist;
                        closest = r;
                    }
                }
                sorted[0] = random[closest];
                random[closest] = random[0];
                random[0] = null;

                // ...then find closest in random to any in sorted
                for (int s = 0; ++ s < ncons;) {
                    closest = -1;
                    clodist = 999999999.0;
                    for (int r = s; r < ncons; r ++) {
                        Conn randconn = random[r];
                        double randx  = randconn.comp.pcbPosXPin (randconn.pino);
                        double randy  = randconn.comp.pcbPosYPin (randconn.pino);
                        for (int t = 0; t < s; t ++) {
                            Conn sortconn = sorted[t];
                            double sortx  = sortconn.comp.pcbPosXPin (sortconn.pino);
                            double sorty  = sortconn.comp.pcbPosYPin (sortconn.pino);
                            double dist   = Math.abs (randx - sortx) + Math.abs (randy - sorty);
                            if (clodist > dist) {
                                clodist = dist;
                                closest = r;
                            }
                        }
                    }

                    // move 'the closest in random to any in sorted' to sorted
                    sorted[s] = random[closest];
                    random[closest] = random[s];
                    random[s] = null;

                    // accumulate that closest distance
                    cachednetlen += clodist;
                }
            }
        }
        return cachednetlen;
    }

    public void invalNetLen ()
    {
        cachednetlen = Double.NaN;
    }

    // holds a connection to the network
    public static class Conn {
        public final Comp comp;     // which component (Comp.Trans, Comp.Diode, Comp.Resis, etc)
        public final String pino;   // which pin (PIN_?)

        public Conn (Comp comp, String pino)
        {
            this.comp = comp;
            this.pino = pino;
        }
    }

    private static class PreWire {
        public Conn a;
        public Conn b;
    }
}
