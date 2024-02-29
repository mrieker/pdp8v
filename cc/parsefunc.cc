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
///////////////////////////////////////////////////////////////////
//                                                               //
//  Parse source tokens of a function into abstract syntax tree  //
//  Generate internal representation (prims)                     //
//  Optimize primitives                                          //
//  Generate rough machine code                                  //
//  Optimize machine code                                        //
//  Write assembly language to output file                       //
//                                                               //
///////////////////////////////////////////////////////////////////

#include <assert.h>

#include "decl.h"
#include "error.h"
#include "token.h"

static std::map<char const *,int,cmp_str> lastprinteds;

// fill in this->funcbody from upcoming tokens
// already have this->memstrtype, this->params and this->paramscope filled in
void FuncDecl::parseFuncBody ()
{
    assert (this->params     != nullptr);
    assert (this->paramscope != nullptr);
    assert (this->retaddr    == nullptr);

    // variable to save return address in
    // EntryPrim::assemprim() generates code that sets it
    this->retaddr = new VarDecl ("__retaddr", paramscope, getDefTok (), voidtype->getPtrType (), KW_NONE);

    // label to jump to for a return statement
    this->retlabel = new LabelStmt (paramscope, getDefTok ());
    this->retlabel->name = "__return";

    // variable that holds return value
    // if return-by-reference,
    //   __retvalue is set to point to where return value goes in the function prolog code
    // else,
    //   __retvalue is returned in R0/R1 by the epilog code
    Type *rettype = this->getFuncType ()->getRetType ();
    if (rettype != voidtype) {
        if (this->getFuncType ()->isRetByRef (getDefTok ())) {
            VarDecl *retvalvar = new VarDecl ("__retvalue", paramscope, getDefTok (), rettype->getPtrType (), KW_NONE);
            this->retvalue = new PtrDecl (paramscope, getDefTok (), retvalvar);
        } else {
            this->retvalue = new VarDecl ("__retvalue", paramscope, getDefTok (), rettype, KW_NONE);
        }
    }

    // variable that gets set to 'this' pointer on entry
    // EntryPrim::assemprim() generates code that sets it
    if ((this->getEncType () != nullptr) && (this->getStorClass () != KW_STATIC)) {
        this->thisptr = new VarDecl ("this", paramscope, getDefTok (), this->getEncType ()->getPtrType ()->getConstType (), KW_NONE);
    }

    // parse through statements in the function body
    // note that the next token in the stream is a '{' so we get a BlockStmt
    this->funcbody = this->parseStatement (this->paramscope, nullptr);

    // resolve forward-reference gotos
    for (GotoStmt *gs : this->gotostmts) {
        LabelStmt *ls = gs->getStmtScope ()->lookuplabel (gs->name);
        if (ls == nullptr) {
            printerror (gs->getStmtToken (), "goto undefined label '%s'", gs->name);
        } else {
            gs->label = ls;
        }
    }

    // generate primitives
    generfunc ();

    // optimize prims
    optprims ();

    // allocate variables on stack
    allocfunc ();

    // generate rough machine code
    this->machfile = new MachFile (this);
    for (Prim *prim = entryprim; prim != nullptr; prim = prim->linext) {
        prim->assemprim (this->machfile);
    }

    // optimize machine code
    machfile->optmachs ();

    // write assembly language to output file
    fprintf (sfile, "; --------------\n");
    fprintf (sfile, "\t.psect\t%s\n", TEXTPSECT);
    fprintf (sfile, "\t.align\t2\n");

    int linelen = 256;
    char *linebuf = (char *) malloc (linelen);
    char const *lastfile = "";
    FILE *lastopen = nullptr;
    int lastline = 0;
    auto lastprinted = lastprinteds.find (lastfile);
    for (Mach *mach = machfile->firstmach; mach != nullptr; mach = mach->nextmach) {

        // see where in source file the machine language instruction came from
        Token *token = mach->token;
        if (token != intdeftok) {

            // maybe different source file than last instruction
            if (strcmp (token->getFile (), lastfile) != 0) {

                // close old file and open new one
                if (lastopen != nullptr) fclose (lastopen);
                lastfile = token->getFile ();
                lastopen = fopen (lastfile, "r");

                // next read from the file would be line 1
                lastline = 0;

                // see if we have previously printed any lines from the file for some previous function
                // make sure we keep track so we don't keep printing same lines over and over
                lastprinted = lastprinteds.find (lastfile);
                if (lastprinted == lastprinteds.end ()) {
                    auto insd = lastprinteds.insert ({ lastfile, 0 });
                    if (insd.second) lastprinted = insd.first;
                }
            }

            // print source lines up to and including the one for the machine language instruction
            if (lastopen != nullptr) {
                while (lastline < token->getLine ()) {

                    // read next source file line
                    ++ lastline;
                    char *p = linebuf;
                    while (true) {
                        if (fgets (p, linebuf + linelen - p, lastopen) == nullptr) {
                            fprintf (stderr, "eof readline %s line %d\n", lastfile, lastline);
                            assert_false ();
                        }
                        p += strlen (p);
                        if ((p > linebuf) && (p[-1] == '\n')) break;
                        int offs = p - linebuf;
                        linelen += linelen / 2;
                        linebuf  = (char *) realloc (linebuf, linelen);
                        p = linebuf + offs;
                    }

                    // if we haven't printed the line before, print it to output file and remember it was printed
                    if (lastprinted->second < lastline) {
                        fprintf (sfile, "; %s:%d: %s", lastfile, lastline, linebuf);
                        lastprinted->second = lastline;
                    }
                }
            }
        }

        // convert machine language to assembly language and write to file
        mach->dumpmach (sfile);
    }
    free (linebuf);

    // close last source file and flush assembly language file
    if (lastopen != nullptr) fclose (lastopen);
    fflush (sfile);

    // if just output constructor, generate initialization and termination functions
    if (strcmp (getName (), "__ctor") == 0) {
        StructType *structtype = getEncType ();
        if (structtype != nullptr) {
            structtype->genInitFunc ();
            structtype->genTermFunc ();
        }
    }
}

