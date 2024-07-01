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

// GUI panel wrapper for raspictl

// ./guiraspictl -randmem -printinstr
// ./guiraspictl ../silly/doubleroll.oct
// runs raspictl by calling its main() in a thread

// can also do client/server:
//  on raspi:
//   ./guiraspictl -listen 1234
//  on homepc:
//   ./guiraspictl -connect raspi:1234 -randmem -printinstr

import java.awt.Color;
import java.awt.Dimension;
import java.awt.Font;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.GridLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.io.EOFException;
import java.io.InputStream;
import java.io.OutputStream;
import java.net.ServerSocket;
import java.net.Socket;
import javax.swing.BoxLayout;
import javax.swing.JButton;
import javax.swing.JComponent;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.SwingUtilities;
import javax.swing.Timer;

public class GUI extends JPanel {

    public final static int UPDMS = 100;

    public final static byte CB_RUN    = 1;
    public final static byte CB_HALT   = 2;
    public final static byte CB_CYCLE  = 3;
    public final static byte CB_SAMPLE = 4;
    public final static byte CB_SETSR  = 5;
    public final static byte CB_RDMEM  = 6;
    public final static byte CB_WRMEM  = 7;
    public final static byte CB_RESET  = 8;

    public final static int G_CLOCK =       0x4;   // GPIO[02]   out: send clock signal to cpu
    public final static int G_RESET =       0x8;   // GPIO[03]   out: send reset signal to cpu
    public final static int G_DATA  =    0xFFF0;   // GPIO[15:04] bi: data bus bits
    public final static int G_LINK  =   0x10000;   // GPIO[16]    bi: link bus bit
    public final static int G_IOS   =   0x20000;   // GPIO[17]   out: skip next instruction (valid in IOT.2 only)
    public final static int G_QENA  =   0x80000;   // GPIO[19]   out: we are sending data out to cpu
    public final static int G_IRQ   =  0x100000;   // GPIO[20]   out: send interrupt request to cpu
    public final static int G_DENA  =  0x200000;   // GPIO[21]   out: we are receiving data from cpu
    public final static int G_JUMP  =  0x400000;   // GPIO[22]    in: doing JMP/JMS instruction
    public final static int G_IOIN  =  0x800000;   // GPIO[23]    in: I/O instruction (in state IOT.1)
    public final static int G_DFRM  = 0x1000000;   // GPIO[24]    in: accompanying READ and/or WRITE address uses data frame
    public final static int G_READ  = 0x2000000;   // GPIO[25]    in: cpu wants to read from us
    public final static int G_WRITE = 0x4000000;   // GPIO[26]    in: cpu wants to write to us
    public final static int G_IAK   = 0x8000000;   // GPIO[27]    in: interrupt acknowledge

    public final static int G_DATA0 = G_DATA & - G_DATA;    // lowest bit of G_DATA

    public final static int ST_FETCH1 = 0x00;       // from shadow.h
    public final static int ST_FETCH2 = 0x01;
    public final static int ST_INTAK1 = 0x08;

    public final static int disasmlen = 21;
    public final static String nuldisasm = "                     ";

    // access the processor one way or another
    public abstract static class IAccess {
        public int     acl;
        public int     ma; // df in <14:12>
        public int     pc; // if in <14:12>
        public int     ir;
        public int     st;
        public int     gpio;
        public int     sr;
        public long    cycs;
        public boolean halt;

        public abstract void run ();
        public abstract void halt ();
        public abstract void cycle (boolean ins);
        public abstract void sample ();
        public abstract void setsr (int sr);
        public abstract int rdmem (int addr);
        public abstract int wrmem (int addr, int data);
        public abstract void reset (int addr);
    }

    // run the GUI with the given processor access
    public static IAccess access;

    public static void main (String[] args)
            throws Exception
    {
        // maybe enter server mode
        //  java GUI -server <port>
        if ((args.length > 1) && args[0].equals ("-listen")) {
            int port = 0;
            try {
                port = Integer.parseInt (args[1]);
            } catch (Exception e) {
                System.err.println ("bad/missing -listen port number");
                System.exit (1);
            }
            if (args.length > 2) {
                System.err.println ("extra parameters after -listen port number");
                System.exit (1);
            }
            runServer (port);
            System.exit (0);
        }

        // not server, create window and show it
        JFrame jframe = new JFrame ("PDP-8/V");
        jframe.setDefaultCloseOperation (JFrame.EXIT_ON_CLOSE);
        jframe.setContentPane (new GUI ());
        SwingUtilities.invokeLater (new Runnable () {
            @Override
            public void run ()
            {
                jframe.pack ();
                jframe.setLocationRelativeTo (null);
                jframe.setVisible (true);
            }
        });

        // if -connect, use TCP to connect to server and access the processor that way
        if ((args.length > 1) && args[0].equals ("-connect")) {
            String host = "localhost";
            int i = args[1].indexOf (':');
            String portstr = args[1];
            if (i >= 0) {
                host = portstr.substring (0, i);
                portstr = portstr.substring (++ i);
            }
            int port = 0;
            try {
                port = Integer.parseInt (portstr);
            } catch (Exception e) {
                System.err.println ("bad port number " + portstr);
                System.exit (1);
            }
            Socket socket = new Socket (host, port);
            String[] tcpargs = new String[args.length-2];
            for (i = 0; i < tcpargs.length; i ++) {
                tcpargs[i] = args[i+2];
            }
            access = new TCPAccess (socket, tcpargs);
        }

        // no -connect, access processor directly
        else {
            access = new DirectAccess (args);
        }

        // set initial SR contents from envar
        String srenv = System.getenv ("switchregister");
        int srint = 0;
        if (srenv != null) {
            try {
                srint = Integer.parseInt (srenv, 8);
            } catch (NumberFormatException nfe) {
                System.err.println ("bad switchregister envar value " + srenv);
                System.exit (1);
            }
        }
        writeswitches (srint);
        access.setsr (srint);

        // set initial display to match processor
        updisplay.actionPerformed (null);
    }

