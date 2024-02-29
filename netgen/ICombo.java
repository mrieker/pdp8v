
import java.io.PrintStream;

public interface ICombo {
    public Network[] getIComboOutputs ();
    public Iterable<Network> getIComboInputs ();
    public void printIComboCSource (PrintStream ps);
}
