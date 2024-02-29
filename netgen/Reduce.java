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
 * Reduce the flat token stream from tokenization
 * to textured modules
 */

import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.LinkedList;

public class Reduce {

    private HashMap<String,Module> modules;     // all modules reduced from source
    private HashSet<String> instnamesused;      // instantiation names used in a module
    private String instvarsuf;                  // current module variable name suffix
    private UserModule module;                  // current module being reduced

    // reduce the given source tokens into module definitions
    public HashMap<String,Module> reduce (Token tokens)
    {
        modules = new HashMap<> ();

        Module blmod = new BufLEDModule ("", 1, false);
        modules.put (blmod.name, blmod);

        Module connmod = new ConnModule ("", null);
        modules.put (connmod.name, connmod);

        Module daomod = new DAOModule ("");
        modules.put (daomod.name, daomod);

        Module dffmod = new DFFModule ("", null);
        modules.put (dffmod.name, dffmod);

        Module dlatmod = new DLatModule ("", null);
        modules.put (dlatmod.name, dlatmod);

        Module lblmod = new LabelModule ("", null);
        modules.put (lblmod.name, lblmod);

        Module pulsemod = new PulseModule ("");
        modules.put (pulsemod.name, pulsemod);

        Module pwrmod = new PwrConnModule ("", new String[] { "GND" });
        modules.put (pwrmod.name, pwrmod);

        Module rasmod = new RasPiModule ("");
        modules.put (rasmod.name, rasmod);

        try {
            while (!(tokens instanceof Token.End)) {
                tokens = parseModule (tokens);
            }
        } catch (ReduceEx ce) {
            System.err.println (ce.token.getLoc () + ": " + ce.message);
            return null;
        }

        return modules;
    }

    // module modname (paramtype paramname[hidim:lodim], ...)
    // {
    //   statements ...
    // }
    private Token parseModule (Token tok)
            throws ReduceEx
    {
        // get module name
        if (!tok.isDel (Token.DEL.MODULE)) {
            throw new ReduceEx (tok, "expecting 'module'");
        }
        tok = tok.next;
        Token tokmodname = tok;
        String modname = tok.getSym ();
        if (modname == null) throw new ReduceEx (tok, "expecting module name");
        if (modules.get (modname) != null) throw new ReduceEx (tok, "duplicate module name " + modname);
        tok = tok.next;
        if (!tok.isDel (Token.DEL.OPRN)) throw new ReduceEx (tok, "expecting ( after module name");
        tok = tok.next;

        module = new UserModule ();
        module.name = modname;
        modules.put (modname, module);

        // make sure they don't use same instantiation name more than once in a module
        instnamesused = new HashSet<> ();

        // no module being instantiated so variable names are used as is
        instvarsuf = "";

        // get parameters
        LinkedList<OpndLhs> paramlist = new LinkedList<> ();
        if (! tok.isDel (Token.DEL.CPRN)) {
            while (true) {
                tok = parseModuleParam (paramlist, tok);
                if (tok.isDel (Token.DEL.CPRN)) break;
                if (! tok.isDel (Token.DEL.COMMA)) throw new ReduceEx (tok, "expecting , or ) after module parameter");
                tok = tok.next;
            }
        }
        module.params = paramlist.toArray (new OpndLhs[0]);

        // skip over )
        tok = tok.next;

        // fill in module body
        module.tokstart = tok;
        tok = parseModuleBody (tok);

        // make sure all wires and output pins have writers
        // skip checking input pins and open-collector wires/outputs
        for (OpndLhs variable : module.variables.values ()) {
            String err = variable.checkWriters ();
            if (err != null) {
                throw new ReduceEx (tokmodname, "module " + module.name + " output/wire " + variable.name + " " + err);
            }
        }

        return tok;
    }