    //////////////
    //  SERVER  //
    //////////////

    // act as TCP server to control the processor
    public static void runServer (int port)
            throws Exception
    {
        // get inbound connection from client
        ServerSocket serversocket = new ServerSocket (port);
        System.out.println ("GUI: listening on port " + port);
        Socket socket = serversocket.accept ();
        System.out.println ("GUI: connection accepted");
        InputStream istream = socket.getInputStream ();
        OutputStream ostream = socket.getOutputStream ();

        // read command line arguments
        int argc = readShort (istream);
        String[] args = new String[argc];
        for (int i = 0; i < argc; i ++) {
            args[i] = readString (istream);
        }

        // spawn raspictl as a thread with given command line args
        access = new DirectAccess (args);

        // read and process incoming command bytes
        byte[] sample = new byte[25];
        for (int cmdbyte; (cmdbyte = istream.read ()) >= 0;) {
            switch (cmdbyte) {
                case CB_RUN: {
                    access.run ();
                    break;
                }
                case CB_HALT: {
                    access.halt ();
                    break;
                }
                case CB_CYCLE: {
                    int ins = istream.read ();
                    access.cycle (ins != 0);
                    break;
                }
                case CB_SAMPLE: {
                    access.sample ();

                    sample[ 0] = (byte)(access.acl);
                    sample[ 1] = (byte)(access.acl  >>  8);
                    sample[ 2] = (byte)(access.ma);
                    sample[ 3] = (byte)(access.ma   >>  8);
                    sample[ 4] = (byte)(access.pc);
                    sample[ 5] = (byte)(access.pc   >>  8);
                    sample[ 6] = (byte)(access.ir);
                    sample[ 7] = (byte)(access.ir   >>  8);
                    sample[ 8] = (byte)(access.st);
                    sample[ 9] = (byte)(access.st   >>  8);
                    sample[10] = (byte)(access.gpio);
                    sample[11] = (byte)(access.gpio >>  8);
                    sample[12] = (byte)(access.gpio >> 16);
                    sample[13] = (byte)(access.gpio >> 24);
                    sample[14] = (byte)(access.cycs);
                    sample[15] = (byte)(access.cycs >>  8);
                    sample[16] = (byte)(access.cycs >> 16);
                    sample[17] = (byte)(access.cycs >> 24);
                    sample[18] = (byte)(access.cycs >> 32);
                    sample[19] = (byte)(access.cycs >> 40);
                    sample[20] = (byte)(access.cycs >> 48);
                    sample[21] = (byte)(access.cycs >> 56);
                    sample[22] = (byte)(access.halt ? 1 : 0);
                    sample[23] = (byte)(access.sr);
                    sample[24] = (byte)(access.sr >> 8);

                    ostream.write (sample);
                    break;
                }
                case CB_SETSR: {
                    int sr = readShort (istream);
                    access.setsr (sr);
                    break;
                }
                case CB_RDMEM: {
                    int addr = readShort (istream);
                    int rc = access.rdmem (addr);
                    sample[0] = (byte)(rc);
                    sample[1] = (byte)(rc >>  8);
                    sample[2] = (byte)(rc >> 16);
                    sample[3] = (byte)(rc >> 24);
                    ostream.write (sample, 0, 4);
                    break;
                }
                case CB_WRMEM: {
                    int addr = readShort (istream);
                    int data = readShort (istream);
                    int rc = access.wrmem (addr, data);
                    sample[0] = (byte)(rc);
                    sample[1] = (byte)(rc >>  8);
                    sample[2] = (byte)(rc >> 16);
                    sample[3] = (byte)(rc >> 24);
                    ostream.write (sample, 0, 4);
                    break;
                }
                case CB_RESET: {
                    int addr = readShort (istream);
                    access.reset ((short) addr);
                    break;
                }
                default: {
                    throw new Exception ("bad command byte received " + cmdbyte);
                }
            }
        }
        socket.close ();
    }

