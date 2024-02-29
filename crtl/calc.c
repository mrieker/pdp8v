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

// calculator program

#include <assert.h>
#include <complex.h>
#include <errno.h>
#include <hode.h>
#include <math.h>
#include <readline.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MAXSAVED 99

typedef unsigned int uint;

typedef ComplexD FnFunc ();
typedef ComplexD OpFunc (ComplexD left, ComplexD rite);

struct Fn {
    char name[6];
    FnFunc *func;
};

struct Op {
    char name[4];
    OpFunc *func;
};

struct UserDef {
    UserDef *next;
    char *name;

    UserDef ();
    virtual ~UserDef ();
    virtual ComplexD eval () = 0;
    virtual void print (FILE *out) = 0;
};

struct Parm {
    Parm *next;
    char name[1];
};

struct UserFunc : UserDef {
    Parm *parms;
    char *body;

    UserFunc ();
    virtual ~UserFunc ();
    virtual ComplexD eval ();
    virtual void print (FILE *out);
};

struct UserVar : UserDef {
    ComplexD valu;

    virtual ~UserVar ();
    virtual ComplexD eval ();
    virtual void print (FILE *out);
};

char *lineptr;
char const *helpname;
char const prompt[] = "\n> ";
ComplexD savedvals[MAXSAVED];
double d2r = 1.0;
double r2d = 1.0;
FILE *logfile;
uint nextsaved;
UserDef *userdefs;

bool readinput (FILE *in, char **lb_r);
void execute ();
bool compute (ComplexD *val_r);
ComplexD evalexpr (uint preced);
ComplexD getval ();
char skipspaces ();
uint skipvarname ();
void throwerror (char const *fmt, ...);
void printval (FILE *out, ComplexD const *val);
void lprintf (FILE *stream, char const *fmt, ...);

int main (int argc, char **argv)
{
    // if present, compute single command-line expression and print result
    if (argc > 1) {

        // make args into one space-separated null-terminated string
        int totlen = 0;
        for (int i = 0; ++ i < argc;) {
            totlen += strlen (argv[i]) + 1;
        }
        lineptr = malloc (totlen);
        totlen = 0;
        for (int i = 0; ++ i < argc;) {
            if (totlen > 0) lineptr[totlen-1] = ' ';
            int len = strlen (argv[i]) + 1;
            memcpy (lineptr + totlen, argv[i], len);
            totlen += len;
        }

        // compute the value
        ComplexD val;
        if (! compute (&val)) return 1;

        // print result without any decoration
        printval (stdout, &val);

        // success
        return 0;
    }

    // no command-line parameters, read from stdin and print to stdout
    helpname = "";
    char *h = argv[0];
    int i = strlen (h);
    if ((i > 3) && (strcmp (h + i - 4, ".hex") == 0)) {
        h[--i] = 'p';
        h[--i] = 'l';
        printf ("calculator, type help to get help\n");
        helpname = h;
    }
    logfile = fopen ("calc.log", "a");
    if (logfile == NULL) {
        fprintf (stderr, "error creating logfile calc.log: %s\n", strerror (errno));
    } else {
        fprintf (logfile, "\n--------\n");
    }

    ReadLine rl;
    int rlrc = rl.open (0);
    char *linebuf = NULL;

    while (1) {

        // read line from stdin, exit loop if eof
        if (rlrc < 0) {
            if (! readinput (stdin, &linebuf)) break;
        } else {
            linebuf = rl.read (prompt);
            if (linebuf == NULL) break;
        }

        // maybe log the input line
        if (logfile != NULL) {
            fprintf (logfile, "%s%s\n", prompt, linebuf);
        }

        // skip blank lines
        // don't save in readline history
        lineptr = linebuf;
        if (skipspaces () == 0) {
            if (rlrc >= 0) free (linebuf);
            continue;
        }

        // maybe save in readline history
        if (rlrc >= 0) rl.save (linebuf);

        // execute calculation
        execute ();
    }

    if (logfile != NULL) {
        fprintf (logfile, "%s*EOF*\n", prompt);
        fclose (logfile);
    }

    return 0;
}

