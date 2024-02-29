
// given a .rep report file, output where all the DFF inputs and output are
//  javac PrintDFFs.java
//  java PrintDFFs < boardname.rep

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.InputStreamReader;
import java.util.TreeMap;

public class PrintDFFs {

    // all bits of a gang of DFFs
    public static class DFFGang {
        public String name;         // "fetch2/seqcirc"
        public String kind;         // ".DFF/" or ".DLat/" (null for gate)
        public TreeMap<Integer,DFFBit> dffbits = new TreeMap<> ();
    }

    // one bit of a gang of DFFs
    public static class DFFBit {
        public DFFGang gang;
        public int bitno;
        public TreeMap<String,Nand> nands = new TreeMap<> ();
    }

    // one of the six NANDs composing a bit of a gang of DFFs
    public static class Nand {
        public DFFBit dffbit;
        public String letter;       // "a".."f" (null for gate)
        public Device output;
        public TreeMap<String,Device> inputs = new TreeMap<> ();
    }

    // a network found in the report file
    public static class Network {
        public String name;         // "Q.0._fetch2d/seqcirc[0:0]5"
        public TreeMap<String,DevPin> devpins = new TreeMap<> ();
    }

    // an input diode or a 3-pin jack going to a triode
    public static class Device {
        public String name;         // "D2_2.9.axorb/alucirc[11:0]3"
        public String locn;         // "D423    3.1,  7.7    78.74, 195.58"
        public TreeMap<Integer,DevPin> devpins = new TreeMap<> ();

        // get string showing diode or jack location
        // also get network associated with diode or jack
        //  we only care about pin 1 of the diode (cathode)
        //                 and pin 1 of the jack (plate)
        public String getinfo ()
        {
            StringBuilder sb = new StringBuilder ();
            sb.append (locn);
            DevPin devpin1 = devpins.get (1);
            if (devpin1 != null) {
                if (name.startsWith ("J")) {
                    sb.append ("  =>  ");
                    sb.append (devpin1.network.name);
                    String sep = "\n            =>  ";
                    for (DevPin devpin : devpin1.network.devpins.values ()) {
                        if (devpin != devpin1) {
                            String refdevname = devpin.device.name;
                            sb.append (sep);
                            sb.append (refdevname);
                            sb.append ('<');
                            sb.append (devpin.pinno);
                            sb.append ('>');
                            if (refdevname.startsWith ("Conn/") && refdevname.endsWith ("con")) {
                                consig (sb, refdevname.charAt (5), devpin.pinno);
                            }
                            sep = "\n                ";
                        }
                    }
                } else {
                    sb.append ("  <=  ");
                    String netname = devpin1.network.name;
                    sb.append (netname);
                    if (netname.startsWith ("I") && netname.endsWith ("con")) {
                        int i = netname.indexOf ('/');
                        int conpin = Integer.parseInt (netname.substring (1, i));
                        char conlet = netname.charAt (++ i);
                        consig (sb, conlet, conpin);
                    }
                }
            }
            return sb.toString ();
        }

        public static void consig (StringBuilder sb, char conlet, int conpin)
        {
            TreeMap<Integer,String> signals = conpins.get (conlet);
            if (signals == null) return;
            String signal = signals.get (conpin);
            if (signal == null) return;
            sb.append (" = ");
            sb.append (signal);
        }
    }

    // a pin of an input diode or 3-pin jack
    public static class DevPin {
        public Device device;
        public int pinno;
        public Network network;
    }

    // all things found in the report file that we care about
    public static TreeMap<String,Device>  devices   = new TreeMap<> ();  // input diodes and 3-pin jacks
    public static TreeMap<String,DFFGang> dffgangs  = new TreeMap<> ();  // gangs of DFFs, DLats
    public static TreeMap<String,DFFGang> gategangs = new TreeMap<> ();  // gangs of gates
    public static TreeMap<String,Network> networks  = new TreeMap<> ();  // single-bit networks

    // abcd pinno => signal name
    public static TreeMap<Character,TreeMap<Integer,String>> conpins = new TreeMap<> ();

