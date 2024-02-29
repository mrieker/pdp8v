
// alternate bus wires between F.Cu and B.Cu to minimize cross-talk

// javac Alternate.java
// java Alternate < old-backplane.kicad_pcb > new-backplane.kicad_pcb

import java.io.BufferedReader;
import java.io.EOFException;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.util.HashMap;
import java.util.LinkedList;

public class Alternate {
    public static void main (String[] args)
            throws Exception
    {
        // read existing kicad_pcb file
        BufferedReader br = new BufferedReader (new InputStreamReader (System.in));
        ParenValue topvalue = (ParenValue) parseValue (br);

        // get all network names therein
        HashMap<Integer,String> netnames = new HashMap<> ();
        for (Value topchilds : topvalue.children) {
            if (topchilds instanceof ParenValue) {
                ParenValue pv = (ParenValue) topchilds;
                if ((pv.children.size () == 3) && (pv.children.get (0) instanceof SymbolValue) && (pv.children.get (1) instanceof IntValue) && (pv.children.get (2) instanceof StringValue)) {
                    SymbolValue sv = (SymbolValue) pv.children.get (0);
                    if (sv.symbol.equals ("net")) {
                        IntValue    ii = (IntValue) pv.children.get (1);
                        StringValue ss = (StringValue) pv.children.get (2);
                        netnames.put (ii.integer, ss.string);
                    }
                }
            }
        }

        // change all odd-pinned traces from F.Cu to B.Cu
        for (Value topchilds : topvalue.children) {
            if (topchilds instanceof ParenValue) {
                ParenValue pv = (ParenValue) topchilds;
                if ((pv.children.size () > 1) && (pv.children.get (0) instanceof SymbolValue)) {
                    SymbolValue sv = (SymbolValue) pv.children.get (0);
                    if (sv.symbol.equals ("segment")) {
                        SymbolValue layer = null;
                        String netname = null;
                        for (Value val : pv.children) {
                            if (val instanceof ParenValue) {
                                ParenValue pval = (ParenValue) val;
                                if ((pval.children.size () > 1) && (pval.children.get (0) instanceof SymbolValue)) {
                                    switch (((SymbolValue) pval.children.get (0)).symbol) {
                                        case "layer": {
                                            if ((pval.children.size () == 2) && (pval.children.get (1) instanceof SymbolValue)) {
                                                layer = (SymbolValue) pval.children.get (1);
                                            }
                                            break;
                                        }
                                        case "net": {
                                            if ((pval.children.size () == 2) && (pval.children.get (1) instanceof IntValue)) {
                                                int netnum = ((IntValue) pval.children.get (1)).integer;
                                                netname = netnames.get (netnum);
                                            }
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        if ((layer != null) && (netname != null) && netname.startsWith ("Net-(J") && netname.endsWith (")")) {
                            int i = netname.indexOf ("-Pad");
                            if (i > 0) {
                                try {
                                    int padnum = Integer.parseInt (netname.substring (i + 4, netname.length () - 1));
                                    layer.symbol = ((padnum & 1) != 0) ? "F.Cu" : "B.Cu";
                                } catch (NumberFormatException nfe) { }
                            }
                        }
                    }
                }
            }
        }

        // output updated kicad_pcb file
        topvalue.printit (0, System.out);
        System.out.println ("\n");
    }

    // parse next value from input stream
    // input stream positioned to next read terminating character
    public static Value parseValue (BufferedReader br)
            throws Exception
    {
        char ch = skipSpaces (br);
        switch (ch) {

            // ( value value ... ) => ParenValue
            case '(': {
                LinkedList<Value> children = new LinkedList<> ();
                while (skipSpaces (br) != ')') {
                    br.reset ();
                    children.add (parseValue (br));
                }
                ParenValue pv = new ParenValue ();
                pv.children = children;
                return pv;
            }

            // " ... " => StringValue
            case '"': {
                StringBuilder sb = new StringBuilder ();
                while (true) {
                    int ich = br.read ();
                    if (ich < 0) throw new EOFException ("eof parsing string");
                    ch = (char) ich;
                    if (ch == '"') break;
                    if (ch == '\\') {
                        ich = br.read ();
                        if (ich < 0) throw new EOFException ("eof parsing string");
                        ch = (char) ich;
                        switch (ch) {
                            case 'b': ch = '\b'; break;
                            case 'n': ch = '\n'; break;
                            case 'r': ch = '\r'; break;
                            case 't': ch = '\t'; break;
                        }
                    }
                    sb.append (ch);
                }
                StringValue sv = new StringValue ();
                sv.string = sb.toString ();
                return sv;
            }

            // number => IntValue, FloatValue
            //   else => SymbolValue
            default: {
                StringBuilder sb = new StringBuilder ();
                sb.append (ch);
                while (true) {
                    br.mark (1);
                    int ich = br.read ();
                    if (ich < 0) break;
                    ch = (char) ich;
                    if (ch == ')') {
                        br.reset ();
                        break;
                    }
                    if (ch <= ' ') break;
                    sb.append (ch);
                }
                String st = sb.toString ();
                try {
                    int intval = Integer.parseInt (st);
                    IntValue iv = new IntValue ();
                    iv.integer = intval;
                    return iv;
                } catch (NumberFormatException nfe) { }
                try {
                    double fltval = Double.parseDouble (st);
                    FloatValue fv = new FloatValue ();
                    fv.floater = fltval;
                    return fv;
                } catch (NumberFormatException nfe) { }
                SymbolValue sv = new SymbolValue ();
                sv.symbol = sb.toString ();
                return sv;
            }
        }
    }

    // return next non-blank character from input stream
    // set up so br.reset() will undo the return character
    public static char skipSpaces (BufferedReader br)
            throws Exception
    {
        while (true) {
            br.mark (1);
            int ch = br.read ();
            if (ch < 0) throw new EOFException ("eof parsing value");
            if (ch > ' ') return (char) ch;
        }
    }

    // base for all parsed values
    public static abstract class Value {

        // print value
        // - for non-ParenValues, just print the value itself, no spaces before or after, no newline after
        // - for ParenValues, print open paren where we are followed by first child if any
        //                    each subsequent value is on newline indented with pl+1 spaces
        //                    close paren is on newline indented with pl spaces with no following newline
        public abstract void printit (int pl, PrintStream ps);
    }

    public static class ParenValue extends Value {
        public LinkedList<Value> children;

        @Override
        public void printit (int pl, PrintStream ps)
        {
            ps.print ('(');
            switch (children.size ()) {
                case 0: {
                    break;
                }
                case 1: {
                    children.getFirst ().printit (pl + 1, ps);
                    break;
                }
                default: {
                    boolean havecomplexchild = false;
                    for (Value child : children) {
                        havecomplexchild |= child instanceof ParenValue;
                    }
                    boolean first = true;
                    for (Value child : children) {
                        if (! first) {
                            if (havecomplexchild) {
                                ps.print ('\n');
                                for (int j = 0; j < pl + 1; j ++) ps.print ("  ");
                            } else {
                                ps.print (' ');
                            }
                        }
                        child.printit (pl + 1, ps);
                        first = false;
                    }
                    if (havecomplexchild) {
                        ps.print ('\n');
                        for (int j = 0; j < pl; j ++) ps.print ("  ");
                    }
                    break;
                }
            }
            ps.print (')');
        }
    }

    public static class StringValue extends Value {
        public String string;

        @Override
        public void printit (int pl, PrintStream ps)
        {
            ps.print ('"');
            for (int i = 0; i < string.length (); i ++) {
                char ch = string.charAt (i);
                switch (ch) {
                    case '\b': ps.print ("\\b"); break;
                    case '\n': ps.print ("\\n"); break;
                    case '\r': ps.print ("\\r"); break;
                    case '\t': ps.print ("\\t"); break;
                    case '\"': ps.print ("\\\""); break;
                    case '\\': ps.print ("\\\\"); break;
                    default: ps.print (ch); break;
                }
            }
            ps.print ('"');
        }
    }

    public static class IntValue extends Value {
        public int integer;

        @Override
        public void printit (int pl, PrintStream ps)
        {
            ps.print (integer);
        }
    }

    public static class FloatValue extends Value {
        public double floater;

        @Override
        public void printit (int pl, PrintStream ps)
        {
            ps.print (floater);
        }
    }

    public static class SymbolValue extends Value {
        public String symbol;

        @Override
        public void printit (int pl, PrintStream ps)
        {
            ps.print (symbol);
        }
    }
}