    // read string from client
    // - two-byte little endian bytecount
    // - byte string
    public static String readString (InputStream istream)
            throws Exception
    {
        int len = readShort (istream);
        byte[] bytes = new byte[len];
        for (int i = 0; i < len;) {
            int rc = istream.read (bytes, i, len - i);
            if (rc <= 0) throw new EOFException ("EOF reading network");
            i += rc;
        }
        return new String (bytes);
    }

    // read short from client
    // - little endian
    public static int readShort (InputStream istream)
            throws Exception
    {
        int value = istream.read ();
        value |= istream.read () << 8;
        if (value < 0) throw new EOFException ("EOF reading network");
        return value;
    }

    //////////////
    //  CLIENT  //
    //////////////

    // access the processor via TCP connection
    public static class TCPAccess extends IAccess {
        public InputStream istream;
        public OutputStream ostream;

        public byte[] samplebytes = new byte[25];

        public TCPAccess (Socket socket, String[] args)
            throws Exception
        {
            istream = socket.getInputStream ();
            ostream = socket.getOutputStream ();

            byte[] shortbytes = new byte[2];
            shortbytes[0] = (byte)args.length;
            shortbytes[1] = (byte)(args.length >> 8);
            ostream.write (shortbytes);
            for (String arg : args) {
                byte[] argbytes = arg.getBytes ();
                shortbytes[0] = (byte)argbytes.length;
                shortbytes[1] = (byte)(argbytes.length >> 8);
                ostream.write (shortbytes);
                ostream.write (argbytes);
            }
        }

        @Override
        public void run ()
        {
            try {
                ostream.write (CB_RUN);
            } catch (Exception e) {
                e.printStackTrace ();
                System.exit (1);
            }
        }

        @Override
        public void halt ()
        {
            try {
                ostream.write (CB_HALT);
            } catch (Exception e) {
                e.printStackTrace ();
                System.exit (1);
            }
        }

        @Override
        public void cycle (boolean ins)
        {
            try {
                ostream.write (CB_CYCLE);
                ostream.write ((byte)(ins ? 1 : 0));
            } catch (Exception e) {
                e.printStackTrace ();
                System.exit (1);
            }
        }

        @Override
        public void sample ()
        {
            try {
                ostream.write (CB_SAMPLE);
                for (int i = 0; i < 25;) {
                    int rc = istream.read (samplebytes, i, 25 - i);
                    if (rc <= 0) throw new EOFException ("eof reading network");
                    i += rc;
                }
            } catch (Exception e) {
                e.printStackTrace ();
                System.exit (1);
            }

            acl  = ((samplebytes[1] & 0xFF) << 8) | (samplebytes[0] & 0xFF);
            ma   = ((samplebytes[3] & 0xFF) << 8) | (samplebytes[2] & 0xFF);
            pc   = ((samplebytes[5] & 0xFF) << 8) | (samplebytes[4] & 0xFF);
            ir   = ((samplebytes[7] & 0xFF) << 8) | (samplebytes[6] & 0xFF);
            st   = ((samplebytes[9] & 0xFF) << 8) | (samplebytes[8] & 0xFF);
            gpio = ((samplebytes[13] & 0xFF) << 24) | ((samplebytes[12] & 0xFF) << 16) | ((samplebytes[11] & 0xFF) << 8) | (samplebytes[10] & 0xFF);
            cycs = ((samplebytes[21] & 0xFF) << 56) | ((samplebytes[20] & 0xFF) << 48) | ((samplebytes[19] & 0xFF) << 40) | ((samplebytes[18] & 0xFF) << 32) | ((samplebytes[17] & 0xFF) << 24) | ((samplebytes[16] & 0xFF) << 16) | ((samplebytes[15] & 0xFF) << 8) | (samplebytes[14] & 0xFF);
            halt = samplebytes[22] != 0;
            sr   = ((samplebytes[24] & 0xFF) << 8) | (samplebytes[23] & 0xFF);
        }

        @Override
        public void setsr (int sr)
        {
            byte[] msg = { CB_SETSR, (byte) sr, (byte) (sr >> 8) };
            try {
                ostream.write (msg);
            } catch (Exception e) {
                e.printStackTrace ();
                System.exit (1);
            }
        }

        @Override
        public int rdmem (int addr)
        {
            byte[] msg = { CB_RDMEM, (byte) addr, (byte) (addr >> 8) };
            byte[] reply = new byte[4];
            try {
                ostream.write (msg);
                for (int i = 0; i < 4;) {
                    int rc = istream.read (samplebytes, i, 4 - i);
                    if (rc <= 0) throw new EOFException ("eof reading network");
                    i += rc;
                }
            } catch (Exception e) {
                e.printStackTrace ();
                System.exit (1);
            }
            return ((samplebytes[3] & 0xFF) << 24) | ((samplebytes[2] & 0xFF) << 16) | ((samplebytes[1] & 0xFF) << 8) | (samplebytes[0] & 0xFF);
        }

