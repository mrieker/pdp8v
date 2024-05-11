
// list pads on a circuit board in sorted order

// javac ListPads.java
// java ListPads old.kicad_pcb

import java.io.BufferedReader;
import java.io.EOFException;
import java.io.FileReader;
import java.io.PrintStream;
import java.util.HashMap;
import java.util.Iterator;
import java.util.LinkedList;
import java.util.TreeMap;

public class ListPads {
    public final static Value[] emptyvaluearray = new Value[0];

    public static void main (String[] args)
            throws Exception
    {
        Board oldboard = new Board (args[0]);

        LinkedList<ParenValue> modules = oldboard.topvalue.getNamedChildren ("module");
        TreeMap<String,ParenValue> sortedmodules = new TreeMap<> ();
        for (ParenValue module : modules) {
            String modname = getModuleName (module);
            if (modname != null) {
                sortedmodules.put (modname, module);
            }
        }

        modules = oldboard.topvalue.getNamedChildren ("footprint");
        for (ParenValue module : modules) {
            String modname = getModuleName (module);
            if (modname != null) {
                sortedmodules.put (modname, module);
            }
        }

        for (String modname : sortedmodules.keySet ()) {
            ParenValue module = sortedmodules.get (modname);
            LinkedList<ParenValue> pads = module.getNamedChildren ("pad");
            TreeMap<String,ParenValue> sortedpads = new TreeMap<> ();
            for (ParenValue pad : pads) {
                Value[] padarray = pad.getArray ();
                if (padarray.length >= 2) sortedpads.put (padarray[1].getString (), pad);
            }
            for (String padnum : sortedpads.keySet ()) {
                System.out.println (modname + " " + padnum);
            }
        }
    }

    public static String getModuleName (ParenValue module)
    {
        LinkedList<ParenValue> fptexts = module.getNamedChildren ("fp_text");
        for (ParenValue fptext : fptexts) {
            Value[] fparray = fptext.getArray ();
            if ((fparray.length >= 3) && (fparray[1] instanceof StringValue) && fparray[1].getString ().equals ("reference") &&
                (fparray[2] instanceof StringValue)) return fparray[2].getString ();
        }
        return null;
    }

    public static class Board {

        public ParenValue topvalue;

        public Board (String filename)
                throws Exception
        {
            topvalue = (ParenValue) parseValue (new BufferedReader (new FileReader (filename)));
        }

        // parse next value from input stream
        // input stream positioned to next read terminating character
        private static Value parseValue (BufferedReader br)
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
                    return new ParenValue (children);
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
                    return new StringValue (sb.toString ());
                }

                // number => IntegerValue, FloatValue
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
                        IntegerValue iv = new IntegerValue (intval, st);
                        return iv;
                    } catch (NumberFormatException nfe) { }
                    try {
                        double fltval = Double.parseDouble (st);
                        FloatValue fv = new FloatValue (fltval, st);
                        return fv;
                    } catch (NumberFormatException nfe) { }
                    return new SymbolValue (sb.toString ());
                }
            }
        }

        // return next non-blank character from input stream
        // set up so br.reset() will undo the return character
        private static char skipSpaces (BufferedReader br)
                throws Exception
        {
            while (true) {
                br.mark (1);
                int ch = br.read ();
                if (ch < 0) throw new EOFException ("eof parsing value");
                if (ch > ' ') return (char) ch;
            }
        }
    }

    // base for all parsed values
    public static class Value {

        public Value[] getArray () { throw new RuntimeException ("not a paren"); }
        public double  getFloat () { throw new RuntimeException ("not a float"); }
        public int   getInteger () { throw new RuntimeException ("not an integer"); }
        public String getString () { throw new RuntimeException ("not a string"); }
        public String getSymbol () { throw new RuntimeException ("not a symbol"); }
        public LinkedList<ParenValue> getNamedChildren (String name) { throw new RuntimeException ("not a paren"); }

        // print value
        // - for non-ParenValues, just print the value itself, no spaces before or after, no newline after
        // - for ParenValues, print open paren where we are followed by first child if any
        //                    each subsequent value is on newline indented with pl+1 spaces
        //                    close paren is on newline indented with pl spaces with no following newline
        public void printit (int pl, PrintStream ps)
        {
            ps.print (toString ());
        }
    }

    public static class ParenValue extends Value {
        public LinkedList<Value> children;

        public ParenValue ()
        {
            children = new LinkedList<> ();
        }

        public ParenValue (LinkedList<Value> ch)
        {
            children = ch;
        }

        // return array of all children
        @Override
        public Value[] getArray () { return children.toArray (emptyvaluearray); }

        // return list of matching parenvalue children
        @Override
        public LinkedList<ParenValue> getNamedChildren (String name)
        {
            LinkedList<ParenValue> namedchildren = new LinkedList<> ();
            for (Value child : children) {
                if (child instanceof ParenValue) {
                    Value[] childarray = child.getArray ();
                    if ((childarray.length > 0) && childarray[0].toString ().equals (name)) {
                        namedchildren.add ((ParenValue) child);
                    }
                }
            }
            return namedchildren;
        }

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
        public final String string;

        private String tos;

        public StringValue (String st)
        {
            string = st;
        }

        @Override
        public String getString () { return string; }

        @Override
        public String toString ()
        {
            if (tos == null) {
                StringBuilder sb = new StringBuilder ();
                sb.append ('"');
                for (int i = 0; i < string.length (); i ++) {
                    char ch = string.charAt (i);
                    switch (ch) {
                        case '\b': sb.append ("\\b"); break;
                        case '\n': sb.append ("\\n"); break;
                        case '\r': sb.append ("\\r"); break;
                        case '\t': sb.append ("\\t"); break;
                        case '\"': sb.append ("\\\""); break;
                        case '\\': sb.append ("\\\\"); break;
                        default: sb.append (ch); break;
                    }
                }
                sb.append ('"');
                tos = sb.toString ();
            }
            return tos;
        }
    }

    public static class SymbolValue extends StringValue {
        public final String symbol;

        public SymbolValue (String sy)
        {
            super (sy);
            symbol = sy;
        }

        @Override
        public String getSymbol () { return symbol; }

        @Override
        public String toString ()
        {
            return symbol;
        }
    }

    public static class FloatValue extends SymbolValue {
        public final double floater;

        public FloatValue (double d, String s)
        {
            super (s);
            floater = d;
        }

        @Override
        public double getFloat () { return floater; }
    }

    public static class IntegerValue extends FloatValue {
        public final int integer;

        public IntegerValue (int i)
        {
            this (i, Integer.toString (i));
        }

        public IntegerValue (int i, String s)
        {
            super (i, s);
            integer = i;
        }

        @Override
        public int getInteger () { return integer; }
    }
}
