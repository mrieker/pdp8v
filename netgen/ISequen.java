
import java.io.PrintStream;
import java.util.TreeMap;

public interface ISequen {
    public int printISequenDecl (PrintStream ps, int bai);
    public void printISequenUnDecl (PrintStream ps);
    public void printISequenKer (PrintStream ps);
    public void printISequenChunk (PrintStream ps);
    public Network[] getISequenOutNets ();
    public String getISequenName ();
    public TreeMap<String,Network> getISequenInNets ();
}
