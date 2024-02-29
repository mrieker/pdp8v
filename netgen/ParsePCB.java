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

// Parse a KiCad PCB file
// Print out component references, values, positions, connections

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;
import java.util.TreeMap;

public class ParsePCB {
    public static BufferedReader inrdr;
    public static int linum;

    public static void main (String[] args)
            throws Exception
    {
        // parse the scheme syntax
        inrdr = new BufferedReader (new InputStreamReader (System.in));
        linum = 1;
        Value value = parseValue ();
        for (int ch; (ch = inrdr.read ()) >= 0;) {
            if (ch == '\n') linum ++;
            if (ch > ' ') {
                throw new Exception ("extra tokens at end at line " + linum);
            }
        }

        // print it all out for debugging
        // value.print ("");

        // should be in form (kicad_pcb ...)
        if (! "kicad_pcb".equals (value.getarrname ())) {
            throw new Exception ("top-level not a 'kicad_pcb'");
        }
        ArrValue kicadpcb = (ArrValue) value;

        // scan for networks and components
        TreeMap<Integer,String> netnames = new TreeMap<> ();
        TreeMap<String,Compon>  compons  = new TreeMap<> ();

        for (Value elem : kicadpcb.elems) {

            // (net <number> <name>)
            if ("net".equals (elem.getarrname ())) {
                ArrValue net = (ArrValue) elem;
                NumValue netnum = (NumValue) net.elems[1];
                netnames.put ((int) Math.round (netnum.num), net.elems[2].oneline ());
            }

            // (module ...)
            if ("module".equals (elem.getarrname ())) {
                ArrValue module = (ArrValue) elem;
                Compon compon = new Compon (module);
                if (compon.refer != null) {
                    compons.put (compon.refer, compon);
                }
            }
        }

        // print networks
        // freepcb renumbers nets but names stay the same
        ////for (Integer netnum : netnames.keySet ()) {
        ////    String netname = netnames.get (netnum);
        ////    if (! netname.startsWith ("\"")) netname = '"' + netname + '"';
        ////    System.out.println (String.format ("net:%05d  %s", netnum, netname));
        ////}

        // print components
        for (String refer : compons.keySet ()) {
            compons.get (refer).print (netnames);
        }
    }

    // parse one value from inrdr
    public static Value parseValue ()
            throws Exception
    {
        int ch;
        do {
            ch = inrdr.read ();
            if (ch < 0) throw new Exception ("end of file parsing token at line " + linum);
            if (ch == '\n') linum ++;
        } while (ch <= ' ');

        if (ch == ')') return null;

        if (ch == '(') {
            LinkedList<Value> elems = new LinkedList<> ();
            while (true) {
                Value elem = parseValue ();
                if (elem == null) break;
                elems.addLast (elem);
            }
            ArrValue arrval = new ArrValue ();
            arrval.elems = elems.toArray (new Value[elems.size()]);
            return arrval;
        }

        if (ch == '"') {
            StringBuilder sb = new StringBuilder ();
            sb.append ('"');
            while (true) {
                ch = inrdr.read ();
                if (ch < 0) throw new Exception ("end of file parsing token at line " + linum);
                if (ch == '\n') linum ++;
                if (ch == '"') break;
                switch (ch) {
                    case '\b': sb.append ("\\b"); break;
                    case '\n': sb.append ("\\n"); break;
                    case '\r': sb.append ("\\r"); break;
                    case '\t': sb.append ("\\t"); break;
                    case '\\': sb.append ('\\'); sb.append ((char) inrdr.read ()); break;
                    default: sb.append ((char) ch); break;
                }
            }
            sb.append ('"');
            StrValue strval = new StrValue ();
            strval.str = sb.toString ();
            return strval;
        }

        StringBuilder sb = new StringBuilder ();
        do {
            sb.append ((char) ch);
            inrdr.mark (1);
            ch = inrdr.read ();
            if (ch < 0) throw new Exception ("end of file parsing token at line " + linum);
            if (ch == '\n') linum ++;
        } while ((ch > ' ') && (ch != ')') && (ch != '"'));
        inrdr.reset ();
        String st = sb.toString ();
        try {
            double num = Double.parseDouble (st);
            NumValue numval = new NumValue ();
            numval.num = num;
            numval.str = st;
            return numval;
        } catch (NumberFormatException nfe) {
        }
        SymValue symval = new SymValue ();
        symval.sym = st;
        return symval;
    }

    // base value class

    public static abstract class Value {
        public String getarrname () { return null; }    // if array with symbol for first element, first element name
        public String oneline () { return null; }       // if value is not an array, the string representation of the value
        public abstract void print (String indent);     // print the value on stdout
    }

    // parsed array (array of values between (...))