        @Override
        public int wrmem (int addr, int data)
        {
            byte[] msg = { CB_WRMEM, (byte) addr, (byte) (addr >> 8), (byte) data, (byte) (data >> 8) };
            byte[] reply = new byte[4];
            try {
                ostream.write (msg);
                for (int i = 0; i < 4;) {
                    int rc = istream.read (samplebytes, i, 4 - i);
                    if (rc <= 0) throw new EOFException ("eof reading network");
                    i += rc;
                }
            } catch (Exception e) {
                e.printStackTrace ();
                System.exit (1);
            }
            return ((samplebytes[3] & 0xFF) << 24) | ((samplebytes[2] & 0xFF) << 16) | ((samplebytes[1] & 0xFF) << 8) | (samplebytes[0] & 0xFF);
        }

        @Override
        public void reset (int addr)
        {
            byte[] msg = { CB_RESET, (byte) addr, (byte) (addr >> 8) };
            try {
                ostream.write (msg);
            } catch (Exception e) {
                e.printStackTrace ();
                System.exit (1);
            }
        }
    }

    /////////////////////
    //  DIRECT ACCESS  //
    /////////////////////

    // access the processor via direct access
    public static class DirectAccess extends IAccess {

        public DirectAccess (String[] args)
        {
            // set up SR contents so raspictl won't crash
            GUIRasPiCtl.setsr (0);

            // spawn raspictl as a thread with given command line args
            RasPiCtlThread raspictlthread = new RasPiCtlThread ();
            raspictlthread.args = args;
            raspictlthread.start ();

            // wait for raspictl to start up, including loading memory and initializing PC
            GUIRasPiCtl.sethalt (true);
        }

        @Override
        public void run ()
        {
            GUIRasPiCtl.sethalt (false);
        }

        @Override
        public void halt ()
        {
            GUIRasPiCtl.sethalt (true);
        }

        @Override
        public void cycle (boolean ins)
        {
            GUIRasPiCtl.sethalt (true);
            if (ins) GUIRasPiCtl.stepins ();
                else GUIRasPiCtl.stepcyc ();
        }

        @Override
        public void sample ()
        {
            acl  = GUIRasPiCtl.getacl  ();
            ma   = GUIRasPiCtl.getma   ();
            pc   = GUIRasPiCtl.getpc   ();
            ir   = GUIRasPiCtl.getir   ();
            st   = GUIRasPiCtl.getst   ();
            gpio = GUIRasPiCtl.getgpio ();
            cycs = GUIRasPiCtl.getcycs ();
            halt = GUIRasPiCtl.gethalt ();
            sr   = GUIRasPiCtl.getsr   ();
        }

        @Override
        public void setsr (int sr)
        {
            GUIRasPiCtl.setsr (sr);
        }

        @Override
        public int rdmem (int addr)
        {
            return GUIRasPiCtl.rdmem (addr);
        }

        @Override
        public int wrmem (int addr, int data)
        {
            return GUIRasPiCtl.wrmem (addr, data);
        }

        @Override
        public void reset (int addr)
        {
            while (! GUIRasPiCtl.reset (addr)) GUIRasPiCtl.sethalt (true);
        }
    }

    // run the raspictl program in its own thread
    private static class RasPiCtlThread extends Thread {
        public String[] args;

        @Override
        public void run ()
        {
            String[] argv = new String[args.length+2];
            argv[0] = "GUIRasPiCtl";
            argv[1] = "-guimode";
            for (int i = 0; i < args.length; i ++) {
                argv[i+2] = args[i];
            }
            int rc = GUIRasPiCtl.main (argv);
            System.exit (rc);
        }
    }

    ////////////////////
    //  GUI ELEMENTS  //
    ////////////////////

    public static long lastcc = -1;

    public static LED gpioiakled;
    public static LED gpiowrled;
    public static LED gpiordled;
    public static LED gpiodfled;
    public static LED gpioioled;
    public static LED gpiojmpled;
    public static LED gpiodenled;
    public static LED gpioirqled;
    public static LED gpioqenled;
    public static LED gpioiosled;
    public static LED gpioresled;
    public static LED gpioclkled;
    public static LED[] mbleds = new LED[13];

    public static LED linkled;
    public static LED[] acleds = new LED[12];
    public static LED[] maleds = new LED[15];
    public static LED[] pcleds = new LED[15];
    public static Switch[] switches = new Switch[15];
    public static LED runled;
    public static LED[] irleds = new LED[3];
    public static LED[] stleds = new LED[9];

    public static JLabel cyclecntlabel;
    public static HaltRunButton haltrunbutton;
    public static StepCycleButton stepcyclebutton;
    public static StepInstrButton stepinstrbutton;
    public static JLabel disassemlabel;

