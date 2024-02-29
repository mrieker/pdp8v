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
 * Interactively edit board layout and generate a .map file.
 *
 *  ./netgen.sh ../modules/whole.mod -gen seq -pcbmap ../modules/seq.map -mapedit seq-new.map
 */

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.Graphics;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.awt.event.WindowAdapter;
import java.awt.event.WindowEvent;
import java.io.BufferedReader;
import java.io.FileReader;
import java.io.PrintWriter;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashSet;
import java.util.LinkedList;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;

public class MapEdit {
    public final static int PIXPER10TH =  13;   // pixels per 10th inch
    public final static int BOARDL10TH =  10;   // board left margin 10th inch

    public final static int CONSH10TH  =  10;   // height reserved for edge connectors

    private static boolean editcomplete;
    private static Font labelfont = new Font ("Serif", Font.PLAIN, 10);
    private static GenCtx genctx;
    private static HashSet<Network> ournets = new HashSet<> ();
    private static JFrame jframe;
    private static LinkedList<Comp> allcomps = new LinkedList<> ();
    private static Object editinprog = new Object ();

    public static void main (GenCtx genctx, String mapedname)
            throws Exception
    {
        MapEdit.genctx = genctx;

        // get list of networks not including ones internal to placeables
        // we are trying to minimize the total length of these networks
        // we assume the internal networks are of minimal length already
        for (Placeable comp : genctx.placeables.values ()) {
            Network[] extnets = comp.getExtNets ();
            for (Network extnet : extnets) {
                ournets.add (extnet);
            }
        }

        // create a frame and fill in with all components in the grid
        MyPanel jpanel = new MyPanel ();
        Dimension psize = new Dimension (genctx.boardwidth * PIXPER10TH, genctx.boardheight * PIXPER10TH);
        jpanel.setMaximumSize (psize);
        jpanel.setMinimumSize (psize);
        jpanel.setPreferredSize (psize);
        jpanel.setLayout (null);

        JScrollPane jscroll = new JScrollPane (jpanel);

        jframe = new JFrame ("MapEdit");
        updtotnetlen ();
        jframe.setSize (1200, 800);
        jframe.setDefaultCloseOperation (JFrame.DISPOSE_ON_CLOSE);
        jframe.add (jscroll);
        jframe.addWindowListener (new WindowAdapter () {
            @Override
            public void windowClosing (WindowEvent we)
            {
                synchronized (editinprog) {
                    editcomplete = true;
                    editinprog.notifyAll ();
                }
            }
        });
        jframe.setVisible (true);

        // wait for jframe closed
        synchronized (editinprog) {
            while (! editcomplete) {
                editinprog.wait ();
            }
        }

        // write out map file
        System.out.println ("MapEdit: writing " + mapedname);
        Placeable[] comparray = genctx.placeables.values ().toArray (new Placeable[0]);
        Arrays.sort (comparray, new Comparator<Placeable> () {
            @Override
            public int compare (Placeable a, Placeable b)
            {
                int ax = (int) Math.round (a.getPosX () * 2);
                int ay = (int) Math.round (a.getPosY () * 2);
                int bx = (int) Math.round (b.getPosX () * 2);
                int by = (int) Math.round (b.getPosY () * 2);

                // put connectors at beginning of file
                boolean aiscon = ay < CONSH10TH * 2;
                boolean biscon = by < CONSH10TH * 2;
                if (aiscon & biscon) {
                    if (ay != by) return ay - by;
                    return ax - bx;
                }
                if (aiscon & ! biscon) return -1;
                if (! aiscon & biscon) return  1;

                // everything else is sorted by column
                if (ax != bx) return ax - bx;
                return ay - by;
            }
            @Override
            public boolean equals (Object o)
            {
                return o == this;
            }
        });
        PrintWriter pw = new PrintWriter (mapedname);
        for (Placeable comp : comparray) {
            pw.println ("manual " + comp.getName () + " [" + comp.getPosX () + "," + comp.getPosY () + "]");
        }
        pw.close ();
        System.out.println ("MapEdit:   wrote " + mapedname);
    }

    private static class MyPanel extends JPanel implements MouseListener, MouseMotionListener {

        private double mousedownx10th;
        private double mousedowny10th;
        private int mousedownxpix;
        private int mousedownypix;
        private Placeable mousecomp;

        public MyPanel ()
        {
            addMouseListener (this);
            addMouseMotionListener (this);
        }

        @Override  // MouseListener
        public void mouseClicked (MouseEvent me)
        { }

        @Override  // MouseListener
        public void mouseEntered (MouseEvent me)
        { }

        @Override  // MouseListener
        public void mouseExited (MouseEvent me)
        { }

