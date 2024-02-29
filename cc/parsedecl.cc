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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "error.h"
#include "token.h"

// note:
//  char const *a, b;
//   const follows through to b
//   * does not follow through to b

// parse <cvptrmoddedtype> <varname> [ '(' <params> ... ')' ]
//  input:
//   PE_BLOCK  : parsing declaration within function body
//   PE_GLOBAL : parsing a global declaration
//   PE_STRUCT : parsing a struct/union member declaration
//   PE_PARAM  : parsing a function parameter declaration
//   PE_TCAST  : parsing a type cast type
//  output:
//   declaration(s) entered in this scope
//   declaration(s) appended to declvec
//   swallows terminator (';' or '}') for PE_BLOCK, PE_GLOBAL, PE_STRUCT
//   leaves terminator (',' or ')') in stream for PE_PARAM, PE_TCAST
void Scope::parsevaldecl (PvEnv pvenv, DeclVec *declvec)
{

    // maybe it is structtype '::' ['~'] structtype '(' ...
    // if so, make it look like 'void' structtype '::' '__ctor'/'__dtor' '(' ...
    Token *nametok = gettoken ();
    char const *name = nametok->getSym ();
    if (name != nullptr) {

        // see if first name is a struct type name
        StructType *structtype = this->getStructType (name);
        if (structtype != nullptr) {

            // see if next token is '::'
            Token *cctok = gettoken ();
            if (cctok->isKw (KW_COLON2)) {

                // next might be optional '~' followed by same struct type name
                char const *entname = "__ctor";
                Token *tildetok = nullptr;
                Token *name2tok = gettoken ();
                if (name2tok->isKw (KW_TILDE)) {
                    entname  = "__dtor";
                    tildetok = name2tok;
                    name2tok = gettoken ();
                }
                char const *name2 = name2tok->getSym ();
                if ((name2 != nullptr) && (strcmp (name2, name) == 0)) {

                    // ok, push token back substituting '__ctor' for structtype or '__dtor' for '~' structtype
                    pushtoken (new SymToken (name2tok->getFile (), name2tok->getLine (), name2tok->getColn (), entname));

                    // then push back 'void' structtype '::' and go parse as a declaration
                    pushtoken (cctok);
                    pushtoken (nametok);
                    pushtoken (new SymToken (nametok->getFile (), nametok->getLine (), nametok->getColn (), voidtype->getName ()));
                    goto pushed;
                }

                // not what we're looking for, push back all we fetched
                pushtoken (name2tok);
                if (tildetok != nullptr) pushtoken (tildetok);
            }
            pushtoken (cctok);
        }
    }
    pushtoken (nametok);
pushed:;

    assert (pvenv != PE_TCAST);
    parsedecl (pvenv, declvec);
}

Type *Scope::parsetypecast ()
{
    DeclVec declvec;
    parsedecl (PE_TCAST, &declvec);
    return (declvec.size () != 1) ? nullptr : declvec[0]->castType ();
}

void Scope::parsedecl (PvEnv pvenv, DeclVec *declvec)
{
    // splice storage class off the front (extern, static, typedef, virtual)
    // otherwise assume it is global
    Keywd storclass = KW_NONE;
    Token *sctok = nullptr;
    while (true) {
        Token *tok = gettoken ();
        Keywd kw = tok->getKw ();
        if ((kw != KW_EXTERN) && (kw != KW_STATIC) && (kw != KW_TYPEDEF) && (kw != KW_VIRTUAL)) {
            pushtoken (tok);
            break;
        }
        if (storclass != KW_NONE) {
            printerror (tok, "conflicting storage class");
        }
        storclass = kw;
        sctok = tok;
    }

    switch (pvenv) {
        case PE_PARAM: {
            if (storclass != KW_NONE) {
                printerror (sctok, "storage class not allowed for function parameter");
            }
            break;
        }
        case PE_STRUCT: {
            if ((storclass != KW_NONE) && (storclass != KW_STATIC) && (storclass != KW_VIRTUAL)) {
                printerror (sctok, "storage class not allowed for struct/union member");
            }
            break;
        }
        case PE_TCAST: {
            if (storclass != KW_NONE) {
                printerror (sctok, "storage class not allowed for type cast");
            }
            break;
        }
        default: break;
    }

    // parse type at beginning of line
    // - it can be a named type of a variable
    //   next comes variable name(s) followed by ';'
    // - it can be a return type from a function
    //   next comes the function name followed by '(' params ')' then '{' body '}' or ';'
    // - it can be a struct/union reference and/or definition
    //   struct/unions get entered in the scope as a type if not already
    //   next comes variable name(s) followed by ';'
    Type *begoflinetype = parsecvmoddedtype (); // get type + const / volatile
                                                // do not parse *
                                                // applies to all variables in the statement

    // might just have 'enum/struct/union <typename> { ... };'
    Token *sdtok = gettoken ();
    if (sdtok->isKw (KW_SEMI)) {
        if (declvec != nullptr) declvec->push_back (begoflinetype);
        return;
    }
    pushtoken (sdtok);

    // next comes function or variable names
    StructType *enctype = nullptr;
    FuncType *functype = nullptr;
    std::vector<VarDecl *> *funcparams = nullptr;
    ParamScope *funcparamscope = nullptr;
    bool    hasarrsize  = false;
    char const *varname = nullptr;
    Token  *nexttok     = nullptr;
    Token  *varnametok  = nullptr;
    Type   *vartype     = nullptr;
    tsize_t arrsize     = 0;
    while (true) {

        // maybe there are some '*'s possibly with embedded/trailing const/volatile
        // those apply to only this variable (not any others after a comma)
        vartype = parsetypemods (begoflinetype);

        // if type cast, no function/variable name
        if (pvenv == PE_TCAST) {
            nexttok = gettoken ();
            break;
        }

        // now there should be a function/variable/typedef name
        //TODO parse things like '(' '*' name ')'
        varnametok = gettoken ();
        varname = varnametok->getSym ();
        if (varname == nullptr) {
            throwerror (varnametok, "expecting function/variable/type name");
        }

        // maybe that was struct::, if so, save enclosing type and get the following function/variable name
        nexttok = gettoken ();
        if (nexttok->isKw (KW_COLON2)) {
            Decl *encdecl = this->lookupdecl (varname);
            enctype = (encdecl == nullptr) ? nullptr : encdecl->castStructType ();
            if (enctype == nullptr) {
                throwerror (varnametok, "unknown struct/union type '%s'", varname);
            }
            varnametok = gettoken ();
            varname    = varnametok->getSym ();
            if (varname == nullptr) {
                throwerror (varnametok, "expecting function/variable name after '%s'::", enctype->getName ());
            }
            if (pvenv != PE_GLOBAL) {
                throwerror (varnametok, "struct name qualifier not allowed in this context");
            }
            if (storclass != KW_NONE) {
                throwerror (varnametok, "storage class such as static, virtual, not allowed here");
            }
        } else {
            pushtoken (nexttok);
        }

        // maybe there is a '[' size ']'
        //  eg, use 'int * const' for int arrays
        //  'const' to disallow setting the array to something
        //  ie, disallow:
        //      char buf[23];
        //      buf = "abc";
        hasarrsize = parsearrsize (&arrsize);
        if (hasarrsize) {
            if (storclass == KW_TYPEDEF) {
                throwerror (varnametok, "cannot typedef an array");
            }
            vartype = vartype->getPtrType ()->getConstType ();
        }

        nexttok = gettoken ();

        // parameter parsing stops here
        if (pvenv == PE_PARAM) break;

        // if no ',' or '=', exit the loop and do more analysis before declaring it
        if (! nexttok->isKw (KW_COMMA) && ! nexttok->isKw (KW_EQUAL)) break;

        // that name is followed by a ',' or '=', so declare the variable
        VarDecl *vardecl = declarevariable (varname, varnametok, enctype, vartype, storclass, hasarrsize, arrsize);
        if (vardecl != nullptr) {
            if (declvec != nullptr) declvec->push_back (vardecl);

            // if var name followed by '=', parse the initialization expression
            if (nexttok->isKw (KW_EQUAL)) {
                Expr *initexpr = parseinitexpr (vartype, hasarrsize, &arrsize);
                if (hasarrsize) vardecl->setArrSize (arrsize);
                vardecl->setInitExpr (initexpr);
                nexttok = gettoken ();
            } else {

                // if struct or array of struct, give it struct init expr to fill in vtable pointers
                maybeoutputstructinit (vardecl);
            }

            // if global scope, output label and .blkb or .word etc to allocate storage
            if ((pvenv == PE_GLOBAL) && (storclass != KW_TYPEDEF)) {
                vardecl->allocstaticvar ();
                vardecl->outputstaticvar ();
            }
        }

        // all done if no ','
        if (! nexttok->isKw (KW_COMMA)) goto gotdef;
    }

    // if '(' params ')', the type so far is a return type
    // ...and the params make up the rest of the function type
    if (nexttok->isKw (KW_OPAREN)) {
        vartype = parsefunctype (vartype, &funcparams, &funcparamscope);
        // the 'abc' in 'void abc ()' is constant in same sense that a '3' is constant
        vartype = vartype->getConstType ();
        nexttok = gettoken ();
    }

    // if typecast, that's it, return the type
    if (pvenv == PE_TCAST) {
        pushtoken (nexttok);
        if (declvec != nullptr) declvec->push_back (vartype);
        return;
    }

    // if a function, declare the function
    functype = vartype->stripCVMod ()->castFuncType ();
    if (functype != nullptr) {
        if (pvenv == PE_PARAM) {
            throwerror (varnametok, "function not valid as function parameter");
        }

        // check for 'typedefdFuncType funcname'
        // if so, the funcparams,funcparamscope will be null
        //   so they need to be copied from functype
        if (funcparams == nullptr) {
            funcparamscope = new ParamScope (this);
            funcparams = new std::vector<VarDecl *> ();
            for (VarDecl *oldparam : *functype->getRawParams ()) {
                VarDecl *newparam = new VarDecl (oldparam->getName (), funcparamscope, oldparam->getDefTok (), oldparam->getType (), KW_NONE);
                funcparams->push_back (newparam);
            }
        }

        // maybe we are at top level declaring the body of a function that is member of a struct
        FuncDecl *funcdecl = nullptr;
        if (enctype != nullptr) {

            // declaring body of a struct member function
            // struct should already contain function prototype
            Decl *olddecl = enctype->getMemScope ()->lookupdecl (varname);
            if (olddecl == nullptr) {
                // if we have <structname>::<structname>();
                // ...that means put the __init() & __term() function code right here
                if ((strcmp (varname, "__ctor") == 0) && (funcparams->size () == 0) && nexttok->isKw (KW_SEMI)) {
                    enctype->genInitFunc ();
                    enctype->genTermFunc ();
                    return;
                }
                throwerror (varnametok, "struct '%s' does not contain definition of '%s'", enctype->getName (), varname);
            }
            funcdecl = olddecl->castFuncDecl ();
            if (funcdecl == nullptr) {
                throwerror (varnametok, "struct '%s' member '%s' is not a function", enctype->getName (), varname);
            }
            storclass = funcdecl->getStorClass ();
            if (! funcdecl->compatredecl (functype, storclass, funcparams, funcparamscope)) {
                throwerror (varnametok, "struct '%s' member function '%s' incompatible declaration", enctype->getName (), varname);
            }
        } else {

            // not a struct member function
            // make function declaration entry
            // - or re-use an old identical one (like extern ...)
            Decl *olddecl = this->lookupdecl (varname);
            if ((olddecl != nullptr) && (olddecl->getScope () == this)) {
                funcdecl = olddecl->castFuncDecl ();
                if ((funcdecl == nullptr) || ! funcdecl->compatredecl (functype, storclass, funcparams, funcparamscope)) {
                    throwerror (varnametok, "function '%s' redeclaration incompatible", varname);
                }
            } else {
                funcdecl = new FuncDecl (varname, this, varnametok, functype, storclass, funcparams, funcparamscope);
            }
        }
        funcparamscope->funcdecl = funcdecl;
        if (declvec != nullptr) declvec->push_back (funcdecl);

        // check for '=' '0' to indicate pure virtual
        if ((pvenv == PE_STRUCT) && (storclass == KW_VIRTUAL) && nexttok->isKw (KW_EQUAL)) {
            Token *zerotok = gettoken ();
            NumValue nv;
            if ((zerotok->getNumValue (&nv) == NC_SINT) && (nv.s == 0)) {
                funcdecl->purevirt = true;
                nexttok = gettoken ();
            } else {
                pushtoken (zerotok);
            }
        }

        // parse initialization expression, ie, function body
        // - function: '{' body '}' or ';'
        if (nexttok->isKw (KW_OBRACE)) {
            if (storclass == KW_TYPEDEF) {
                printerror (nexttok, "cannot have function '%s' body in a typedef", funcdecl->getEncName ());
            }
            if (funcdecl->getFuncBody () != nullptr) {
                printerror (nexttok, "redefinition of function '%s' body", funcdecl->getEncName ());
            }
            if (funcdecl->purevirt) {
                printerror (nexttok, "pure virtual function '%s' cannot have body", funcdecl->getEncName ());
            }
            if (pvenv == PE_STRUCT) {
                printerror (nexttok, "cannot put function body '%s' in struct definition", funcdecl->getEncName ());
            }
            funcdecl->setDefTok (nexttok);
            pushtoken (nexttok);
            funcdecl->parseFuncBody ();
        } else {
            if (! nexttok->isKw (KW_SEMI)) {
                throwerror (nexttok, "expecting ';' or '{' after function '%s' header", varname);
            }
        }
        return;
    }

    // not a function
    // declare the variable/typedef
    {
        VarDecl *vardecl = declarevariable (varname, varnametok, enctype, vartype, storclass, hasarrsize, arrsize);
        if (vardecl != nullptr) {
            if (declvec != nullptr) declvec->push_back (vardecl);

            // if struct or array of struct, give it struct init expr to fill in vtable pointers
            maybeoutputstructinit (vardecl);

            // if global scope, output label and .blkb or .word etc to allocate storage
            if ((pvenv == PE_GLOBAL) && (storclass != KW_TYPEDEF)) {
                vardecl->allocstaticvar ();
                vardecl->outputstaticvar ();
            }
        }
    }
gotdef:;

    // parse termination character
    if (pvenv == PE_PARAM) {
        // function parameter, leave terminating ',' or ')' yet to be processed
        pushtoken (nexttok);
    } else {
        // block, global, member:  swallow the terminating ';'
        if (! nexttok->isKw (KW_SEMI)) {
            throwerror (nexttok, "expecting ';' or '=' after variable declaration");
        }
    }
}

