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

// add nto (number triodes off) and ntt (number total triodes) parameters
// to the board Verilog files

// javac AddNTO.java
// java AddNTO < <board>circ.v > <board>circ_nto.v

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.LinkedList;

public class AddNTO {
    public static void main (String[] args)
        throws Exception
    {
        // gather up list of all StandCell outputs into outvars
        // gather up list of all DFFModule, DLatModule nto outputs into outntos
        // also stick _nto suffix on module name and add nto parameter on the end

        int numtottris = 0;
        ArrayList<String> outvars = new ArrayList<> ();
        ArrayList<String> outntos = new ArrayList<> ();
        BufferedReader in = new BufferedReader (new InputStreamReader (System.in));
        String line;
        while ((line = in.readLine ()) != null) {
            if (line.startsWith ("endmodule")) break;
            if (line.startsWith ("module")) {
                line = line.replace (" (", "_nto (");
                line = line.replace (");", ", nto, ntt);");
            }
            boolean isdffmod  = line.startsWith ("    DFFModule");
            boolean isdlatmod = line.startsWith ("    DLatModule");
            if (isdffmod | isdlatmod) {
                int noutmod = outntos.size () + 1;
                String outmod = "mod_" + noutmod;
                outntos.add (outmod);
                System.out.println ("    wire[2:0] " + outmod + ";");
                line = line.replace (");", ", .nto(" + outmod + "));");
                if (isdffmod)  numtottris += 6;
                if (isdlatmod) numtottris += 4;
            }
            System.out.println (line);
            if (line.startsWith ("    StandCell")) {
                int _q = line.indexOf ("._Q(");
                int cp = line.indexOf (")", _q);
                String outvar = line.substring (_q + 4, cp);
                outvars.add (outvar);
                numtottris ++;
            }
        }

        // output nto adders to add up all the gates outputting '1'
        // this gives us the number of triodes turned off
        // keep going deeper and deeper until we end up with just one value

        System.out.println ("");

        int width = 3;
        int noutvars = (1 << width) - 1;
        int ntonum = 0;
        while (outvars.size () > 1) {
            for (int i = 0; i < outvars.size (); i += noutvars) {
                String outnto = "nto_" + ++ ntonum;
                System.out.print ("    reg[" + (width - 1) + ":0] " + outnto + "; always @(posedge uclk) " + outnto + " <= ");
                for (int j = 0; (j < noutvars) && (i + j < outvars.size ()); j ++) {
                    if (j > 0) System.out.print (" + ");
                    System.out.print (outvars.get (i + j));
                }
                System.out.println (";");
                outntos.add (outnto);
            }
            outvars = outntos;
            outntos = new ArrayList<> ();
            width += 3;
        }

        // output the final value to the module's nto parameter
        // it's always 10 bits cuz we have 620 triodes in the whole computer

        System.out.println ("");
        System.out.println ("    output[9:0] nto; assign nto = " + outvars.get (0) + ";");
        System.out.println ("    output[9:0] ntt; assign ntt = " + numtottris + ";");

        // output the endmodule statement

        System.out.println (line);
    }
}
