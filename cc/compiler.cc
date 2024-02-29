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

#include "decl.h"
#include "error.h"
#include "scope.h"
#include "token.h"

bool errorflag;
FILE *sfile;
FloatType *flt32type, *flt64type;
FuncDecl *freefunc, *malloc2func;
GlobalScope *globalscope;
IntegType *int8type, *int16type, *int32type, *int64type;
IntegType *uint8type, *uint16type, *uint32type, *uint64type;
IntegType *booltype, *chartype, *inttype, *sizetype, *ssizetype;
StructType *trymarktype;
Token *intdeftok;
tsize_t trashsize;
VarDecl *trystackvar;
VoidType *voidtype;

int main (int argc, char **argv)
{
    setlinebuf (stdout);

    for (int i = 0; ++ i < argc;) {
        if (strcasecmp (argv[i], "-s") == 0) {
            if ((++ i >= argc) || (argv[i][0] == '-')) {
                fprintf (stderr, "missing sfile after -s option\n");
                return 1;
            }
            sfile = fopen (argv[i], "w");
            if (sfile == NULL) {
                fprintf (stderr, "error creating %s: %m\n", argv[i]);
                return 1;
            }
            continue;
        }
        if (argv[i][0] == '-') {
            fprintf (stderr, "unknown option %s\n", argv[i]);
            return 1;
        }
        srcfilenames.push_back (argv[i]);
    }
    if (sfile == nullptr) {
        fprintf (stderr, "missing -s <sfile> option\n");
        return 1;
    }

    intdeftok   = new KwToken ("<internal>", 0, 0, KW_EOF);
    globalscope = new GlobalScope ();

    // fundamental built-in types
    flt32type   = new FloatType  ("__flt32_t",  globalscope, intdeftok, 4);
    flt64type   = new FloatType  ("__flt64_t",  globalscope, intdeftok, 8);
    booltype    = new IntegType  ("__bool_t",   globalscope, intdeftok, 1, false);
    int8type    = new IntegType  ("__int8_t",   globalscope, intdeftok, 1, true);
    int16type   = new IntegType  ("__int16_t",  globalscope, intdeftok, 2, true);
    int32type   = new IntegType  ("__int32_t",  globalscope, intdeftok, 4, true);
    int64type   = new IntegType  ("__int64_t",  globalscope, intdeftok, 8, true);
    uint8type   = new IntegType  ("__uint8_t",  globalscope, intdeftok, 1, false);
    uint16type  = new IntegType  ("__uint16_t", globalscope, intdeftok, 2, false);
    uint32type  = new IntegType  ("__uint32_t", globalscope, intdeftok, 4, false);
    uint64type  = new IntegType  ("__uint64_t", globalscope, intdeftok, 8, false);
    voidtype    = new VoidType   ("__void",     globalscope, intdeftok);

    // bunch of typedefs
    assert (sizeof (tsize_t) == 2);
    new VarDecl ("__size_t",       globalscope, intdeftok, (sizetype = uint16type), KW_TYPEDEF);
    new VarDecl ("__ssize_t",      globalscope, intdeftok, (ssizetype = int16type), KW_TYPEDEF);

    new VarDecl ("double",         globalscope, intdeftok, flt64type,  KW_TYPEDEF);
    new VarDecl ("float",          globalscope, intdeftok, flt32type,  KW_TYPEDEF);

    new VarDecl ("char",           globalscope, intdeftok, (chartype = int8type),   KW_TYPEDEF);
    new VarDecl ("short",          globalscope, intdeftok, int16type,  KW_TYPEDEF);
    new VarDecl ("int",            globalscope, intdeftok, (inttype  = int16type),  KW_TYPEDEF);
    new VarDecl ("long",           globalscope, intdeftok, int32type,  KW_TYPEDEF);

    new VarDecl ("signed char",    globalscope, intdeftok, int8type,   KW_TYPEDEF);
    new VarDecl ("signed short",   globalscope, intdeftok, int16type,  KW_TYPEDEF);
    new VarDecl ("signed int",     globalscope, intdeftok, int16type,  KW_TYPEDEF);
    new VarDecl ("signed long",    globalscope, intdeftok, int32type,  KW_TYPEDEF);

    new VarDecl ("unsigned char",  globalscope, intdeftok, uint8type,  KW_TYPEDEF);
    new VarDecl ("unsigned short", globalscope, intdeftok, uint16type, KW_TYPEDEF);
    new VarDecl ("unsigned int",   globalscope, intdeftok, uint16type, KW_TYPEDEF);
    new VarDecl ("unsigned long",  globalscope, intdeftok, uint32type, KW_TYPEDEF);

    new VarDecl ("bool",           globalscope, intdeftok, booltype,   KW_TYPEDEF);
    new VarDecl ("void",           globalscope, intdeftok, voidtype,   KW_TYPEDEF);

    //  struct __trymark_t {
    //      __trymark_t *outer;     // next outer trymark on the trystack (or 0 if none)
    //      void        *descr;     // try block descriptor (catches & finally)
    //      void *thrownobjptr;     // for use by __throw routine to save thrown object pointer
    //      void *throwntypeid;     // for use by __throw routine to save thrown type id
    //  };
    trymarktype = new StructType ("__trymark_t", globalscope, intdeftok, false);
    MemberScope *trymarkmscope = new MemberScope (globalscope);
    std::vector<FuncDecl *> *trymarkmfuncs = new std::vector<FuncDecl *> ();
    std::vector<VarDecl  *> *trymarkmvars  = new std::vector<VarDecl  *> ();
    trymarkmvars->push_back (new VarDecl ("outer", trymarkmscope, intdeftok, trymarktype->getPtrType (), KW_NONE));
    trymarkmvars->push_back (new VarDecl ("descr", trymarkmscope, intdeftok, voidtype->getPtrType    (), KW_NONE));
    trymarkmvars->push_back (new VarDecl ("thrownobjptr", trymarkmscope, intdeftok, voidtype->getPtrType (), KW_NONE));
    trymarkmvars->push_back (new VarDecl ("throwntypeid", trymarkmscope, intdeftok, voidtype->getPtrType (), KW_NONE));
    trymarktype->setMembers (trymarkmscope, trymarkmfuncs, trymarkmvars);

    // void *__malloc2 (__size_t nmemb, __size_t size);
    ParamScope *malloc2pscope = new ParamScope (globalscope);
    std::vector<VarDecl *> *malloc2prms = new std::vector<VarDecl *> ();
    malloc2prms->push_back (new VarDecl ("nmemb", malloc2pscope, intdeftok, sizetype, KW_NONE));
    malloc2prms->push_back (new VarDecl ("size", malloc2pscope, intdeftok, sizetype, KW_NONE));
    FuncType *malloc2type = FuncType::create (intdeftok, voidtype->getPtrType (), malloc2prms);
    malloc2func = new FuncDecl ("__malloc2", globalscope, intdeftok, malloc2type, KW_NONE, malloc2prms, malloc2pscope);

    // void free (__void *ptr);
    ParamScope *freepscope = new ParamScope (globalscope);
    std::vector<VarDecl *> *freeprms = new std::vector<VarDecl *> ();
    freeprms->push_back (new VarDecl ("ptr", freepscope, intdeftok, voidtype->getPtrType (), KW_NONE));
    FuncType *freetype = FuncType::create (intdeftok, voidtype, freeprms);
    freefunc = new FuncDecl ("free", globalscope, intdeftok, freetype, KW_NONE, freeprms, freepscope);

    // compile top-level statements
    while (true) {
        Token *tok = gettoken ();
        if (tok->isKw (KW_EOF)) break;
        pushtoken (tok);

        try {
            // statements at this level fall into these categories:
            //  extern/global/static/typedef variable declaration
            //  struct/union type prototype
            //  struct/union type definition
            //  function prototype
            //  function definition
            globalscope->parsevaldecl (PE_GLOBAL, nullptr);
        } catch (Error *err) {
            handlerror (err);
        }
    }

    if (errorflag) return 1;

    /* dump abstract syntax tree
    DeclMap const *declmap = globalscope->getDecls ();
    for (DeclMap::const_iterator it = declmap->begin (); it != declmap->end (); it ++) {
        it->second->dumpdecl ();
        printf (";\n");
    } */

    if (trashsize > 0) {
        fprintf (sfile, "; --------------\n");
        fprintf (sfile, "\t.psect\t.75.trash,OVR\n");
        fprintf (sfile, "\t.align\t%u\n", MAXALIGN);
        fprintf (sfile, "__trash: .blkb\t%u\n", trashsize);
    }
    fprintf (sfile, "; --------------\n");
    fclose (sfile);

    return 0;
}