// read line from input into linebuf
// extend linebuf as needed to handle whole line
bool readinput (FILE *in, char **lb_r)
{
    char *lb = *lb_r;
    if (lb == NULL) *lb_r = lb = malloc (64);
    __size_t siz = allocsz (lb);
    if (fgets (lb, siz, in) == NULL) return 0;
    int len = strlen (lb);
    if (len == 0) return 0;
    while (lb[len-1] != '\n') {
        siz += siz / 2;
        *lb_r = lb = realloc (lb, siz);
        if (fgets (lb + len, siz - len, in) == NULL) goto rtn1;
        len += strlen (lb + len);
    }
    lb[len-1] = 0;
rtn1:
    return 1;
}

// execute computation
//  in:
//   lineptr = first non-blank char in line
void execute ()
{
    // check for varname=expression
    char *vareq = strchr (lineptr, '=');
    if (vareq != NULL) {
        char *varname = lineptr;
        uint varnamelen = skipvarname ();
        if (varnamelen > 0) {
            UserDef *userdef =  NULL;
            skipspaces ();

            // if '=' after func/var name, it's a variable definition
            if (lineptr == vareq) {

                // compute expression before altering any definitions
                ++ lineptr;
                UserVar *var = new UserVar;
                if (compute (&var->valu)) {
                    userdef = var;
                } else {
                    delete var;
                }
            } else {

                // intervening parameter names, get param name list
                UserFunc *func = new UserFunc;
                Parm **lparm = &func->parms;
                do {
                    char *parmname = lineptr;
                    uint parmnamelen = skipvarname ();
                    if (parmnamelen == 0) {
                        fprintf (stderr, "invalid param name <%s>\n", parmname);
                        delete func;
                        return;
                    }
                    Parm *parm = malloc (parmnamelen + sizeof *parm);
                    parm->next = NULL;
                    memcpy (parm->name, parmname, parmnamelen);
                    parm->name[parmnamelen] = 0;
                    *lparm = parm;
                    lparm = &parm->next;
                    skipspaces ();
                } while (lineptr != vareq);

                // save function body (rest of line)
                ++ lineptr;
                skipspaces ();
                func->body = strdup (lineptr);
                userdef = func;
            }

            if (userdef != NULL) {
                userdef->name = malloc (varnamelen + 1);
                memcpy (userdef->name, varname, varnamelen);
                userdef->name[varnamelen] = 0;

                // delete old definition of same name if any
                UserDef *ud;
                for (UserDef **lud = &userdefs; (ud = *lud) != NULL;) {
                    if (strcmp (ud->name, userdef->name) == 0) {
                        *lud = ud->next;
                        delete ud;
                    } else {
                        lud = &ud->next;
                    }
                }

                // link in new definition
                userdef->next = userdefs;
                userdefs = userdef;

                // print it out
                userdef->print (stdout);
            }
            return;
        }
        fprintf (stderr, "bad variable name <%.*s>\n", vareq - varname, varname);
        return;
    }

    // compute value
    ComplexD val;
    if (compute (&val)) {

        // save result in saved-values array
        savedvals[nextsaved] = val;

        // print result along with saved-values array index
        lprintf (stdout, " $%u = ", ++ nextsaved);
        printval (stdout, &val);

        if (nextsaved == MAXSAVED) nextsaved = 0;
    }
}

// compute expression value
//  input:
//   lineptr = points to expression string
//  output:
//   returns 0: error message printed
//           1: successful, *val_r = resultant value
bool compute (ComplexD *val_r)
{
    try {
        *val_r = evalexpr (0);
        if (skipspaces () != 0) {
            throwerror ("extra stuff at end of line at <%s>", lineptr);
        }
        return 1;
    } catch (char *err) {
        lprintf (stderr, "%s\n", err);
        free (err);
    } catch (char const *err) {
        if (err[0] != 0) lprintf (stderr, "%s\n", err);
    }
    return 0;
}