// parse '[' integer ']' subscript
bool Scope::parsearrsize (tsize_t *arrsize_r)
{
    Token *tok = gettoken ();
    if (! tok->isKw (KW_OBRKT)) {
        pushtoken (tok);
        return false;
    }
    tok = gettoken ();
    if (tok->isKw (KW_CBRKT)) {
        *arrsize_r = 0;
        return true;
    }
    pushtoken (tok);
    *arrsize_r = parseintconstexpr ();
    tok = gettoken ();
    if (! tok->isKw (KW_CBRKT)) {
        throwerror (tok, "expecting ']' at end of subscript");
    }
    return true;
}

// make entry in this scope for a variable
// this may be a legal re-definition
//  input:
//   varname = variable's name
//   varnametok = name token for error messages
//   vartype = variable's type
//   storclass = KW_EXTERN, KW_STATIC, KW_TYPEDEF, KW_NONE
//   hasarrsize = has a '[' ... ']', vartype already has that taken into account
//   arrsize = number of elements in array (can be modified by subsequent initialization expression)
//  output:
//   returns entered declaration or previously entered definition if legal
//   returns nullptr for duplicate typedef entry
//   throws error if illegal re-definition
VarDecl *Scope::declarevariable (char const *varname, Token *varnametok, StructType *enctype, Type *vartype, Keywd storclass, bool hasarrsize, tsize_t arrsize)
{
    // if declaring enctype::varname, we are at the global scope and declaring a static variable of a struct
    // the variable should already declared as a static member of the struct
    if (enctype != nullptr) {
        assert (this == globalscope);
        Decl *memdecl = enctype->getMemScope ()->lookupdecl (varname);
        if (memdecl == nullptr) {
            throwerror (varnametok, "struct '%s' does not contain definition for '%s'", enctype->getName (), varname);
        }
        VarDecl *memvardecl = memdecl->castVarDecl ();
        if ((memvardecl == nullptr) || (memvardecl->getStorClass () != KW_STATIC)) {
            throwerror (varnametok, "struct '%s' member '%s' is not a static variable", enctype->getName (), varname);
        }
        return memvardecl;
    }

    // declaring something else

    // if struct or array of structs, print error message if there are exposed pure virtual functions
    /*** maybe declaring parameter to method of same struct type being declared so structtype is not yet defined
    if (storclass != KW_TYPEDEF) {
        Type *eletype = vartype->stripCVMod ();
        if (hasarrsize) eletype = eletype->castPtrType ()->getBaseType ()->stripCVMod ();
        eletype->isAbstract (varnametok, nullptr);
    }***/

    // some duplicate definitions are allowed, so see if there is already one defined
    Decl *olddecl = lookupdecl (varname);
    if ((olddecl != nullptr) && (olddecl->getScope () == this)) {
        Keywd oldstclas;
        tsize_t oldarrsize;

        // allow for duplicate typedefs
        // - this allows 'typedef struct A A'
        //   'struct A' was previously entered by itself
        if (storclass == KW_TYPEDEF) {
            assert (! hasarrsize);
            Type *oldtype = olddecl->castType ();
            if (oldtype == vartype) return nullptr;
        }

        // the old decl must be a variable of exact same type
        VarDecl *oldvard = olddecl->castVarDecl ();
        if (oldvard == nullptr) goto bad;
        if (oldvard->getType () != vartype) goto bad;
        if (oldvard->isArray () != hasarrsize) goto bad;
        oldarrsize = hasarrsize ? oldvard->getArrSize () : 0;

        // not allowed if either is a stack variable
        oldstclas = oldvard->getStorClass ();
        if (this != globalscope) {
            if (storclass == KW_NONE) goto bad;
            if (oldstclas == KW_NONE) goto bad;
        }

        // allowed if both are typedefs, bad if just one is typedef
        if ((storclass == KW_TYPEDEF) && (oldstclas == KW_TYPEDEF)) {
            assert (! hasarrsize);
            return oldvard;
        }
        if ((storclass == KW_TYPEDEF) || (oldstclas == KW_TYPEDEF)) goto bad;

        // allowed if both are extern
        if ((storclass == KW_EXTERN) && (oldstclas == KW_EXTERN)) {
            //TODO arrsize
            return oldvard;
        }

        // if old extern and new global, toss old extern
        if ((oldstclas == KW_EXTERN) && (storclass == KW_NONE)) {
            if ((oldarrsize != 0) && (oldarrsize != arrsize)) goto bad;
            goto tossold;
        }

        // if old global and new extern, use old global
        if ((oldstclas == KW_NONE) && (storclass == KW_EXTERN)) {
            if ((arrsize != 0) && (arrsize != oldarrsize)) goto bad;
            return oldvard;
        }

        // disallow all else
    bad:;
        printerror (varnametok, "re-declaration of '%s' type '%s'", varname, vartype->getName ());
        throwerror (olddecl->getDefTok (), "... old declaration here");

        // get rid of old decl (eg extern) and replace with new decl (eg global)
    tossold:;
        this->erasedecl (varname);
    }

    VarDecl *vardecl = new VarDecl (varname, this, varnametok, vartype, storclass);
    if (hasarrsize) vardecl->setArrSize (arrsize);
    return vardecl;
}

// parse <paramdecl>, ... ')' and create/lookup corresponding function type
//  input:
//   rettype = function return type (might be 'void')
//   params = a newly created empty vector
//  output:
//   returns parsed functype (might be old functype with old params vector)
//   params filled in with parameter types/names
FuncType *Scope::parsefunctype (Type *rettype, std::vector<VarDecl *> **params_r, ParamScope **paramscope_r)
{
    std::vector<VarDecl *> *params = new std::vector<VarDecl *> ();
    ParamScope *paramscope = new ParamScope (this);
    Token *tok = gettoken ();
    if (! tok->isKw (KW_CPAREN)) {
        while (true) {
            pushtoken (tok);
            DeclVec declvec;
            paramscope->parsevaldecl (PE_PARAM, &declvec);
            VarDecl *vardecl = (declvec.size () != 1) ? nullptr : declvec[0]->castVarDecl ();
            if (vardecl == nullptr) {
                throwerror (tok, "expecting function parameter declaration");
            }
            params->push_back (vardecl);
            tok = gettoken ();
            if (! tok->isKw (KW_COMMA)) break;
            tok = gettoken ();
            if (tok->isKw (KW_DOTDOTDOT)) {
                VarDecl *varargdecl = new VarDecl ("__varargs", paramscope, tok, voidtype, KW_NONE);
                params->push_back (varargdecl);
                tok = gettoken ();
                break;
            }
        }
        if (! tok->isKw (KW_CPAREN)) {
            throwerror (tok, "expecting ')' at end of function parameters");
        }
    }
    *params_r = params;
    *paramscope_r = paramscope;

    // create new func type or find existing one that matches the signature
    return FuncType::create (tok, rettype, params);
}

// parse <cvmoddedtype> { '*' [ 'const' | 'volatile' ] ... }
//  input:
//   *tok_r = first token to be parsed
//  output:
//   *tok_r = token just past last one parsed
//   *permtype_r = type that persists in a declaration
//   returns resultant type
Type *Scope::parsetypemods (Type *type)
{
    while (true) {
        Token *tok = gettoken ();
        switch (tok->getKw ()) {
            case KW_CONST: {
                if (type->isVolType ()) {
                    printerror (tok, "cannot mix const and volatile");
                } else {
                    type = type->getConstType ();
                }
                break;
            }
            case KW_STAR: {
                type = type->getPtrType ();
                break;
            }
            case KW_VOLATILE: {
                if (type->isConstType ()) {
                    printerror (tok, "cannot mix const and volatile");
                } else {
                    type = type->getVolType ();
                }
                break;
            }
            default: {
                pushtoken (tok);
                return type;
            }
        }
    }
}

// parse [ 'const' | 'volatile' ] <namedtype> [ 'const' | 'volatile' ]
// does not parse '*'
Type *Scope::parsecvmoddedtype ()
{
    // handle prefix-style 'const' or 'volatile'
    Keywd cvmod = KW_NONE;
    while (true) {
        Token *tok = gettoken ();
        Keywd kw = tok->getKw ();
        if ((kw != KW_CONST) && (kw != KW_VOLATILE)) {
            pushtoken (tok);
            break;
        }
        if ((cvmod != KW_NONE) && (cvmod != kw)) {
            printerror (tok, "cannot mix const and volatile");
        }
        cvmod = kw;
    }

    // handle 'char', 'int', 'MyType', 'struct MyStruct ...', etc
    Type *type = parsenamedtype ();

    // handle suffix-style 'const' or 'volatile'
    while (true) {
        Token *tok = gettoken ();
        Keywd kw = tok->getKw ();
        if ((kw != KW_CONST) && (kw != KW_VOLATILE)) {
            pushtoken (tok);
            break;
        }
        if ((cvmod != KW_NONE) && (cvmod != kw)) {
            printerror (tok, "cannot mix const and volatile");
        }
        cvmod = kw;
    }

    // maybe modify named type with 'const' or 'volatile'
    switch (cvmod) {
        case KW_CONST: {
            type = type->getConstType ();
            break;
        }
        case KW_NONE: break;
        case KW_VOLATILE: {
            type = type->getVolType ();
            break;
        }
        default: assert_false ();
    }

    return type;
}