Stmt *FuncDecl::parseStatement (Scope *scope, SwitchStmt *outerswitch)
{
    Token *tok = gettoken ();
    switch (tok->getKw ()) {
        case KW_BREAK: {
            GotoStmt *gotostmt = new GotoStmt (scope, tok);
            if (this->breaklbl == nullptr) {
                printerror (tok, "break statement not allowed here");
            } else {
                gotostmt->name  = this->breaklbl->name;
                gotostmt->label = this->breaklbl;
            }
            swallowsemi ();
            return gotostmt;
        }

        case KW_CASE: {
            if (outerswitch == nullptr) {
                throwerror (tok, "no enclosing switch statement for case");
            }
            CaseStmt *cs = new CaseStmt (scope, tok);
            Expr *loexpr = scope->castToType (tok, outerswitch->valuexpr->getType (), scope->parsexpression (false, true));
            cs->lont = loexpr->getType ()->stripCVMod ()->castNumType ();
            cs->lonc = loexpr->isNumConstExpr (&cs->lonv);
            if ((cs->lonc != NC_SINT) && (cs->lonc != NC_UINT)) {
                throwerror (loexpr->exprtoken, "case expression must be integer constant");
            }
            assert (cs->lont != nullptr);
            tok = gettoken ();
            if (tok->isKw (KW_DOTDOTDOT)) {
                Expr *hiexpr = scope->castToType (tok, outerswitch->valuexpr->getType (), scope->parsexpression (false, true));
                cs->hint = hiexpr->getType ()->stripCVMod ()->castNumType ();
                cs->hinc = hiexpr->isNumConstExpr (&cs->hinv);
                if ((cs->hinc != NC_SINT) && (cs->hinc != NC_UINT)) {
                    throwerror (hiexpr->exprtoken, "case expression must be integer constant");
                }
                assert (cs->hint != nullptr);
                tok = gettoken ();
            }
            if (! tok->isKw (KW_COLON)) {
                throwerror (tok, "expecting '...' or ':' after case value");
            }
            outerswitch->casestmts.push_back (cs);
            return cs;
        }

        case KW_CONTINUE: {
            GotoStmt *gotostmt = new GotoStmt (scope, tok);
            if (this->contlbl == nullptr) {
                printerror (tok, "continue statement not allowed here");
            } else {
                gotostmt->name  = this->contlbl->name;
                gotostmt->label = this->contlbl;
            }
            swallowsemi ();
            return gotostmt;
        }

        case KW_DEFAULT: {
            CaseStmt *cs = new CaseStmt (scope, tok);
            tok = gettoken ();
            if (! tok->isKw (KW_COLON)) {
                throwerror (tok, "expecting ':' after default");
            }
            if (outerswitch == nullptr) {
                printerror (cs->getStmtToken (), "no enclosing switch statement for default");
            } else if (outerswitch->defstmt != nullptr) {
                printerror (cs->getStmtToken (), "multiple defaults for same switch");
            } else {
                outerswitch->defstmt = cs;
            }
            return cs;
        }

        //  delete pointer;
        case KW_DELETE: {

            bool arrstyle = false;
            Token *arrtok = gettoken ();
            if (arrtok->isKw (KW_OBRKT)) {
                arrtok = gettoken ();
                if (! arrtok->isKw (KW_CBRKT)) {
                    printerror (arrtok, "expecting ']' after 'delete['");
                    pushtoken (arrtok);
                }
                arrstyle = true;
            } else {
                pushtoken (arrtok);
            }

            // get pointer expression
            Expr *mempointer = scope->parsexpression (false, false);
            Type *exptype = mempointer->getType ();
            PtrType *ptrtype = exptype->stripCVMod ()->castPtrType ();
            if (ptrtype == nullptr) {
                throwerror (mempointer->exprtoken, "pointer expression required, not '%s'", exptype->getName ());
            }
            Type *basetype = ptrtype->getBaseType ();

            // maybe it's a struct with a destructor function, generate call to destructor
            // if deleting a rootward part of a leaf, __term() is virtual so we terminate the leafmost type first then all the rootward types
            // ...and the leafmost __term() returns pointer to leafmost type
            mempointer = basetype->callTermFunc (scope, tok, mempointer, arrstyle);

            // call free(mempointer)
            CallExpr *callfree = new CallExpr (scope, tok);
            callfree->funcexpr = new ValExpr (scope, tok, freefunc);
            callfree->argexprs.push_back (mempointer);

            ExprStmt *exprstmt = new ExprStmt ();
            exprstmt->expr = callfree;

            swallowsemi ();
            return exprstmt;
        }

        case KW_DO: {
            LabelStmt *oldbreaklbl = this->breaklbl;
            LabelStmt *oldcontlbl  = this->contlbl;

            this->breaklbl = newTempLabelStmt (scope, tok);
            this->contlbl  = newTempLabelStmt (scope, tok);
            LabelStmt *looplbl = newTempLabelStmt (scope, tok);

            BlockStmt *blkstmt = new BlockStmt (scope, tok);
            blkstmt->stmts.push_back (looplbl);
            blkstmt->stmts.push_back (parseStatement (scope, nullptr));
            blkstmt->stmts.push_back (this->contlbl);

            Token *whiletok = gettoken ();
            if (! whiletok->isKw (KW_WHILE)) {
                throwerror (whiletok, "expecting 'while' for do statement");
            }

            GotoStmt *gotostmt = new GotoStmt (scope, tok);
            gotostmt->name = looplbl->name;
            gotostmt->label = looplbl;

            IfStmt *ifstmt = new IfStmt (scope, tok);
            ifstmt->condexpr = parseParenExpr (scope);
            ifstmt->gotostmt = gotostmt;

            blkstmt->stmts.push_back (ifstmt);
            blkstmt->stmts.push_back (this->breaklbl);

            swallowsemi ();
            this->breaklbl = oldbreaklbl;
            this->contlbl  = oldcontlbl;
            return blkstmt;
        }

        case KW_ELSE: {
            throwerror (tok, "else not part of if statement");
        }

        //  {
        //      initstmt;
        //    looplbl:
        //      if (! condexpr) goto breaklbl;
        //      bodystmt;
        //    contlbl:
        //      stepstmt;
        //      goto looplbl;
        //    breaklbl:
        //  }
        case KW_FOR: {
            Scope *forscope = new BlockScope (scope);

            LabelStmt *oldbreaklbl = this->breaklbl;
            LabelStmt *oldcontlbl  = this->contlbl;

            this->breaklbl = newTempLabelStmt (forscope, tok);
            this->contlbl  = newTempLabelStmt (forscope, tok);
            LabelStmt *looplbl = newTempLabelStmt (forscope, tok);

            BlockStmt *blkstmt = new BlockStmt (forscope, tok);

            tok = gettoken ();
            if (! tok->isKw (KW_OPAREN)) {
                throwerror (tok, "expecting '(' after 'for'");
            }
            tok = gettoken ();
            if (! tok->isKw (KW_SEMI)) {
                pushtoken (tok);
                if (scope->istypenext ()) {
                    blkstmt->stmts.push_back (parseDeclStmt (forscope));
                } else {
                    blkstmt->stmts.push_back (parseExprStmt (forscope));
                }
            }

            blkstmt->stmts.push_back (looplbl);

            tok = gettoken ();
            if (! tok->isKw (KW_SEMI)) {
                pushtoken (tok);
                GotoStmt *gotobreakstmt = new GotoStmt (forscope, tok);
                gotobreakstmt->name = this->breaklbl->name;
                gotobreakstmt->label = this->breaklbl;
                IfStmt *ifstmt   = new IfStmt (forscope, tok);
                ifstmt->condrev  = true;
                ifstmt->condexpr = forscope->parsexpression (false, false);
                ifstmt->gotostmt = gotobreakstmt;
                blkstmt->stmts.push_back (ifstmt);
                tok = gettoken ();
                if (! tok->isKw (KW_SEMI)) {
                    throwerror (tok, "expecting ';' at end of condition");
                }
            }

            Stmt *stepstmt = nullptr;
            tok = gettoken ();
            if (! tok->isKw (KW_CPAREN)) {
                pushtoken (tok);
                ExprStmt *exprstmt = new ExprStmt ();
                exprstmt->expr = forscope->parsexpression (false, false);
                stepstmt = exprstmt;
                tok = gettoken ();
                if (! tok->isKw (KW_CPAREN)) {
                    throwerror (tok, "expecting ')' at end of increment");
                }
            }

            blkstmt->stmts.push_back (parseStatement (forscope, nullptr));

            GotoStmt *gotoloopstmt = new GotoStmt (forscope, tok);
            gotoloopstmt->name = looplbl->name;
            gotoloopstmt->label = looplbl;

            blkstmt->stmts.push_back (this->contlbl);
            if (stepstmt != nullptr) blkstmt->stmts.push_back (stepstmt);
            blkstmt->stmts.push_back (gotoloopstmt);
            blkstmt->stmts.push_back (this->breaklbl);

            this->breaklbl = oldbreaklbl;
            this->contlbl  = oldcontlbl;
            return blkstmt;
        }

        case KW_GOTO: {
            GotoStmt *gotostmt = new GotoStmt (scope, tok);
            Token *labeltok = gettoken ();
            char const *labelsym = labeltok->getSym ();
            if (labelsym == nullptr) {
                throwerror (labeltok, "expecting statement label for goto statement");
            }
            gotostmt->name  = labelsym;
            gotostmt->label = nullptr;
            this->gotostmts.push_back (gotostmt);
            swallowsemi ();
            return gotostmt;
        }

        case KW_IF: {
            IfStmt *ifstmt     = new IfStmt (scope, tok);
            ifstmt->condexpr   = parseParenExpr (scope);
            Stmt *thenstmt     = parseStatement (scope, nullptr);
            GotoStmt *thengoto = thenstmt->castGotoStmt ();
            if (thengoto != nullptr) {
                ifstmt->gotostmt = thengoto;
                return ifstmt;
            }

            //  {
            //      if (!(condexpr)) goto donelbl;
            //      thenstmt;
            //    donelbl:
            //  }

            //  {
            //      if (!(condexpr)) goto elselbl;
            //      thenstmt;
            //      goto donelbl;
            //    elselbl:
            //      elsestmt;
            //    donelbl:
            //  }

            ifstmt->condrev = true;

            LabelStmt *donelbl = newTempLabelStmt (scope, tok);
            GotoStmt *donegoto = new GotoStmt (scope, tok);
            donegoto->name  = donelbl->name;
            donegoto->label = donelbl;

            BlockStmt *blkstmt = new BlockStmt (scope, tok);
            blkstmt->stmts.push_back (ifstmt);
            blkstmt->stmts.push_back (thenstmt);

            Token *elsetok = gettoken ();
            if (elsetok->isKw (KW_ELSE)) {
                LabelStmt *elselbl = newTempLabelStmt (scope, tok);
                GotoStmt *elsegoto = new GotoStmt (scope, tok);
                elsegoto->name  = elselbl->name;
                elsegoto->label = elselbl;

                ifstmt->gotostmt = elsegoto;
                blkstmt->stmts.push_back (donegoto);
                blkstmt->stmts.push_back (elselbl);
                blkstmt->stmts.push_back (parseStatement (scope, nullptr));
            } else {
                ifstmt->gotostmt = donegoto;
                pushtoken (elsetok);
            }

            blkstmt->stmts.push_back (donelbl);

            return blkstmt;
        }

        case KW_OBRACE: {
            Scope *newscope = new BlockScope (scope);
            BlockStmt *blkstmt = new BlockStmt (newscope, tok);
            while (true) {
                Token *bracetok = gettoken ();
                if (bracetok->isKw (KW_CBRACE)) break;
                pushtoken (bracetok);
                try {
                    Stmt *substmt = parseStatement (newscope, nullptr);
                    blkstmt->stmts.push_back (substmt);
                } catch (Error *err) {
                    handlerror (err);
                }
            }
            return blkstmt;
        }

        //  {
        //      __retvalue = return value;
        //      goto __return;
        //  }
        case KW_RETURN: {
            BlockStmt *bs = new BlockStmt (scope, tok);

            FuncType *functype = this->getFuncType ();
            if (functype->getRetType () != voidtype) {
                Expr *rx = scope->parsexpression (false, false);
                scope->checkAsneqCompat (tok, functype->getRetType (), rx->getType ());
                rx = scope->castToType (tok, functype->getRetType (), rx);

                AsnopExpr *ax = new AsnopExpr (scope, tok);
                ax->leftexpr = new ValExpr (scope, tok, this->retvalue);
                ax->riteexpr = rx;

                ExprStmt *xs = new ExprStmt ();
                xs->expr = ax;

                bs->stmts.push_back (xs);
            }

            GotoStmt *gs = new GotoStmt (scope, tok);
            gs->name = this->retlabel->name;
            gs->label = this->retlabel;
            bs->stmts.push_back (gs);

            swallowsemi ();
            return bs;
        }

        case KW_SEMI: {
            return new NullStmt (scope, tok);
        }

        case KW_SWITCH: {
            LabelStmt *oldbreaklbl = this->breaklbl;

            SwitchStmt *switchstmt = new SwitchStmt (scope, tok);
            switchstmt->breaklbl = this->breaklbl = newTempLabelStmt (scope, tok);
            switchstmt->defstmt  = nullptr;
            switchstmt->valuexpr = parseParenExpr (scope);
            tok = gettoken ();
            if (! tok->isKw (KW_OBRACE)) {
                throwerror (tok, "expecting '{' for switch statement block");
            }
            while (true) {
                tok = gettoken ();
                if (tok->isKw (KW_CBRACE)) break;
                pushtoken (tok);
                switchstmt->bodystmts.push_back (parseStatement (scope, switchstmt));
            }

            this->breaklbl = oldbreaklbl;
            return switchstmt;
        }

        case KW_THROW: {
            ThrowStmt *ts = new ThrowStmt (scope, tok);
            tok = gettoken ();
            if (! tok->isKw (KW_SEMI)) {
                pushtoken (tok);
                ts->thrownexpr = scope->parsexpression (false, false);
                swallowsemi ();
            }
            return ts;
        }

        case KW_TRY: {
            LabelStmt *oldbreaklbl = this->breaklbl;
            LabelStmt *oldcontlbl  = this->contlbl;
            LabelStmt *oldretlabel = this->retlabel;

            TryScope *tryscope = new TryScope (scope);
            if (oldbreaklbl != nullptr) this->breaklbl = tryscope->wraplabel (oldbreaklbl);
            if (oldcontlbl  != nullptr) this->contlbl  = tryscope->wraplabel (oldcontlbl);
            if (oldretlabel != nullptr) this->retlabel = tryscope->wraplabel (oldretlabel);

            TryStmt  *trystmt = new TryStmt (tryscope, tok);
            trystmt->bodystmt = parseStatement (tryscope, nullptr);
            trystmt->fintoken = tok;
            trystmt->finstmt  = nullptr;
            while (true) {
                tok = gettoken ();
                if (! tok->isKw (KW_CATCH)) break;
                tok = gettoken ();
                if (! tok->isKw (KW_OPAREN)) {
                    throwerror (tok, "expecting '(' after 'catch'");
                }
                ParamScope *catchpscope = new ParamScope (tryscope);
                DeclVec catchpvector;
                do {
                    catchpscope->parsevaldecl (PE_PARAM, &catchpvector);
                    tok = gettoken ();
                } while (tok->isKw (KW_COMMA));
                if (! tok->isKw (KW_CPAREN)) {
                    throwerror (tok, "expecting ')' after catch block parameter");
                }
                if ((catchpvector.size () < 1) || (catchpvector.size () > 2)) {
                    throwerror (tok, "must have exactly one parameter to 'catch' block");
                }
                VarDecl *catchparam = catchpvector.at (0)->castVarDecl ();
                if ((catchparam == nullptr) || (catchparam->getType ()->stripCVMod ()->castPtrType () == nullptr)) {
                    throwerror (tok, "catch block parameter must be a pointer variable");
                }
                VarDecl *tcatchparam = nullptr;
                if (catchpvector.size () > 1) {
                    tcatchparam = catchpvector.at (1)->castVarDecl ();
                    Type *tctype = voidtype->getConstType ()->getPtrType ()->getConstType ()->getPtrType ();
                    if ((tcatchparam == nullptr) || (tcatchparam->getType ()->stripCVMod () != tctype)) {
                        throwerror (tok, "second catch block parameter must be a variable of type '%s'", tctype->getName ());
                    }
                }
                Stmt *catchstmt = parseStatement (catchpscope, nullptr);
                trystmt->catches.push_back ({ { catchparam, tcatchparam }, catchstmt });
            }
            if (tok->isKw (KW_FINALLY)) {
                trystmt->fintoken = tok;
                trystmt->finstmt = parseStatement (tryscope, nullptr);
            } else {
                pushtoken (tok);
            }
            if ((trystmt->finstmt == nullptr) && (trystmt->catches.size () == 0)) {
                printerror (trystmt->getStmtToken (), "try has no catches or finally");
            }

            this->breaklbl = oldbreaklbl;
            this->contlbl  = oldcontlbl;
            this->retlabel = oldretlabel;
            return trystmt;
        }

        case KW_WHILE: {
            LabelStmt *oldbreaklbl = this->breaklbl;
            LabelStmt *oldcontlbl  = this->contlbl;

            this->breaklbl = newTempLabelStmt (scope, tok);
            this->contlbl  = newTempLabelStmt (scope, tok);

            GotoStmt *gotobreakstmt = new GotoStmt (scope, tok);
            gotobreakstmt->name = this->breaklbl->name;
            gotobreakstmt->label = this->breaklbl;

            GotoStmt *gotocontstmt = new GotoStmt (scope, tok);
            gotocontstmt->name = this->contlbl->name;
            gotocontstmt->label = this->contlbl;

            IfStmt *ifstmt   = new IfStmt (scope, tok);
            ifstmt->condrev  = true;
            ifstmt->condexpr = parseParenExpr (scope);
            ifstmt->gotostmt = gotobreakstmt;

            BlockStmt *blkstmt = new BlockStmt (scope, tok);
            blkstmt->stmts.push_back (this->contlbl);
            blkstmt->stmts.push_back (ifstmt);
            blkstmt->stmts.push_back (parseStatement (scope, nullptr));
            blkstmt->stmts.push_back (gotocontstmt);
            blkstmt->stmts.push_back (this->breaklbl);

            this->breaklbl = oldbreaklbl;
            this->contlbl  = oldcontlbl;
            return blkstmt;
        }

        // these keywords are only at the beginning of a declaration statement
        case KW_EXTERN:
        case KW_STATIC:
        case KW_TYPEDEF: {
            pushtoken (tok);
            return parseDeclStmt (scope);
        }

        // don't have explicit keyword telling us what type of statement we have
        // if the statement begins with a type, assume it is a declaration statement
        // otherwise, assume it is an expression statement
        default: {

            // maybe it is label ':'
            char const *name = tok->getSym ();
            if (name != nullptr) {
                Token *tok2 = gettoken ();
                if (tok2->isKw (KW_COLON)) {
                    LabelStmt *labelstmt = new LabelStmt (scope, tok2);
                    labelstmt->name = name;
                    scope->enterlabel (labelstmt);
                    return labelstmt;
                }
                pushtoken (tok2);
            }

            // if begins with type, process as a declaration
            // otherwise, process as a stand-alone expression
            pushtoken (tok);
            return scope->istypenext () ?
                    (Stmt *) parseDeclStmt (scope) :
                    (Stmt *) parseExprStmt (scope);
        }
    }
}