    // parse module parameter declaration
    //  input:
    //   tok = points to IN, OUT keyword
    //  output:
    //   returns token pointing to , or )
    //   add parameter to paramlist
    private Token parseModuleParam (LinkedList<OpndLhs> paramlist, Token tok)
            throws ReduceEx
    {
        // get parameter type, IN, OUT
        Token.DEL kw = tok.getDel ();
        if ((kw != Token.DEL.IN) && (kw != Token.DEL.OUT)) {
            throw new ReduceEx (tok, "expecting ), in, out for module parameter");
        }
        tok = tok.next;

        // get parameter name and add to list of parameters and variables for the module
        Token tokvar = tok;
        DimmedVar dv = new DimmedVar ();
        tok = parseDimmedVar (tok, dv);

        // if no dimension given, use [0:0]
        if (dv.hidim < dv.lodim) {
            dv.lodim = 0;
            dv.hidim = 0;
        }

        switch (kw) {
            case IN: {
                IParam ip = new IParam (dv.vnamestr, paramlist.size ());
                ip.lodim  = dv.lodim;
                ip.hidim  = dv.hidim;
                assert ip.hidim >= ip.lodim;
                if (module.variables.get (ip.name) != null) throw new ReduceEx (tokvar, "duplicate variable name " + ip.name);
                module.variables.put (ip.name, ip);
                paramlist.add (ip);
                break;
            }
            case OUT: {
                OParam op = new OParam (dv.vnamestr, paramlist.size ());
                op.lodim  = dv.lodim;
                op.hidim  = dv.hidim;
                assert op.hidim >= op.lodim;
                if (module.variables.get (op.name) != null) throw new ReduceEx (tokvar, "duplicate variable name " + op.name);
                module.variables.put (op.name, op);
                paramlist.add (op);
                break;
            }
        }
        return tok;
    }

    // fill in module body
    //  input:
    //   tok = points to '{' at beginning of module
    //  output:
    //   returns just past the '}' at end of module
    //   module contents filled in
    private Token parseModuleBody (Token tok)
            throws ReduceEx
    {
        if (! tok.isDel (Token.DEL.OBRC)) throw new ReduceEx (tok, "expecting { at beginning of module body");
        tok = tok.next;
        tok = parseStatementBlock (tok);
        return tok.next;
    }

    // parse a block of statements
    //  input:
    //   tok = points just past the opening {
    //  output:
    //   tok = points at the closing }
    private Token parseStatementBlock (Token tok)
            throws ReduceEx
    {
        while (! tok.isDel (Token.DEL.CBRC)) {
            tok = parseModuleStatement (tok);
        }
        return tok;
    }

