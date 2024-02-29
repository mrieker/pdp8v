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
 * Left-hand side operands, eg, wire, output pins.
 * Can be written to as well as read.
 * Also serves for input pins cuz when module is instantiated,
 * the outer module will write to those input pins.
 */

import java.io.PrintStream;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.LinkedList;

public class OpndLhs extends OpndRhs {
    public int lodim;                   // lowest bit number (inclusive) (0: scalar)
    public int hidim;                   // highest bit number (inclusive) (0: scalar)

    private LinkedList<WField> writers; // things that write this operand

    public OpndLhs (String name)
    {
        this ();
        this.name = name;
    }

    public OpndLhs ()
    {
        writers = new LinkedList<> ();
    }

    // add writer to this variable (wire, output parameter)
    // the reducer should make sure they aren't writing to an input pin
    // returns null if successful, else returns error message string
    public String addWriter (WField wfield)
    {
        assert wfield.refdvar == this;
        writers.add (wfield);

        //TODO : check for duplicates
        return null;
    }

    public Iterable<WField> iteratewriters ()
    {
        return writers;
    }

    // see if the wire or output pin has sufficient writers
    // if not, return error message, else return null
    // this implementation is for internal wires
    // input pins (IParam) and output pins (OParam) override as needed
    public String checkWriters ()
    {
        if (hidim < lodim) System.err.println ("OpndLhs.checkWriters: " + name + " hidim=" + hidim + " lodim=" + lodim);
        assert hidim >= lodim;

        // must have exactly one writer for every bit
        int width = hidim - lodim + 1;
        assert width < 64;
        long written = 0;
        for (WField wfield : writers) {
            for (int i = wfield.lodim; i <= wfield.hidim; i ++) {
                long bit = 1L << (i - lodim);
                if ((written & bit) != 0) {
                    return "multiple writers for bit " + i;
                }
                written |= bit;
            }
        }
        if (written != ((1L << width) - 1)) {
            return "some or all bits not written (mask written " + Long.toHexString (written) + ")";
        }

        return null;
    }

    // debugging
    public String getLhsName ()
    {
        StringBuilder sb = new StringBuilder ();
        sb.append (name);
        sb.append ('[');
        sb.append (hidim);
        sb.append (':');
        sb.append (lodim);
        sb.append (']');
        return sb.toString ();
    }

    //// GENERATION ////

    // example:
    //  wire abc[9:4];
    //  wire def[15:11];
    //  abc[7:6] = def[13:12];   << we are doing the one for [6]
    //   this  = 'abc'
    //   rbit  = 2 (ie, 6 - 4)
    //   fanin = how much abc[6] loads 'def[12]'
    //   writer.refdvar = this
    //   writer.lodim   = 6
    //   writer.hidim   = 7
    //   writer.operand = 'def'

    // get all writers wire-anded together and return the wire-and of the given bitslice of the bus
    // if only one writer to the given bitslice, just pass it through to our caller
    private Network[] networks;
    @Override
    public Network generate (GenCtx genctx, int rbit)
    {
        int bw = busWidth ();
        if (networks == null) {
            networks = new Network[bw];
        }
        Network network = networks[rbit];
        if (network == null) {

            // absolute bit number being written (6 in the above example)
            int abit = lodim + rbit;
            assert (lodim <= abit) && (abit <= hidim);

            // count how many writers we have to this bit of the wired-and
            int nwriters = 0;
            WField lastwriter = null;
            for (WField writer : writers) {
                assert writer.hidim >= writer.lodim;
                assert writer.hidim <= hidim;
                assert writer.lodim >= lodim;
                if ((writer.lodim <= abit) && (abit <= writer.hidim)) {
                    nwriters ++;
                    lastwriter = writer;
                }
            }

            switch (nwriters) {

                // if none, return VCC
                case 0: {
                    networks[rbit] = network = genctx.nets.get ("VCC");
                    break;
                }

                // if one, return that one writer (the usual non-wired-and case)
                case 1: {
                    networks[rbit] = network = getWriterNetwork (genctx, lastwriter, abit);
                    break;
                }

                // multiple writers, the wired-and case
                // wrap all the writers in a WiredAnd gate-like object
                // the main program will merge the networks in the netlist and pcb files
                // be sure to set networks[rbit] before calling generate()
                // ...for the writer networks in case of recursion
                default: {
                    WiredAnd wiredand = new WiredAnd (genctx, rbit + "." + name, nwriters);
                    networks[rbit] = network = wiredand;

                    // scan through all the writers to any bits of 'abc'
                    int i = 0;
                    for (WField writer : writers) {
                        Network wn = getWriterNetwork (genctx, writer, abit);
                        if (wn != null) {
                            network.daotag |= wn.daotag;
                            wiredand.subnets[i++] = wn;
                        }
                    }
                    assert i == nwriters;
                    break;
                }
            }
        }
        return network;
    }