        @Override  // MouseListener
        public void mousePressed (MouseEvent me)
        {
            mousecomp = null;
            mousedownxpix = me.getX ();
            mousedownypix = me.getY ();
            mousedownx10th = Double.NaN;
            mousedowny10th = Double.NaN;

            // find component being moved if any
            for (Placeable comp : genctx.placeables.values ()) {
                int xpix = (int) Math.round (comp.getPosX   () * PIXPER10TH);
                int ypix = (int) Math.round (comp.getPosY   () * PIXPER10TH);
                int wpix = (int) Math.round (comp.getWidth  () * PIXPER10TH);
                int hpix = (int) Math.round (comp.getHeight () * PIXPER10TH);
                if ((mousedownxpix > xpix) && (mousedownxpix < xpix + wpix) && (mousedownypix > ypix) && (mousedownypix < ypix + hpix)) {
                    mousecomp = comp;
                    mousedownx10th = comp.getPosX ();
                    mousedowny10th = comp.getPosY ();
                }
            }

            repaint ();
        }

        @Override  // MouseListener
        public void mouseReleased (MouseEvent me)
        {
            mousecomp = null;
            repaint ();
        }

        @Override  // MouseMotionListener
        public void mouseDragged (MouseEvent me)
        {
            if (mousecomp != null) {
                int mouselastxpix = me.getX ();
                int mouselastypix = me.getY ();

                double newx10th = mousedownx10th + (mouselastxpix - mousedownxpix + PIXPER10TH / 2) / PIXPER10TH;
                double newy10th = mousedowny10th + (mouselastypix - mousedownypix + PIXPER10TH / 2) / PIXPER10TH;

                if ((newx10th != mousecomp.getPosX ()) || (newy10th != mousecomp.getPosY ())) {

                    // set new position so net len calculation will work
                    mousecomp.setPosXY (newx10th, newy10th);
                    repaint ();
                    for (Network extnet : mousecomp.getExtNets ()) {
                        extnet.invalNetLen ();
                    }
                    updtotnetlen ();
                }
            }
        }

        @Override  // MouseMotionListener
        public void mouseMoved (MouseEvent me)
        { }

        @Override
        protected void paintComponent (Graphics g)
        {
            g.setColor (Color.GRAY);
            g.fillRect (0, 0, genctx.boardwidth * PIXPER10TH, genctx.boardheight * PIXPER10TH);

            // draw all placeables in their current position
            // highlight any selected one in yellow, others are white background
            g.setFont (labelfont);
            for (Placeable comp : genctx.placeables.values ()) {
                double x10th, y10th;
                if (comp == mousecomp) {
                    x10th = mousedownx10th;
                    y10th = mousedowny10th;
                    g.setColor (Color.YELLOW);
                } else {
                    x10th = comp.getPosX ();
                    y10th = comp.getPosY ();
                    g.setColor (Color.WHITE);
                }
                int xpix = (int) Math.round (x10th * PIXPER10TH);
                int ypix = (int) Math.round (y10th * PIXPER10TH);
                int wpix = (int) Math.round (comp.getWidth  () * PIXPER10TH);
                int hpix = (int) Math.round (comp.getHeight () * PIXPER10TH);
                g.fillRoundRect (xpix, ypix, wpix, hpix, 10, 10);
                g.setColor (Color.BLACK);
                g.drawRoundRect (xpix, ypix, wpix, hpix, 10, 10);
                g.drawString (comp.getName (), xpix + 10, ypix + PIXPER10TH);
            }

            // if one is being dragged, show its new position in red
            if (mousecomp != null) {
                int xpix = (int) Math.round (mousecomp.getPosX () * PIXPER10TH);
                int ypix = (int) Math.round (mousecomp.getPosY () * PIXPER10TH);
                int wpix = mousecomp.getWidth  () * PIXPER10TH;
                int hpix = mousecomp.getHeight () * PIXPER10TH;
                g.setColor (Color.RED);
                g.fillRoundRect (xpix, ypix, wpix, hpix, 10, 10);
            }

            // draw the tube connector grid with green lines
            g.setColor (Color.GREEN);
            for (int y = CONSH10TH; y <= genctx.boardheight; y += StandCell.CONECVERT) {
                g.drawLine (0, y * PIXPER10TH, genctx.boardwidth * PIXPER10TH, y * PIXPER10TH);
            }
            for (int x = 0; x <= genctx.boardwidth; x += StandCell.CONECHORIZ) {
                g.drawLine (x * PIXPER10TH, CONSH10TH * PIXPER10TH, x * PIXPER10TH, genctx.boardheight * PIXPER10TH);
            }
        }
    }

    // display total network length
    private static void updtotnetlen ()
    {
        double totnetlen = 0.0;
        for (Network net : ournets) totnetlen += net.getNetLen ();
        jframe.setTitle ("MapEdit " + totnetlen);
    }

    // get 3-pin connector column this standard cell is in
    private static int getConCol (double x10th)
    {
        return (int) Math.round ((x10th - BOARDL10TH + StandCell.CONECHORIZ * 10) / StandCell.CONECHORIZ) - 10;
    }

    // get 3-pin connector row this standard cell is nearest
    private static int getConRow (double y10th)
    {
        return (int) Math.floor ((y10th - CONSH10TH  + StandCell.CONECVERT  * 10) / StandCell.CONECVERT) - 10;
    }
}