    // parse statement in the module
    //  input:
    //   tok = point to first token in statement
    //  output:
    //   returns tok pointing to next statement or }
    //   module updated
    private Token parseModuleStatement (Token tok)
            throws ReduceEx
    {
        // skip null statements
        if (tok.isDel (Token.DEL.SEMI)) return tok.next;

        // wire name[hidim:lodim], ...;
        if (tok.isDel (Token.DEL.WIRE)) {
            tok = tok.next;

            while (true) {

                // get wire name and dimensions
                Token tokvar = tok;
                DimmedVar dv = new DimmedVar ();
                tok = parseDimmedVar (tok, dv);

                // if no dimensions given, use [0:0]
                if (dv.hidim < dv.lodim) {
                    dv.lodim = 0;
                    dv.hidim = 0;
                }

                // fill in wire
                OpndLhs wire = new OpndLhs ();
                wire.name    = dv.vnamestr + instvarsuf;
                wire.lodim   = dv.lodim;
                wire.hidim   = dv.hidim;

                assert wire.hidim >= wire.lodim;
                if (module.variables.get (wire.name) != null) throw new ReduceEx (tokvar, "duplicate wire name " + wire.name);
                module.variables.put (wire.name, wire);

                // maybe another wire declaration in same statement
                if (tok.isDel (Token.DEL.SEMI)) break;
                if (! tok.isDel (Token.DEL.COMMA)) throw new ReduceEx (tok, "expecting , or ; after wire declaration");
                tok = tok.next;
            }
            tok = tok.next;
            return tok;
        }

        // interconn connector,...;
        if (tok.isDel (Token.DEL.INTERCONN)) {
            tok = tok.next;

            HashMap<String,LinkedList<OpndLhs>> connpinss = new HashMap<> ();

            while (true) {

                // get connector instantiation name
                Token tokvar = tok;
                DimmedVar dv = new DimmedVar ();
                tok = parseDimmedVar (tok, dv);
                if (dv.hidim >= dv.lodim) {
                    throw new ReduceEx (dv.vnametok, "no dimensions allowed");
                }

                // find all variables ending with the instantion name
                // they are named {IO}pinname/conninstname
                // save them in hashmap indexed by pinname
                String slashname = "/" + dv.vnamestr + instvarsuf;
                boolean foundpin = false;
                for (String varname : module.variables.keySet ()) {
                    if (varname.endsWith (slashname)) {
                        if (! varname.startsWith ("I") && ! varname.startsWith ("O")) {
                            throw new ReduceEx (dv.vnametok, "not a connector cuz " + varname + " not a pin name");
                        }
                        String pinname = varname.substring (1, varname.length () - slashname.length ());
                        if (pinname.indexOf ('/') >= 0) {
                            throw new ReduceEx (dv.vnametok, "not a connector cuz " + varname + " not a pin name");
                        }
                        LinkedList<OpndLhs> connpins = connpinss.get (pinname);
                        if (connpins == null) {
                            connpins = new LinkedList <> ();
                            connpinss.put (pinname, connpins);
                        }
                        connpins.add (module.variables.get (varname));
                        foundpin = true;
                    }
                }
                if (! foundpin) {
                    for (String v : module.variables.keySet ()) {
                        System.out.println (v + " = " + module.variables.get (v).getClass ().getName ());
                    }
                    throw new ReduceEx (dv.vnametok, "no pins found for connector " + slashname.substring (1));
                }

                // maybe another connector
                if (tok.isDel (Token.DEL.SEMI)) break;
                if (! tok.isDel (Token.DEL.COMMA)) throw new ReduceEx (tok, "expecting , or ; after connector");
                tok = tok.next;
            }

            // install jumper wires between the corresponding pins
            for (String pinname : connpinss.keySet ()) {

                // see which connectors drive and which sample the signal on this wire
                LinkedList<OpndLhs> connpins = connpinss.get (pinname);
                LinkedList<ConnModule.IPinOutput> ipins = new LinkedList<> ();
                LinkedList<OpndRhs> opinlist = new LinkedList<> ();
                for (OpndLhs connpin : connpins) {
                    if (connpin instanceof ConnModule.IPinOutput) {
                        ipins.add ((ConnModule.IPinOutput) connpin);
                    } else {
                        opinlist.add (connpin);
                    }
                }

                // tell all 'i' pins what 'o' pin is driving the wire
                // if more than one 'o' pin, it is wire-and semantics
                // if no 'o' pin, it is floating 1
                OpndRhs[] opinarray = opinlist.toArray (new OpndRhs[0]);
                for (ConnModule.IPinOutput ipin : ipins) {
                    ipin.jumpers = opinarray;
                }
            }

            tok = tok.next;
            return tok;
        }

        // jumper inputpin = outputpin;
        if (tok.isDel (Token.DEL.JUMPER)) {
            tok = tok.next;

            // get input pin name
            Token tokvarin = tok;
            DimmedVar dvin = new DimmedVar ();
            tok = parseDimmedVar (tok, dvin);
            if (dvin.hidim >= dvin.lodim) {
                throw new ReduceEx (dvin.vnametok, "no dimensions allowed");
            }
            if (! tok.isDel (Token.DEL.EQUAL)) {
                throw new ReduceEx (tok, "expecting = after input pin name");
            }
            tok = tok.next;

            OpndLhs varin = module.variables.get (dvin.vnamestr + instvarsuf);
            if (varin == null) {
                throw new ReduceEx (dvin.vnametok, "undefined input pin " + dvin.vnamestr);
            }
            if (!(varin instanceof ConnModule.IPinOutput)) {
                throw new ReduceEx (dvin.vnametok, "pin " + dvin.vnamestr + " is not an input pin");
            }

            // get output pin or other expression
            ExpressionParser ep = new ExpressionParser (tok, "jumper_" + varin.name);
            tok = ep.tok;
            if (! tok.isDel (Token.DEL.SEMI)) {
                throw new ReduceEx (tok, "expecting ; after output pin name");
            }
            tok = tok.next;

            // jumper it
            ((ConnModule.IPinOutput) varin).jumpers = new OpndRhs[] { ep.result };
            return tok;
        }

        // maybe instantiate another module
        //  instancename : modulename (argument, ...) ;
        // treats the module being instantiated as a macro, reparsing it here
        String sym = tok.getSym ();
        if ((sym != null) && tok.next.isDel (Token.DEL.COLON)) {
            Token toksym = tok;
            tok = tok.next.next;
            String instmodname = tok.getSym ();
            if (instmodname == null) throw new ReduceEx (tok, "expecting module name after " + sym + " :");
            Module origmodule = modules.get (instmodname);
            if (origmodule == null) throw new ReduceEx (tok, "module " + instmodname + " not defined");
            if (origmodule == module) throw new ReduceEx (tok, "cannot recursively instantiate module " + instmodname);
            tok = tok.next;

            // when we copy origmodule into this module, rename all variables in origmodule
            // (including parameters) to have this suffix
            String newivs = "/" + sym + instvarsuf;

            // don't allow same instantiation name more than once
            // ...or the internal variables will get stomped on when instantiating the module
            if (instnamesused.contains (newivs)) {
                throw new ReduceEx (toksym, "cannot use same instantiation name more than once " + sym);
            }
            instnamesused.add (newivs);

            // maybe it has tokens after the name before the (
            Object[] instconfig_r = new Object[1];
            try {
                tok = origmodule.getInstConfig (tok, newivs, instconfig_r);
            } catch (Exception e) {
                throw new ReduceEx (tok, e.getMessage ());
            }
            Object instconfig = instconfig_r[0];

            // should be a ( next
            if (! tok.isDel (Token.DEL.OPRN)) throw new ReduceEx (tok, "expecting argument opening ( after module name " + instmodname);

            // set up a list of wires mirroring the inner module's parameters
            // the wires have the same names as the parameters except with "/instname" appended
            OpndLhs[] paramwires;
            try {
                paramwires = origmodule.getInstParams (module, newivs, instconfig);
            } catch (Exception e) {
                throw new ReduceEx (tok, e.getMessage ());
            }

            // re-parse the module's source code as if it were part of this module's source code,
            // appending "/instname" to all it's internal variables
            // skip this step for builtins cuz they did their magic in getInstPsrams()
            if (origmodule instanceof UserModule) {
                String oldivs = instvarsuf;
                instvarsuf = newivs;
                try {
                    parseModuleBody (((UserModule) origmodule).tokstart);
                } finally {
                    instvarsuf = oldivs;
                }
            }

            // skip past the '('
            tok = tok.next;

            // get all the arguments passed to the module
            // hook them up to the paramwires
            int argn = 0;
            if (! tok.isDel (Token.DEL.CPRN)) {
                while (true) {

                    // we might have name :, explicitly saying which argument they are setting
                    String prmname = tok.getSym ();
                    if ((prmname != null) && tok.next.isDel (Token.DEL.COLON)) {
                        int i = paramwires.length;
                        while (-- i >= 0) {
                            if (paramwires[i].name.equals (prmname + newivs)) break;
                        }
                        if (i < 0) throw new ReduceEx (tok, "unknown parameter " + prmname + " for module " + origmodule.name);
                        argn = i;
                        tok = tok.next.next;
                    }

                    if (argn >= paramwires.length) {
                        throw new ReduceEx (tok, "too many arguments to module " + origmodule.name);
                    }

                    if (origmodule.isInstParamIn (instconfig, argn)) {

                        // IN can get a general expression
                        // the result of the expression gets written to the paramwires[argn] wire
                        // where inputs to the module's copied gates can get it
                        OpndLhs argvar = paramwires[argn];
                        ExpressionParser ep = new ExpressionParser (tok, argvar.name + "[" + argvar.lodim + ":" + argvar.lodim + "]");
                        WField wfield  = new WField ();
                        wfield.refdvar = argvar;
                        wfield.lodim   = argvar.lodim;
                        wfield.hidim   = argvar.hidim;
                        wfield.operand = ep.result;

                        // add instantiation to list that writes the wire
                        // puque if conflict
                        String err = wfield.refdvar.addWriter (wfield);
                        if (err != null) throw new ReduceEx (tok, err);

                        tok = ep.tok;
                    } else {

                        // OUT can take var[hidim:lodim]
                        // the instantiated module writes to the paramwires[argn] wire
                        // so we can read from that wire and write to the given variable
                        if (! tok.isDel (Token.DEL.COMMA) && ! tok.isDel (Token.DEL.CPRN)) {
                            DimmedVar dv = new DimmedVar ();
                            tok = parseDimmedVar (tok, dv);
                            WField wfield = dv.lookuplhs (module, instvarsuf);
                            wfield.operand = paramwires[argn];

                            // add instantiation to list that writes the wire or output
                            // puque if conflict
                            String err = wfield.refdvar.addWriter (wfield);
                            if (err != null) throw new ReduceEx (dv.vnametok, err);
                        }
                    }

                    argn ++;

                    if (tok.isDel (Token.DEL.CPRN)) break;
                    if (! tok.isDel (Token.DEL.COMMA)) throw new ReduceEx (tok, "expecting , or ) after module argument");
                    tok = tok.next;
                }
            }
            /* TODO MESSED UP - there is no arguments[] and it didn't say whether the arg was set or not
            for (int i = 0; i < arguments.length; i ++) {
                OpndLhs param = origmodule.params[i];
                if ((arguments[i] == null) && (param instanceof IParam)) {
                    throw new ReduceEx (tok, "module " + origmodule.name + " requires input parameter [" + i + "] " + param.name);
                }
            }
            */
            tok = tok.next;
            if (! tok.isDel (Token.DEL.SEMI)) throw new ReduceEx (tok, "expecting ; after module argument list closing )");

            return tok.next;
        }

        // maybe an assignment statement
        //  variablename = expression;
        if (sym != null) {

            // parse variablename with optional [hidim:lodim]
            // missing [hidim:lodim] means writing the whole variable
            DimmedVar dv = new DimmedVar ();
            tok = parseDimmedVar (tok, dv);
            WField wfield = dv.lookuplhs (module, instvarsuf);

            // parse associated expression
            Token tokeq = tok;
            if (! tok.isDel (Token.DEL.EQUAL)) throw new ReduceEx (tok, "expecting = for assignment statement");
            tok = tok.next;
            ExpressionParser ep = new ExpressionParser (tok, wfield.refdvar.name + "[" + wfield.hidim + ":" + wfield.lodim + "]");
            wfield.operand = ep.result;

            // add expression to list that writes the wire or output
            // puque if conflict
            String err = wfield.refdvar.addWriter (wfield);
            if (err != null) throw new ReduceEx (tokeq, err);

            tok = ep.tok;
            if (! tok.isDel (Token.DEL.SEMI)) throw new ReduceEx (tok, "expecting ; at end of assignment statement");
            return tok.next;
        }

        // doesn't fit any of those forms
        throw new ReduceEx (tok, "unknown statement");
    }