    // update display with processor state
    public final static ActionListener updisplay =
        new ActionListener () {
            @Override
            public void actionPerformed (ActionEvent ae)
            {
                // read values from raspictl (either directly or via tcp)
                access.sample ();
                int     acl  = access.acl;
                int     ma   = access.ma;
                int     pc   = access.pc;
                int     ir   = access.ir;
                int     st   = access.st;
                int     gpio = access.gpio;
                long    cycs = access.cycs;
                boolean halt = access.halt;
                int     sr   = access.sr;

                // update display LEDs
                gpioiakled.setOn ((gpio & G_IAK)   != 0);
                gpiowrled .setOn ((gpio & G_WRITE) != 0);
                gpiordled .setOn ((gpio & G_READ)  != 0);
                gpiodfled .setOn ((gpio & G_DFRM)  != 0);
                gpioioled .setOn ((gpio & G_IOIN)  != 0);
                gpiojmpled.setOn ((gpio & G_JUMP)  != 0);
                gpiodenled.setOn ((gpio & G_DENA)  != 0);
                gpioirqled.setOn ((gpio & G_IRQ)   != 0);
                gpioqenled.setOn ((gpio & G_QENA)  != 0);
                gpioiosled.setOn ((gpio & G_IOS)   != 0);
                gpioresled.setOn ((gpio & G_RESET) != 0);
                gpioclkled.setOn ((gpio & G_CLOCK) != 0);

                writembleds (gpio / G_DATA0);

                linkled.setOn ((acl & 010000) != 0);
                for (int i = 0; i < 12; i ++) {
                    acleds[i].setOn ((acl & (04000 >> i)) != 0);
                }
                for (int i = 0; i < 15; i ++) {
                    pcleds[i].setOn ((pc & (040000 >> i)) != 0);
                }

                writeswitches (sr);
                runled.setOn (! halt);
                haltrunbutton.setHalt (halt);

                for (int i = 0; i < 3; i ++) {
                    irleds[i].setOn ((ir & (04000 >> i)) != 0);
                }
                for (int i = 0; i < 9; i ++) {
                    stleds[i].setOn ((st & 15) == i);
                }

                writemaleds (ma);

                if (lastcc != cycs) {
                    lastcc = cycs;
                    cyclecntlabel.setText (String.format ("%,9d   ", cycs));
                }

                String dis = nuldisasm;
                if (st != ST_FETCH1) {
                    dis = GUIRasPiCtl.disassemble (ir, pc);
                    String oct = Integer.toOctalString (ir);
                    int octlen = oct.length ();
                    if (octlen < 4) oct = "0000".substring (octlen) + oct;
                    int len = dis.length () + 7;
                    if (len < disasmlen) {
                        dis = "  " + oct + ':' + dis + nuldisasm.substring (len);
                    } else if (len == disasmlen) {
                        dis = "  " + oct + ':' + dis;
                    } else {
                        dis = "  " + oct + ':' + dis.substring (0, disasmlen - 7);
                    }
                }
                disassemlabel.setText (dis);
            }
        };

