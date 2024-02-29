
// takes output from -myverilog option to netgen
// generates list of all stuckon names
// and locates the corresponding tube jack on the circut board

// make     # to make sure whole.mod and .rep files are up to date
// ../netgen/netgen.sh whole.mod -gen proc -myverilog stuckon.mod 2>&1 | grep -v ' not in place$'
// java StuckOnsExtract | truesort

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.InputStreamReader;
import java.util.HashMap;

public class StuckOnsExtract {

    // contents of a <boardname>.rep file giving location of 3-pin tube board jacks
    public static class RepFile {
        public String shortname;
        public HashMap<String,String> locations;
    }

    public static HashMap<String,RepFile> repfiles;

    public static void main (String[] args)
        throws Exception
    {
        ProcessRepFile ("acl");
        ProcessRepFile ("alu");
        ProcessRepFile ("ma");
        ProcessRepFile ("pc");
        ProcessRepFile ("seq");
    }

    public static void ProcessRepFile (String module)
        throws Exception
    {
        String slashmodcirc = "/" + module + "circ";
        int slashmodcirclen = slashmodcirc.length ();

        RepFile repfile = readRepFile (module);
        for (String jdot : repfile.locations.keySet ()) {
            if (jdot.startsWith ("J.etfill")) continue;
            String stuckon = null;
            int i = jdot.indexOf (slashmodcirc);
            if (i >= 0) {
                int m = i + slashmodcirclen;

                // DFF_malo_ma_0_f  <=  J.f.0.DFF/malo/macirc
                //                               j    i      m
                if (m == jdot.length ()) {
                    int j = jdot.indexOf ('/');
                    if (j >= 0) {
                        String instance = jdot.substring (j + 1, i);            // "malo"
                        String[] parts = jdot.substring (0, j).split ("[.]");   // "J", "f", "0", "DFF"
                        if ((parts.length == 4) && parts[0].equals ("J")) {
                            stuckon = parts[3] + "_" + instance + "_" + module + "_" + parts[2] + "_" + parts[1];
                        }
                    }
                }

                // Q_0__dfrm_seq_0_0_2  <=  J.0._dfrm/seqcirc[0:0]2
                //                                   i       m j k
                else if (jdot.charAt (i + slashmodcirclen) == '[') {
                    String[] parts = jdot.substring (0, i).split ("[.]");       // "J", "0", "_dfrm"
                    if ((parts.length == 3) && parts[0].equals ("J")) {
                        int j = jdot.indexOf (":", m);
                        if (j >= 0) {
                            int k = jdot.indexOf ("]", j);
                            if (k >= 0) {
                                String rbit  = parts[1].replace ("~", "_");
                                String hibit = jdot.substring (m + 1, j);
                                String lobit = jdot.substring (j + 1, k);
                                String seq   = jdot.substring (k + 1);
                                stuckon = "Q_" + rbit + "_" + parts[2] + "_" + module + "_" + hibit + "_" + lobit + "_" + seq;
                            }
                        }
                    }
                }
            }

            if (stuckon == null) {
                System.err.println ("undecodable " + jdot);
            } else {
                String locn = repfile.locations.get (jdot);
                System.out.println (stuckon + " # " + jdot + "  " + locn);
            }
        }
    }

    // read module.rep file
    // extract all the jack location lines
    //  J.0._intak1d/seqcirc[0:0]7  J28     9.9, 11.6   251.46, 294.64
    //  ^key                        ^value
    public static RepFile readRepFile (String module)
        throws Exception
    {
        RepFile repfile = new RepFile ();
        repfile.locations = new HashMap<> ();
        BufferedReader br = new BufferedReader (new FileReader (module + ".rep"));
        for (String line; (line = br.readLine ()) != null;) {
            line = line.trim ();
            if (line.startsWith ("Module ")) break;
        }
        for (String line; (line = br.readLine ()) != null;) {
            line = line.trim ();
            if (line.startsWith ("Network:")) break;
            if (line.startsWith ("J.")) {
                int i = line.indexOf (" ");
                int j;
                for (j = i; j < line.length (); j ++) {
                    if (line.charAt (j) != ' ') break;
                }
                repfile.locations.put (line.substring (0, i), line.substring (j));
            }
        }
        br.close ();
        return repfile;
    }
}