    // parse expression and return result
    //  input:
    //   tok = point to first token of expression
    //   targname = target of the expression (used to name gates)
    //  output:
    //   returns terminating token, eg, ; or ,
    //   and returns resulting as an operand
    private class ExpressionParser {
        public OpndRhs result;
        public Token tok;

        private int gateno;
        private String targname;

        public ExpressionParser (Token token, String targname)
                throws ReduceEx 
        {
            String lighted = null;
            boolean opencoll = false;
            if (token.isDel (Token.DEL.LED)) {
                if (! token.next.isDel (Token.DEL.COLON) || ((lighted = token.next.next.getSym ()) == null)) {
                    throw new ReduceEx (token, "led not followed by :label");
                }
                token = token.next.next.next;
            }
            if (token.isDel (Token.DEL.OC)) {
                opencoll = true;
                token = token.next;
            }
            if ((lighted != null) && opencoll) {
                throw new ReduceEx (token, "cannot be both lighted and open-collector");
            }
            tok = token;
            this.targname = targname;
            result = parseExpression ();
            if ((lighted != null) || opencoll) {
                if (! (result instanceof Gate)) {
                    throw new ReduceEx (token, "'led' or 'oc' without any transistors");
                }
                ((Gate) result).setLighted  (lighted);
                if (opencoll) ((Gate) result).setOpenColl ();
            }
        }