// parse named type such as 'int', 'MyType', 'struct MyStruct ...'
// does not parse modifiers such as 'const', 'volatile', '*'
Type *Scope::parsenamedtype ()
{
    Token *tok = gettoken ();

    // parse 'struct' | 'union' ...
    Keywd kw = tok->getKw ();
    if ((kw == KW_STRUCT) || (kw == KW_UNION)) {
        Token *kwtok = tok;
        tok = gettoken ();

        // get struct/union name or <anon-number> if no name present
        char const *structname = tok->getSym ();
        if (structname == nullptr) {
            static uint32_t anonnum;

            char *anonname = new char[18];
            sprintf (anonname, "<anon-%u>", ++ anonnum);
            structname = anonname;
        } else {
            tok = gettoken ();
        }

        // see if there is such a struct/union already declared
        StructType *structtype = nullptr;
        Decl *structdecl = lookupdecl (structname);
        if (structdecl != nullptr) {
            structtype = structdecl->castStructType ();
            if (structtype == nullptr) {
                throwerror (kwtok, "name '%s' is not a struct/union type", structname);
            }
            if (structtype->getUnyun () != (kw == KW_UNION)) {
                printerror (kwtok, "mixed struct/union use of '%s'", structname);
            }
        }

        // if not, make a declaration entry with contents as-of-yet unspecified
        if (structtype == nullptr) {
            structtype = new StructType (structname, this, kwtok, (kw == KW_UNION));
        }

        // if definition present, parse it, filling in struct/union contents
        if (tok->isKw (KW_COLON) || tok->isKw (KW_OBRACE)) {
            if (structtype->isDefined ()) {
                printerror (tok, "struct/union '%s' contents already defined", structtype->getName ());
            }

            // get list of parent structs
            if (tok->isKw (KW_COLON)) {
                while (true) {
                    tok = gettoken ();
                    char const *extname = tok->getSym ();
                    if (extname == nullptr) {
                        throwerror (tok, "expecting parent struct name");
                    }
                    Decl *extdecl = this->lookupdecl (extname);
                    if (extdecl == nullptr) {
                        throwerror (tok, "unknown parent struct '%s'", extname);
                    }
                    StructType *exttype = extdecl->castStructType ();
                    if ((exttype == nullptr) || exttype->getUnyun ()) {
                        throwerror (tok, "can only extend structs, not '%s'", extname);
                    }
                    structtype->exttypes.push_back (exttype);
                    tok = gettoken ();
                    if (tok->isKw (KW_OBRACE)) break;
                    if (! tok->isKw (KW_COMMA)) {
                        throwerror (tok, "expecting ',' or '{' in extension list");
                    }
                }
            }

            // get list of members
            MemberScope *mscope = new MemberScope (this);
            std::vector<FuncDecl *> *funcs = new std::vector<FuncDecl *> ();
            std::vector<VarDecl *>  *vars  = new std::vector<VarDecl  *> ();
            while (true) {
                tok = gettoken ();
                if (tok->isKw (KW_CBRACE)) break;

                // maybe it is ['virtual'] ['~'] structtype '(' ...
                // if so, make it look like 'void' '__ctor'/'__dtor' '(' ...
                Token *virtoken = nullptr;
                if (tok->isKw (KW_VIRTUAL)) {
                    virtoken = tok;
                    tok = gettoken ();
                }
                char const *entname = "__ctor";
                Token *tildetok = nullptr;
                Token *name2tok = tok;
                if (tok->isKw (KW_TILDE)) {
                    entname  = "__dtor";
                    tildetok = tok;
                    name2tok = gettoken ();
                }
                char const *name2 = name2tok->getSym ();
                if ((name2 != nullptr) && (strcmp (name2, structtype->getName ()) == 0)) {
                    Token *optok = gettoken ();
                    if (optok->isKw (KW_OPAREN)) {

                        // ok, push back '__ctor' or '__dtor' instead of structtype or '~' structtype
                        pushtoken (optok);
                        pushtoken (new SymToken (name2tok->getFile (), name2tok->getLine (), name2tok->getColn (), entname));

                        // splice in a 'void' in front of that
                        pushtoken (new SymToken (name2tok->getFile (), name2tok->getLine (), name2tok->getColn (), voidtype->getName ()));

                        // splice back the 'virtual' in front of that
                        if (virtoken != nullptr) pushtoken (virtoken);
                        goto parsemem;
                    }

                    // not what we want, push everything back
                    pushtoken (optok);
                }
                pushtoken (name2tok);
                if (tildetok != nullptr) pushtoken (tildetok);
                if (virtoken != nullptr) pushtoken (virtoken);
            parsemem:;

                // parse member declaration
                DeclVec declvec;
                mscope->parsevaldecl (PE_STRUCT, &declvec);
                for (Decl *decl : declvec) {
                    VarDecl *vardecl = decl->castVarDecl ();
                    if (vardecl != nullptr) {
                        vars->push_back (vardecl);
                        continue;
                    }
                    FuncDecl *funcdecl = decl->castFuncDecl ();
                    if (funcdecl != nullptr) {
                        if (strcmp (funcdecl->getName (), "__ctor") == 0) {
                            if (funcdecl->getStorClass () != KW_NONE) {
                                printerror (funcdecl->getDefTok (), "constructor must be normal instance function");
                            }
                            if (funcdecl->getFuncType ()->getRawParams ()->size () != 0) {
                                printerror (funcdecl->getDefTok (), "constructor must not have any parameters, use static functions");
                            }
                        }
                        if (strcmp (funcdecl->getName (), "__dtor") == 0) {
                            if (funcdecl->getStorClass () != KW_VIRTUAL) {
                                printerror (funcdecl->getDefTok (), "destructor must be virtual function");
                            }
                            if (funcdecl->getFuncType ()->getRawParams ()->size () != 0) {
                                printerror (funcdecl->getDefTok (), "destructor must not have any parameters");
                            }
                            // virtualness is handled by __term() wrapper virtual function
                            funcdecl->setStorClass (KW_NONE);
                        }
                        funcs->push_back (funcdecl);
                        continue;
                    }
                    printerror (tok, "struct/union member '%s' not a var or func", decl->getName ());
                }
            }
            structtype->setMembers (mscope, funcs, vars);
            tok = gettoken ();
        }

        pushtoken (tok);
        return structtype;
    }

    // parse 'enum' ...
    // - makes a typedef enumname int;
    //   then declares constants of type int
    if (tok->isKw (KW_ENUM)) {
        Token *nametok = gettoken ();
        char const *enumname = nametok->getSym ();
        if (enumname == nullptr) {
            throwerror (nametok, "expecting enum type name");
        }

        // see if there is such an enum already declared
        IntegType *enumtype = nullptr;
        Decl *enumdecl = lookupdecl (enumname);
        if (enumdecl != nullptr) {
            ValDecl *enumvaldecl = enumdecl->castValDecl ();
            if ((enumvaldecl == nullptr) || (enumvaldecl->getStorClass () != KW_TYPEDEF) || (enumvaldecl->getEquivType () != inttype)) {
                throwerror (nametok, "name '%s' is not an enum type", enumname);
            }
            enumtype = inttype;
        }

        // if not, make a declaration entry as a 'typedef int <enumname>'
        if (enumtype == nullptr) {
            enumtype = inttype;
            new VarDecl (enumname, this, nametok, inttype, KW_TYPEDEF);
        }

        // if definition present, parse it
        tok = gettoken ();
        if (tok->isKw (KW_OBRACE)) {
            int64_t enumvalue = 0;
            do {
                tok = gettoken ();
                if (tok->isKw (KW_CBRACE)) break;

                // should have symbol for a constant name
                Token *valuetok = tok;
                char const *valuename = valuetok->getSym ();
                if (valuename == nullptr) {
                    throwerror (valuetok, "enum symbol expected");
                }

                // maybe have '=' expression
                tok = gettoken ();
                if (tok->isKw (KW_EQUAL)) {
                    Expr *expr = parsexpression (true, false);
                    NumValue nv;
                    NumCat nc = expr->isNumConstExpr (&nv);
                    switch (nc) {
                        case NC_SINT: enumvalue = nv.s; break;
                        case NC_UINT: enumvalue = nv.u; break;
                        default: {
                            throwerror (expr->exprtoken, "requires compile-time integer constant");
                        }
                    }
                    tok = gettoken ();
                }

                // declare the constant of type 'int'
                assert (enumtype == inttype);
                new EnumDecl (valuename, this, valuetok, enumtype, enumvalue);
                enumvalue ++;

                // repeat if comma present
            } while (tok->isKw (KW_COMMA));

            // should terminate on a '}'
            if (! tok->isKw (KW_CBRACE)) {
                throwerror (tok, "expecting ',' or '}' after enum value");
            }

            tok = gettoken ();
        }

        pushtoken (tok);
        return enumtype;
    }

    // maybe have 'signed' or 'unsigned' prefix on integer type
    Token *signtok = nullptr;
    if (tok->isKw (KW_SIGNED) || tok->isKw (KW_UNSIGNED)) {
        signtok = tok;
        tok = gettoken ();
    }

    // get type name
    char const *typenm = tok->getSym ();
    if (typenm == nullptr) {
        if (signtok == nullptr) {
            throwerror (tok, "expecting type name");
        }
        pushtoken (tok);
        tok = signtok;
        typenm = "int";
    }

    // add 'signed ' or 'unsigned ' prefix to type name
    // if used on anything other than builtin integer types, will result in type not defined error
    if (signtok != nullptr) {
        char *signnm = new char[10+strlen(typenm)];
        switch (signtok->getKw ()) {
            case KW_SIGNED: {
                strcpy (signnm, "signed ");
                break;
            }
            case KW_UNSIGNED: {
                strcpy (signnm, "unsigned ");
                break;
            }
            default: assert_false ();
        }
        strcat (signnm, typenm);
        typenm = signnm;
    }

    // look it up then make sure it is a type
    // - builtin types (such as 'double'): gets the corresponding Type object in the globalscope
    // - struct/union types: gets the corresponding StructType object
    // - function types: gets the corresponding FuncType object
    // - typedef types: gets the equivalent type
    Decl *typedecl = lookupdecl (typenm);
    if (typedecl == nullptr) {
        throwerror (tok, "undefined type '%s'", typenm);
    }
    Type *type = typedecl->getEquivType ();
    if (type == nullptr) {
        throwerror (tok, "expecting type at '%s'", typenm);
    }
    return type;
}

// if variable is a struct or array of struct var, output code to initialize any vtable pointers
// assume there is no initialization expression present
void Scope::maybeoutputstructinit (VarDecl *vardecl)
{
    Keywd storclass = vardecl->getStorClass ();
    if ((storclass != KW_EXTERN) && (storclass != KW_TYPEDEF)) {
        bool isarray = vardecl->isArray ();
        Type *vartype = vardecl->getType ();
        StructType *st = (isarray ? vartype->stripCVMod ()->castPtrType ()->getBaseType () : vartype)->stripCVMod ()->castStructType ();
        if ((st != nullptr) && st->hasinitfunc ()) {
            Token *varnametok = vardecl->getDefTok ();
            if (isarray) {
                tsize_t arrsize = vardecl->getArrSize ();
                ArrayInitExpr *aix = new ArrayInitExpr (this, varnametok);
                aix->valuetype = st;
                aix->numarrayelems = arrsize;
                for (tsize_t index = 0; index < arrsize; index ++) {
                    StructInitExpr *six = new StructInitExpr (this, varnametok);
                    six->structype = st;
                    std::pair<tsize_t,Expr *> pair (index, six);
                    aix->exprs.push_back (pair);
                }
                vardecl->setInitExpr (aix);
            } else {
                StructInitExpr *six = new StructInitExpr (this, varnametok);
                six->structype = st;
                vardecl->setInitExpr (six);
            }
        }
    }
}

// helpers for parsexpression() below

#define exstack_pop_back() ({ assert (! exstack.empty ()); Expr *ex = exstack.back (); exstack.pop_back (); ex; })

// - see if the next tokens in the stream are the start of a time
//   excludes structtype '::' cuz that's the start of a variable
//   pushes all consumed tokens back to the stream
bool Scope::istypenext ()
{
    Token *tok1 = gettoken ();
    if (tok1->getSym () != nullptr) {
        Token *tok2 = gettoken ();
        if (tok2->isKw (KW_COLON2)) {
            pushtoken (tok2);
            pushtoken (tok1);
            return false;
        }
        pushtoken (tok2);
    }
    pushtoken (tok1);
    return istypetoken (tok1);
}

// - see if the token is the start of a type
bool Scope::istypetoken (Token *tok)
{
    switch (tok->getKw ()) {
        case KW_CONST:
        case KW_SIGNED:
        case KW_STRUCT:
        case KW_UNION:
        case KW_UNSIGNED:
        case KW_VOLATILE: return true;
        default: break;
    }
    char const *name = tok->getSym ();
    if (name == nullptr) return false;
    Decl *decl = this->lookupdecl (name);
    if (decl == nullptr) return false;
    return decl->getEquivType () != nullptr;
}

// parse variable initialization expression
Expr *Scope::parseinitexpr (Type *vartype, bool isarray, tsize_t *arrsize_r)
{
    // character array initialized from a quoted string
    //  vartype = char pointer type
    if (isarray && (vartype->stripCVMod ()->castPtrType ()->getBaseType ()->stripCVMod () == chartype)) {
        Token *tok = gettoken ();
        StrToken *strtok = tok->castStrToken ();
        if (strtok != nullptr) {
            strtok->extend ();
            if (*arrsize_r == 0) *arrsize_r = strtok->getStrlen () + 1;
            StringInitExpr *six = new StringInitExpr (this, tok);
            six->strtok  = strtok;
            six->arrsize = *arrsize_r;
            return six;
        }
        pushtoken (tok);
    }

    // other arrays initialized by '{' ... '}'
    //  vartype = some pointer type
    if (isarray) {
        Token *tok = gettoken ();
        if (! tok->isKw (KW_OBRACE)) {
            throwerror (tok, "expecting '{' at beginning of array initialization");
        }
        Type *valtype = vartype->stripCVMod ()->castPtrType ()->getBaseType ()->stripCVMod ();
        ArrayInitExpr *aix = new ArrayInitExpr (this, tok);
        aix->valuetype = valtype;
        tsize_t highestindex = 0;
        tsize_t index = 0;
        while (true) {
            tok = gettoken ();
            if (tok->isKw (KW_CBRACE)) break;
            if (tok->isKw (KW_OBRKT)) {
                index = parseintconstexpr ();
                tok = gettoken ();
                if (! tok->isKw (KW_CBRKT) || ! gettoken ()->isKw (KW_EQUAL)) {
                    throwerror (tok, "expecting '] =' at end of array init subscript");
                }
            } else {
                pushtoken (tok);
            }
            //TODO arrays of arrays
            std::pair<tsize_t,Expr *> pair (index, parseinitexpr (valtype, false, nullptr));
            aix->exprs.push_back (pair);
            if (highestindex < ++ index) highestindex = index;
            tok = gettoken ();
            if (! tok->isKw (KW_COMMA)) break;
        }
        if (! tok->isKw (KW_CBRACE)) {
            throwerror (tok, "expecting '}' or ',' in array initialization");
        }
        if (*arrsize_r == 0) *arrsize_r = highestindex;
        aix->numarrayelems = *arrsize_r;
        return aix;
    }

    // structs/unions initialized by '{' ... '}'
    //  vartype = some struct/union type
    Token *tok = gettoken ();
    StructType *structype = vartype->stripCVMod ()->castStructType ();
    if ((structype != nullptr) && tok->isKw (KW_OBRACE)) {
        StructInitExpr *six = new StructInitExpr (this, tok);
        six->structype = structype;
        tsize_t index = 0;
        while (true) {

            // check for '}' meaning no more members
            tok = gettoken ();
            if (tok->isKw (KW_CBRACE)) break;

            // check for '.' member
            StructType *memstruct;
            VarDecl *memvar;
            tsize_t memoffset;
            if (tok->isKw (KW_DOT)) {
                tok = gettoken ();
                char const *name = tok->getSym ();
                if ((name == nullptr) || ! gettoken ()->isKw (KW_EQUAL)) {
                    throwerror (tok, "expecting 'name =' for struct/union init member");
                }
                FuncDecl *memfunc;
                if (! structype->getMemberByName (tok, nullptr, name, &memstruct, &memfunc, &memvar, &memoffset, &index)) {
                    throwerror (tok, "unknown struct/union '%s' member '%s'", structype->getName (), name);
                }
                if (memfunc != nullptr) {
                    throwerror (tok, "struct/union '%s' member function '%s' not initializeable", structype->getName (), name);
                }
            } else {
                if (! structype->getMemberByIndex (tok, index, &memstruct, &memvar, &memoffset)) {
                    throwerror (tok, "too many initialization values");
                }
                pushtoken (tok);
            }

            // parse the initialization value expression and save with member index
            tsize_t memarrsize = memvar->isArray () ? memvar->getArrSize () : 0;
            Expr *iexpr = parseinitexpr (memvar->getType (), memvar->isArray (), &memarrsize);
            if (memvar->isArray ()) memvar->setArrSize (memarrsize);
            auto ok = six->exprs.insert ({ index, iexpr });
            if (! ok.second) printerror (iexpr->exprtoken, "duplicate init of index %u", index);

            // skip over the following comma or close brace
            index ++;
            tok = gettoken ();
            if (! tok->isKw (KW_COMMA)) break;
        }
        if (! tok->isKw (KW_CBRACE)) {
            throwerror (tok, "expecting '}' or ',' in struct/union initialization");
        }
        return six;
    }

    // everything else gets simple initialization expressions
    //  vartype = some scalar (numeric, pointer) type
    pushtoken (tok);
    return castToType (tok, vartype, parsexpression (true, false));
}