    // see if the given writer writes the given abit of this bus
    // if so, generate the writer circuit and return its output network
    // otherwise return null
    private Network getWriterNetwork (GenCtx genctx, WField writer, int abit)
    {
        assert writer.hidim >= writer.lodim;
        assert writer.hidim <= hidim;
        assert writer.lodim >= lodim;

        // see if this writer is writing the rbit we are checking
        // make sure abit is in range of bits being written (7:6 in the example)
        if ((writer.lodim <= abit) && (abit <= writer.hidim)) {

            // we are loading relative bit 0 of the def[13:12] field
            // operand is an RField that relocates to 0 to 12
            int rb = (abit - writer.lodim) % writer.operand.busWidth ();
            return writer.operand.generate (genctx, rb);
        }

        return null;
    }

    // print propagation chain for this operand
    public void printProp (PropPrinter pp, int rbit)
    {
        // absolute bit number being written (6 in the above example)
        int abit = lodim + rbit;
        assert (lodim <= abit) && (abit <= hidim);

        // print prop chains for all writers of this bit
        int nwriters = 0;
        WField lastwriter = null;
        for (WField writer : writers) {
            assert writer.hidim >= writer.lodim;
            assert writer.hidim <= hidim;
            assert writer.lodim >= lodim;
            if ((writer.lodim <= abit) && (abit <= writer.hidim)) {
                int rb = (abit - writer.lodim) % writer.operand.busWidth ();
                writer.operand.printProp (pp, rb);
            }
        }
    }

    //// SIMULATION ////

    @Override
    protected int busWidthWork ()
    {
        assert hidim >= lodim;
        return hidim - lodim + 1;
    }

    // value can be forced with force command in sim script
    public boolean forceable ()
    {
        return false;
    }

    // compute the value on the wire by shifting and wire-anding all the writers
    // wire-anding occurs if there are overlapping writers and should only happen for open-collector wires
    @Override
    protected void stepSimWork (int t)
            throws HaltedException
    {
        // if more than one writer present, assume wire-and
        int any0 = 0;   // mask of bits that at least one writer has given a definite 0
        int not1 = 0;   // mask of bits that at least one writer has given other than a definite 1

        // loop through all the writers
        for (WField wfield : writers) {

            // width of field gives us number of bits being written
            assert wfield.hidim >= wfield.lodim;
            int  wf  = wfield.hidim - wfield.lodim + 1;

            // duplicate input value to width of field being written
            int  wi  = wfield.operand.busWidth ();
            long in  = wfield.operand.getSimOutput (t);
            int in0  = (int) in;
            int in1  = (int) (in >>> 32);
            int dup0 = 0;
            int dup1 = 0;
            for (int i = 0; i < wf; i += wi) {
                dup0 |= in0;
                dup1 |= in1;
                in0 <<= wi;
                in1 <<= wi;
            }

            int wfm = (int) ~(-1L << wf);
            dup0 &= wfm;
            dup1 &= wfm;

            // low wf bits of dup0 say which bits of the input are steady zeroes (higher bits are zeroes)
            // low wf bits of dup1 say which bits of the input are steady ones (higher bits are zeroes)

            dup1 ^= wfm;

            // low wf bits of dup1 say which bits of the input are not steady ones (higher bits are zeroes)

            // shift into place in output
            dup0 <<= wfield.lodim - lodim;
            dup1 <<= wfield.lodim - lodim;

            // merge into output wire-and style
            any0 |= dup0;   // these bits are definitely zeroes cuz at least one input had a steady zero
            not1 |= dup1;   // these bits cannot be ones because either at least one input had a steady zero or at least one input was in transition
        }

        // save wire-anded result
        setSimOutput (t, (((long) ~not1) << 32) | (any0 & 0xFFFFFFFFL));
    }
}
