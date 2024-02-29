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
 * Built-in label module.
 */

import java.io.PrintStream;

public class LabelModule extends Module {
    private Config config;
    private QOutput qoutput;
    private String instvarsuf;

    private Comp.Label complabel;

    public LabelModule (String instvarsuf, Config config)
    {
        name = "Label" + instvarsuf;
        this.config     = config;
        this.qoutput    = new QOutput ("@@@", 0);
        this.instvarsuf = instvarsuf;

        params = new OpndLhs[] { };
    }

    // this gets the width token after the module name and before the (
    // eg, inst : Label "string" intsize ();
    //                   ^ token is here
    //                            ^ return token here
    @Override
    public Token getInstConfig (Token token, String instvarsuf, Object[] instconfig_r)
            throws Exception
    {
        Config config = new Config ();
        config.size = 1;
        while (! "(".equals (token.getDel ())) {
            if (token instanceof Token.Int) {
                config.size = (int) ((Token.Int) token).value;
                token = token.next;
                continue;
            }
            if (token instanceof Token.Str) {
                config.tokstr = (Token.Str) token;
                token = token.next;
                continue;
            }
            break;
        }
        instconfig_r[0] = config;
        return token;
    }

    private static class Config {
        public int size;
        public Token.Str tokstr;
    }

    // make a new Label module instance
    @Override
    public OpndLhs[] getInstParams (UserModule target, String instvarsuf, Object instconfig)
            throws Exception
    {
        Config config = (Config) instconfig;
        LabelModule lminst = new LabelModule (instvarsuf, config);

        // make a dummy component that gets installed on circuit board
        target.generators.add (lminst.qoutput);

        // no parameters to fill in
        return new OpndLhs[0];
    }

    // the internal circuitry that writes to the output wires

    private class QOutput extends OParam {
        public QOutput (String name, int index)
        {
            super (name, index);
        }

        @Override
        public String checkWriters ()
        {
            return null;
        }

        @Override
        public Network generate (GenCtx genctx, int rbit)
        {
            if (complabel == null) {
                assert instvarsuf != null;
                assert config.tokstr != null;
                assert config.tokstr.value != null;
                complabel = new Comp.Label (genctx, null, instvarsuf, config.tokstr.value, config.size);
                genctx.addPlaceable (complabel);
            }
            // wire-and '1' with whatever
            return genctx.nets.get ("VCC");
        }

        @Override
        public void printProp (PropPrinter pp, int rbit)
        { }

        @Override
        protected int busWidthWork ()
        {
            return 1;
        }

        @Override
        protected void stepSimWork (int t)
                throws HaltedException
        {
            // wire-and '1' with whatever
            setSimOutput (t, 0x100000000L);
        }
    }
}
