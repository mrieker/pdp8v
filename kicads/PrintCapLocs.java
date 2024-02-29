
// print locations of capacitors

// javac PrintCapLocs.java
// java PrintCapLocs < someboard.kicad_pcb

import java.io.BufferedReader;
import java.io.EOFException;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.util.LinkedList;
import java.util.TreeMap;

public class PrintCapLocs {
    public static void main (String[] args)
            throws Exception
    {
        // read existing kicad_pcb file
        BufferedReader br = new BufferedReader (new InputStreamReader (System.in));
        ParenValue topvalue = (ParenValue) parseValue (br);

        // get all modules therein
        TreeMap<String,Integer> compcounts = new TreeMap<> ();
        for (Value topchilds : topvalue.children) {
            if (! (topchilds instanceof ParenValue)) continue;
            ParenValue pv = (ParenValue) topchilds;
            if (pv.children.size () < 3) continue;
            if (! (pv.children.get (0) instanceof SymbolValue)) continue;
            if (! (pv.children.get (1) instanceof SymbolValue)) continue;
            SymbolValue pv0sv = (SymbolValue) pv.children.get (0);
            if (! pv0sv.symbol.equals ("module")) continue;

            // get module name
            SymbolValue pv1sv = (SymbolValue) pv.children.get (1);
            String comptype = pv1sv.symbol;

            // get refno from (fp_text reference <refno> ...)
            String comprefno = null;
            double atxins = Double.NaN;
            double atyins = Double.NaN;
            for (Value pvv : pv.children) {
                if (! (pvv instanceof ParenValue)) continue;
                ParenValue pvvv = (ParenValue) pvv;
                if ((pvvv.children.size () >= 3) &&
                    (pvvv.children.get (0) instanceof SymbolValue) && ((SymbolValue) pvvv.children.get (0)).symbol.equals ("at") &&
                    (pvvv.children.get (1) instanceof FloatValue) &&
                    (pvvv.children.get (2) instanceof FloatValue)) {
                    atxins = ((FloatValue) pvvv.children.get (1)).floater / 25.4;
                    atyins = ((FloatValue) pvvv.children.get (2)).floater / 25.4;
                    continue;
                }
                if (pvvv.children.size () < 3) continue;
                if (! (pvvv.children.get (0) instanceof SymbolValue)) continue;
                if (! (pvvv.children.get (1) instanceof SymbolValue)) continue;
                if (! (pvvv.children.get (2) instanceof SymbolValue)) continue;
                if (! ((SymbolValue) pvvv.children.get (0)).symbol.equals ("fp_text")) continue;
                if (! ((SymbolValue) pvvv.children.get (1)).symbol.equals ("reference")) continue;
                comprefno = ((SymbolValue) pvvv.children.get (2)).symbol;
                break;
            }
            if ((comprefno != null) && comprefno.startsWith ("C")) {
                System.out.printf (" %5.1f  %5.1f  %s\n", atxins, atyins, comprefno);
            }
        }
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
