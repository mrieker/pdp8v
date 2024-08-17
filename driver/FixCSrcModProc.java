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

// convert csrcmod_proc.cc generated by netgen to new format
// new format adds if statements that depend on enabled boards (boardena)

import java.io.BufferedReader;
import java.io.InputStreamReader;

public class FixCSrcModProc
{
    public static void main (String[] args)
            throws Exception
    {
        String[][] paddles = new String[4][32];
        int readingpaddle = -1;
        int writingpaddle = -1;
        String lastmodname = null;

        BufferedReader stdin = new BufferedReader (new InputStreamReader (System.in));
        for (String line; (line = stdin.readLine ()) != null;) {

            // check various statements that need an 'if (boardena & BE_<modname>)' prefix
            String modname = null;
            if (line.startsWith ("    nto += ")) {
                modname = getModNameFromAssign (line);
            }
            if (line.startsWith ("    DFFStep (")) {
                modname = getModNameFromDFFStep (line);
            }
            if (line.startsWith ("    DLatStep (")) {
                modname = getModNameFromDLatStep (line);
            }
            if (line.startsWith ("    out_DAO_")) {
                modname = getModNameFromDAO (line);
            }
            if (line.endsWith ("_step;")) {
                modname = getModNameFromStep (line);
            }

            // check for 'uint32_t CSrcMod_proc::read<paddle>conwork ()' that begins paddle reading method
            if (line.startsWith ("uint32_t CSrcMod_proc::read") && line.endsWith ("conwork ()")) {
                readingpaddle = line.charAt (27) - 'a';
            }

            // check for 'void CSrcMod_proc::wrote<paddle>conwork (uint32_t valu)' that begins paddle writing method
            // insert mask argument
            if (line.startsWith ("void CSrcMod_proc::write") && line.endsWith ("conwork (uint32_t valu)")) {
                writingpaddle = line.charAt (24) - 'a';
                line = line.replace ("(", "(uint32_t mask, ");
            }

            // modify prototype for 'void CSrcMod_proc::wrote<paddle>conwork (uint32_t valu)'
            if (line.startsWith ("    virtual void write") && line.endsWith ("conwork (uint32_t valu);")) {
                line = line.replace ("(", "(uint32_t mask, ");
            }

            // if statement in paddle reading method, get symbol for the pin and which bit number it is for
            // & (Q_f_0_DFF_pc08_pc ? ~ 0 : ~ (1U<<27))
            if (readingpaddle >= 0) {
                int i = line.indexOf ("& (");
                if (i >= 0) {
                    int j = line.indexOf (" ? ", i);
                    int k = line.indexOf ("1U<<", j);
                    int l = line.indexOf ("))", k);
                    String symname = line.substring (i + 3, j);
                    int bitno = Integer.parseInt (line.substring (k + 4, l));
                    paddles[readingpaddle][bitno] = symname;
                }
            }

            // if end of paddle writing method, output statements that write the paddle bits
            if (line.equals ("}")) {
                readingpaddle = -1;
                if (writingpaddle >= 0) {
                    for (int bitno = 0; bitno < 32; bitno ++) {
                        String symname = paddles[writingpaddle][bitno];
                        if (symname != null) {
                            System.out.println ("    if ((mask >> " + bitno + ") & 1) " +
                                    symname + " = (valu >> " + bitno + ") & 1;");
                        }
                    }
                    writingpaddle = -1;
                }
            }

            // output statement possibly subject to 'if (boardena & BE_<modname>)'
            if ((lastmodname != null) && ! lastmodname.equals (modname)) {
                System.out.println ("  }");
                lastmodname = null;
            }
            if ((modname != null) && ! modname.equals (lastmodname)) {
                System.out.println ("  if (boardena & BE_" + modname + ") {");
                lastmodname = modname;
            }
            System.out.println (line);
        }
    }

    // nto += Q_0__allones_pc_2_2_2 = !((Q_f_0_DFF_pc00_pc & Q_f_0_DFF_pc01_pc));
    //                     ^^
    public static String getModNameFromAssign (String line)
    {
        int i = line.indexOf (" = ");
        String[] parts = line.substring (11, i).split ("_");
        return parts[parts.length-4];
    }

    // DLatStep (__9_MQ, Q_e_0_DFF_fetch2_seq, Q_f_0_DFF_intak1_seq, 1, &Q_c_0_DLat_ireg09_seq, ...
    //                                                                                     ^^^
    public static String getModNameFromDLatStep (String line)
    {
        int i = line.indexOf ("&Q_c_");
        int j = line.indexOf (",", i);
        String[] parts = line.substring (i, j).split ("_");
        return parts[parts.length-1];
    }

    // out_DAO_acqzda2_acl = Q_e_0_DFF_aclo_acl & Q_e_1_DFF_aclo_acl ...
    //                 ^^^
    public static String getModNameFromDAO (String line)
    {
        int i = line.indexOf (" = ");
        int j = line.lastIndexOf ("_", i);
        return line.substring (j + 1, i);
    }

    // DFFStep (Q_0_rotq_acl_8_8_8 & Q_0__ac_sca_acl_0_0_1, Q_0_actc_acl_0_0_2, 1, 1, Q_e_0_DFF_achi_acl, Q_f_0_DFF_...
    //                                                                                               ^^^
    public static String getModNameFromDFFStep (String line)
    {
        int i = line.indexOf (", Q_e_") + 2;
        int j = line.indexOf (",", i);
        String[] parts = line.substring (i, j).split ("_");
        return parts[parts.length-1];
    }

    // Q_e_0_DFF_achi_acl = Q_e_0_DFF_achi_acl_step;
    //                ^^^
    public static String getModNameFromStep (String line)
    {
        int i = line.indexOf (" = ");
        int j = line.lastIndexOf ("_", i);
        return line.substring (j + 1, i);
    }
}