    public static void main (String[] args)
        throws Exception
    {
        // read .rep file into DFF list
        BufferedReader br = new BufferedReader (new InputStreamReader (System.in));
        for (String line; (line = br.readLine ()) != null;) {
            if (line.startsWith ("Module ")) break;
        }
        for (String line; (line = br.readLine ()) != null;) {
            line = line.trim ();
            if (line.startsWith ("Wired Ands:")) break;

            checkForDFF (line, ".DFF/");
            checkForDFF (line, ".DLat/");
            checkForGate (line);
        }

        // read in networks
        for (String line; (line = br.readLine ()) != null;) {
            line = line.trim ();
            if (line.startsWith ("Component values:")) break;
            if (line.startsWith ("Network:")) {
                int i = line.indexOf ("(load");
                String netname = line.substring (8, i).trim ();
                Network network = networks.get (netname);
                if (network == null) {
                    network = new Network ();
                    network.name = netname;
                    networks.put (netname, network);
                }
                while ((line = br.readLine ()) != null) {
                    line = line.trim ();
                    if (line.equals ("")) break;
                    i = line.indexOf ("<");
                    String devname = line.substring (0, i).trim ();
                    int pinno = Integer.parseInt (line.substring (i + 1, line.length () - 1));
                    Device device = devices.get (devname);
                    if (device == null) {
                        device = new Device ();
                        device.name = devname;
                        device.locn = "";
                        devices.put (devname, device);
                    }
                    DevPin devpin  = new DevPin (); // link pin of device to network
                    devpin.device  = device;
                    devpin.pinno   = pinno;
                    devpin.network = network;
                    device.devpins.put (pinno, devpin);
                    network.devpins.put (line, devpin);
                }
            }
        }

        br = new BufferedReader (new FileReader ("../modules/cons.inc"));
        char conlet = 0;
        for (String line; (line = br.readLine ()) != null;) {
            line = line.trim ();
            if (line.equals ("")) {
                conlet = 0;
                continue;
            }
            for (String x; !line.equals (x = line.replace ("  ", " "));) line = x;
            String[] parts = line.split (" ");
            if ((parts.length == 3) && parts[0].equals ("#define") && parts[1].endsWith ("body") && parts[2].equals ("\\")) {
                conlet = parts[1].charAt (0);
                continue;
            }
            if ((conlet != 0) && (parts.length > 2) && parts[1].equals ("/*")) {
                int pinno = Integer.parseInt (parts[2]);
                TreeMap<Integer,String> signals = conpins.get (conlet);
                if (signals == null) {
                    signals = new TreeMap<> ();
                    conpins.put (conlet, signals);
                }
                String signal = parts[0].replace (",", "");
                signals.put (pinno, signal);
            }
        }
        br.close ();

        // print out locations of the inputs and outputs
        for (DFFGang dffgang : dffgangs.values ()) {
            for (DFFBit dffbit : dffgang.dffbits.values ()) {
                System.out.println ("");
                System.out.println (dffbit.bitno + dffgang.kind + dffgang.name);
                if (dffgang.kind.equals (".DFF/")) printDFFBit (dffbit);
                if (dffgang.kind.equals (".DLat/")) printDLatBit (dffbit);
            }
        }
        for (DFFGang gategang : gategangs.values ()) {
            for (DFFBit gatebit : gategang.dffbits.values ()) {
                System.out.println ("");
                int bitno = gatebit.bitno;
                System.out.println ((bitno < 0 ? "~" + (-1 - bitno) : "" + bitno) + "." + gategang.name);
                printGateBit (gatebit);
            }
        }
    }

    // line = "D_3.b.0.DFF/defer1/seqcirc  D508    2.1,  7.5    53.34, 190.50"
    //                i                  j
    public static void checkForDFF (String line, String kind)
    {
        if (!line.startsWith ("J") && !line.startsWith ("D_")) return;
        int i = line.indexOf (kind);
        if (i < 0) return;

        int j = line.indexOf (" ", i);
        String gangname = line.substring (i + kind.length (), j);
        DFFGang dffgang = dffgangs.get (gangname);
        if (dffgang == null) {
            dffgang = new DFFGang ();
            dffgang.name = gangname;
            dffgang.kind = kind;
            dffgangs.put (gangname, dffgang);
        }

        // parts[] = "D_3", "b", "0"
        String[] parts = line.substring (0, i).split ("[.]");

        int bitno = Integer.parseInt (parts[2]);
        DFFBit dffbit = dffgang.dffbits.get (bitno);
        if (dffbit == null) {
            dffbit = new DFFBit ();
            dffbit.gang = dffgang;
            dffbit.bitno = bitno;
            dffgang.dffbits.put (bitno, dffbit);
        }

        Nand nand = dffbit.nands.get (parts[1]);
        if (nand == null) {
            nand = new Nand ();
            nand.dffbit = dffbit;
            nand.letter = parts[1];
            dffbit.nands.put (parts[1], nand);
        }

        Device device = new Device ();
        device.name = line.substring (0, j).trim ();
        device.locn = line.substring (j).trim ();
        devices.put (device.name, device);

        if (parts[0].equals ("J")) nand.output = device;
        else nand.inputs.put (parts[0], device);
    }

