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

import java.io.PrintStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;

public class Gate extends OpndRhs {
    public enum TYPE {
        AND, AO, AOI, NAND, NOR, NOT, OR;
    }

    private TYPE type;              // AND,OR,NOT etc
    private OpndRhs[] operands;     // all the inputs to the gate
    private String lighted;         // include an led (lights when output is zero)
    private boolean opencoll;       // omit output pullup resistor

    // create gate object, possibly optimizing it
    public static OpndRhs create (String name, TYPE type, OpndRhs[] operands)
    {
        Gate newgate = new Gate ();
        newgate.name = name;

        // maybe we can find AND-OR arrangement
        // OR gate must have at least one AND gate input
        if (type == TYPE.OR) {
            Gate[]    andops    = new Gate[operands.length];
            OpndRhs[] nonandops = new OpndRhs[operands.length];
            int numandops = 0;
            int numnonandops = 0;
            int totandops = 0;
            for (OpndRhs op : operands) {
                Gate gop;
                if ((op instanceof Gate) && ((gop = ((Gate) op)).type == TYPE.AND)) {
                    totandops += gop.operands.length;
                    andops[numandops++] = gop;
                } else {
                    nonandops[numnonandops++] = op;
                }
            }
            if (numandops > 0) {
                OpndRhs[] aoopnds = new OpndRhs[numnonandops+numandops+totandops];
                int i = 0;
                for (int j = 0; j < numnonandops; j ++) {
                    aoopnds[i++] = nonandops[j];
                }
                for (int j = 0; j < numandops; j ++) {
                    aoopnds[i++] = null;
                    for (OpndRhs andop : andops[j].operands) {
                        aoopnds[i++] = andop;
                    }
                }
                assert i == aoopnds.length;
                newgate.type = TYPE.AO;
                newgate.operands = aoopnds;
                return newgate;
            }
        }

        if (type == TYPE.NOT) {
            assert operands.length == 1;
            OpndRhs nottedop = operands[0];
            if (nottedop instanceof Gate) {
                Gate nottedgate = (Gate) nottedop;
                switch (nottedgate.type) {
                    case AND: {
                        newgate.type = TYPE.NAND;
                        newgate.operands = nottedgate.operands;
                        return newgate;
                    }
                    case AO: {
                        newgate.type = TYPE.AOI;
                        newgate.operands = nottedgate.operands;
                        return newgate;
                    }
                    case NOT: {
                        assert nottedgate.operands.length == 1;
                        return nottedgate.operands[0];
                    }
                    case OR: {
                        newgate.type = TYPE.NOR;
                        newgate.operands = nottedgate.operands;
                        return newgate;
                    }
                }
            }
        }

        newgate.type = type;
        newgate.operands = operands;
        return newgate;
    }

    public void setLighted (String label)
    {
        lighted = label;
    }

    public void setOpenColl ()
    {
        opencoll = true;
    }

    private Gate () { }

    //// GENERATION ////

