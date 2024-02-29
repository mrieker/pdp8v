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
 * Run simulation in TCL interpreter
 */

import java.io.BufferedReader;
import java.io.FileReader;
import java.io.InputStreamReader;
import java.io.PrintStream;
import java.util.HashMap;
import java.util.LinkedList;

import tcl.lang.Command;
import tcl.lang.Interp;
import tcl.lang.TCL;
import tcl.lang.TclException;
import tcl.lang.TclInteger;
import tcl.lang.TclNumArgsException;
import tcl.lang.TclObject;
import tcl.lang.TclString;

public class Simulator {
    private static class PauseTclException extends TclException {
        public PauseTclException (Interp interp)
        {
            super (interp, "hit a pause");
        }
    }

    public HashMap<String,Module> modules;

    private HashMap<String,ForceValue> forcevalues;
    private int simtime;
    private LinkedList<OpndLhs> monitorvalues;
    private Module selectedmodule;

    public Simulator ()
    {
        forcevalues = new HashMap<> ();
        monitorvalues = new LinkedList<> ();
    }

    public void simulate (String simfile)
    {
        Interp interp = new Interp ();
        try {
            interp.eval ("puts {simulate...}");
            interp.createCommand ("echo", new EchoCommand ());
            interp.createCommand ("examine", new ExamineCommand ());
            interp.createCommand ("exists", new ExistsCommand ());
            interp.createCommand ("force", new ForceCommand ());
            interp.createCommand ("module", new ModuleCommand ());
            interp.createCommand ("monitor", new MonitorCommand ());
            interp.createCommand ("pause", new PauseCommand ());
            interp.createCommand ("raspi", new RasPiCommand ());
            interp.createCommand ("run", new RunCommand ());
            updateSimtime (interp);
            if (simfile.equals ("-")) {
                BufferedReader br = new BufferedReader (new InputStreamReader (System.in));
                while (true) {
                    ////System.out.print ((simtime * OpndRhs.SIM_STEPNS) + "ns> ");
                    String line = br.readLine ();
                    if (line == null) break;
                    interp.resetResult ();
                    try {
                        interp.eval (line);
                    } catch (TclException te) {
                        printTclException (interp, te, System.out);
                    }
                }
                System.out.println ();
            } else {
                try {
                    interp.evalFile (simfile);
                } catch (PauseTclException te) {
                    printTclException (interp, te, System.err);
                } catch (TclException te) {
                    printTclException (interp, te, System.err);
                    throw te;
                }
            }
        } catch (Exception e) {
            e.printStackTrace ();
            System.exit (1);
        } finally {
            interp.dispose ();
        }
    }

    private void printTclException (Interp interp, TclException te, PrintStream ps)
    {
        int cc = te.getCompletionCode ();
        switch (cc) {
            case TCL.BREAK:    { ps.println ("TCL.BREAK"); break; }
            case TCL.CONTINUE: { ps.println ("TCL.CONTINUE"); break; }
            case TCL.ERROR:    {
                ps.println ("TCL.ERROR " + interp.getResult ());
                try {
                    TclObject errorInfoObj = interp.getVar ("errorInfo", null, TCL.GLOBAL_ONLY);
                    ps.println ("  " + errorInfoObj.toString ());
                } catch (TclException te2) {
                    // Do nothing when var does not exist.
                }
                break;
            }
            case TCL.RETURN:   { ps.println ("TCL.RETURN " + interp.getResult ()); break; }
            default: { ps.println ("TclException " + cc); break; }
        }
    }

    // format examine command return string for a given variable (wire, in or out pin)
    private static String examineString (OpndLhs variable, int t)
            throws HaltedException
    {
        return variable.getSimString (t, 'x', '1');
    }

    // simtime was just changed, update the tcl-level 'now' global variable
    private void updateSimtime (Interp interp)
            throws TclException
    {
        TclObject nowobj = TclInteger.newInstance (simtime * OpndRhs.SIM_STEPNS);
        interp.setVar ("now", nowobj, TCL.GLOBAL_ONLY);
    }

    // echo string ...
    // - displays the string on stdout
    private class EchoCommand implements Command {
        public void cmdProc (Interp interp, TclObject argv[])
                throws TclException
        {
            String sep = "";
            String str = null;
            for (int i = 0; ++ i < argv.length;) {
                if (str != null) {
                    System.out.print (sep);
                    System.out.print (str);
                    sep = " ";
                }
                str = argv[i].toString ();
            }
            if (str != null) {
                System.out.print (sep);
                System.out.println (str);
            }
        }
    }