    // line = "D1_2.~0._clok1/seqcirc[0:0]1  D508    2.1,  7.5    53.34, 190.50"
    //           i                         j
    public static void checkForGate (String line)
    {
        if (!line.startsWith ("J") && !line.startsWith ("D")) return;
        int j = line.indexOf (" ");
        if (j < 0) return;
        String[] parts = line.substring (0, j).split ("[.]");
        if (parts.length != 3) return;
        if (!parts[0].equals ("J")) {
            if (!parts[0].startsWith ("D")) return;
            int i = parts[0].indexOf ("_");
            if (i < 0) return;
        }

        String gangname = parts[2];
        DFFGang gategang = gategangs.get (gangname);
        if (gategang == null) {
            gategang = new DFFGang ();
            gategang.name = gangname;
            gategangs.put (gangname, gategang);
        }

        int bitno;
        if (parts[1].startsWith ("~")) {
            bitno = ~ Integer.parseInt (parts[1].substring (1));
        } else {
            bitno = Integer.parseInt (parts[1]);
        }
        DFFBit gatebit = gategang.dffbits.get (bitno);
        if (gatebit == null) {
            gatebit = new DFFBit ();
            gatebit.gang = gategang;
            gatebit.bitno = bitno;
            gategang.dffbits.put (bitno, gatebit);
        }

        Nand nand = gatebit.nands.get ("");
        if (nand == null) {
            nand = new Nand ();
            nand.dffbit = gatebit;
            gatebit.nands.put ("", nand);
        }

        Device device = new Device ();
        device.name = line.substring (0, j).trim ();
        device.locn = line.substring (j).trim ();
        devices.put (device.name, device);

        if (parts[0].equals ("J")) nand.output = device;
        else nand.inputs.put (parts[0], device);
    }

    public static void printDFFBit (DFFBit dffbit)
    {
        Nand anand = dffbit.nands.get ("a");
        Nand bnand = dffbit.nands.get ("b");
        Nand cnand = dffbit.nands.get ("c");
        Nand dnand = dffbit.nands.get ("d");
        Nand enand = dffbit.nands.get ("e");
        Nand fnand = dffbit.nands.get ("f");

        String config = anand.inputs.size () + " " + bnand.inputs.size () + " " + cnand.inputs.size () + " " +
                        dnand.inputs.size () + " " + enand.inputs.size () + " " + fnand.inputs.size ();
        System.out.println ("    config: " + config);

        System.out.println ("     Q out: " + fnand.output.getinfo ());
        System.out.println ("    _Q out: " + enand.output.getinfo ());

        switch (config) {

            // 2 3 2 2 2 2 : D,T inputs
            case "2 3 2 2 2 2": {
                System.out.println ("a    D  in: " + anand.inputs.get ("D_1").getinfo ());
                System.out.println ("b    T  in: " + bnand.inputs.get ("D_2").getinfo ());
                System.out.println ("c    T  in: " + cnand.inputs.get ("D_1").getinfo ());
                break;
            }

            // 2 3 2 3 2 3 : D,T,_PS inputs
            case "2 3 2 3 2 3": {
                System.out.println ("a    D  in: " + anand.inputs.get ("D_1").getinfo ());
                System.out.println ("b    T  in: " + bnand.inputs.get ("D_2").getinfo ());
                System.out.println ("c    T  in: " + cnand.inputs.get ("D_1").getinfo ());
                System.out.println ("d  _PS  in: " + dnand.inputs.get ("D_3").getinfo ());
                System.out.println ("f  _PS  in: " + fnand.inputs.get ("D_3").getinfo ());
                break;
            }

            // 3 3 2 2 2 2 : D,T,_SC inputs
            case "3 3 2 2 2 2": {
                System.out.println ("a  _SC  in: " + anand.inputs.get ("D_1").getinfo ());
                System.out.println ("a    D  in: " + anand.inputs.get ("D_2").getinfo ());
                System.out.println ("b    T  in: " + bnand.inputs.get ("D_2").getinfo ());
                System.out.println ("c    T  in: " + cnand.inputs.get ("D_1").getinfo ());
                break;
            }

            // 3 3 3 2 3 2 : D,T,_PC inputs
            case "3 3 3 2 3 2": {
                System.out.println ("a  _PC  in: " + anand.inputs.get ("D_1").getinfo ());
                System.out.println ("a    D  in: " + anand.inputs.get ("D_2").getinfo ());
                System.out.println ("b    T  in: " + bnand.inputs.get ("D_2").getinfo ());
                System.out.println ("c  _PC  in: " + cnand.inputs.get ("D_1").getinfo ());
                System.out.println ("c    T  in: " + cnand.inputs.get ("D_2").getinfo ());
                break;
            }

            default: {
                System.out.println ("            UNKNOWN CONFIG");
                break;
            }
        }
    }