// +, -, *, /, ** operators
ComplexD opadd (ComplexD left, ComplexD rite) { return left.add (rite); }
ComplexD opsub (ComplexD left, ComplexD rite) { return left.sub (rite); }
ComplexD opmul (ComplexD left, ComplexD rite) { return left.mul (rite); }
ComplexD opdiv (ComplexD left, ComplexD rite) { return left.div (rite); }
ComplexD oppow (ComplexD left, ComplexD rite) { return left.pow (rite); }
ComplexD oplog (ComplexD left, ComplexD rite) { return rite.log ().div (left.log ()); }
ComplexD opang (ComplexD left, ComplexD rite) { double a = rite.real * d2r; return ComplexD::make (left.real * cos (a), left.real * sin (a)); }

Op const optabl0[] = {
    { "+",  opadd },
    { "-",  opsub },
    { "",   NULL  }
};

Op const optabl1[] = {
    { "*",  opmul },
    { "/",  opdiv },
    { "",   NULL  }
};

Op const optabl2[] = {
    { "**", oppow },
    { "//", oplog },
    { "",   NULL  }
};

Op const optabl3[] = {
    { "@",  opang },
    { "",   NULL  }
};

#define NPRECED 4
Op const *const optabls[NPRECED] = { optabl0, optabl1, optabl2, optabl3 };

// evaluate expression from input string
//  input:
//   lineptr = points to input string
//   preced = 0: handle +,-,*,/,**,//,@
//            1: handle *,/,**,//,@, stop on +,-
//            2: handle **,//,@, stop on +,-,*,/
//            3: handle @, stop on +,-,*,/,**,//
//  output:
//   returns expression value
//   lineptr = updated past string evaluated
//   throws errors
ComplexD evalexpr (uint preced)
{
    uint precp1 = preced + 1;
    ComplexD leftval = (precp1 < NPRECED) ? evalexpr (precp1) : getval ();

    while (skipspaces () != 0) {
        Op const *op;
        for (op = optabls[preced]; op->name[0] != 0; op ++) {
            uint len = strlen (op->name);
            if (memcmp (lineptr, op->name, len) == 0) {
                lineptr += len;
                goto goodop;
            }
        }
        break;
    goodop:;

        ComplexD riteval = (precp1 < NPRECED) ? evalexpr (precp1) : getval ();
        leftval = op->func (leftval, riteval);
    }

    return leftval;
}

// named functions
ComplexD fnabs   () { ComplexD a = getval (); return ComplexD::make (a.abs (), 0); }
ComplexD fnacos  () { ComplexD a = getval (); return a.acos ().mulr (r2d); }
ComplexD fnarg   () { ComplexD a = getval (); return ComplexD::make (a.arg () * r2d, 0); }
ComplexD fnasin  () { ComplexD a = getval (); return a.asin ().mulr (r2d); }
ComplexD fnatan  () { ComplexD a = getval (); return a.atan ().mulr (r2d); }
ComplexD fncos   () { ComplexD a = getval (); return a.mulr (d2r).cos (); }
ComplexD fndeg   () { r2d = 180.0 / M_PI; d2r = M_PI / 180.0; return getval (); }
ComplexD fndump  () { for (UserDef *ud = userdefs; ud != NULL; ud = ud->next) ud->print (stdout); throw ""; }
ComplexD fne     () { return ComplexD::make (M_E, 0); }
ComplexD fnen    () { ComplexD a = getval (); return a.exp (); }
ComplexD fnhelp  () { int hfd = open (helpname, O_RDONLY, 0); if (hfd < 0) throw "help file missing"; char buf[128]; int rc; while ((rc = read (hfd, buf, sizeof buf)) > 0) stdout->put (buf, rc); close (hfd); throw ""; }
ComplexD fnhypot () { ComplexD a = getval (); ComplexD b = getval (); return a.sq ().add (b.sq ()).sqrt (); }
ComplexD fnim    () { ComplexD a = getval (); return ComplexD::make (a.imag, 0); }
ComplexD fnln    () { ComplexD a = getval (); return a.log (); }
ComplexD fnpi    () { return ComplexD::make (M_PI, 0); }
ComplexD fnquit  () { if (logfile != NULL) fclose (logfile); exit (0); }
ComplexD fnrad   () { r2d = 1.0; d2r = 1.0; return getval (); }
ComplexD fnre    () { ComplexD a = getval (); return ComplexD::make (a.real, 0); }
ComplexD fnsin   () { ComplexD a = getval (); return a.mulr (d2r).sin (); }
ComplexD fnsq    () { ComplexD a = getval (); return a.sq   (); }
ComplexD fnsqrt  () { ComplexD a = getval (); return a.sqrt (); }
ComplexD fntan   () { ComplexD a = getval (); return a.mulr (d2r).tan (); }