    // generate circuit for the given bitslice of the gate array
    //  input:
    //   rbit = relative bitnumber in the array
    //  output:
    //   returns network for the gate's output
    private StandCell[] standcells;     // cells with output of the requested function
    @Override
    public Network generate (GenCtx genctx, int rbit)
    {
        // set up a standard cell for the rbit bitslice the gate
        // it holds the transistor, associated diodes and resistors
        int bw = busWidth ();
        if (standcells == null) {
            standcells = new StandCell[bw];
        }

        // if we haven't already, generate electronics for the rbit bitslice of the gate
        StandCell cell = standcells[rbit];
        if (cell == null) {
            standcells[rbit] = cell = new StandCell (genctx, rbit + "." + name); ////, lighted, opencoll);
            genctx.addPlaceable (cell);
            genctx.verilogs.add (cell);
            genctx.icombos.add (cell);
            genctx.printlocs.put (cell.name, cell);

            switch (type) {

                // and-or cell has an inverter tacked on an and-or-invert
                //  standcells[] = cell = output of the inverter
                case AO: {
                    StandCell invcell = new StandCell (genctx, "~" + rbit + "." + name);
                    genctx.addPlaceable (invcell);
                    genctx.verilogs.add (invcell);
                    genctx.icombos.add (invcell);
                    genctx.printlocs.put (invcell.name, invcell);
                    cell.addInput (0, invcell.getOutput ());
                    cell.close ();
                    fillAOIGate (genctx, rbit, invcell);
                    break;
                }

                // and-or-invert gate
                //  first group of operands up to null are single or inputs
                //  remainder groups are and groups
                case AOI: {
                    fillAOIGate (genctx, rbit, cell);
                    break;
                }

                // and cell has an inverter tacked on a nand
                //  standcells[] = cell = output of the inverter
                case AND: {
                    StandCell invcell = new StandCell (genctx, "~" + rbit + "." + name);
                    genctx.addPlaceable (invcell);
                    genctx.verilogs.add (invcell);
                    genctx.icombos.add (invcell);
                    genctx.printlocs.put (invcell.name, invcell);
                    cell.addInput (0, invcell.getOutput ());
                    cell.close ();
                    fillNandGate (genctx, rbit, invcell);
                    break;
                }

                // nand gate outputs as a single and group to the cell
                case NAND: {
                    fillNandGate (genctx, rbit, cell);
                    break;
                }

                // or cell has an inverter tacked on a nor
                //  standcells[] = cell = output of the inverter
                case OR: {
                    StandCell invcell = new StandCell (genctx, "~" + rbit + "." + name);
                    genctx.addPlaceable (invcell);
                    genctx.verilogs.add (invcell);
                    genctx.icombos.add (invcell);
                    genctx.printlocs.put (invcell.name, invcell);
                    cell.addInput (0, invcell.getOutput ());
                    cell.close ();
                    fillNorGate (genctx, rbit, invcell);
                    break;
                }

                // nor gate outputs as one-element or groups to the cell
                // inverter is just a one-input nor gate
                case NOR:
                case NOT: {
                    fillNorGate (genctx, rbit, cell);
                    break;
                }

                default: assert false;
            }
        }

        return cell.getOutput ();
    }

    private void fillAOIGate (GenCtx genctx, int rbit, StandCell cell)
    {
        int orindex = 0;
        int i;
        for (i = 0; i < operands.length; i ++) {
            OpndRhs opnd = operands[i];
            if (opnd == null) break;
            int bi = opnd.busWidth ();
            cell.addInput (orindex ++, opnd.generate (genctx, rbit % bi));
        }
        while (++ i < operands.length) {
            OpndRhs opnd = operands[i];
            if (opnd == null) orindex ++;
            else {
                int bi = opnd.busWidth ();
                cell.addInput (orindex, opnd.generate (genctx, rbit % bi));
            }
        }
        cell.close ();
    }

    private void fillNandGate (GenCtx genctx, int rbit, StandCell cell)
    {
        for (OpndRhs opnd : operands) {
            int bi = opnd.busWidth ();
            cell.addInput (0, opnd.generate (genctx, rbit % bi));
        }
        cell.close ();
    }

    private void fillNorGate (GenCtx genctx, int rbit, StandCell cell)
    {
        int orindex = 0;
        for (OpndRhs opnd : operands) {
            int bi = opnd.busWidth ();
            cell.addInput (orindex ++, opnd.generate (genctx, rbit % bi));
        }
        cell.close ();
    }

    // print propagation chain for this gate
    @Override
    public void printProp (PropPrinter pp, int rbit)
    {
        pp.println ("Gate." + type, this, rbit);
        pp.inclevel ();
        for (OpndRhs opnd : operands) {
            if (opnd != null) {
                int bi = opnd.busWidth ();
                opnd.printProp (pp, rbit % bi);
            }
        }
        pp.declevel ();
    }

    //// SIMULATION ////

    @Override
    protected int busWidthWork ()
    {
        int widest = 1;
        for (OpndRhs operand : operands) {
            if (operand != null) {
                int w = operand.busWidth ();
                if (widest < w) widest = w;
            }
        }
        return widest;
    }

