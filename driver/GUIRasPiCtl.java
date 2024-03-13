
// interface between GUI.java and raspictl

// javac -h . GUIRasPiCtl.java

public class GUIRasPiCtl {

    static {
        try {
            String unamem = System.getProperty ("unamem");  // java -Dunamem=`uname -m`
            System.loadLibrary ("GUIRasPiCtl." + unamem);   // looks for libGUIRasPiCtl.`uname -m`.so
        } catch (Throwable t) {
            t.printStackTrace (System.err);
            System.exit (1);
        }
    }

    public native static int main (String[] args);
    public native static int getacl ();
    public native static int getma ();
    public native static int getpc ();
    public native static int getir ();
    public native static int getst ();
    public native static int getgpio ();
    public native static long getcycs ();
    public native static void setsr (int sr);
    public native static void sethalt (boolean halt);
    public native static void stepcyc ();
    public native static String disassemble (int ir, int pc);
    public native static int rdmem (int addr);
    public native static int wrmem (int addr, int data);
    public native static void reset (int addr);
}
