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
 * Right-side operand
 * provides a calculated value (gate output, input pin, wire)
 * output pin too cuz can read from output pin like it is a wire
 * also read-only subfield of any of the above
 */

import java.io.PrintStream;
import java.util.HashMap;
import java.util.LinkedList;

public abstract class OpndRhs {
    public String name;

    //// GENERATION ////

    // generate circuitry if not already and return network for this gate/flipflop output
    // note that things like flipflops have more than one output and so they have more than
    //  one OpndRhs object, so this will return either the Q or _Q network
    //  input:
    //   comps = list of all components for whole circuit board
    //   nets  = list of all networks for whole circuit board
    //   rbit  = relative bitslice number for this gate/flipflop pin
    //  output:
    //   returns network the bitslice is connected to
    public abstract Network generate (GenCtx genctx, int rbit);

    // print propagation chain for this operand
    public void printProp (PropPrinter pp, int rbit)
    {
        pp.println (this.getClass ().getName (), this, rbit);
    }

    //// SIMULATION ////

    protected abstract int busWidthWork ();
    protected abstract void stepSimWork (int t) throws HaltedException;

    public final static int SIM_NSTEPS = 2048;
    public final static int SIM_STEPNS = 2; // time steps are in 2ns units
    public final static int SIM_HOLD   = 2; // transistor output holds steady 4ns after any input change
    public final static int SIM_PROP   = 5; // transistor output steady 10ns after input stabilized

    private int buswidth;
    private int stepsperlong;
    private int validto;

    // top half of long are one bits
    // bottom half of long are zero bits
    private long[] simhistory;

    public final int busWidth ()
    {
        if (buswidth == 0) {
            buswidth = -1;                  // in case of recursion
            buswidth = busWidthWork ();     // compute bus width
        }
        return buswidth;
    }

    public final void stepSim (int t)
            throws HaltedException
    {
        for (int tt; (tt = validto) <= t;) {
            setSimOutput (tt, 0);           // in case of recursion
            stepSimWork (tt);               // compute step value
        }
    }

    // gate calls this to see what some input to the gate is
    //  input:
    //   t = current step number
    //  output:
    //   composite value over previous time SIM_PROP to SIM_HOLD steps ago
    //    ...0...0 for those bits invalid or changed somewhere in interval
    //    ...0...1 for those bits that stayed steady zero throughout
    //    ...1...0 for those bits that stayed steady one throughout
    public long getSimInput (int t)
            throws HaltedException
    {
        if (t < SIM_PROP) return 0;
        long h = ~(-1L << buswidth);
        h |= h << 32;
        for (int i = t - SIM_PROP; i < t - SIM_HOLD; i ++) {
            long m = getSimOutput (i);
            // accum known zeroes and known ones
            h &= m;
            // if neither, done scanning
            if (h == 0) break;
        }
        return h;
    }

    // gate calls this to set its value at the current step
    // also called by main to set value of input pins
    // h = ...0...0 transition
    //     ...0...1 zero
    //     ...1...0 one
    public void setSimOutput (int t, long h)
    {
        if (simhistory == null) {
            buswidth = busWidth ();
            stepsperlong = 32 / buswidth;
            simhistory = new long[(SIM_NSTEPS+stepsperlong-1)/stepsperlong];
        }
        if ((validto != t) && (validto != t + 1)) {
            System.err.println ("OpndRhs.setSimOutput*: name=" + name + " t=" + t + " validto=" + validto);
        }
        assert (validto == t) || (validto == t + 1);
        validto = t + 1;
        t %= SIM_NSTEPS;
        int longindex = t / stepsperlong;               // point to long to be written
        int stepwithinlong = t % stepsperlong;          // get index of buswidth bits within that long
        long m = ~(-1L << buswidth);                    // make mask for buswidth bits at subindex 0
        m  |= m << 32;
        h  &= m;
        assert (h & (h >>> 32)) == 0;                   // make sure h is valid
        assert (h & ~ m) == 0;                          // ...it shouldn't contain bits outside buswidth
        h <<= stepwithinlong * buswidth;                // shift value and mask
        m <<= stepwithinlong * buswidth;
        simhistory[longindex] = (simhistory[longindex] & ~m) | h;
    }

    // called by main to get value at some step
    // also called by things like RField to get current step's value
    public long getSimOutput (int t)
            throws HaltedException
    {
        if (t < 0) return 0;

        // step this element's sim state to cover requested time if not already
        // recursively calls stepSim() for whatever is needed to compute this one
        stepSim (t);

        // get value at time t
        t %= SIM_NSTEPS;
        int longindex = t / stepsperlong;
        int stepwithinlong = t % stepsperlong;
        long h = simhistory[longindex];
        h >>>= stepwithinlong * buswidth;
        long m = ~(-1L << buswidth);
        m |= m << 32;
        h &= m;
        assert (h & (h >>> 32)) == 0;
        return h;
    }

    // get value string for printout at a given time point
    public String getSimString (int t, char und, char one)
            throws HaltedException
    {
        long h = getSimOutput (t);
        int w = busWidth ();
        char[] ch = new char[w];
        while (-- w >= 0) {
            long m = h & 0x100000001L;
            if (m == 0) ch[w] = und;        // usually 'x'
            else if (m == 1) ch[w] = '0';
            else {
                assert m == 0x100000000L;
                ch[w] = one;                // usually '1'
            }
            h >>>= 1;
        }
        return new String (ch);
    }

    // smear an input bus to be as wide as the output bus
    //  input:
    //   t   = time to sample input at
    //   obw = output bus width
    //  output:
    //   returns input value smeared to output width
    public long getSmearedInput (int t, int obw)
            throws HaltedException
    {
        int  ibw  = busWidth ();
        long ival = getSimInput (t);
        long oval = 0;
        assert ((((-1L << ibw) & 0xFFFFFFFFL) * 0x100000001L) & ival) == 0;
        for (int i = 0; i < obw; i += ibw) {
            ival  &= (~(-1L << obw)) * 0x100000001L;
            oval  |= ival;
            ival <<= ibw;
            ival  &= (-1L << (ibw + 32)) | 0xFFFFFFFFL;
        }
        return oval;
    }
}