        // parse an expression starting at tok
        // return the resultant value as a single operand
        private OpndRhs parseExpression ()
                throws ReduceEx 
        {
            OpndRhs opnd = parseOrOperand ();
            if (! tok.isDel (Token.DEL.OR)) return opnd;
            LinkedList<OpndRhs> orterms = new LinkedList<> ();
            orterms.add (opnd);
            while (true) {
                tok = tok.next;
                opnd = parseOrOperand ();
                orterms.add (opnd);
                if (! tok.isDel (Token.DEL.OR)) break;
            }
            return Gate.create (targname + ++ gateno, Gate.TYPE.OR, orterms.toArray (new OpndRhs[0]));
        }

        // parse an expression that is an operand to an | operator
        private OpndRhs parseOrOperand ()
                throws ReduceEx
        {
            OpndRhs opnd = parseAndOperand ();
            if (! tok.isDel (Token.DEL.AND)) return opnd;
            LinkedList<OpndRhs> andterms = new LinkedList<> ();
            andterms.add (opnd);
            while (true) {
                tok = tok.next;
                opnd = parseAndOperand ();
                andterms.add (opnd);
                if (! tok.isDel (Token.DEL.AND)) break;
            }
            return Gate.create (targname + ++ gateno, Gate.TYPE.AND, andterms.toArray (new OpndRhs[0]));
        }