ComplexD fnload ()
{
    skipspaces ();
    FILE *f = fopen (lineptr, "r");
    if (f == NULL) throwerror ("error %d opening load file %s", errno, lineptr);
    char *lb = NULL;
    while (readinput (f, &lb)) {
        lineptr = lb;
        if (skipspaces ()) execute ();
    }
    fclose (f);
    free (lb);
    throw "";
}

ComplexD fnsave ()
{
    skipspaces ();
    FILE *f = fopen (lineptr, "w");
    if (f == NULL) throwerror ("error %d creating save file %s", errno, lineptr);
    for (UserDef *ud = userdefs; ud != NULL; ud = ud->next) {
        ud->print (f);
    }
    if (fclose (f) < 0) throwerror ("error %d closing save file %s", errno, lineptr);
    throw "";
}

Fn const fntabl[] = {
    { "abs",   fnabs   },
    { "acos",  fnacos  },
    { "arg",   fnarg   },
    { "asin",  fnasin  },
    { "atan",  fnatan  },
    { "cos",   fncos   },
    { "deg",   fndeg   },
    { "dump",  fndump  },
    { "e",     fne     },
    { "en",    fnen    },
    { "help",  fnhelp  },
    { "hypot", fnhypot },
    { "im",    fnim    },
    { "ln",    fnln    },
    { "load",  fnload  },
    { "pi",    fnpi    },
    { "quit",  fnquit  },
    { "rad",   fnrad   },
    { "re",    fnre    },
    { "save",  fnsave  },
    { "sin",   fnsin   },
    { "sq",    fnsq    },
    { "sqrt",  fnsqrt  },
    { "tan",   fntan   },
    { "",      NULL    }
};

// get next value from input string
//  input:
//   lineptr = points to input string
//  output:
//   returns value
//   lineptr = updated past value
//   throws errors
ComplexD getval ()
{
    char *endnum;
    ComplexD val;

    char ch = skipspaces ();
    if (ch == '(') {
        lineptr ++;
        val = evalexpr (0);
        if (skipspaces () != ')') {
            throwerror ("unbalanced () at <%s>", lineptr);
        }
        lineptr ++;
        goto ret;
    }
    if (ch == '-') {
        lineptr ++;
        val = getval ().neg ();
        goto ret;
    }
    if (ch == '$') {
        uint idx = strtoul (lineptr + 1, &endnum, 10) - 1;
        if ((errno != 0) || (idx >= MAXSAVED)) {
            throwerror ("bad saved number at <%s>", lineptr);
        }
        val = savedvals[idx];
        lineptr = endnum;
        goto ret;
    }
    char *varname = lineptr;
    uint varnamelen = skipvarname ();
    if (varnamelen > 0) {
        for (UserDef *ud = userdefs; ud != NULL; ud = ud->next) {
            uint len = strlen (ud->name);
            if ((len == varnamelen) && (memcmp (ud->name, varname, len) == 0)) {
                val = ud->eval ();
                goto ret;
            }
        }
        for (Fn const *fn = fntabl; fn->name[0] != 0; fn ++) {
            uint len = strlen (fn->name);
            if ((len == varnamelen) && (memcmp (fn->name, varname, len) == 0)) {
                val = fn->func ();
                goto ret;
            }
        }
        throwerror ("unknown function/variable <%.*s>", varnamelen, varname);
    }
    val.real = 0.0;
    val.imag = 0.0;
    if (ch != 'j') {
        errno = 0;
        val.real = strtod (lineptr, &endnum);
        if (errno != 0) throwerror ("unknown operand at <%s>", lineptr);
        lineptr = endnum;
        ch = skipspaces ();
    }
    if (ch == 'j') {
        errno = 0;
        val.imag = strtod (++ lineptr, &endnum);
        if (errno != 0) val.imag = 1.0;
                 else lineptr = endnum;
    }

ret:
    return val;
}