// parse expression, reduce to compile-time integer constant
tsize_t Scope::parseintconstexpr ()
{
    Expr *expr = parsexpression (false, false);
    NumValue nv;
    NumCat nc = expr->isNumConstExpr (&nv);
    switch (nc) {
        case NC_SINT: return nv.s;
        case NC_UINT: return nv.u;
        default: {
            throwerror (expr->exprtoken, "requires compile-time integer constant");
        }
    }
}

// parse expression
//  input:
//   stoponcomma = ',' treated as delimeter
//   stoponcolon = ':' treated as delimeter
//  output:
//   returns parsed expression
//   token stream points at terminating token
Expr *Scope::parsexpression (bool stoponcomma, bool stoponcolon)
{
    std::vector<Type *> cxstack;
    std::vector<Expr *> exstack;
    std::vector<std::pair<Token *,Opcode>> opstack;

    while (true) {

        // get operand then push on exstack
        // also push any prefix unary operators on opstack
        Token *tok = gettoken ();
        switch (tok->getKw ()) {

            // special processing for 'alignof (type)'
            // ...though it might be 'alignof (expr)'
            case KW_ALIGNOF: {
                Token *opartok = gettoken ();
                if (opartok->isKw (KW_OPAREN)) {
                    Token *typetok = gettoken ();
                    if (this->istypetoken (typetok)) {

                        // 'alignof (type)' - push an SizeofExpr expression
                        pushtoken (typetok);
                        SizeofExpr *sx = new SizeofExpr (this, tok);
                        sx->opcode  = OP_ALIGNOF;
                        sx->subtype = this->parsetypecast ();
                        Token *cpartok = gettoken ();
                        if (! cpartok->isKw (KW_CPAREN)) {
                            throwerror (typetok, "expecting ')' at end of '(' type ')'");
                        }
                        exstack.push_back (sx);
                        break;
                    }
                    pushtoken (typetok);
                }
                pushtoken (opartok);

                // 'alignof expr' - push a OP_ALIGNOF unary operator
                opstack.emplace_back (tok, OP_ALIGNOF);
                continue;
            }

            case KW_AMPER: {
                opstack.emplace_back (tok, OP_ADDROF);
                continue;
            }

            case KW_EXCLAM: {
                opstack.emplace_back (tok, OP_LOGNOT);
                continue;
            }

            case KW_MINUS: {
                opstack.emplace_back (tok, OP_NEGATE);
                continue;
            }

            case KW_MINUSMINUS: {
                opstack.emplace_back (tok, OP_PREDEC);
                continue;
            }

            // special processing for 'new basetype[arrsubscr]'
            //  structtype::__init (malloc2 (numelem, size), numelem)
            case KW_NEW: {

                // parse basetype and optional [arrsubscr]
                Type *basetype  = this->parsetypemods (this->parsecvmoddedtype ());
                Expr *arrsubscr = nullptr;
                Token *obkttok  = gettoken ();
                if (obkttok->isKw (KW_OBRKT)) {
                    arrsubscr = this->parsexpression (false, false);
                    Token *cbkttok = gettoken ();
                    if (! cbkttok->isKw (KW_CBRKT)) {
                        throwerror (cbkttok, "expecting ']' at end of subscript");
                    }
                    arrsubscr = castToType (cbkttok, sizetype, arrsubscr);
                } else {
                    pushtoken (obkttok);
                    arrsubscr = ValExpr::createsizeint (this, tok, 1, false);
                }

                // call malloc2(nmemb,size)
                CallExpr *callmalloc2 = new CallExpr (this, tok);
                callmalloc2->funcexpr = new ValExpr (this, tok, malloc2func);
                callmalloc2->argexprs.push_back (arrsubscr);
                callmalloc2->argexprs.push_back (ValExpr::createsizeint (this, tok, basetype->getTypeSize (tok), false));

                // cast result to (basetype *)
                Expr *mempointer = this->castToType (tok, basetype->getPtrType (), callmalloc2);

                // maybe it's a struct with an init function (fills in any vtable pointers and calls any constructors)
                mempointer = basetype->callInitFunc (this, tok, mempointer);

                // value of the new ... is the memory pointer, casted to requested type
                exstack.push_back (mempointer);
                break;
            }

            case KW_OPAREN: {
                Token *ttok = gettoken ();
                if (this->istypetoken (ttok)) {
                    pushtoken (ttok);
                    Type *totype = this->parsetypecast ();
                    ttok = gettoken ();
                    if (! ttok->isKw (KW_CPAREN)) {
                        throwerror (ttok, "expecting ')' at end of cast-to type");
                    }
                    cxstack.push_back (totype);
                    opstack.emplace_back (tok, OP_CCAST);
                    continue;
                }
                if (ttok->isKw (KW_OBRACE)) {
                    exstack.push_back (this->parseblockexpr (ttok));
                } else {
                    pushtoken (ttok);
                    Expr *subexpr = this->parsexpression (false, false);
                    exstack.push_back (subexpr);
                }
                tok = gettoken ();
                if (! tok->isKw (KW_CPAREN)) {
                    throwerror (tok, "expecting ')' at end of '(' expression ')'");
                }
                break;
            }

            case KW_PLUS: {
                opstack.emplace_back (tok, OP_MOVE);
                continue;
            }

            case KW_PLUSPLUS: {
                opstack.emplace_back (tok, OP_PREINC);
                continue;
            }

            // special processing for 'sizeof (type)'
            // ...though it might be 'sizeof (expr)'
            case KW_SIZEOF: {
                Token *opartok = gettoken ();
                if (opartok->isKw (KW_OPAREN)) {
                    Token *typetok = gettoken ();
                    if (this->istypetoken (typetok)) {

                        // 'sizeof (type)' - push a SizeofExpr expression
                        pushtoken (typetok);
                        SizeofExpr *sx = new SizeofExpr (this, tok);
                        sx->opcode  = OP_SIZEOF;
                        sx->subtype = this->parsetypecast ();
                        Token *cpartok = gettoken ();
                        if (! cpartok->isKw (KW_CPAREN)) {
                            throwerror (typetok, "expecting ')' at end of '(' type ')'");
                        }
                        exstack.push_back (sx);
                        break;
                    }
                    pushtoken (typetok);
                }
                pushtoken (opartok);

                // 'sizeof expr' - push a OP_SIZEOF unary operator
                opstack.emplace_back (tok, OP_SIZEOF);
                continue;
            }

            case KW_STAR: {
                opstack.emplace_back (tok, OP_DEREF);
                continue;
            }

            case KW_TILDE: {
                opstack.emplace_back (tok, OP_BITNOT);
                continue;
            }

            default: {
                switch (tok->getTType ()) {

                    // numeric constant
                    case TT_NUM: {
                        NumToken *numtok = tok->castNumToken ();
                        ValExpr *vx = new ValExpr (this, tok, NumDecl::create (numtok));
                        exstack.push_back (vx);
                        break;
                    }

                    // string constant
                    case TT_STR: {
                        StrToken *strtok = tok->castStrToken ();
                        strtok->extend ();
                        ValExpr *vx = new ValExpr (this, tok, StrDecl::create (strtok));
                        exstack.push_back (vx);
                        break;
                    }

                    // symbol (variable or function)
                    case TT_SYM: {
                        ValExpr *vx = new ValExpr (this, tok, nullptr);

                        // being part of an expression, it should be defined at this point
                        char const *symname = tok->getSym ();
                        Decl *decl = this->lookupdecl (symname);
                        if (decl == nullptr) {
                            throwerror (tok, "undefined variable '%s'", symname);
                        }

                        // if followed by '::', it is referencing a static struct member
                        Token *cctok = gettoken ();
                        if (cctok->isKw (KW_COLON2)) {
                            StructType *structtype = decl->castStructType ();
                            if (structtype == nullptr) {
                                throwerror (tok, "expecting struct type left of '::' instead of '%s'", symname);
                            }

                            // following the '::' is the static struct member
                            Token *tok2 = gettoken ();
                            char const *name2 = tok2->getSym ();
                            if (name2 == nullptr) {
                                throwerror (tok2, "expecting struct '%s' member name right of '::'", symname);
                            }

                            // see if member can be found in struct
                            StructType *memstruct;
                            FuncDecl *memfuncdecl;
                            VarDecl *memvardecl;
                            tsize_t memoffset;
                            tsize_t memindex;
                            if (! structtype->getMemberByName (tok2, nullptr, name2, &memstruct, &memfuncdecl, &memvardecl, &memoffset, &memindex)) {
                                throwerror (tok2, "struct '%s' does not contain member '%s' ", symname, name2);
                            }
                            vx->valdecl = (memfuncdecl != nullptr) ? (ValDecl *) memfuncdecl : (ValDecl *) memvardecl;
                            if (vx->valdecl->getStorClass () != KW_STATIC) {
                                printerror (tok2, "struct '%s' member '%s' is not static", symname, name2);
                            }
                        } else {

                            // no '::', just a normal variable
                            pushtoken (cctok);
                            vx->valdecl = decl->castValDecl ();
                            if (vx->valdecl == nullptr) {
                                throwerror (tok, "expecting function/variable at this point");
                            }
                        }
                        exstack.push_back (vx);
                        break;
                    }
                    default: {
                        throwerror (tok, "unknown expression operand");
                    }
                }
                break;
            }
        }

        // get binary operator (or post unary operators)
        Opcode binop;
    getbop:
        tok = gettoken ();
        switch (tok->getKw ()) {
            case KW_AMPER:      { binop = OP_BITAND;  break; }
            case KW_ANDAND:     { binop = OP_LOGAND;  break; }
            case KW_ANDEQ:      { binop = OP_ASNAND;  break; }
            case KW_CIRCUM:     { binop = OP_BITXOR;  break; }
            case KW_CIRCUMEQ:   { binop = OP_ASNXOR;  break; }
            case KW_DOT:        { binop = OP_MEMDOT;  break; }
            case KW_EQEQ:       { binop = OP_CMPEQ;   break; }
            case KW_EQUAL:      { binop = OP_ASNEQ;   break; }
            case KW_GTANG:      { binop = OP_CMPGT;   break; }
            case KW_GTEQ:       { binop = OP_CMPGE;   break; }
            case KW_GTGT:       { binop = OP_SHR;     break; }
            case KW_GTGTEQ:     { binop = OP_ASNSHR;  break; }
            case KW_LTANG:      { binop = OP_CMPLT;   break; }
            case KW_LTEQ:       { binop = OP_CMPLE;   break; }
            case KW_LTLT:       { binop = OP_SHL;     break; }
            case KW_LTLTEQ:     { binop = OP_ASNSHL;  break; }
            case KW_MINUS:      { binop = OP_SUB;     break; }
            case KW_MINUSEQ:    { binop = OP_ASNSUB;  break; }
            case KW_MINUSMINUS: { binop = OP_POSTDEC; break; }
            case KW_NOTEQ:      { binop = OP_CMPNE;   break; }
            case KW_OBRKT:      { binop = OP_SUBSCR;  break; }
            case KW_OPAREN:     { binop = OP_FUNCALL; break; }
            case KW_ORBAR:      { binop = OP_BITOR;   break; }
            case KW_OREQ:       { binop = OP_ASNOR;   break; }
            case KW_OROR:       { binop = OP_LOGOR;   break; }
            case KW_PERCENT:    { binop = OP_MOD;     break; }
            case KW_PERCENTEQ:  { binop = OP_ASNMOD;  break; }
            case KW_PLUS:       { binop = OP_ADD;     break; }
            case KW_PLUSEQ:     { binop = OP_ASNADD;  break; }
            case KW_PLUSPLUS:   { binop = OP_POSTINC; break; }
            case KW_POINT:      { binop = OP_MEMPTR;  break; }
            case KW_QMARK:      { binop = OP_QMARK;   break; }
            case KW_SLASH:      { binop = OP_DIV;     break; }
            case KW_SLASHEQ:    { binop = OP_ASNDIV;  break; }
            case KW_STAR:       { binop = OP_MUL;     break; }
            case KW_STAREQ:     { binop = OP_ASNMUL;  break; }

            case KW_COLON: {
                if (stoponcolon) {
                    pushtoken (tok);
                    binop = OP_END;
                } else {
                    binop = OP_COLON;
                }
                break;
            }

            case KW_COMMA: {
                if (stoponcomma) {
                    pushtoken (tok);
                    binop = OP_END;
                } else {
                    binop = OP_COMMA;
                }
                break;
            }

            // don't know what it is, assume it is end of expression
            default: {
                pushtoken (tok);
                binop = OP_END;
                break;
            }
        }

        // process stacked operators that have higher or same precedence as new operator
        while (! opstack.empty ()) {
            Opcode stackedop = opstack.back ().second;
            if (PRECED (stackedop) < PRECED (binop)) break;
            if (PRRTOL (stackedop) && (PRECED (stackedop) == PRECED (binop))) break;
            Token *stackedtok = opstack.back ().first;
            opstack.pop_back ();

            switch (stackedop) {

                case OP_COLON: {
                    if (opstack.empty () || (opstack.back ().second != OP_QMARK)) {
                        throwerror (stackedtok, "colon without matching question mark");
                    }
                    opstack.pop_back ();
                    Expr *falsex = exstack_pop_back ();
                    Expr *trueex = exstack_pop_back ();
                    Expr *condex = exstack_pop_back ();
                    Type *truetype = trueex->getType ()->stripCVMod ();
                    Type *falstype = falsex->getType ()->stripCVMod ();
                    if ((truetype->castNumType () != nullptr) && (falstype->castNumType () != nullptr)) {
                        Type *restype = getStrongerArith (stackedtok, truetype, falstype);
                        trueex = castToType (stackedtok, restype, trueex);
                        falsex = castToType (stackedtok, restype, falsex);
                    }
                    QMarkExpr *qx = new QMarkExpr (this, stackedtok);
                    qx->falsexpr  = falsex;
                    qx->trueexpr  = trueex;
                    qx->condexpr  = condex;
                    exstack.push_back (qx);
                    break;
                }

                case OP_QMARK: {
                    throwerror (stackedtok, "question mark without matching colon");
                }

                // copy right-hand operand to left-hand operand
                case OP_ASNEQ: {
                    Expr *riteexpr = exstack_pop_back ();
                    Expr *leftexpr = exstack_pop_back ();
                    Type *lefttype = leftexpr->getType ();
                    Type *ritetype = riteexpr->getType ();

                    checkAsneqCompat (stackedtok, lefttype, ritetype);

                    AsnopExpr *ax  = new AsnopExpr (this, stackedtok);
                    ax->leftexpr   = leftexpr;
                    ax->riteexpr   = castToType (stackedtok, lefttype, riteexpr);
                    exstack.push_back (ax);
                    break;
                }

                // pointer +-= scaled integer
                // numeric +-= numeric
                case OP_ASNADD:
                case OP_ASNSUB:
                case OP_ASNSHL:
                case OP_ASNSHR:
                case OP_ASNMOD:
                case OP_ASNAND:
                case OP_ASNXOR:
                case OP_ASNOR:
                case OP_ASNMUL:
                case OP_ASNDIV: {
                    Expr *riteexpr = exstack_pop_back ();
                    Expr *leftexpr = exstack_pop_back ();
                    Type *lefttype = leftexpr->getType ()->stripCVMod ();

                    // get opcode suitable for BinopExpr
                    Opcode binopcode;
                    switch (stackedop) {
                        case OP_ASNADD: binopcode = OP_ADD; break;
                        case OP_ASNSUB: binopcode = OP_SUB; break;
                        case OP_ASNSHL: binopcode = OP_SHL; break;
                        case OP_ASNSHR: binopcode = OP_SHR; break;
                        case OP_ASNMOD: binopcode = OP_MOD; break;
                        case OP_ASNAND: binopcode = OP_BITAND; break;
                        case OP_ASNXOR: binopcode = OP_BITXOR; break;
                        case OP_ASNOR:  binopcode = OP_BITOR;  break;
                        case OP_ASNMUL: binopcode = OP_MUL; break;
                        case OP_ASNDIV: binopcode = OP_DIV; break;
                        default: assert_false ();
                    }

                    // maybe left is *(...)
                    UnopExpr *leftunopexpr = leftexpr->castUnopExpr ();
                    if ((leftunopexpr != nullptr) && (leftunopexpr->opcode == OP_DEREF)) {

                        // left is a dereference, get pointer into temp for multiple uses in case it is complicated
                        VarDecl *leftptrtemp = leftunopexpr->newTempVarDecl (leftunopexpr->subexpr->getType ());

                        AsnopExpr *px = new AsnopExpr (this, stackedtok);
                        px->leftexpr  = new ValExpr (this, stackedtok, leftptrtemp);
                        px->riteexpr  = leftunopexpr->subexpr;

                        // extract old value by reading from the pointer
                        UnopExpr *ux = new UnopExpr (this, stackedtok);
                        ux->opcode   = OP_DEREF;
                        ux->subexpr  = px;
                        ux->restype  = lefttype;

                        // perform arithmetic using that old value
                        Expr *bx = newBinopArith (stackedtok, binopcode, ux, riteexpr);

                        // store value back using the temp pointer
                        UnopExpr *xx = new UnopExpr (this, stackedtok);
                        xx->opcode   = OP_DEREF;
                        xx->subexpr  = new ValExpr (this, stackedtok, leftptrtemp);
                        xx->restype  = lefttype;

                        AsnopExpr *ax = new AsnopExpr (this, stackedtok);
                        ax->leftexpr  = xx;
                        ax->riteexpr  = castToType (stackedtok, lefttype, bx);

                        // that stored value is the expression's value
                        exstack.push_back (ax);
                    } else {

                        // left is simple variable, compute arithmetic using old variable contents
                        Expr *bx = newBinopArith (stackedtok, binopcode, leftexpr, riteexpr);

                        // store result back in same variable
                        AsnopExpr *ax = new AsnopExpr (this, stackedtok);
                        ax->leftexpr  = leftexpr;
                        ax->riteexpr  = castToType (stackedtok, lefttype, bx);

                        // that stored value is the expression's value
                        exstack.push_back (ax);
                    }
                    break;
                }

                // both operands can be any type
                // no auto-casting
                case OP_COMMA: {
                    BinopExpr *bx = new BinopExpr (this, stackedtok);
                    bx->opcode    = stackedop;
                    bx->riteexpr  = exstack_pop_back ();
                    bx->leftexpr  = exstack_pop_back ();
                    bx->restype   = bx->riteexpr->getType ();
                    exstack.push_back (bx);
                    break;
                }

                // cast each opeand to boolean
                // the operands must be numerics or pointers
                case OP_LOGOR:
                case OP_LOGAND: {
                    Expr *riteexpr = exstack_pop_back ();
                    Expr *leftexpr = exstack_pop_back ();
                    BinopExpr *bx  = new BinopExpr (this, stackedtok);
                    bx->opcode     = stackedop;
                    bx->leftexpr   = castToType (stackedtok, booltype, leftexpr);
                    bx->riteexpr   = castToType (stackedtok, booltype, riteexpr);
                    bx->restype    = booltype;
                    exstack.push_back (bx);
                    break;
                }

                // perform arithmetic on two stacked operands, push result back on stack
                case OP_ADD:
                case OP_SUB:
                case OP_SHL:
                case OP_SHR:
                case OP_BITOR:
                case OP_BITXOR:
                case OP_BITAND:
                case OP_DIV:
                case OP_MUL:
                case OP_MOD: {
                    Expr *riteexpr = exstack_pop_back ();
                    Expr *leftexpr = exstack_pop_back ();
                    exstack.push_back (newBinopArith (stackedtok, stackedop, leftexpr, riteexpr));
                    break;
                }

                // both operands must be numeric (ints, floats) or compatible pointers
                // weaker auto-cast to stronger
                case OP_CMPEQ:
                case OP_CMPNE:
                case OP_CMPGE:
                case OP_CMPGT:
                case OP_CMPLE:
                case OP_CMPLT: {
                    Expr *riteexpr = exstack_pop_back ();
                    Expr *leftexpr = exstack_pop_back ();
                    Type *lefttype = leftexpr->getType ()->stripCVMod ();
                    Type *ritetype = riteexpr->getType ()->stripCVMod ();

                    // check for pointer comparison
                    PtrType *leftptrtype = lefttype->castPtrType ();
                    PtrType *riteptrtype = ritetype->castPtrType ();
                    if (leftptrtype != nullptr) {
                        if ((leftptrtype != voidtype->getPtrType ()) && (riteptrtype != voidtype->getPtrType ()) && (leftptrtype != riteptrtype)) {
                            throwerror (stackedtok, "compare pointers of different types '%s' vs '%s'", lefttype->getName (), ritetype->getName ());
                        }
                        BinopExpr *bx = new BinopExpr (this, stackedtok);
                        bx->opcode    = stackedop;
                        bx->leftexpr  = castToType (stackedtok, sizetype, leftexpr);
                        bx->riteexpr  = castToType (stackedtok, sizetype, riteexpr);
                        bx->restype   = booltype;
                        exstack.push_back (bx);
                        break;
                    }

                    // must be numerics, promote weaker type to stronger type
                    Type *restype = getStrongerArith (stackedtok, lefttype, ritetype);
                    BinopExpr *bx = new BinopExpr (this, stackedtok);
                    bx->opcode    = stackedop;
                    bx->leftexpr  = castToType (stackedtok, restype, leftexpr);
                    bx->riteexpr  = castToType (stackedtok, restype, riteexpr);
                    bx->restype   = booltype;
                    exstack.push_back (bx);
                    break;
                }

                // operand must be a numeric or a pointer
                case OP_PREINC: {
                    exstack.push_back (preIncDec (stackedtok, OP_ADD, exstack_pop_back ()));
                    break;
                }
                case OP_PREDEC: {
                    exstack.push_back (preIncDec (stackedtok, OP_SUB, exstack_pop_back ()));
                    break;
                }

                case OP_ALIGNOF:
                case OP_SIZEOF: {
                    SizeofExpr *sx = new SizeofExpr (this, stackedtok);
                    sx->opcode  = stackedop;
                    sx->subexpr = exstack_pop_back ();
                    exstack.push_back (sx);
                    break;
                }

                // any arithmetic type, no casting needed
                case OP_MOVE:
                case OP_NEGATE: {
                    Expr *subexpr = exstack_pop_back ();
                    Type *subtype = subexpr->getType ()->stripCVMod ();
                    FloatType *flttype = subtype->castFloatType ();
                    IntegType *inttype = subtype->castIntegType ();
                    if ((flttype == nullptr) && (inttype == nullptr)) {
                        throwerror (stackedtok, "operand must be numeric, not '%s'", subtype->getName ());
                    }
                    UnopExpr *ux = new UnopExpr (this, stackedtok);
                    ux->opcode  = stackedop;
                    ux->subexpr = subexpr;
                    ux->restype = subtype;
                    exstack.push_back (ux);
                    break;
                }

                // any integer arithmetic type, no casting needed
                case OP_BITNOT: {
                    Expr *subexpr = exstack_pop_back ();
                    Type *subtype = subexpr->getType ()->stripCVMod ();
                    IntegType *inttype = subtype->castIntegType ();
                    if (inttype == nullptr) {
                        throwerror (stackedtok, "operand must be integer numeric, not '%s'", subtype->getName ());
                    }
                    UnopExpr *ux = new UnopExpr (this, stackedtok);
                    ux->opcode  = stackedop;
                    ux->subexpr = subexpr;
                    ux->restype = subtype;
                    exstack.push_back (ux);
                    break;
                }

                // any arithmetic or pointer type, cast to boolean
                case OP_LOGNOT: {
                    Expr *subexpr = exstack_pop_back ();
                    Type *subtype = subexpr->getType ()->stripCVMod ();
                    FloatType *flttype = subtype->castFloatType ();
                    IntegType *inttype = subtype->castIntegType ();
                    PtrType   *ptrtype = subtype->castPtrType ();
                    if ((flttype == nullptr) && (inttype == nullptr) && (ptrtype == nullptr)) {
                        throwerror (stackedtok, "operand must be numeric or pointer, not '%s'", subtype->getName ());
                    }
                    UnopExpr *ux = new UnopExpr (this, stackedtok);
                    ux->opcode  = OP_LOGNOT;
                    ux->subexpr = castToType (stackedtok, booltype, subexpr);
                    ux->restype = booltype;
                    exstack.push_back (ux);
                    break;
                }

                // operand must be a pointer, no casting
                case OP_DEREF: {
                    Expr *subexpr = exstack_pop_back ();
                    Type *subtype = subexpr->getType ()->stripCVMod ();
                    PtrType *ptrtype = subtype->castPtrType ();
                    if (ptrtype == nullptr) {
                        throwerror (stackedtok, "operand must be pointer, not '%s'", subtype->getName ());
                    }
                    UnopExpr *ux = new UnopExpr (this, stackedtok);
                    ux->opcode  = OP_DEREF;
                    ux->subexpr = subexpr;
                    ux->restype = ptrtype->getBaseType ();
                    exstack.push_back (ux);
                    break;
                }

                // operand can be any value expression, except numeric/string constants or whole array
                case OP_ADDROF: {
                    Expr *subexpr = exstack_pop_back ();

                    // &*x => x
                    UnopExpr *subunop = subexpr->castUnopExpr ();
                    if ((subunop != nullptr) && (subunop->opcode == OP_DEREF)) {
                        exstack.push_back (subunop->subexpr);
                        break;
                    }

                    // get address of variable
                    UnopExpr *ux = new UnopExpr (this, stackedtok);
                    ux->opcode  = OP_ADDROF;
                    ux->subexpr = subexpr;
                    ux->restype = subexpr->getType ()->getPtrType ();
                    exstack.push_back (ux);
                    break;
                }

                case OP_CCAST: {
                    assert (! cxstack.empty ());
                    Type *totype = cxstack.back ();
                    Expr *subexpr = exstack_pop_back ();
                    cxstack.pop_back ();
                    exstack.push_back (castToType (stackedtok, totype, subexpr));
                    break;
                }

                default: assert_false ();
            }
        }

        // if end of expression, the opstack should be empty as OP_END has lowest precedence
        // ...and so also the exstack should have exactly one element
        if (binop == OP_END) break;

        // some operators handle their right-hand operands specially
        switch (binop) {

            // saw a '(' for making a function call
            // function name/expr is on exstack
            // arguments yet to be parsed from source
            case OP_FUNCALL: {

                // get function expression, eg, direct function name or pointer to function
                Expr *funcexpr = exstack_pop_back ();

                // if pointer without the '*', splice in the '*' to get the function itself
                Type *exprtype = funcexpr->getType ()->stripCVMod ();
                PtrType *ptrtype = exprtype->castPtrType ();
                if (ptrtype != nullptr) {
                    exprtype = ptrtype->getBaseType ();
                    UnopExpr *dx = new UnopExpr (this, tok);
                    dx->opcode   = OP_DEREF;
                    dx->subexpr  = funcexpr;
                    dx->restype  = exprtype;
                    funcexpr = dx;
                }

                // now we should have a function type
                FuncType *functype = exprtype->stripCVMod ()->castFuncType ();
                if (functype == nullptr) {
                    throwerror (tok, "calling non-function type '%s'", exprtype->getName ());
                }

                // get function entrypoint
                CallExpr *cx = new CallExpr (this, tok);
                cx->funcexpr = funcexpr;

                // get arguments
                Token *argtok = gettoken ();
                if (! argtok->isKw (KW_CPAREN)) {
                    pushtoken (argtok);
                    do {

                        // first token of argument expression for error messages
                        argtok = gettoken ();
                        pushtoken (argtok);

                        // compute argument value
                        Expr *argexpr = this->parsexpression (true, false);

                        // maybe cast argument to parameter type
                        tsize_t i = cx->argexprs.size ();
                        if (i < functype->getNumParams ()) {
                            Type *argtype = argexpr->getType ();
                            Type *prmtype = functype->getParamType (i);
                            checkAsneqCompat (argtok, prmtype, argtype);
                            argexpr = castToType (argtok, prmtype, argexpr);

                            assert (argexpr->getType ()->getTypeAlign (argtok) == functype->getParamType (i)->getTypeAlign (argtok));
                            assert (argexpr->getType ()->getTypeSize  (argtok) == functype->getParamType (i)->getTypeSize  (argtok));
                        } else if (! functype->hasVarArgs ()) {
                            throwerror (argtok, "too many arguments to function");
                        }

                        // add argument to argument list
                        cx->argexprs.push_back (argexpr);

                        // check for more arguments
                        argtok = gettoken ();
                    } while (argtok->isKw (KW_COMMA));
                    if (! argtok->isKw (KW_CPAREN)) {
                        throwerror (argtok, "expecting ')' at end of function call arguments");
                    }
                }

                // make sure we have enough arguments
                if (cx->argexprs.size () < functype->getNumParams ()) {
                    throwerror (argtok, "not enough arguments to function");
                }

                // push the call on the operand stack, the value is the function's return value
                exstack.push_back (cx);
                goto getbop;
            }

            // saw '.' or '->' for accessing struct/union members
            // struct/union or pointer is on exstack
            case OP_MEMDOT:             // .
            case OP_MEMPTR: {           // ->

                // get struct or pointer to struct
                Expr *structexpr = exstack_pop_back ();

                // get struct's type
                Type *exptype = structexpr->getType ()->stripCVMod ();
                StructType *structtype = exptype->castStructType ();
                PtrType *ptrtype = exptype->castPtrType ();
                if (ptrtype != nullptr) {
                    structtype = ptrtype->getBaseType ()->stripCVMod ()->castStructType ();
                }
                if (structtype == nullptr) {
                    throwerror (tok, "must have struct/union or pointer to such, not '%s'", exptype->getName ());
                }
                if (! structtype->isDefined ()) {
                    throwerror (tok, "attempting to access struct/union '%s' members when not yet defined", structtype->getName ());
                }

                // get member name
                Token *memtoken  = gettoken ();
                SymToken *memsym = memtoken->castSymToken ();
                if (memsym == nullptr) {
                    throwerror (memtoken, "expecting struct/union member name");
                }

                // maybe member name is prefixed with parenttype::
                bool ignvirt = false;
                char const *parentstruct = nullptr;
                Token *cctok = gettoken ();
                if (! cctok->isKw (KW_COLON2)) {
                    pushtoken (cctok);
                } else {
                    ignvirt = true;
                    parentstruct = memsym->getSym ();
                    memtoken = gettoken ();
                    memsym = memtoken->castSymToken ();
                    if (memsym == nullptr) {
                        throwerror (memtoken, "expecting struct/union member name");
                    }
                }

                // look for it in struct type
                StructType *memstruct = nullptr;    // parent found in or same as structtype if in child
                FuncDecl   *memfunc   = nullptr;    // member function declaration
                VarDecl    *memvar    = nullptr;    // member variable declaration
                tsize_t     memoffs   = 0;          // offset from base of struct
                tsize_t     memindex  = 0;          // index of member in struct
                if (! structtype->getMemberByName (memtoken, parentstruct, memsym->getSym (), &memstruct, &memfunc, &memvar, &memoffs, &memindex)) {
                    throwerror (memtoken, "member '%s' not found in struct/union '%s'", memsym->getSym (), structtype->getName ());
                }

                // if we don't have pointer to struct/union, make pointer to struct/union
                if (ptrtype == nullptr) {
                    ptrtype      = structtype->getPtrType ();   //TODO preserve const/volatile attributes
                    UnopExpr *ax = new UnopExpr (this, tok);
                    ax->opcode   = OP_ADDROF;
                    ax->subexpr  = structexpr;
                    ax->restype  = ptrtype;
                    structexpr   = ax;
                }

                // maybe accessing member function (not a member that is a pointer to a function)
                if (memfunc != nullptr) {
                    MFuncExpr *mfx = new MFuncExpr (this, tok);
                    mfx->ptrexpr   = castToType (tok, memstruct->getPtrType (), structexpr);
                    mfx->memstruct = memstruct;
                    mfx->memfunc   = memfunc;
                    mfx->ignvirt   = ignvirt;
                    exstack.push_back (mfx);
                    goto getbop;
                }

                // cast pointer-to-struct type to a pointer-to-member type
                //  if we have something like:
                //    char member[80];
                //  ...we want char *
                //  if we have something like:
                //    int member;
                //  ...we want int *
                //  if we have something like:
                //    SomeOtherStruct member;
                //  ...we need to cast to void * first so it won't try rootward cast and add an offset or print error message
                CastExpr *cx = new CastExpr (this, tok);
                cx->casttype = memvar->isArray () ? memvar->getType () : memvar->getType ()->getPtrType ();  //TODO preserve const/volatile attributes
                if (memvar->getType ()->castStructType () == nullptr) {
                    cx->subexpr = structexpr;
                } else {
                    CastExpr *cv = new CastExpr (this, tok);
                    cv->casttype = voidtype->getPtrType ();
                    cv->subexpr  = structexpr;
                    cx->subexpr  = cv;
                }

                // add offset to pointer, type being pointer-to-member type
                // BinopExpr OP_ADD assumes caller has done any pointer arithmetic scaling already
                BinopExpr *bx = new BinopExpr (this, tok);
                bx->opcode    = OP_ADD;
                bx->leftexpr  = cx;
                bx->riteexpr  = ValExpr::createsizeint (this, tok, memoffs, false);
                bx->restype   = cx->casttype;

                if (memvar->isArray ()) {

                    // member is an array, use result of that add as a pointer to the array which is the value of the expression
                    exstack.push_back (bx);
                } else {

                    // dereference to access the member, type being the member's type
                    UnopExpr *dx  = new UnopExpr (this, tok);
                    dx->opcode    = OP_DEREF;
                    dx->subexpr   = bx;
                    dx->restype   = memvar->getType ();        //TODO preserve const/volatile attributes
                    exstack.push_back (dx);
                }
                goto getbop;
            }

            // saw '++' or '--' after a variable
            // variable is on exstack
            case OP_POSTINC: {
                exstack.push_back (postIncDec (tok, OP_ADD, exstack_pop_back ()));
                goto getbop;
            }
            case OP_POSTDEC: {
                exstack.push_back (postIncDec (tok, OP_SUB, exstack_pop_back ()));
                goto getbop;
            }

            // saw '[' for accessing array elements
            // array/pointer is on exstack
            case OP_SUBSCR: {

                // get expression inside '[' ... ']'
                Expr *riteexpr = this->parsexpression (false, false);
                Token *cbtok = gettoken ();
                if (! cbtok->isKw (KW_CBRKT)) {
                    throwerror (cbtok, "expecting ']' at end of subscript");
                }

                // get expression before brackets
                Expr *leftexpr = exstack_pop_back ();

                // get expression types
                Type *lefttype = leftexpr->getType ()->stripCVMod ();
                Type *ritetype = riteexpr->getType ()->stripCVMod ();
                IntegType *leftinttype = lefttype->castIntegType ();
                IntegType *riteinttype = ritetype->castIntegType ();
                PtrType   *leftptrtype = lefttype->castPtrType ();
                PtrType   *riteptrtype = ritetype->castPtrType ();

                // pointer + integer
                Expr *ptrplusint = nullptr;
                if ((leftptrtype != nullptr) && (riteinttype != nullptr)) {
                    ptrplusint = ptrPlusMinusInt (tok, OP_ADD, leftexpr, riteexpr);
                }

                // integer + pointer
                // swap the two, make it pointer + integer
                if ((leftinttype != nullptr) && (riteptrtype != nullptr)) {
                    ptrplusint = ptrPlusMinusInt (tok, OP_ADD, riteexpr, leftexpr);
                }

                if (ptrplusint == nullptr) {
                    throwerror (cbtok, "must have pointer and integer, not '%s' and '%s'", lefttype->getName (), ritetype->getName ());
                }

                // slap a '*' on the front of ptr+int
                UnopExpr *ux = new UnopExpr (this, tok);
                ux->opcode  = OP_DEREF;
                ux->subexpr = ptrplusint;
                ux->restype = ptrplusint->getType ()->stripCVMod ()->castPtrType ()->getBaseType ();
                exstack.push_back (ux);
                goto getbop;
            }

            // most operators get pushed on stack and we handle them later after fetching right-hand operator
            default: {
                opstack.emplace_back (tok, binop);
                break;
            }
        }
    }

    // end of expression, return result
    assert (opstack.size () == 0);
    assert (exstack.size () == 1);
    return exstack.back ();
}