    // examine variable
    // - returns the variable as a binary bit string (0, 1, x)
    private class ExamineCommand implements Command {
        public void cmdProc (Interp interp, TclObject argv[])
                throws TclException
        {
            boolean throwup = true;
            String varname = null;
            for (int i = 0; ++ i < argv.length;) {
                String arg = argv[i].toString ();
                if (arg.equals ("-nothrow")) {
                    throwup = false;
                    continue;
                }
                if (arg.startsWith ("-")) {
                    throw new TclException (interp, "unknown option " + arg);
                }
                if (varname != null) {
                    throw new TclNumArgsException (interp, 1, argv, "variable");
                }
                varname = arg;
            }
            if (varname == null) {
                throw new TclNumArgsException (interp, 1, argv, "variable");
            }

            // get varname and parse any [hidim:lodim] suffix
            int lodim = 0;
            int hidim = -1;
            try {
                int obrkt = varname.indexOf ('[');
                if (obrkt >= 0) {
                    int cbrkt = varname.indexOf (']');
                    int colon = varname.indexOf (':');
                    if (colon < 0) {
                        lodim = hidim = Integer.parseInt (varname.substring (obrkt + 1, cbrkt));
                    } else {
                        hidim = Integer.parseInt (varname.substring (obrkt + 1, colon));
                        lodim = Integer.parseInt (varname.substring (colon + 1, cbrkt));
                        if (hidim < lodim) throw new Exception ("hidim " + hidim + " lt lodim " + lodim);
                    }
                    varname = varname.substring (0, obrkt);
                }
            } catch (Exception e) {
                throw new TclException (interp, e.getMessage ());
            }

            // locate variable in current module definition
            if (selectedmodule == null) throw new TclException (interp, "no module selected (use module <modulename> command)");
            String result;
            OpndLhs variable = selectedmodule.variables.get (varname);
            if (variable == null) {
                if (throwup) throw new TclException (interp, "variable " + varname + " not defined in module " + selectedmodule.name);
                result = "";
            } else {

                // get whole string then extract requested field
                try {
                    result = examineString (variable, simtime - 1);
                } catch (HaltedException he) {
                    throw new TclException (interp, "halt instruction");
                }
                if (hidim >= lodim) {
                    if ((lodim < variable.lodim) || (hidim > variable.hidim)) {
                        throw new TclException (interp, "dimensions [" + hidim + ":" + lodim + "] out of range of " + variable.name + "[" + variable.hidim + ":" + variable.lodim + "]");
                    }
                    result = result.substring (variable.hidim - hidim, variable.hidim - lodim + 1);
                }
            }

            interp.setResult (result);
        }
    }

    // see if variable exists
    // - returns true or false
    private class ExistsCommand implements Command {
        public void cmdProc (Interp interp, TclObject argv[])
                throws TclException
        {
            if (argv.length != 2) {
                throw new TclNumArgsException (interp, 1, argv, "variable");
            }

            // get varname
            String varname = argv[1].toString ();

            // try to locate variable in current module definition
            if (selectedmodule == null) throw new TclException (interp, "no module selected (use module <modulename> command)");
            boolean exists = selectedmodule.variables.containsKey (varname);

            interp.setResult (exists);
        }
    }

    // force [-anywire] [-novarok] [inputpin [value]]
    // - writes inputpin with the given value at beginning of each sim step
    // - omitting inputpin and value cancels all force commands
    // - omitting value cancels any force command for the inputpin
    private class ForceCommand implements Command {
        public void cmdProc (Interp interp, TclObject argv[])
                throws TclException
        {
            boolean anywire = false;
            boolean novarok = false;
            String varname = null;
            String valstr = null;
            for (int i = 0; ++ i < argv.length;) {
                String arg = argv[i].toString ();
                if (arg.equalsIgnoreCase ("-anywire")) {
                    anywire = true;
                    continue;
                }
                if (arg.equalsIgnoreCase ("-novarok")) {
                    novarok = true;
                    continue;
                }
                if (arg.startsWith ("-")) {
                    throw new TclNumArgsException (interp, 1, argv, "unknown option " + arg);
                }
                if (varname == null) {
                    varname = arg;
                    continue;
                }
                if (valstr == null) {
                    valstr = arg;
                    continue;
                }
                throw new TclNumArgsException (interp, 1, argv, "too many arguments");
            }
            if (varname == null) {
                forcevalues.clear ();
                return;
            }
            if (valstr == null) {
                forcevalues.remove (varname);
                return;
            }

            long value = decodeForceValue (interp, valstr);

            if (selectedmodule == null) throw new TclException (interp, "no module selected (use module <modulename> command)");
            OpndLhs variable = selectedmodule.variables.get (varname);
            if (variable == null) {
                if (novarok) return;
                throw new TclException (interp, "input pin " + varname + " not defined in module " + selectedmodule.name);
            }
            if (! anywire && ! variable.forceable ()) throw new TclException (interp, "input pin " + varname + " is not an input pin");

            ForceValue forcevalue = new ForceValue ();
            forcevalue.iparam = variable;
            forcevalue.value  = value;

            forcevalues.put (varname, forcevalue);
        }
    }