    @Override
    protected void stepSimWork (int t)
            throws HaltedException
    {
        switch (type) {

            // compute each column independently
            //  if any input steady zero, output is zero
            //  if all inputs steady one, output is one
            //  otherwise, output is invalid
            case AND: {
                setSimOutput (t, stepSimAnd (t, operands, 0, operands.length));
                break;
            }

            case AO: {
                setSimOutput (t, stepSimAO (t, operands));
                break;
            }

            case AOI: {
                long in = stepSimAO (t, operands);
                setSimOutput (t, (in << 32) | (in >>> 32));
                break;
            }

            case NAND: {
                long in = stepSimAnd (t, operands, 0, operands.length);
                setSimOutput (t, (in << 32) | (in >>> 32));
                break;
            }

            case NOR: {
                long in = stepSimOr (t, operands, 0, operands.length);
                setSimOutput (t, (in << 32) | (in >>> 32));
                break;
            }

            // just swap the long to flip 0<->1
            case NOT: {
                assert operands.length == 1;
                OpndRhs operand = operands[0];
                long in = operand.getSimInput (t);
                setSimOutput (t, (in << 32) | (in >>> 32));
                break;
            }

            // compute each column independently
            //  if any input steady one, output is one
            //  if all inputs steady zero, output is zero
            //  otherwise, output is invalid
            case OR: {
                setSimOutput (t, stepSimOr (t, operands, 0, operands.length));
                break;
            }

            default: assert false;
        }
    }

    // compute each column independently
    private long stepSimAO (int t, OpndRhs[] opnds)
            throws HaltedException
    {
        // up to first null entry is just misc inputs ORd together
        int j = -1;
        while (++ j < opnds.length) if (opnds[j] == null) break;
        long ored = stepSimOr (t, opnds, 0, j);
        int all0  = (int) ored;
        int any1  = (int) (ored >>> 32);

        // remaining are groups of AND terms separated by nulls
        // then OR the AND results with previous results
        while (++ j < opnds.length) {
            int k = j;
            while (++ j < opnds.length) if (opnds[j] == null) break;
            long in = stepSimAnd (t, opnds, k, j);
            int in0 = (int) in;
            int in1 = (int) (in >>> 32);
            assert (in0 & in1) == 0;
            all0 &= in0;
            any1 |= in1;
        }
        assert (all0 & any1) == 0;
        return (((long) any1) << 32) | (all0 & 0xFFFFFFFFL);
    }

    // compute each column independently
    //  if any input steady zero, output is zero
    //  if all inputs steady one, output is one
    //  otherwise, output is invalid
    private long stepSimAnd (int t, OpndRhs[] opnds, int beg, int end)
            throws HaltedException
    {
        int any0 = 0;
        int all1 = 0xFFFFFFFF;
        int wo = busWidth ();
        for (int j = beg; j < end; j ++) {
            OpndRhs opnd = opnds[j];
            int  wi = opnd.busWidth ();
            long in = opnd.getSimInput (t);
            int in0 = (int) in;
            int in1 = (int) (in >>> 32);
            assert (in0 & in1) == 0;
            int dup0 = 0;
            int dup1 = 0;
            for (int i = 0; i < wo; i += wi) {
                dup0 |= in0;
                dup1 |= in1;
                in0 <<= wi;
                in1 <<= wi;
            }
            any0 |= dup0;
            all1 &= dup1;
        }
        assert (any0 & all1) == 0;
        return (((long) all1) << 32) | (any0 & 0xFFFFFFFFL);
    }

    // compute each column independently
    //  if any input steady one, output is one
    //  if all inputs steady zero, output is zero
    //  otherwise, output is invalid
    private long stepSimOr (int t, OpndRhs[] opnds, int beg, int end)
            throws HaltedException
    {
        int all0 = 0xFFFFFFFF;
        int any1 = 0;
        int wo = busWidth ();
        for (int j = beg; j < end; j ++) {
            OpndRhs opnd = opnds[j];
            int  wi = opnd.busWidth ();
            long in = opnd.getSimInput (t);
            int in0 = (int) in;
            int in1 = (int) (in >>> 32);
            assert (in0 & in1) == 0;
            int dup0 = 0;
            int dup1 = 0;
            for (int i = 0; i < wo; i += wi) {
                dup0 |= in0;
                dup1 |= in1;
                in0 <<= wi;
                in1 <<= wi;
            }
            all0 &= dup0;
            any1 |= dup1;
        }
        assert (all0 & any1) == 0;
        return (((long) any1) << 32) | (all0 & 0xFFFFFFFFL);
    }
}