// came across a second declaration of a function with the same name
bool FuncDecl::compatredecl (FuncType *functype, Keywd storclass, std::vector<VarDecl *> *params, ParamScope *paramscope)
{
    // not compatible if param types or return type doesn't match
    if (functype != getType ()->stripCVMod ()) return false;

    // not compatible if static vs static doesn't match
    // same with typedef
    if ((getStorClass () == KW_STATIC) && (storclass != KW_STATIC)) return false;
    if ((getStorClass () == KW_TYPEDEF) && (storclass != KW_TYPEDEF)) return false;

    // not compatible if extern/nothing doesn't match
    if (((getStorClass () == KW_NONE) || (getStorClass () == KW_EXTERN)) &&
        (storclass != KW_NONE) && (storclass != KW_EXTERN)) return false;

    // if no function body yet, use the given parameter list
    // ...in case this is a definition, we will have correct param var names
    if (getFuncBody () == nullptr) {
        this->params = params;
        this->paramscope = paramscope;
    }

    return true;
}

// check to see if the assignment types are compatible
void Scope::checkAsneqCompat (Token *optok, Type *lefttype, Type *ritetype)
{
    if (lefttype->isConstType ()) {
        printerror (optok, "cannot assign to const type '%s'", lefttype->getName ());
    }
    lefttype = lefttype->stripCVMod ();

    // allow identical types
    ritetype = ritetype->stripCVMod ();
    if (lefttype == ritetype) return;

    // only remaining allowed involve floats, integers, pointers
    FloatType *leftfloat = lefttype->castFloatType ();
    FloatType *ritefloat = ritetype->castFloatType ();
    IntegType *leftinteg = lefttype->castIntegType ();
    IntegType *riteinteg = ritetype->castIntegType ();
    PtrType *leftptrtype = lefttype->castPtrType ();
    PtrType *riteptrtype = ritetype->castPtrType ();
    FuncType *ritefunc = ritetype->castFuncType ();

    // allow functype* = functype
    if ((ritefunc != nullptr) && (leftptrtype != nullptr) && (leftptrtype->getBaseType ()->stripCVMod () == ritefunc)) return;

    // allow lefttype='sometype/void const *' and ritetype='sometype *'
    // allow lefttype='roottype *' and ritetype='leaftype *'
    // but disallow lefttype='sometype/void *' and ritetype='sometype const *'
    if ((leftptrtype != nullptr) && (riteptrtype != nullptr)) {
        tsize_t offset;
        Type *leftdereftype = leftptrtype->getBaseType ()->stripCVMod ();
        Type *ritedereftype = riteptrtype->getBaseType ();
        if (leftdereftype == ritedereftype) return;
        if (leftdereftype == voidtype) return;
        if (ritedereftype == voidtype) return;
        bool ok = ritedereftype->stripCVMod ()->isLeafwardOf (leftdereftype->stripCVMod (), &offset);
        if (! ok) goto badcombo;
        return;
    }

    // anything else with pointers not allowed
    if ((leftptrtype != nullptr) || (riteptrtype != nullptr)) goto badcombo;

    // allow float->double
    if ((leftfloat != nullptr) && (ritefloat != nullptr)) {
        if (leftfloat->getTypeSize (optok) < ritefloat->getTypeSize (optok)) goto badcombo;
        return;
    }

    // allow char->long and other such
    if ((leftinteg != nullptr) && (riteinteg != nullptr)) {
        if (leftinteg->getTypeSize (optok) < riteinteg->getTypeSize (optok)) goto badcombo;
        return;
    }

    // allow integer->floating
    if ((leftfloat != nullptr) && (riteinteg != nullptr)) return;

    // all others fail
badcombo:
    throwerror (optok, "cannot implicitly assign '%s' to '%s'", ritetype->getName (), lefttype->getName ());
}