    // module modulename
    // - select the top-level module being simulated
    // - clears any previous force commands and resets simtime clock
    private class ModuleCommand implements Command {
        public void cmdProc (Interp interp, TclObject argv[])
                throws TclException
        {
            if (argv.length != 2) {
                throw new TclNumArgsException (interp, 1, argv, "modulename or ? for list");
            }
            String modulename = argv[1].toString ();

            if (modulename.equals ("?")) {
                for (String modname : modules.keySet ()) {
                    Module module = modules.get (modname);
                    System.out.print ("  " + modname + " (");
                    String sep = "";
                    for (OpndLhs param : module.params) {
                        System.out.print (sep);
                        if (param instanceof IParam) {
                            System.out.print ("in ");
                        }
                        if (param instanceof OParam) {
                            System.out.print ("out ");
                        }
                        System.out.print (param.name + "[" + param.hidim + ":" + param.lodim + "]");
                        sep = ", ";
                    }
                    System.out.println (")");
                }
            } else {
                forcevalues.clear ();
                simtime = 0;
                updateSimtime (interp);
                selectedmodule = modules.get (modulename);
                if (selectedmodule == null) {
                    throw new TclException (interp, "undefined module " + modulename);
                }
            }
        }
    }

    // monitor variable ...
    // - displays the listed variables at every simulation step (run command)
    // - previous list is cleared and replaced with the supplied list
    private class MonitorCommand implements Command {
        public void cmdProc (Interp interp, TclObject argv[])
                throws TclException
        {
            if (selectedmodule == null) throw new TclException (interp, "no module selected (use module <modulename> command)");
            monitorvalues.clear ();
            for (int i = 0; ++ i < argv.length;) {
                String varname = argv[i].toString ();
                OpndLhs variable = selectedmodule.variables.get (varname);
                if (variable == null) throw new TclException (interp, "variable " + varname + " not defined in module " + selectedmodule.name);
                monitorvalues.add (variable);
            }
        }
    }

    // pause
    // - aborts the script
    private class PauseCommand implements Command {
        public void cmdProc (Interp interp, TclObject argv[])
                throws TclException
        {
            throw new PauseTclException (interp);
        }
    }

    // raspi <instance> <command...>
    private class RasPiCommand implements Command {
        public void cmdProc (Interp interp, TclObject argv[])
                throws TclException
        {
            if (argv.length < 3) {
                throw new TclNumArgsException (interp, 1, argv, "<instance> <command...>");
            }
            String instname = argv[1].toString ();
            String[] args = new String[argv.length-2];
            for (int i = 0; i < args.length; i ++) {
                args[i] = argv[i+2].toString ();
            }

            if (selectedmodule == null) throw new TclException (interp, "no module selected (use module <modulename> command)");
            String result;
            try {
                result = RasPiModule.simCmd (selectedmodule, instname, args, simtime);
            } catch (Exception e) {
                throw new TclException (interp, e.getMessage ());
            }

            if (result != null) interp.setResult (result);
        }
    }

