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
 * Wired-AND circuit
 */

import java.util.Iterator;
import java.util.LinkedList;

public class WiredAnd extends Network {
    public Network[] subnets;

    public WiredAnd (GenCtx genctx, String name, int nwriters)
    {
        super (genctx, name);
        subnets = new Network[nwriters];
        assert genctx.wiredands.get (name) == null;
        genctx.wiredands.put (name, this);
    }

    // iterator over all connections of this network
    //  and all its subnets and all their subnets, etc
    @Override
    public Iterable<Conn> connIterable ()
    {
        return new WAConnIterable ();
    }

    private class WAConnIterable implements Iterable <Conn> {

        @Override
        public Iterator<Conn> iterator ()
        {
            return new WAConnIterator ();
        }
    }

    private class WAConnIterator implements Iterator<Conn> {
        private int subnetindex;
        private Iterator<Conn> currentiter;

        // start out by iterating over any direct connections to this wired-anded network
        public WAConnIterator ()
        {
            currentiter = connections.iterator ();
        }

        @Override
        public boolean hasNext ()
        {
            // see if anything left on what we are doing
            while (! currentiter.hasNext ()) {
                // if not, stop if no more subnets left to scan
                if (subnetindex >= subnets.length) return false;
                // more subnets, scan that subnet and all its subnets
                currentiter = subnets[subnetindex++].connIterable ().iterator ();
            }
            return true;
        }

        @Override
        public Conn next ()
        {
            hasNext ();
            return currentiter.next ();
        }
    }
}