// apply ++ or -- after evaluating the sub-expression
Expr *Scope::postIncDec (Token *optok, Opcode addsub, Expr *subexpr)
{
    static int pidtempseq;

    // see if subexpr is a pointer dereference
    // if so, get pointer into a temp and then just use that
    // ...in case pointer is complex and/or has side effects
    // (* ptrtemp = (valtemp = * (ptrtemp = subexpr->subexpr)) +- 1) , valtemp
    UnopExpr *subunopexpr = subexpr->castUnopExpr ();
    if ((subunopexpr != nullptr) && (subunopexpr->opcode == OP_DEREF)) {

        Expr *ptrexpr = subunopexpr->subexpr;

        char *ptrtempname;
        asprintf (&ptrtempname, "<pid-%d>", ++ pidtempseq);
        VarDecl *ptrtempdecl = new VarDecl (ptrtempname, this, optok, ptrexpr->getType (), KW_NONE);

        AsnopExpr *ptrtempassign = new AsnopExpr (this, optok);
        ptrtempassign->leftexpr  = new ValExpr (this, optok, ptrtempdecl);
        ptrtempassign->riteexpr  = ptrexpr;

        UnopExpr *readfromptr = new UnopExpr (this, optok);
        readfromptr->opcode   = OP_DEREF;
        readfromptr->subexpr  = ptrtempassign;
        readfromptr->restype  = subexpr->getType ();

        char *valtempname;
        asprintf (&valtempname, "<pid-%d>", ++ pidtempseq);
        VarDecl *valtempdecl = new VarDecl (valtempname, this, optok, subexpr->getType (), KW_NONE);

        AsnopExpr *valtempassign = new AsnopExpr (this, optok);
        valtempassign->leftexpr = new ValExpr (this, optok, valtempdecl);
        valtempassign->riteexpr = readfromptr;

        BinopExpr *calcnewval = new BinopExpr (this, optok);
        calcnewval->opcode    = addsub;
        calcnewval->leftexpr  = valtempassign;
        calcnewval->riteexpr  = getIncrement (optok, valtempassign);
        calcnewval->restype   = subexpr->getType ();

        UnopExpr *writeviaptr = new UnopExpr (this, optok);
        writeviaptr->opcode   = OP_DEREF;
        writeviaptr->subexpr  = new ValExpr (this, optok, ptrtempdecl);
        writeviaptr->restype  = subexpr->getType ();

        AsnopExpr *updateassign = new AsnopExpr (this, optok);
        updateassign->leftexpr = writeviaptr;
        updateassign->riteexpr = calcnewval;

        BinopExpr *result = new BinopExpr (this, optok);
        result->opcode   = OP_COMMA;
        result->leftexpr = updateassign;
        result->riteexpr = new ValExpr (this, optok, valtempdecl);
        result->restype  = subexpr->getType ();
        return result;
    }

    // see if we have a simple variable
    // if so, access it directly
    // (subexpr = (valtemp = subexpr) +- 1) , valtemp
    ValExpr *subvalexpr = subexpr->castValExpr ();
    if (subvalexpr != nullptr) {
        char *valtempname;
        asprintf (&valtempname, "<pid-%d>", ++ pidtempseq);
        VarDecl *valtempdecl = new VarDecl (valtempname, this, optok, subexpr->getType (), KW_NONE);

        AsnopExpr *valtempassign = new AsnopExpr (this, optok);
        valtempassign->leftexpr  = new ValExpr (this, optok, valtempdecl);
        valtempassign->riteexpr  = subvalexpr;

        BinopExpr *calcnewval = new BinopExpr (this, optok);
        calcnewval->opcode    = addsub;
        calcnewval->leftexpr  = valtempassign;
        calcnewval->riteexpr  = getIncrement (optok, valtempassign);
        calcnewval->restype   = subexpr->getType ();

        AsnopExpr *updateassign = new AsnopExpr (this, optok);
        updateassign->leftexpr = subvalexpr;
        updateassign->riteexpr = calcnewval;

        BinopExpr *result = new BinopExpr (this, optok);
        result->opcode = OP_COMMA;
        result->leftexpr = updateassign;
        result->riteexpr = new ValExpr (this, optok, valtempdecl);
        result->restype  = subexpr->getType ();
        return result;
    }

    throwerror (optok, "cannot be incremented/decremented");
}