    // build the display
    public GUI ()
    {
        setLayout (new BoxLayout (this, BoxLayout.Y_AXIS));

        JPanel ledbox = new JPanel ();
        ledbox.setLayout (new BoxLayout (ledbox, BoxLayout.X_AXIS));
        add (ledbox);

        JPanel bits1412 = new JPanel ();
        JPanel bits1109 = new JPanel ();
        JPanel bits0806 = new JPanel ();
        JPanel bits0503 = new JPanel ();
        JPanel bits0200 = new JPanel ();
        JPanel bit_1    = new JPanel ();

        bits1412.setBackground (Color.ORANGE);
        bits1109.setBackground (Color.YELLOW);
        bits0806.setBackground (Color.ORANGE);
        bits0503.setBackground (Color.YELLOW);
        bits0200.setBackground (Color.ORANGE);
        bit_1   .setBackground (Color.YELLOW);

        ledbox.add (new JLabel (" "));
        ledbox.add (bits1412);
        ledbox.add (new JLabel ("  "));
        ledbox.add (bits1109);
        ledbox.add (new JLabel ("  "));
        ledbox.add (bits0806);
        ledbox.add (new JLabel ("  "));
        ledbox.add (bits0503);
        ledbox.add (new JLabel ("  "));
        ledbox.add (bits0200);
        ledbox.add (new JLabel ("  "));
        ledbox.add (bit_1);
        ledbox.add (new JLabel (" "));

        bits1412.setLayout (new GridLayout (9, 3));
        bits1109.setLayout (new GridLayout (9, 3));
        bits0806.setLayout (new GridLayout (9, 3));
        bits0503.setLayout (new GridLayout (9, 3));
        bits0200.setLayout (new GridLayout (9, 3));
        bit_1.setLayout (new GridLayout (9, 1));

        // row 0 - gpio labels
        bits1412.add (centeredLabel (""));
        bits1412.add (centeredLabel (""));
        bits1412.add (centeredLabel (""));
        bits1109.add (centeredLabel ("RD"));
        bits1109.add (centeredLabel ("WR"));
        bits1109.add (centeredLabel ("DF"));
        bits0806.add (centeredLabel ("IO"));
        bits0806.add (centeredLabel ("JMP"));
        bits0806.add (centeredLabel ("IAK"));
        bits0503.add (centeredLabel ("DE"));
        bits0503.add (centeredLabel ("QE"));
        bits0503.add (centeredLabel ("IRQ"));
        bits0200.add (centeredLabel ("IOS"));
        bits0200.add (centeredLabel ("RES"));
        bits0200.add (centeredLabel ("CLK"));
        bit_1.add (centeredLabel (""));

        // row 1 - gpio control bits
        bits1412.add (new JLabel (""));
        bits1412.add (new JLabel (""));
        bits1412.add (new JLabel (""));
        bits1109.add (gpiordled  = new LED ());
        bits1109.add (gpiowrled  = new LED ());
        bits1109.add (gpiodfled  = new LED ());
        bits0806.add (gpioioled  = new LED ());
        bits0806.add (gpiojmpled = new LED ());
        bits0806.add (gpioiakled = new LED ());
        bits0503.add (gpiodenled = new LED ());
        bits0503.add (gpioqenled = new LED ());
        bits0503.add (gpioirqled = new LED ());
        bits0200.add (gpioiosled = new LED ());
        bits0200.add (gpioresled = new LED ());
        bits0200.add (gpioclkled = new LED ());
        bit_1.add (new JLabel (""));

        // row 2 - gpio link/data bits
        bits1412.add (new JLabel (""));
        bits1412.add (new JLabel (""));
        bits1412.add (mbleds[0] = new LED ());
        for (int i =  1; i <  4; i ++) bits1109.add (mbleds[i] = new LED ());
        for (int i =  4; i <  7; i ++) bits0806.add (mbleds[i] = new LED ());
        for (int i =  7; i < 10; i ++) bits0503.add (mbleds[i] = new LED ());
        for (int i = 10; i < 13; i ++) bits0200.add (mbleds[i] = new LED ());
        bit_1.add (new JLabel ("MB"));

        // row 3 - link and AC
        bits1412.add (new JLabel (""));
        bits1412.add (new JLabel (""));
        bits1412.add (linkled = new LED ());
        for (int i =  0; i <  3; i ++) bits1109.add (acleds[i] = new LED ());
        for (int i =  3; i <  6; i ++) bits0806.add (acleds[i] = new LED ());
        for (int i =  6; i <  9; i ++) bits0503.add (acleds[i] = new LED ());
        for (int i =  9; i < 12; i ++) bits0200.add (acleds[i] = new LED ());
        bit_1.add (new JLabel ("AC"));

        // row 4 - DF/MA register
        for (int i =  0; i <  3; i ++) bits1412.add (maleds[i] = new LED ());
        for (int i =  3; i <  6; i ++) bits1109.add (maleds[i] = new LED ());
        for (int i =  6; i <  9; i ++) bits0806.add (maleds[i] = new LED ());
        for (int i =  9; i < 12; i ++) bits0503.add (maleds[i] = new LED ());
        for (int i = 12; i < 15; i ++) bits0200.add (maleds[i] = new LED ());
        bit_1.add (new JLabel ("MA"));

        // row 5 - IF/PC register
        for (int i =  0; i <  3; i ++) bits1412.add (pcleds[i] = new LED ());
        for (int i =  3; i <  6; i ++) bits1109.add (pcleds[i] = new LED ());
        for (int i =  6; i <  9; i ++) bits0806.add (pcleds[i] = new LED ());
        for (int i =  9; i < 12; i ++) bits0503.add (pcleds[i] = new LED ());
        for (int i = 12; i < 15; i ++) bits0200.add (pcleds[i] = new LED ());
        bit_1.add (new JLabel ("PC"));

        // row 6 - switch register
        for (int i =  0; i <  3; i ++) bits1412.add (switches[i] = new Switch ());
        for (int i =  3; i <  6; i ++) bits1109.add (switches[i] = new Switch ());
        for (int i =  6; i <  9; i ++) bits0806.add (switches[i] = new Switch ());
        for (int i =  9; i < 12; i ++) bits0503.add (switches[i] = new Switch ());
        for (int i = 12; i < 15; i ++) bits0200.add (switches[i] = new Switch ());
        bit_1.add (new JLabel ("SR"));

        // row 7 - IR and state bits
        bits1412.add (runled = new LED ());
        bits1412.add (new JLabel (""));
        bits1412.add (new JLabel (""));
        for (int i =  0; i <  3; i ++) bits1109.add (irleds[i] = new LED ());
        bits0806.add (stleds[8] = new LED ());
        for (int i = 0; i < 2; i ++) bits0806.add (stleds[i] = new LED ());
        for (int i = 2; i < 5; i ++) bits0503.add (stleds[i] = new LED ());
        for (int i = 5; i < 8; i ++) bits0200.add (stleds[i] = new LED ());
        bit_1.add (new JLabel (""));

        // row 8 - state bit labels
        bits1412.add (centeredLabel ("RUN"));
        bits1412.add (centeredLabel (""));
        bits1412.add (centeredLabel (""));
        bits1109.add (centeredLabel (""));
        bits1109.add (centeredLabel ("IR"));
        bits1109.add (centeredLabel (""));
        bits0806.add (centeredLabel ("IAK"));
        bits0806.add (centeredLabel ("FET"));
        bits0806.add (centeredLabel ("CH"));
        bits0503.add (centeredLabel ("DE"));
        bits0503.add (centeredLabel ("FE"));
        bits0503.add (centeredLabel ("R"));
        bits0200.add (centeredLabel ("EX"));
        bits0200.add (centeredLabel ("E"));
        bits0200.add (centeredLabel ("C"));
        bit_1.add (centeredLabel (""));

        // buttons along the bottom
        JPanel buttonbox1 = new JPanel ();
        buttonbox1.setLayout (new BoxLayout (buttonbox1, BoxLayout.X_AXIS));
        add (buttonbox1);

        cyclecntlabel = new JLabel (" ");
        cyclecntlabel.setFont (new Font (Font.MONOSPACED, Font.PLAIN, cyclecntlabel.getFont ().getSize ()));
        buttonbox1.add (cyclecntlabel);

        buttonbox1.add (haltrunbutton = new HaltRunButton ());
        buttonbox1.add (stepcyclebutton = new StepCycleButton ());
        buttonbox1.add (stepinstrbutton = new StepInstrButton ());

        disassemlabel = new JLabel (nuldisasm);
        disassemlabel.setFont (new Font (Font.MONOSPACED, Font.PLAIN, disassemlabel.getFont ().getSize ()));
        buttonbox1.add (disassemlabel);

        // 2nd row of buttons along bottom
        JPanel buttonbox2 = new JPanel ();
        buttonbox2.setLayout (new BoxLayout (buttonbox2, BoxLayout.X_AXIS));
        add (buttonbox2);

        buttonbox2.add (new LdAddrButton ());
        buttonbox2.add (new ExamButton ());
        buttonbox2.add (new DepButton ());
        buttonbox2.add (new ResetButton ());
        buttonbox2.add (new LdIFPCButton ());
    }

