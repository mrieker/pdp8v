
import java.io.PrintStream;

public interface ICombo {
    public Network[] getIComboOutputs ();
    public Iterable<Network> getIComboInputs ();
    public int printIComboCSource (PrintStream ps);
}