// apply ++ or -- before evaluating the sub-expression
Expr *Scope::preIncDec (Token *optok, Opcode addsub, Expr *subexpr)
{
    static int pidtempseq;

    // see if subexpr is a pointer dereference
    // if so, get pointer into a temp and then just use that
    // ...in case pointer is complex and/or has side effects
    // (* ptrtemp = (* (ptrtemp = subexpr->subexpr)) +- 1)
    UnopExpr *subunopexpr = subexpr->castUnopExpr ();
    if ((subunopexpr != nullptr) && (subunopexpr->opcode == OP_DEREF)) {
        Expr *ptrexpr = subunopexpr->subexpr;

        char *ptrtempname;
        asprintf (&ptrtempname, "<qid-%d>", ++ pidtempseq);
        VarDecl *ptrtempdecl = new VarDecl (ptrtempname, this, optok, ptrexpr->getType (), KW_NONE);

        AsnopExpr *ptrtempassign = new AsnopExpr (this, optok);
        ptrtempassign->leftexpr = new ValExpr (this, optok, ptrtempdecl);
        ptrtempassign->riteexpr = ptrexpr;

        UnopExpr *readfromptr = new UnopExpr (this, optok);
        readfromptr->opcode   = OP_DEREF;
        readfromptr->subexpr  = ptrtempassign;
        readfromptr->restype  = subexpr->getType ();

        BinopExpr *calcnewval = new BinopExpr (this, optok);
        calcnewval->opcode    = addsub;
        calcnewval->leftexpr  = readfromptr;
        calcnewval->riteexpr  = getIncrement (optok, readfromptr);
        calcnewval->restype   = subexpr->getType ();

        UnopExpr *writeviaptr = new UnopExpr (this, optok);
        writeviaptr->opcode   = OP_DEREF;
        writeviaptr->subexpr  = new ValExpr (this, optok, ptrtempdecl);

        writeviaptr->restype  = subexpr->getType ();

        AsnopExpr *updateassign = new AsnopExpr (this, optok);
        updateassign->leftexpr = writeviaptr;
        updateassign->riteexpr = calcnewval;

        return updateassign;
    }

    // see if we have a simple variable
    // if so, access it directly
    // (subexpr = subexpr +- 1)
    ValExpr *subvalexpr = subexpr->castValExpr ();
    if (subvalexpr != nullptr) {
        BinopExpr *calcnewval = new BinopExpr (this, optok);
        calcnewval->opcode    = addsub;
        calcnewval->leftexpr  = subvalexpr;
        calcnewval->riteexpr  = getIncrement (optok, subvalexpr);
        calcnewval->restype   = subexpr->getType ();

        AsnopExpr *updateassign = new AsnopExpr (this, optok);
        updateassign->leftexpr = subvalexpr;
        updateassign->riteexpr = calcnewval;

        return updateassign;
    }

    throwerror (optok, "cannot be incremented/decremented");
}

// get increment value for ++ -- operators on the given value expression
Expr *Scope::getIncrement (Token *optok, Expr *valexpr)
{
    Type *valtype = valexpr->getType ()->stripCVMod ();

    PtrType *ptrtype = valtype->castPtrType ();
    if (ptrtype != nullptr) {
        tsize_t scale = ptrtype->getBaseType ()->getTypeSize (optok);
        return ValExpr::createsizeint (this, optok, scale, false);
    }

    NumCat nc = NC_NONE;
    NumType *nt = nullptr;
    NumValue nv;
    FloatType *flttype = valtype->castFloatType ();
    if (flttype != nullptr) {
        nt = flttype;
        nc = NC_FLOAT;
        nv.f = 1.0;
    }

    IntegType *inttype = valtype->castIntegType ();
    if (inttype != nullptr) {
        nt = inttype;
        if (inttype->getSign ()) {
            nc = NC_SINT;
            nv.s = 1;
        } else {
            nc = NC_UINT;
            nv.u = 1;
        }
    }

    if (nt != nullptr) {
        return new ValExpr (this, optok, NumDecl::createtyped (optok, nt, nc, nv));
    }

    throwerror (optok, "cannot increment/decrement a '%s'", valtype->getName ());
}

// output code to perform arithmetic
// do type casting, scaling as needed
//  input:
//   optok    = source token for error messages
//   opcode   = arithmetic opcode OP_ADD,SUB,SHL,SHR,BITOR,BITAND,BITXOR,DIV,MUL,MOD
//   leftexpr = left-hand operand expression
//   riteexpr = right-hand operand expression
//  output:
//   returns expression that performs arithmetic on the operands
Expr *Scope::newBinopArith (Token *optok, Opcode opcode, Expr *leftexpr, Expr *riteexpr)
{
    switch (opcode) {

        // can be pointer + integer => scale integer by pointed-to size
        // likewise for integer + pointer
        // otherwise, both operands must be numeric, cast weaker type to stronger type
        case OP_ADD: {
            return newBinopAdd (optok, leftexpr, riteexpr);
        }

        // can be pointer - integer => cast pointer to tsize_t, scale integer by pointed-to size, cast diff back to pointer
        // can be pointer - pointer => cast pointers to tsize_t, scale diff by pointed-to size
        // otherwise, both operands must be numeric, cast weaker type to stronger type
        case OP_SUB: {
            return newBinopSub (optok, leftexpr, riteexpr);
        }

        // both operands must be integer numerics
        // no auto-casting needed, but check that both are integers
        case OP_SHL:
        case OP_SHR: {
            getStrongerIArith (optok, leftexpr->getType (), riteexpr->getType ());
            BinopExpr *bx = new BinopExpr (this, optok);
            bx->opcode    = opcode;
            bx->leftexpr  = leftexpr;
            bx->riteexpr  = riteexpr;
            bx->restype   = leftexpr->getType ();
            return bx;
        }

        // if one operand is unsigned and smaller than the other,
        // cast the larger one to smaller size cuz the top bits are zeroes
        // this is useful for 'if (someuint64 & 1) ...'
        case OP_BITAND: {
            IntegType *leftintype = leftexpr->getType ()->stripCVMod ()->castIntegType ();
            IntegType *riteintype = riteexpr->getType ()->stripCVMod ()->castIntegType ();
            if ((leftintype == nullptr) || (riteintype == nullptr)) {
                throwerror (optok, "both operands must be integers");
            }
            tsize_t leftsize = leftintype->getTypeSize (optok);
            tsize_t ritesize = riteintype->getTypeSize (optok);
            if (leftsize < ritesize) {
                if (leftintype->getSign ()) {
                    NumValue nv;
                    NumCat nc = leftexpr->isNumConstExpr (&nv);
                    if (nc != NC_SINT) goto noreducedand;
                    if (nv.s < 0) goto noreducedand;
                }
                riteexpr = castToType (optok, leftintype, riteexpr);
            } else if (ritesize < leftsize) {
                if (riteintype->getSign ()) {
                    NumValue nv;
                    NumCat nc = riteexpr->isNumConstExpr (&nv);
                    if (nc != NC_SINT) goto noreducedand;
                    if (nv.s < 0) goto noreducedand;
                }
                leftexpr = castToType (optok, riteintype, leftexpr);
            }
        noreducedand:;
            // fallthrough
        }

        // both operands must be integer numerics
        // weaker auto-cast to stronger
        case OP_BITOR:
        case OP_BITXOR: {
            Type *restyp  = getStrongerIArith (optok, leftexpr->getType (), riteexpr->getType ());
            BinopExpr *bx = new BinopExpr (this, optok);
            bx->opcode    = opcode;
            bx->leftexpr  = castToType (optok, restyp, leftexpr);
            bx->riteexpr  = castToType (optok, restyp, riteexpr);
            bx->restype   = restyp;
            return bx;
        }

        // if two integer constants, type might be bigger than either one
        case OP_MUL: {
            IntegType *lit = leftexpr->getType ()->stripCVMod ()->castIntegType ();
            IntegType *rit = riteexpr->getType ()->stripCVMod ()->castIntegType ();
            if ((lit != nullptr) && (rit != nullptr)) {
                NumValue lnv, pnv, rnv;
                NumCat lnc = leftexpr->isNumConstExpr (&lnv);
                NumCat rnc = riteexpr->isNumConstExpr (&rnv);
                NumCat pnc = NC_NONE;
                switch (lnc) {
                    case NC_SINT: {
                        switch (rnc) {
                            case NC_SINT: {
                                pnc = NC_SINT;
                                pnv.s = lnv.s * rnv.s;
                                break;
                            }
                            case NC_UINT: {
                                pnc = NC_UINT;
                                pnv.u = lnv.s * rnv.u;
                                break;
                            }
                            default: break;
                        }
                    }
                    case NC_UINT: {
                        switch (rnc) {
                            case NC_SINT: {
                                pnc = NC_UINT;
                                pnv.u = lnv.u * rnv.s;
                                break;
                            }
                            case NC_UINT: {
                                pnc = NC_UINT;
                                pnv.u = lnv.u * rnv.u;
                                break;
                            }
                            default: break;
                        }
                    }
                    default: break;
                }
                if (pnc != NC_NONE) {
                    return ValExpr::createnumconst (this, optok, nullptr, pnc, pnv);
                }
            }
            // fall through
        }

        // both operands must be numeric (ints, floats)
        // no auto-casting (except if floatingpoint, then cast to stronger type)
        // result is stronger of the two types
        case OP_DIV: {
            Type *restyp  = getStrongerArith (optok, leftexpr->getType (), riteexpr->getType ());
            if (restyp->stripCVMod ()->castFloatType () != nullptr) {
                leftexpr = castToType (optok, restyp, leftexpr);
                riteexpr = castToType (optok, restyp, riteexpr);
            }
            BinopExpr *bx = new BinopExpr (this, optok);
            bx->opcode    = opcode;
            bx->leftexpr  = leftexpr;
            bx->riteexpr  = riteexpr;
            bx->restype   = restyp;
            return bx;
        }

        // both operands must be integer numerics
        // no auto-casting
        // result is weaker of the two types
        case OP_MOD: {
            Type *restyp  = getWeakerIArith (optok, leftexpr->getType (), riteexpr->getType ());
            BinopExpr *bx = new BinopExpr (this, optok);
            bx->opcode    = opcode;
            bx->leftexpr  = leftexpr;
            bx->riteexpr  = riteexpr;
            bx->restype   = restyp;
            return bx;
        }

        default: assert_false ();
    }
}

