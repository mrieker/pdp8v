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

import java.util.LinkedList;

public class PropPrinter {
    public PropNode currnode;

    public void inclevel ()
    {
        currnode = currnode.childs.getLast ();
    }

    public void declevel ()
    {
        currnode = currnode.parent;
    }

    public void println (String type, OpndRhs opnd, int rbit)
    {
        PropNode node = new PropNode ();
        node.parent = currnode;
        node.level  = currnode.level + 1;
        node.type   = type;
        node.opnd   = opnd;
        node.rbit   = rbit;

        if (currnode.childs == null) {
            currnode.childs = new LinkedList<> ();
        }
        currnode.childs.add (node);
    }
}
