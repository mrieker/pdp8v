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
 * Tokenize the given file.
 */

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.IOException;

public class Token {

    public static Token parse (Token lasttok, String filename, BufferedReader br)
            throws IOException
    {
        boolean error = false;
        int lineno = 0;
    readline:
        for (String line; (line = br.readLine ()) != null;) {
            ++ lineno;
            int linelen = line.length ();
            for (int i = 0; i < linelen;) {
                char c = line.charAt (i);

                // skip rest of line for comments
                if (c == '#') break;

                // skip over whitespace
                if (c <= ' ') {
                    i ++;
                    continue;
                }

                // quoted strings
                if (c == '"') {
                    StringBuilder sb = new StringBuilder ();
                    Str tok = new Str (filename, lineno, i);
                    while (true) {
                        if (++ i >= linelen) {
                            errmsg (filename, lineno, i, "missing close quote");
                            error = true;
                            continue readline;
                        }
                        c = line.charAt (i);
                        if (c == '"') {
                            i ++;
                            break;
                        }
                        if (c == '\\') {
                            if (++ i >= linelen) {
                                errmsg (filename, lineno, i, "missing close quote");
                                error = true;
                                continue readline;
                            }
                            c = line.charAt (i);
                            switch (c) {
                                case 'b': c = '\b'; break;
                                case 'n': c = '\n'; break;
                                case 'r': c = '\r'; break;
                                case 't': c = '\t'; break;
                            }
                        }
                        sb.append (c);
                    }
                    tok.value = sb.toString ();
                    lasttok = lasttok.next = tok;
                    continue;
                }

                // integer numbers
                // - [width radix]digits
                if (c >= '0' && c <= '9') {
                    Int tok = new Int (filename, lineno, i);

                    // find end of string of digits, letters, underscores
                    int j = i;
                    while (++ j < linelen) {
                        c = line.charAt (j);
                        if (c >= 'a' && c <= 'z') c += 'A' - 'a';
                        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c == '_')) continue;
                        break;
                    }

                    // compute specified value
                    long width = 0;
                    int radix = 10;
                    long val  = 0;
                    while (i < j) {

                        // process decimal digits at the beginning
                        // - if that's all we have, that's our value
                        c = line.charAt (i);
                        if (c >= '0' && c <= '9') {
                            long newval = val * 10 + c - '0';
                            if (newval / 10 != val) {
                                errmsg (filename, lineno, i, "numeric overflow");
                                error = true;
                                continue readline;
                            }
                            val = newval;
                            i ++;
                            continue;
                        }

                        // next better be a radix specifier digit
                        // - digits before the radix letter give the width
                        if (c >= 'a' && c <= 'z') c += 'A' - 'a';
                        switch (c) {
                            case 'B': {
                                width = val;
                                radix = 2;
                                val   = 0;
                                break;
                            }
                            case 'D': {
                                width = val;
                                radix = 10;
                                val   = 0;
                                break;
                            }
                            case 'O': {
                                width = val;
                                radix = 8;
                                val   = 0;
                                break;
                            }
                            case 'X': {
                                width = val;
                                radix = 16;
                                val   = 0;
                                break;
                            }
                            default: {
                                errmsg (filename, lineno, i, "bad numeric char " + c);
                                error = true;
                                continue readline;
                            }
                        }
                        if (width > 63) {
                            errmsg (filename, lineno, i, "width too large");
                            error = true;
                            continue readline;
                        }

                        // process digits after radix letter in that radix
                        while (++ i < j) {
                            c = line.charAt (i);
                            if (c >= 'a' && c <= 'z') c += 'A' - 'a';
                            long newval = -1;
                            if (c >= '0' && c <= '9' && c < '0' + radix) {
                                newval = val * radix + c - '0';
                            } else if (c >= 'A' && c < 'A' + radix - 10) {
                                newval = val * radix + c - 'A' - 10;
                            } else {
                                errmsg (filename, lineno, i, "bad numeric base " + radix + " digit");
                                error = true;
                                continue readline;
                            }
                            if (newval / radix != val) {
                                errmsg (filename, lineno, i, "numeric overflow");
                                error = true;
                                continue readline;
                            }
                            val = newval;
                        }
                    }

                    // save resultant value in a token
                    if ((width > 0) && (val >> width != 0)) {
                        errmsg (filename, lineno, i, "number too big for width " + width);
                        error = true;
                        continue readline;
                    }
                    tok.radix = radix;
                    tok.width = (int) width;
                    tok.value = val;
                    lasttok = lasttok.next = tok;
                    continue;
                }

                // symbol names (or keyword)
                if ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c == '_')) {
                    int j = i;
                    int k = i;
                    while (++ j < linelen) {
                        c = line.charAt (j);
                        if ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || (c == '_') || (c == '.')) continue;
                        break;
                    }
                    String str = line.substring (i, j);
                    i = j;
                    for (DEL del : DEL.values ()) {
                        if (str.equalsIgnoreCase (del.str)) {
                            Del tok = new Del (filename, lineno, k);
                            tok.value = del;
                            lasttok = lasttok.next = tok;
                            str = null;
                            break;
                        }
                    }
                    if (str != null) {
                        Sym tok = new Sym (filename, lineno, k);
                        tok.value = str;
                        lasttok = lasttok.next = tok;
                    }
                    continue;
                }

                // delimiters
                String rest = line.substring (i);
                for (DEL del : DEL.values ()) {
                    if (!del.str.equals ("") && rest.startsWith (del.str)) {
                        Del tok = new Del (filename, lineno, i);
                        tok.value = del;
                        lasttok = lasttok.next = tok;
                        rest = null;
                        i += del.str.length ();
                        break;
                    }
                }
                if (rest != null) {
                    errmsg (filename, lineno, i, "invalid character " + c);
                    error = true;
                    continue readline;
                }
            }
        }

        if (error) return null;

        return lasttok;
    }

    private static void errmsg (String fnm, int lno, int cno0, String msg)
    {
        System.err.println (fnm + ":" + lno + "." + (cno0 + 1) + ": " + msg);
    }

    public static enum DEL {
        NONE(""),
        SEMI(";"),
        COLON(":"),
        COMMA(","),
        OPRN("("),
        CPRN(")"),
        OR("|"),
        AND("&"),
        XOR("^"),
        NOT("~"),
        EQUAL("="),
        OBRK("["),
        CBRK("]"),
        OBRC("{"),
        CBRC("}"),
        AT("@"),
        SLASH("/"),
        INTERCONN("interconnect"),
        JUMPER("jumper"),
        LED("led"),
        MODULE("module"),
        OC("oc"),
        IN("in"),
        OUT("out"),
        WIRE("wire");

        public final String str;

        private DEL (String str)
        {
            this.str = str;
        }
    }

    public final String fnm;
    public final int cno;   // one based
    public final int lno;   // one based
    public Token next;      // last (Token.End) points to self

    public Token (String fnm, int lno, int cno0)
    {
        this.fnm = fnm;
        this.lno = lno;
        this.cno = cno0 + 1;
    }

    public boolean isDel (DEL del) { return false; }
    public DEL getDel () { return DEL.NONE; }
    public String getSym () { return null; }
    public String getLoc () { return fnm + ":" + lno + "." + cno; }

    public static class Beg extends Token {
        public Beg ()
        {
            super ("", 0, -1);
        }
        @Override
        public String toString ()
        {
            return "<<beg>>";
        }
    }

    public static class Del extends Token {
        public DEL value;
        public Del (String fnm, int lno, int cno0)
        {
            super (fnm, lno, cno0);
        }
        @Override
        public boolean isDel (DEL del) { return value == del; }
        @Override
        public DEL getDel () { return value; }
        @Override
        public String toString ()
        {
            return value.str;
        }
    }

    public static class End extends Token {
        public End ()
        {
            super ("", 0, -1);
        }
        @Override
        public String toString ()
        {
            return "<<end>>";
        }
    }

    public static class Int extends Token {
        public int radix;
        public int width;
        public long value;
        public Int (String fnm, int lno, int cno0)
        {
            super (fnm, lno, cno0);
        }
        @Override
        public String toString ()
        {
            return Long.toString (value);
        }
    }

    public static class Str extends Token {
        public String value;
        public Str (String fnm, int lno, int cno0)
        {
            super (fnm, lno, cno0);
        }
        @Override
        public String toString ()
        {
            StringBuilder sb = new StringBuilder ();
            sb.append ('"');
            for (int i = 0; i < value.length (); i ++) {
                char c = value.charAt (i);
                switch (c) {
                    case '\b': sb.append ("\\b");  break;
                    case '\n': sb.append ("\\n");  break;
                    case '\r': sb.append ("\\r");  break;
                    case '\t': sb.append ("\\t");  break;
                    case '"':  sb.append ("\\\""); break;
                    case '\\': sb.append ("\\\\"); break;
                    default: sb.append (c); break;
                }
            }
            sb.append ('"');
            return sb.toString ();
        }
    }

    public static class Sym extends Token {
        public String value;
        public Sym (String fnm, int lno, int cno0)
        {
            super (fnm, lno, cno0);
        }
        @Override
        public String getSym () { return value; }
        @Override
        public String toString ()
        {
            return value;
        }
    }
}
