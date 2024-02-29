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
public abstract class Placeable {
    public boolean placed;
    public double linelen;
    public double offsetx, offsety;

    public abstract String getName ();
    public abstract void flipIt ();
    public abstract int getHeight ();
    public abstract int getWidth ();
    public abstract double getPosX ();
    public abstract double getPosY ();
    public abstract void setPosXY (double leftx, double topy);
    public abstract Network[] getExtNets ();

    public void parseManual (String[] words, int i)
            throws Exception
    {
        for (; i < words.length; i ++) {
            switch (words[i].toLowerCase ()) {
                case "flip": {
                    flipIt ();
                    break;
                }
                case "offset": {
                    offsetx = Math.round (Double.parseDouble (words[++i]) * 20.0) / 2.0;
                    offsety = Math.round (Double.parseDouble (words[++i]) * 20.0) / 2.0;
                    break;
                }
                default: {
                    throw new Exception ("unknown keyword " + words[i]);
                }
            }
        }
    }
}