    public static JLabel centeredLabel (String label)
    {
        JLabel jl = new JLabel (label);
        jl.setHorizontalAlignment (JLabel.CENTER);
        return jl;
    }

    // read 15-bit value from SR (switch register) leds
    public static int readswitches ()
    {
        int sr = 0;
        for (int i = 0; i < 15; i ++) {
            Switch sw = switches[i];
            if (sw.ison) sr |= 040000 >> i;
        }
        return sr;
    }

    // write 15-bit value to SR (switch register) leds
    public static void writeswitches (int sr)
    {
        for (int i = 0; i < 15; i ++) {
            Switch sw = switches[i];
            sw.setOn ((sr & (040000 >> i)) != 0);
        }
    }

    // write 15-bit value to MA (memory address) leds
    public static void writemaleds (int maval)
    {
        for (int i = 0; i < 15; i ++) {
            maleds[i].setOn ((maval & (040000 >> i)) != 0);
        }
    }

    // write 13-bit value to MB (memory buffer/gpio link,data) leds
    public static void writembleds (int mbval)
    {
        for (int i = 0; i < 13; i ++) {
            mbleds[i].setOn ((mbval & (010000 >> i)) != 0);
        }
    }

    public static class LED extends JPanel {
        public final static int P = 5;  // padding
        public final static int D = 20; // diameter

        private boolean ison;

        public LED ()
        {
            Dimension d = new Dimension (P + D + P, P + D + P);
            setMaximumSize (d);
            setMinimumSize (d);
            setPreferredSize (d);
            setSize (d);
            repaint ();
        }

        public void setOn (boolean on)
        {
            if (ison != on) {
                ison = on;
                repaint ();
            }
        }

        @Override
        public void paint (Graphics g)
        {
            g.setColor (Color.GRAY);
            g.fillArc (P - 3, P - 3, D + 6, D + 6, 0, 360);
            Color ledcolor = ison ? Color.RED : Color.BLACK;
            g.setColor (ledcolor);
            g.fillArc (P, P, D, D, 0, 360);
        }
    }

    public static class Switch extends JButton implements ActionListener {
        public final static int P = 5;
        public final static int D = 20;

        private boolean ison;

        public Switch ()
        {
            Dimension d = new Dimension (P + D + P, P + D + P);
            setMaximumSize (d);
            setMinimumSize (d);
            setPreferredSize (d);
            setSize (d);
            addActionListener (this);
        }

        @Override  // ActionListener
        public void actionPerformed (ActionEvent ae)
        {
            setOn (! ison);
            access.setsr (readswitches () & 077777);
        }

        public void setOn (boolean on)
        {
            if (ison != on) {
                ison = on;
                repaint ();
            }
        }