        // parse an expression that is an operand to an & operator
        private OpndRhs parseAndOperand ()
                throws ReduceEx
        {
            // try a ~ expr
            if (tok.isDel (Token.DEL.NOT)) {
                tok = tok.next;
                OpndRhs opnd = parseAndOperand ();
                return Gate.create (targname + ++ gateno, Gate.TYPE.NOT, new OpndRhs[] { opnd });
            }

            // try a ( expr )
            if (tok.isDel (Token.DEL.OPRN)) {
                tok = tok.next;
                OpndRhs opnd = parseExpression ();
                if (! tok.isDel (Token.DEL.CPRN)) throw new ReduceEx (tok, "expecting ) at end of sub-expression not " + tok.toString ());
                tok = tok.next;
                return opnd;
            }

            // try an integer constant
            if (tok instanceof Token.Int) {
                Const cons = new Const ();
                cons.name  = tok.getLoc ();
                cons.token = (Token.Int) tok;
                tok = tok.next;
                return cons;
            }

            // must be a variable
            if (!(tok instanceof Token.Sym)) throw new ReduceEx (tok, "unknown expression operand");

            Token tokvar = tok;
            DimmedVar dv = new DimmedVar ();
            tok = parseDimmedVar (tok, dv);
            return dv.lookuprhs (module, instvarsuf);
        }
    }