// make a temporary label for something like a do/for/while loop
LabelStmt *FuncDecl::newTempLabelStmt (Scope *scope, Token *tok)
{
    static tsize_t seq = 100;
    char *buf = new char[20];
    sprintf (buf, "lab.%u", ++ seq);
    LabelStmt *ls = new LabelStmt (scope, tok);
    ls->name = buf;
    return ls;
}

// parse a declaration statement that is inside a function
// capture the declarations in case there are init expressions
DeclStmt *FuncDecl::parseDeclStmt (Scope *scope)
{
    DeclStmt *declstmt = new DeclStmt ();
    scope->parsevaldecl (PE_BLOCK, &declstmt->alldecls);
    return declstmt;
}

// parse an expression as a statement
// usually an assignment expression or a void function call
ExprStmt *FuncDecl::parseExprStmt (Scope *scope)
{
    ExprStmt *exprstmt = new ExprStmt ();
    exprstmt->expr = scope->parsexpression (false, false);
    swallowsemi ();
    return exprstmt;
}

// skip over a dangling ';' at end of statement
void FuncDecl::swallowsemi ()
{
    Token *tok = gettoken ();
    if (! tok->isKw (KW_SEMI)) {
        printerror (tok, "expecting ';' at end of statement");
        pushtoken (tok);
    }
}

// parse an '(' expression ')'
// return the expression within the parentheses
Expr *FuncDecl::parseParenExpr (Scope *scope)
{
    Token *tok = gettoken ();
    if (! tok->isKw (KW_OPAREN)) {
        throwerror (tok, "expecting '(' expression ')'");
    }
    Expr *expr = scope->parsexpression (false, false);
    tok = gettoken ();
    if (! tok->isKw (KW_CPAREN)) {
        throwerror (tok, "expecting ')' at end of '(' expression ')'");
    }
    return expr;
}