    public static void printDLatBit (DFFBit dffbit)
    {
        Nand anand = dffbit.nands.get ("a");
        Nand bnand = dffbit.nands.get ("b");
        Nand cnand = dffbit.nands.get ("c");
        Nand dnand = dffbit.nands.get ("d");

        String config = anand.inputs.size () + " " + bnand.inputs.size () + " " + cnand.inputs.size () + " " +
                        dnand.inputs.size ();
        System.out.println ("    config: " + config);

        System.out.println ("     Q out: " + cnand.output.getinfo ());
        System.out.println ("    _Q out: " + dnand.output.getinfo ());

        switch (config) {

            // 2 2 2 2 : D,G
            case "2 2 2 2": {
                System.out.println ("a    D  in: " + anand.inputs.get ("D_1").getinfo ());
                System.out.println ("a    G  in: " + anand.inputs.get ("D_2").getinfo ());
                System.out.println ("b    G  in: " + bnand.inputs.get ("D_1").getinfo ());
                break;
            }

            // 2 2 2 3 : D,G,_PC
            case "2 2 2 3": {
                System.out.println ("a    D  in: " + anand.inputs.get ("D_1").getinfo ());
                System.out.println ("a    G  in: " + anand.inputs.get ("D_2").getinfo ());
                System.out.println ("b    G  in: " + bnand.inputs.get ("D_1").getinfo ());
                System.out.println ("d  _PC  in: " + dnand.inputs.get ("D_3").getinfo ());
                break;
            }

            // 2 2 3 2 : D,G,_PS
            case "2 2 3 2": {
                System.out.println ("a    D  in: " + anand.inputs.get ("D_1").getinfo ());
                System.out.println ("a    G  in: " + anand.inputs.get ("D_2").getinfo ());
                System.out.println ("b    G  in: " + bnand.inputs.get ("D_1").getinfo ());
                System.out.println ("c  _PS  in: " + cnand.inputs.get ("D_3").getinfo ());
                break;
            }

            default: {
                System.out.println ("            UNKNOWN CONFIG");
                break;
            }
        }
    }

    public static void printGateBit (DFFBit gatebit)
    {
        for (Nand nand : gatebit.nands.values ()) {
            if (nand.output != null) System.out.println ("       out: " + nand.output.getinfo ());
            boolean first = true;
            String lastornum = "";
            for (String iname : nand.inputs.keySet ()) {
                Device input = nand.inputs.get (iname);
                String ornum = iname;
                if (ornum.startsWith ("D")) ornum = ornum.substring (1);
                int i = ornum.indexOf ("_");
                if (i >= 0) ornum = ornum.substring (0, i);
                boolean andterm = ornum.equals (lastornum);
                String prefix = first ? "        in: " :
                              andterm ? "   and  in: " :
                                        "  or    in: ";
                System.out.println (prefix + input.getinfo ());
                first = false;
                lastornum = ornum;
            }
        }
    }
}