        @Override
        public void paint (Graphics g)
        {
            g.setColor (Color.GRAY);
            g.fillArc (P - 3, P - 3, D + 6, D + 6, 0, 360);
            Color ledcolor = ison ? Color.RED : Color.BLACK;
            g.setColor (ledcolor);
            g.fillArc (P, P, D, D, 0, 360);
        }
    }

    private static class HaltRunButton extends JButton implements ActionListener {
        private Timer updatetimer;      // null: processor is known to be halted
                                        //       button says RUN to get it going
                                        // else: processor is known to be running
                                        //       button says HALT to halt it

        public HaltRunButton ()
        {
            super ("RUN");
            addActionListener (this);
        }

        // called by updatetimer
        public void setHalt (boolean halt)
        {
            // processor may have halted on its own (like from a stopat address), so update button
            if (halt && (updatetimer != null)) haltClicked ();
        }

        @Override  // ActionListener
        public void actionPerformed (ActionEvent ae)
        {
            if (updatetimer == null) {
                runClicked ();
            } else {
                haltClicked ();
            }
        }

        private void runClicked ()
        {
            setText ("HALT");

            // tell raspictl main() thread to start/resume clocking tubes
            access.run ();

            // update display at UPDMS rate
            updatetimer = new Timer (UPDMS, updisplay);
            updatetimer.start ();
        }

        private void haltClicked ()
        {
            setText ("RUN");

            // tell raspictl main() thread to stop clocking tubes and wait for it to stop
            access.halt ();

            // shut off timer while halted so it doesn't interfere with LDADDR/DEP/EXAM buttons
            updatetimer.stop ();
            updatetimer = null;

            // make sure we get a final display update
            updisplay.actionPerformed (null);
        }
    }

    private static class StepCycleButton extends JButton implements ActionListener {
        public StepCycleButton ()
        {
            super ("S/CYC");
            addActionListener (this);
        }

        @Override  // ActionListener
        public void actionPerformed (ActionEvent ae)
        {
            access.cycle (false);
            updisplay.actionPerformed (null);
        }
    }

    private static class StepInstrButton extends JButton implements ActionListener, Runnable {
        public StepInstrButton ()
        {
            super ("S/INS");
            addActionListener (this);
        }

        @Override  // ActionListener
        public void actionPerformed (ActionEvent ae)
        {
            // loop with invokeLater() so we can see LEDs update between cycles
            SwingUtilities.invokeLater (this);
        }

        @Override  // Runnable
        public void run ()
        {
            access.cycle (true);
            updisplay.actionPerformed (null);
        }
    }

    ///////////////////////////
    // MEMORY ACCESS BUTTONS //
    ///////////////////////////

    private static int loadedaddress;
    private static int autoincloadedaddress;

    // - load address button
    private static class LdAddrButton extends MemButton {
        public LdAddrButton ()
        {
            super ("LD ADDR");
        }

        @Override  // MemButton
        public void actionPerformed (ActionEvent ae)
        {
            autoincloadedaddress = 0;
            loadedaddress = readswitches () & 077777;
            writemaleds (loadedaddress);
        }
    }

    // - examine button
    private static class ExamButton extends MemButton {
        public ExamButton ()
        {
            super ("EXAM");
        }

        @Override  // MemButton
        public void actionPerformed (ActionEvent ae)
        {
            if (autoincloadedaddress < 0) {
                loadedaddress = (loadedaddress + 1) & 077777;
            }
            writemaleds (loadedaddress);
            int data = access.rdmem (loadedaddress);
            writembleds (data);
            autoincloadedaddress = -1;
        }
    }

    // - deposit button
    private static class DepButton extends MemButton {
        public DepButton ()
        {
            super ("DEP");
        }

        @Override  // MemButton
        public void actionPerformed (ActionEvent ae)
        {
            if (autoincloadedaddress > 0) {
                loadedaddress = (loadedaddress + 1) & 077777;
            }
            writemaleds (loadedaddress);
            int data = readswitches () & 07777;
            int rc = access.wrmem (loadedaddress, data);
            writembleds (data);
            autoincloadedaddress = 1;
        }
    }

    // - reset I/O devices and processor, set IF/PC to -startpc, or as given by .BIN/.RIM file, or 0
    private static class ResetButton extends MemButton {
        private ResetButton ()
        {
            super ("RESET");
        }

        @Override  // MemButton
        public void actionPerformed (ActionEvent ae)
        {
            access.reset (-1);
            updisplay.actionPerformed (null);
        }
    }

    // - reset I/O devices and processor, set IF/PC to 15-bit value in switchregister
    private static class LdIFPCButton extends MemButton {
        private LdIFPCButton ()
        {
            super ("LD IF/PC");
        }

        @Override  // MemButton
        public void actionPerformed (ActionEvent ae)
        {
            access.reset (readswitches () & 077777);
            updisplay.actionPerformed (null);
        }
    }

    private static abstract class MemButton extends JButton implements ActionListener {
        public MemButton (String lbl)
        {
            super (lbl);
            addActionListener (this);
        }

        @Override  // ActionListener
        public abstract void actionPerformed (ActionEvent ae);
    }
}
