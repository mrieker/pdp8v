
import java.io.PrintStream;
import java.util.HashSet;

public interface IVerilog {
    void getVeriNets (HashSet<Network> verinets);
    void printVerilog (PrintStream ps, boolean myverilog);
}
