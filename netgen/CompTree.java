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
 * Contains list of all components in a module instantiation.
 */

import java.io.PrintStream;
import java.util.Arrays;
import java.util.Comparator;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.TreeMap;

public class CompTree {
    public String instname;

    public TreeMap<String,Comp> direct;
    public TreeMap<String,CompTree> subtrees;

    /**
     * Build list of components in the given module instantiation.
     *  Input:
     *   instname = instantiation name
     *              "" for top-level module
     *              else, something beginning with "/"
     *   comp = list of all components created by main program
     */
    public CompTree (String instname, HashMap<String,Comp> comps)
    {
        this.instname = instname;

        subtrees = new TreeMap<> ();
        direct   = new TreeMap<> ();

        int len = instname.length ();
        for (Comp comp : comps.values ()) {
            String name = comp.name;
            if (name.endsWith (instname)) {
                int i = name.lastIndexOf ('/', name.length () - len - 1);
                if (i < 0) {
                    String dirname = name.substring (0, name.length () - len);
                    StringBuilder sb = new StringBuilder ();
                    String[] splits = dirname.split ("[.]");
                    for (int j = splits.length; -- j >= 0;) {
                        sb.append ('.');
                        sb.append (splits[j]);
                    }
                    direct.put (sb.toString (), comp);
                } else {
                    String subname = name.substring (i);
                    CompTree subtree = subtrees.get (subname);
                    if (subtree == null) {
                        subtree = new CompTree (subname, comps);
                        subtrees.put (subname, subtree);
                    }
                }
            }
        }
    }

    // print out our components in a nice list
    // ...and print out submodule components too
    public void print (PrintStream ps, String indent, String modname)
    {
        ps.println (indent + "Module " + modname);

        int longestname = 1;
        int longestref  = 1;
        for (Comp comp : direct.values ()) {
            int len = comp.name.length ();
            if (longestname < len) longestname = len;
            if (comp.ref != null) {
                len = comp.ref.length ();
                if (longestref < len) longestref = len;
            }
        }
        if (longestname > 0) {
            ps.println (indent + "  Direct components:");
            String fmt = "%s    %" + longestname + "s  %-" + longestref + "s   %4.1f, %4.1f   %6.2f, %6.2f";
            for (Comp comp : direct.values ()) {
                ps.println (String.format (fmt, indent, comp.name, comp.ref,
                        comp.posx10th * 0.1,  comp.posy10th * 0.1,
                        comp.posx10th * 2.54, comp.posy10th * 2.54));
            }
        }

        boolean first = true;
        for (CompTree subtree : subtrees.values ()) {
            if (first) ps.println (indent + "  Instantiated module components:");
            subtree.print (ps, indent + "    ", subtree.instname);
            first = false;
        }
    }
}