// skip spaces in input string
//  input:
//   lineptr = points to input string
//  output:
//   returns next non-blank character (or nul if end-of-string or comment)
//   lineptr = advanced to next non-blank character (or nul if end-of-string)
char skipspaces ()
{
    char ch;
    while ((ch = *lineptr) != 0)  {
        if (ch == '#') return 0;
        if (ch > ' ') break;
        lineptr ++;
    }
    return ch;
}

// varname must start with letter
// and may contain additional letters and numbers
// don't allow varname starting with "j" followed by digits cuz it looks like imaginary number
uint skipvarname ()
{
    char *varname = lineptr;
    char ch = *lineptr;
    if (((ch >= 'A') && (ch <= 'Z')) || ((ch >= 'a') && (ch <= 'z'))) {
        bool imag = ch == 'j';
        while (1) {
            ch = *(++ lineptr);
            if (((ch >= 'A') && (ch <= 'Z')) || ((ch >= 'a') && (ch <= 'z')) || (ch == '_')) {
                imag = 0;
                continue;
            }
            if ((ch < '0') || (ch > '9')) break;
        }
        if (imag) lineptr = varname;
    }
    return lineptr - varname;
}

// format and throw error message
void throwerror (char const *fmt, ...)
{
    char *buf;
    va_list ap;

    va_start (ap, fmt);
    vasprintf (&buf, fmt, ap);
    va_end (ap);
    throw buf;
}

// construct user func/var definition
UserDef::UserDef ()
{
    this->next = NULL;
    this->name = NULL;
}

// destruct user func/var definition
UserDef::~UserDef ()
{
    free (this->name);
    this->name = NULL;
}

// construct user func definition
UserFunc::UserFunc ()
{
    this->parms = NULL;
    this->body  = NULL;
}

// destruct user func definition
UserFunc::~UserFunc ()
{
    for (Parm *p; (p = this->parms) != NULL;) {
        this->parms = p->next;
        free (p);
    }
}

// evaluate user function
ComplexD UserFunc::eval ()
{
    UserDef *olduserdefs = userdefs;
    for (Parm *p = this->parms; p != NULL; p = p->next) {
        UserVar *arg = new UserVar;
        arg->name = p->name;
        arg->valu = getval ();
        arg->next = userdefs;
        userdefs  = arg;
    }
    char *oldlineptr = lineptr;
    lineptr = this->body;
    if (! compute (__retvalue)) {
        lprintf (stderr, "error computing function %s\n", this->name);
        __retvalue->real = 0;
        __retvalue->imag = 0;
    }
    lineptr = oldlineptr;
    for (UserDef *arg; (arg = userdefs) != olduserdefs;) {
        arg->name = NULL;
        userdefs  = arg->next;
        delete arg;
    }
    return *__retvalue;
}

// print user func definition
void UserFunc::print (FILE *out)
{
    lprintf (out, " %s", this->name);
    for (Parm *p = this->parms; p != NULL; p = p->next) {
        lprintf (out, " %s", p->name);
    }
    lprintf (out, " = %s\n", this->body);
}

// uservar vtable
UserVar::UserVar ();

UserVar::~UserVar () { }

// evaluate user variable
ComplexD UserVar::eval ()
{
    return this->valu;
}

// print user var definition
void UserVar::print (FILE *out)
{
    lprintf (out, " %s = ", this->name);
    printval (out, &this->valu);
}

// print complex number
void printval (FILE *out, ComplexD const *val)
{
    if (val->imag == 0.0) {
        lprintf (out, "%.16lg\n", val->real);
    } else if (val->real == 0.0) {
        lprintf (out, "j %.16lg\n", val->imag);
    } else {
        lprintf (out, "%.16lg j %.16lg = %.16lg @ %s %.16lg\n",
                val->real, val->imag, val->abs (),
                (r2d == 1.0) ? "rad" : "deg", val->arg () * r2d);
    }
}

// print line to stream and logfile
void lprintf (FILE *stream, char const *fmt, ...)
{
    va_list ap;

    va_start (ap, fmt);
    vfprintf (stream, fmt, ap);
    va_end (ap);

    if (logfile != NULL) {
        va_start (ap, fmt);
        vfprintf (logfile, fmt, ap);
        va_end (ap);
    }
}