// create a BinopExpr that adds the two operands,
// performing scaling as needed for pointer operands
Expr *Scope::newBinopAdd (Token *optok, Expr *leftexpr, Expr *riteexpr)
{
    Type *lefttype = leftexpr->getType ()->stripCVMod ();
    Type *ritetype = riteexpr->getType ()->stripCVMod ();
    IntegType *leftinttype = lefttype->castIntegType ();
    IntegType *riteinttype = ritetype->castIntegType ();
    PtrType   *leftptrtype = lefttype->castPtrType ();
    PtrType   *riteptrtype = ritetype->castPtrType ();

    // pointer + integer
    if ((leftptrtype != nullptr) && (riteinttype != nullptr)) {
        return ptrPlusMinusInt (optok, OP_ADD, leftexpr, riteexpr);
    }

    // integer + pointer
    // swap the two, make it pointer + integer
    if ((leftinttype != nullptr) && (riteptrtype != nullptr)) {
        return ptrPlusMinusInt (optok, OP_ADD, riteexpr, leftexpr);
    }

    // two integer constants - result might be wider than either one
    if ((leftinttype != nullptr) && (riteinttype != nullptr)) {
        NumValue lnv, pnv, rnv;
        NumCat lnc = leftexpr->isNumConstExpr (&lnv);
        NumCat rnc = riteexpr->isNumConstExpr (&rnv);
        NumCat pnc = NC_NONE;
        switch (lnc) {
            case NC_SINT: {
                switch (rnc) {
                    case NC_SINT: {
                        pnc = NC_SINT;
                        pnv.s = lnv.s + rnv.s;
                        break;
                    }
                    case NC_UINT: {
                        pnc = NC_UINT;
                        pnv.u = lnv.s + rnv.u;
                        break;
                    }
                    default: break;
                }
                break;
            }
            case NC_UINT: {
                switch (rnc) {
                    case NC_SINT: {
                        pnc = NC_UINT;
                        pnv.u = lnv.u + rnv.s;
                        break;
                    }
                    case NC_UINT: {
                        pnc = NC_UINT;
                        pnv.u = lnv.u + rnv.u;
                        break;
                    }
                    default: break;
                }
                break;
            }
            default: break;
        }
        if (pnc != NC_NONE) {
            return ValExpr::createnumconst (this, optok, nullptr, pnc, pnv);
        }
    }

    // must be arithmetics
    // scale weaker type to stronger type
    Type *restype = getStrongerArith (optok, lefttype, ritetype);
    BinopExpr *bx = new BinopExpr (this, optok);
    bx->opcode    = OP_ADD;
    bx->leftexpr  = castToType (optok, restype, leftexpr);
    bx->riteexpr  = castToType (optok, restype, riteexpr);
    bx->restype   = restype;
    return bx;
}

// create a BinopExpr that subtractss the two operands,
// performing scaling as needed for pointer operands
Expr *Scope::newBinopSub (Token *optok, Expr *leftexpr, Expr *riteexpr)
{
    Type *lefttype = leftexpr->getType ()->stripCVMod ();
    Type *ritetype = riteexpr->getType ()->stripCVMod ();
    IntegType *leftinttype = lefttype->castIntegType ();
    IntegType *riteinttype = ritetype->castIntegType ();
    PtrType   *leftptrtype = lefttype->castPtrType ();
    PtrType   *riteptrtype = ritetype->castPtrType ();

    // pointer - integer
    if ((leftptrtype != nullptr) && (riteinttype != nullptr)) {
        return ptrPlusMinusInt (optok, OP_SUB, leftexpr, riteexpr);
    }

    // pointer - pointer
    if ((leftptrtype != nullptr) && (riteptrtype != nullptr)) {
        return ptrMinusPtr (optok, leftexpr, riteexpr);
    }

    // two integer constants - result might be wider than either one
    if ((leftinttype != nullptr) && (riteinttype != nullptr)) {
        NumValue lnv, pnv, rnv;
        NumCat lnc = leftexpr->isNumConstExpr (&lnv);
        NumCat rnc = riteexpr->isNumConstExpr (&rnv);
        NumCat pnc = NC_NONE;
        switch (lnc) {
            case NC_SINT: {
                switch (rnc) {
                    case NC_SINT: {
                        pnc = NC_SINT;
                        pnv.s = lnv.s - rnv.s;
                        break;
                    }
                    case NC_UINT: {
                        pnc = NC_UINT;
                        pnv.u = lnv.s - rnv.u;
                        break;
                    }
                    default: break;
                }
                break;
            }
            case NC_UINT: {
                switch (rnc) {
                    case NC_SINT: {
                        pnc = NC_UINT;
                        pnv.u = lnv.u - rnv.s;
                        break;
                    }
                    case NC_UINT: {
                        pnc = NC_UINT;
                        pnv.u = lnv.u - rnv.u;
                        break;
                    }
                    default: break;
                }
                break;
            }
            default: break;
        }
        if (pnc != NC_NONE) {
            return ValExpr::createnumconst (this, optok, nullptr, pnc, pnv);
        }
    }

    // must be arithmetics
    // scale weaker type to stronger type
    Type *restype = getStrongerArith (optok, lefttype, ritetype);
    BinopExpr *bx = new BinopExpr (this, optok);
    bx->opcode    = OP_SUB;
    bx->leftexpr  = castToType (optok, restype, leftexpr);
    bx->riteexpr  = castToType (optok, restype, riteexpr);
    bx->restype   = restype;
    return bx;
}

// perform possible scaled pointer arithmetic
Expr *Scope::ptrPlusMinusInt (Token *optok, Opcode addsub, Expr *leftexpr, Expr *riteexpr)
{
    PtrType *ptrtype = leftexpr->getType ()->stripCVMod ()->castPtrType ();

    // cast integer (right-hand) operand to tsize_t and scale by pointed-to size
    tsize_t scale = ptrtype->getBaseType ()->getTypeSize (optok);
    if (scale > 1) {
        BinopExpr *ritescaled = new BinopExpr (this, riteexpr->exprtoken);
        ritescaled->opcode    = OP_MUL;
        ritescaled->leftexpr  = castToType (optok, sizetype, riteexpr);
        ritescaled->riteexpr  = ValExpr::createsizeint (this, optok, scale, false);
        ritescaled->restype   = sizetype;
        riteexpr = ritescaled;
    }

    // perform addition/subtraction
    BinopExpr *arithexpr = new BinopExpr (this, optok);
    arithexpr->opcode    = addsub;
    arithexpr->leftexpr  = leftexpr;
    arithexpr->riteexpr  = riteexpr;
    arithexpr->restype   = ptrtype;

    return arithexpr;
}

Expr *Scope::ptrMinusPtr (Token *optok, Expr *leftexpr, Expr *riteexpr)
{
    PtrType *leftptrtype = leftexpr->getType ()->stripCVMod ()->castPtrType ();
    PtrType *riteptrtype = riteexpr->getType ()->stripCVMod ()->castPtrType ();
    if (leftptrtype != riteptrtype) {
        throwerror (optok, "pointers must be same type '%s' vs '%s'", leftptrtype->getName (), riteptrtype->getName ());
    }

    // cast pointers to tssize_t then perform subtraction
    BinopExpr *arithexpr = new BinopExpr (this, optok);
    arithexpr->opcode    = OP_SUB;
    arithexpr->leftexpr  = castToType (optok, ssizetype, leftexpr);
    arithexpr->riteexpr  = castToType (optok, ssizetype, riteexpr);
    arithexpr->restype   = ssizetype;

    // scale result
    tsize_t scale = leftptrtype->getBaseType ()->getTypeSize (optok);
    if (scale > 1) {
        BinopExpr *scaleexpr = new BinopExpr (this, optok);
        scaleexpr->opcode    = OP_DIV;
        scaleexpr->leftexpr  = arithexpr;
        scaleexpr->riteexpr  = ValExpr::createsizeint (this, optok, scale, true);
        scaleexpr->restype   = ssizetype;
        arithexpr = scaleexpr;
    }

    return arithexpr;
}

// cast the given expression to the given type if it isn't already
// any numeric type can be cast to any numeric type
// any pointer type can be cast to/from tsize_t (sizetype) or tssize_t (ssizetype)
// any numeric or pointer type can be cast to bool
Expr *Scope::castToType (Token *optok, Type *totype, Expr *expr)
{
    totype = totype->stripCVMod ();
    Type *extype = expr->getType ()->stripCVMod ();

    // if not changing type, allow anything to pass through, including whole structs
    if (totype == extype) return expr;

    FloatType *toflt = totype->castFloatType ();
    IntegType *toint = totype->castIntegType ();
    PtrType   *toptr = totype->castPtrType ();
    FloatType *exflt = extype->castFloatType ();
    IntegType *exint = extype->castIntegType ();
    PtrType   *exptr = extype->castPtrType ();

    // casting to pointer,
    //  ok to cast from tsize_t sized integer
    //  ok to cast from another pointer of any type
    //  ok to cast from integer constant 0 (null/nullptr)
    //  ok to cast from function to function pointer
    if (toptr != nullptr) {
        if ((exint != nullptr) && (exint->getTypeSize (optok) == sizeof (tsize_t))) goto good;
        if (exptr != nullptr) goto good;
        if (expr->isConstZero ()) goto good;
        if (toptr->getBaseType ()->stripCVMod () == extype->castFuncType ()) goto good;
        goto bad;
    }

    if (exptr != nullptr) {
        if ((toint != nullptr) && (toint->getTypeSize (optok) == sizeof (tsize_t))) goto good;
        if (toptr != nullptr) goto good;
        goto bad;
    }

    if ((toflt == nullptr) && (toint == nullptr)) goto bad;
    if ((exflt != nullptr) || (exint != nullptr)) goto good;
bad:
    throwerror (optok, "bad type cast from '%s' to '%s'", extype->getName (), totype->getName ());

good:
    CastExpr *cx = new CastExpr (this, optok);
    cx->casttype = totype;
    cx->subexpr  = expr;
    return cx;
}

// get stronger of the two arithmetic types (floatigpoint, integer)
// throw error if either is not an arithmetic type
Type *Scope::getStrongerArith (Token *optok, Type *lefttype, Type *ritetype)
{
    lefttype = lefttype->stripCVMod ();
    ritetype = ritetype->stripCVMod ();
    FloatType *leftfloat = lefttype->castFloatType ();
    FloatType *ritefloat = ritetype->castFloatType ();
    IntegType *leftinteg = lefttype->castIntegType ();
    IntegType *riteinteg = ritetype->castIntegType ();
    tsize_t leftsize = lefttype->getTypeSize (optok);
    tsize_t ritesize = ritetype->getTypeSize (optok);

    if ((leftfloat != nullptr) && (ritefloat != nullptr)) {
        return (leftsize > ritesize) ? leftfloat : ritefloat;
    }

    if ((leftfloat != nullptr) && (riteinteg != nullptr)) return leftfloat;

    if ((leftinteg != nullptr) && (ritefloat != nullptr)) return ritefloat;

    if ((leftinteg == nullptr) || (riteinteg == nullptr)) {
        throwerror (optok, "expression types '%s' and '%s' must both be numeric", lefttype->getName (), ritetype->getName ());
    }

    return getStrongerIArith (optok, lefttype, ritetype);
}

// get stronger of the two integer arithmetic types
// throw error if either is not an integer arithmetic type
Type *Scope::getStrongerIArith (Token *optok, Type *lefttype, Type *ritetype)
{
    lefttype = lefttype->stripCVMod ();
    ritetype = ritetype->stripCVMod ();
    IntegType *leftinteg = lefttype->castIntegType ();
    IntegType *riteinteg = ritetype->castIntegType ();

    if ((leftinteg == nullptr) || (riteinteg == nullptr)) {
        throwerror (optok, "expression types '%s' and '%s' must both be integers", lefttype->getName (), ritetype->getName ());
    }

    bool unsined = ! leftinteg->getSign () || ! riteinteg->getSign ();
    tsize_t leftsize = lefttype->getTypeSize (optok);
    tsize_t ritesize = ritetype->getTypeSize (optok);
    tsize_t size = (leftsize > ritesize) ? leftsize : ritesize;
    switch (size) {
        case 1: return unsined ? uint8type  : int8type;
        case 2: return unsined ? uint16type : int16type;
        case 4: return unsined ? uint32type : int32type;
        case 8: return unsined ? uint64type : int64type;
        default: assert_false ();
    }
}

// get weakger of the two integer arithmetic types
// throw error if either is not an integer arithmetic type
Type *Scope::getWeakerIArith (Token *optok, Type *lefttype, Type *ritetype)
{
    lefttype = lefttype->stripCVMod ();
    ritetype = ritetype->stripCVMod ();
    IntegType *leftinteg = lefttype->castIntegType ();
    IntegType *riteinteg = ritetype->castIntegType ();

    if ((leftinteg == nullptr) || (riteinteg == nullptr)) {
        throwerror (optok, "expression types '%s' and '%s' must both be integers", lefttype->getName (), ritetype->getName ());
    }

    bool unsined = ! leftinteg->getSign () || ! riteinteg->getSign ();
    tsize_t leftsize = lefttype->getTypeSize (optok);
    tsize_t ritesize = ritetype->getTypeSize (optok);
    tsize_t size = (leftsize < ritesize) ? leftsize : ritesize;
    switch (size) {
        case 1: return unsined ? uint8type  : int8type;
        case 2: return unsined ? uint16type : int16type;
        case 4: return unsined ? uint32type : int32type;
        case 8: return unsined ? uint64type : int64type;
        default: assert_false ();
    }
}

// came across '(' '{' ... '}' ')'
// returns with ')' being next token in stream
BlockExpr *Scope::parseblockexpr (Token *obracetok)
{
    // find function that we are part of
    // ...cuz statements must be parsed in a function context
    FuncDecl *funcdecl = getOuterFuncDecl ();
    if (funcdecl == nullptr) {
        throwerror (obracetok, "block expressions only allowed within functions");
    }

    // parse the statements within the '{' ... '}'
    BlockExpr *bx = new BlockExpr (this, obracetok);
    bx->blockscope = new BlockScope (this);
    Stmt *laststmt = nullptr;
    Token *bracetok;
    while (true) {
        bracetok = gettoken ();
        if (bracetok->isKw (KW_CBRACE)) break;
        pushtoken (bracetok);
        if (laststmt != nullptr) bx->topstmts.push_back (laststmt);
        try {
            laststmt = funcdecl->parseStatement (bx->blockscope, nullptr);
        } catch (Error *err) {
            handlerror (err);
        }
    }

    // the last statement must be an expression statement giving the value
    if (laststmt == nullptr) {
        throwerror (bracetok, "no statements within block expression");
    }
    bx->laststmt = laststmt->castExprStmt ();
    if (bx->laststmt == nullptr) {
        throwerror (laststmt->getStmtToken (), "last statement of block expression is not an expression");
    }

    return bx;
}