    public static class ArrValue extends Value {
        public Value[] elems;

        @Override
        public String getarrname ()
        {
            if (elems.length < 1) return null;
            if (! (elems[0] instanceof SymValue)) return null;
            SymValue sv0 = (SymValue) elems[0];
            return sv0.sym;
        }

        @Override
        public void print (String indent)
        {
            boolean ononeline = true;
            for (Value elem : elems) {
                if (elem.oneline () == null) {
                    ononeline = false;
                    break;
                }
            }
            if (ononeline) {
                System.out.print (indent + "(");
                boolean first = true;
                for (Value elem : elems) {
                    if (! first) System.out.print (" ");
                    System.out.print (elem.oneline ());
                    first = false;
                }
                System.out.println (")");
            } else {
                boolean skipfirst = false;
                String firststring = elems[0].oneline ();
                if (firststring != null) {
                    System.out.println (indent + "(" + firststring);
                    skipfirst = true;
                } else {
                    System.out.println (indent + "(");
                }
                String ind = indent + "  ";
                for (Value elem : elems) {
                    if (! skipfirst) elem.print (ind);
                    skipfirst = false;
                }
                System.out.println (indent + ")");
            }
        }
    }

    // parsed numeric value

    public static class NumValue extends Value {
        public double num;
        public String str;

        @Override
        public String oneline () { return str; }

        @Override
        public void print (String indent)
        {
            System.out.println (indent + str);
        }
    }

    // parsed quoted string

    public static class StrValue extends Value {
        public String str;  // includes quotes and escapes

        @Override
        public String oneline () { return str; }

        @Override
        public void print (String indent)
        {
            System.out.println (indent + str);
        }
    }

    // parsed symbol (unquoted string)

    public static class SymValue extends Value {
        public String sym;

        @Override
        public String oneline () { return sym; }

        @Override
        public void print (String indent)
        {
            System.out.println (indent + sym);
        }
    }

    // component

    public static class Compon {
        public String refer;    // (fp_text reference <ref> ...)
        public String value;    // (fp_text value <ref> ...)
        public int atx100;      // (at x y)
        public int aty100;
        public int[] padnets;   // (pad <idx> ... (net <num> ...) ...)

        public Compon (ArrValue module)
        {
            value = "\"\"";
            int padsize = 0;
            HashMap<Integer,Integer> padnethash = new HashMap<> ();
            for (Value moduleelem : module.elems) {
                if ("at".equals (moduleelem.getarrname ())) {
                    ArrValue moduleat = (ArrValue) moduleelem;
                    atx100 = (int) Math.round (((NumValue) moduleat.elems[1]).num * 100);
                    aty100 = (int) Math.round (((NumValue) moduleat.elems[2]).num * 100);
                }
                if ("fp_text".equals (moduleelem.getarrname ())) {
                    ArrValue modulefptext = (ArrValue) moduleelem;
                    if (modulefptext.elems[1].oneline ().equals ("reference")) {
                        refer = modulefptext.elems[2].oneline ();
                    }
                    if (modulefptext.elems[1].oneline ().equals ("value")) {
                        value = modulefptext.elems[2].oneline ();
                    }
                }
                if ("pad".equals (moduleelem.getarrname ())) {
                    ArrValue modulepad = (ArrValue) moduleelem;
                    int padidx = (int) Math.round (((NumValue) modulepad.elems[1]).num);
                    for (Value modulepadelem : modulepad.elems) {
                        if ("net".equals (modulepadelem.getarrname ())) {
                            ArrValue modulepadnet = (ArrValue) modulepadelem;
                            int padnum = (int) Math.round (((NumValue) modulepadnet.elems[1]).num);
                            padnethash.put (padidx, padnum);
                            if (padsize < padidx + 1) padsize = padidx + 1;
                        }
                    }
                }
            }
            padnets = new int[padsize];
            for (int i = 0; i < padsize; i ++) padnets[i] = -1;
            for (Integer idx : padnethash.keySet ()) {
                padnets[idx] = padnethash.get (idx);
            }
        }

        public void print (TreeMap<Integer,String> netnames)
        {
            System.out.print (String.format ("compon:%-6s  value=%-8s  at=%05d,%05d", refer, value, atx100, aty100));
            for (int idx = 0; idx < padnets.length; idx ++) {
                int netnum = padnets[idx];
                if (netnum >= 0) {
                    String netname = netnames.get (netnum);
                    if (netname == null) netname = Integer.toString (netnum);
                    else {
                        if (! netname.startsWith ("\"")) netname = "\"" + netname;
                        if (! netname.endsWith ("\""))   netname = netname + "\"";
                    }
                    System.out.print (String.format ("  pin.%d=%s", idx, netname));
                }
            }
            System.out.println ();
        }
    }
}