    // run [-verbose] deltatime
    // - step the simulation the given number of nanoseconds
    // - applies any force commands at each step
    // - displays any monitor variables at each step
    // - verbose says to display all output pins at each step
    private class RunCommand implements Command {
        public void cmdProc (Interp interp, TclObject argv[])
                throws TclException
        {
            boolean verbose = false;
            String dtstr = null;
            for (int i = 0; ++ i < argv.length;) {
                String arg = argv[i].toString ();
                if (arg.equals ("-verbose")) {
                    verbose = true;
                    continue;
                }
                if (arg.startsWith ("-")) {
                    dtstr = null;
                    break;
                }
                if (dtstr != null) {
                    dtstr = null;
                    break;
                }
                dtstr = arg;
            }
            if (dtstr == null) {
                throw new TclNumArgsException (interp, 1, argv, "[-verbose] deltatime");
            }

            if (selectedmodule == null) throw new TclException (interp, "no module selected (use module <modulename> command)");

            if (dtstr.endsWith ("ns")) dtstr = dtstr.substring (0, dtstr.length () - 2);
            int dt;
            try {
                dt = Integer.parseInt (dtstr);
            } catch (NumberFormatException nfe) {
                throw new TclException (interp, "bad integer deltatime " + dtstr);
            }
            if (dt % OpndRhs.SIM_STEPNS != 0) {
                throw new TclException (interp, "time " + dt + " not multiple of " + OpndRhs.SIM_STEPNS);
            }
            dt /= OpndRhs.SIM_STEPNS;
            int halted = 0;
            try {
                while (-- dt >= 0) {
                    if (verbose || (monitorvalues.size () > 0)) System.out.println ((simtime * OpndRhs.SIM_STEPNS) + ":");

                    for (ForceValue forcevalue : forcevalues.values ()) {
                        forcevalue.iparam.getSimOutput (simtime - 1);
                        forcevalue.iparam.setSimOutput (simtime, forcevalue.value);
                    }

                    for (OpndLhs variable : selectedmodule.params) {
                        if (variable instanceof OParam) {
                            if (verbose) {
                                System.out.println ("  " + variable.name + " = " + examineString (variable, simtime));
                            } else {
                                variable.getSimOutput (simtime);
                            }
                        }
                    }

                    for (OpndLhs variable : monitorvalues) {
                        System.out.println ("  " + variable.name + " = " + examineString (variable, simtime));
                    }

                    simtime ++;
                }
            } catch (HaltedException he) {
                halted = 1;
            }

            TclObject nowobj = TclInteger.newInstance (halted);
            interp.setVar ("halted", nowobj, TCL.GLOBAL_ONLY);

            updateSimtime (interp);
        }
    }

    // convert numeric string to sim value
    //  decimal by default
    //  starting with 0 followed by digits is octal
    //  starting with 0x is hexadecimal
    //  starting with 0b is binary
    // binary, octal, hexadecimal can have embedded x's to indicate undefined bits
    // returns:
    //  top half: 1 bits where value is 1
    //  bot half: 1 bits where value is 0
    //  if neither set, bit is undefined (x)
    //  will never have both bits set
    private static long decodeForceValue (Interp interp, String s)
            throws TclException
    {
        int  radix = 10;
        int  shift = 0;
        int  valid = 0xFFFFFFFF;
        long value = 0;

        s = s.toLowerCase ();
        int len = s.length ();
        if (len == 0) throw new TclException (interp, "empty numeric string");
        for (int i = 0; i < len; i ++) {
            char c = s.charAt (i);
            if ((c == 'x') && (value == 0) && (i == 1) && (len > 2)) {
                radix = 16;
                shift = 4;
                continue;
            }
            if ((c == 'b') && (value == 0) && (i == 1) && (len > 2)) {
                radix = 2;
                shift = 1;
                continue;
            }
            if ((c == '0') && (i == 0)) {
                radix = 8;
                shift = 3;
                continue;
            }
            if ((c >= '0') && (c <= '9') && (c < '0' + radix)) {
                value = value * radix + c - '0';
                if (shift > 0) {
                    valid = (valid << shift) | (radix - 1);
                }
                if (value > 0xFFFFFFFFL) {
                    throw new TclException (interp, "number too big " + s);
                }
                continue;
            }
            if ((c >= 'a') && (c < 'a' + radix - 10)) {
                assert shift > 0;
                value = (value << shift) | (c - 'a' + 10);
                valid = (valid << shift) | (radix - 1);
                if (value > 0xFFFFFFFFL) {
                    throw new TclException (interp, "number too big " + s);
                }
                continue;
            }
            if ((c == 'x') && (shift > 0)) {
                value <<= shift;
                valid <<= shift;
                if (value > 0xFFFFFFFFL) {
                    throw new TclException (interp, "number too big " + s);
                }
                continue;
            }
            throw new TclException (interp, "bad numeric digit in " +
                    s.substring (0, i) + "<" + c + ">" + s.substring (i + 1));
        }

        assert (value & ~valid & 0xFFFFFFFFL) == 0;
        return (value << 32) | ((value ^ valid) & 0xFFFFFFFFL);
    }

    private static class ForceValue {
        public OpndLhs iparam;
        public long value;
    }
}