    // parse dimensioned variable tokens
    //  input:
    //   tok = points to symbol name token
    //   dv  = newly allocated DimmedVar to fill in
    //  output:
    //   returns token following parsed tokens
    //   dv filled in
    private Token parseDimmedVar (Token tok, DimmedVar dv)
            throws ReduceEx
    {
        dv.vnametok = (Token.Sym) tok;
        dv.vnamestr = tok.getSym ();
        tok = tok.next;

        // look for /instance/...
        while (tok.isDel (Token.DEL.SLASH)) {
            tok = tok.next;
            String ins = tok.getSym ();
            if (ins == null) {
                throw new ReduceEx (tok, "expecting instance name following /");
            }
            dv.vnamestr += "/" + ins;
            tok = tok.next;
        }
        int i = dv.vnamestr.indexOf ('/');
        if (i >= 0) {
            String ins = dv.vnamestr.substring (i) + instvarsuf;
            if (! instnamesused.contains (ins)) {
                throw new ReduceEx (dv.vnametok.next, "unknown module instance " + ins.substring (1));
            }
        }

        // look for [hidim:lodim]
        dv.lodim =  0;
        dv.hidim = -1;
        if (tok.isDel (Token.DEL.OBRK)) {
            tok = tok.next;
            if (!(tok instanceof Token.Int)) throw new ReduceEx (tok, "expecting integer hi dimension");
            dv.lodim = dv.hidim = (int) ((Token.Int) tok).value;
            tok = tok.next;
            if (tok.isDel (Token.DEL.COLON)) {
                tok = tok.next;
                if (!(tok instanceof Token.Int)) throw new ReduceEx (tok, "expecting integer lo dimension");
                dv.lodim = (int) ((Token.Int) tok).value;
                tok = tok.next;
                if (dv.hidim < dv.lodim) throw new ReduceEx (tok, "hidim " + dv.hidim + " lt lodim " + dv.lodim + " for " + dv.vnamestr);
            }
            if (! tok.isDel (Token.DEL.CBRK)) throw new ReduceEx (tok, "expecting ] after [hidim:lodim]");
            tok = tok.next;
        }

        return tok;
    }

    private static class DimmedVar {
        public Token.Sym vnametok;
        public String vnamestr;
        public int lodim;   // if unspecified, lodim=0, hidim=-1
        public int hidim;

        // look up the variable in the given module in the context of writing to it
        public WField lookuplhs (Module mod, String instvarsuf)
                throws ReduceEx
        {
            String varname = vnamestr;
            OpndLhs variable = mod.variables.get (varname + instvarsuf);
            if (variable == null) throw new ReduceEx (vnametok, "undeclared variable referenced " + varname);

            if ((hidim >= lodim) && ((lodim < variable.lodim) || (hidim > variable.hidim))) {
                throw new ReduceEx (vnametok, "specified dims [" + hidim + ":" + lodim + "] exceed variable " + variable.name + " dims [" + variable.hidim + ":" + variable.lodim + "]");
            }

            // if source code didn't specify any dimesions, use the whole variable
            // eg,
            //   wire x[15:0];
            //   x = something;
            //   ^ we are here, write all 16 bits
            if (hidim < lodim) {
                lodim = variable.lodim;
                hidim = variable.hidim;
            }
            assert (hidim >= lodim);

            WField wfield  = new WField ();
            wfield.refdvar = variable;
            wfield.lodim   = lodim;
            wfield.hidim   = hidim;
            return wfield;
        }

        // look up the variable in the given module in the context of reading from it
        public OpndRhs lookuprhs (Module mod, String instvarsuf)
                throws ReduceEx
        {
            OpndLhs variable = mod.variables.get (vnamestr + instvarsuf);
            if (variable == null) throw new ReduceEx (vnametok, "undeclared variable referenced " + vnamestr);

            // if no dimensions specified or dimensions specify the whole variable, just use the raw variable
            if ((hidim < lodim) || ((lodim == variable.lodim) && (hidim == variable.hidim))) {
                return variable;
            }

            if ((lodim < variable.lodim) || (hidim > variable.hidim)) {
                throw new ReduceEx (vnametok, "specified dims [" + hidim + ":" + lodim + "] exceed variable " + variable.name + " dims [" + variable.hidim + ":" + variable.lodim + "]");
            }

            // sub-field of the variable being read
            RField rfield   = new RField ();
            rfield.name     = vnametok.next.getLoc ();
            rfield.refdopnd = variable;
            rfield.lodim    = lodim;
            rfield.hidim    = hidim;
            return rfield;
        }
    }
}
