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

// basic circuit solver
//  make
//  ./runhw.sh circus.hex < test01.circ

#include <assert.h>
#include <complex.h>
#include <hode.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FMT "%g"
#define ISNAN isnanf
#define NAN M_NANF
#define STRTOV strtof
typedef ComplexF CVal;
typedef float SVal;
typedef __uint16_t Number;

struct Matrix;
struct Node;
struct Row;

struct Branch {
    Branch *nextbranch;
    char const *branchname;
    Number branchnum;
    Node *plusnode;
    Node *minusnode;

    void parse ();

    virtual void parsevalues () = 0;
    virtual void addtomatrix (Row *row) = 0;
};

struct Capacitor : Branch {
    SVal farads;

    virtual void parsevalues ();
    virtual void addtomatrix (Row *row);
};

struct Current : Branch {
    CVal amps;

    virtual void parsevalues ();
    virtual void addtomatrix (Row *row);
};

struct CurDepOnCurrent : Branch {
    CVal ratio;
    char *depbranchname;

    virtual void parsevalues ();
    virtual void addtomatrix (Row *row);
};

struct CurDepOnVoltage : Branch {
    CVal ratio;
    char *depposnodename;
    char *depnegnodename;

    virtual void parsevalues ();
    virtual void addtomatrix (Row *row);
};

struct Inductor : Branch {
    SVal henries;

    virtual void parsevalues ();
    virtual void addtomatrix (Row *row);
};

struct Resistor : Branch {
    SVal ohms;

    virtual void parsevalues ();
    virtual void addtomatrix (Row *row);
};

struct Voltage : Branch {
    CVal volts;

    virtual void parsevalues ();
    virtual void addtomatrix (Row *row);
};

struct VoltDepOnCurrent : Branch {
    CVal ratio;
    char *depbranchname;

    virtual void parsevalues ();
    virtual void addtomatrix (Row *row);
};

struct VoltDepOnVoltage : Branch {
    CVal ratio;
    char *depposnodename;
    char *depnegnodename;

    virtual void parsevalues ();
    virtual void addtomatrix (Row *row);
};

struct Node {
    Node *nextnode;
    char const *nodename;
    Number nodenum;
};

struct Row {
    Matrix *matrix;
    CVal *columns;

    virtual ~Row ();
    Row *init (Matrix *matrix);

    void addtonodecolumn (Node *node, CVal value);
    void addtonodecolumnr (Node *node, SVal val);
    void addtobranchcolumn (Branch *branch, CVal value);
    void addtobranchcolumnr (Branch *branch, SVal val);
    void addtobranchcolumni (Branch *branch, SVal val);
    void addtoconstantcolumn (CVal value);

    CVal getconstantcolumn ();
};

struct Matrix {
    Number numnodes;
    Number numbranches;
    Row **rowslist;

    Node *nodelist;
    Branch *branchlist;

    Matrix ();
    virtual ~Matrix ();
    void setup ();
    Row *getnoderow (Node *node);
    Row *getbranchrow (Branch *branch);
    void solve ();
    void print ();
    void reset ();
};

static char linebuf[256];
static char *lineptr;
static Matrix *matrix;
static SVal womega;

static bool readline ();
static bool ateol ();
static char getdelim ();
static void putdelim (char delim);
static char *getname ();
static SVal getreal ();
static CVal getcomplex ();
static Node *findormakenamednode (char *name);
static Node *findnodebyname (char const *name);
static Branch *findbranchbyname (char const *name);
static void acanalysis ();

////////////
//  main  //
////////////

int main (int argc, char **argv)
{
    matrix = new Matrix;

    // inductor  L1 (source, ground) 4.7e-6
    // current   I2 (source, ground) 0.1
    // capacitor C1 (source, vinput) 0.01e-6
    // capacitor C2 (vinput, ground) 470e-12
    // inductor  L2 (vinput, middle) 2e-6
    // capacitor C3 (middle, ground) 940e-12
    // inductor  L3 (middle, voutput) 2e-6
    // capacitor C4 (voutput, ground) 470e-12
    // resistor  R1 (voutput, ground) 50

    while (readline ()) {
        if (ateol ()) continue;

        char *bt = getname ();
        if (bt == NULL) {
            throw "expecting command";
        }


        // branchtype ...
        if (strcasecmp (bt, "capacitor") == 0) {
            new Capacitor->parse ();
        }
        else if (strcasecmp (bt, "current") == 0) {
            new Current->parse ();
        }
        else if (strcasecmp (bt, "curdeponcurrent") == 0) {
            new CurDepOnCurrent->parse ();
        }
        else if (strcasecmp (bt, "curdeponvoltage") == 0) {
            new CurDepOnVoltage->parse ();
        }
        else if (strcasecmp (bt, "inductor") == 0) {
            Inductor *inductor = new Inductor;
            inductor->parse ();
        }
        else if (strcasecmp (bt, "resistor") == 0) {
            new Resistor->parse ();
        }
        else if (strcasecmp (bt, "voltage") == 0) {
            new Voltage->parse ();
        }
        else if (strcasecmp (bt, "voltdeponcurrent") == 0) {
            new VoltDepOnCurrent->parse ();
        }
        else if (strcasecmp (bt, "voltdeponvoltage") == 0) {
            new VoltDepOnVoltage->parse ();
        }

        // command ...
        else if (strcasecmp (bt, "acanal") == 0) {
            acanalysis ();
        }

        // unknown
        else {
            throw "unknown command";
        }

        free (bt);
    }

    return 0;
}

////////////////////////
//  parse input file  //
////////////////////////

static bool readline ()
{
    lineptr = fgets (linebuf, sizeof linebuf, stdin);
    if (lineptr == NULL) return 0;
    fputs (lineptr, stdout);
    if (strchr (lineptr, '\n') == NULL) throw "line too long";
    char *p = strchr (lineptr, '#');
    if (p != NULL) *p = 0;
    return 1;
}

static bool ateol ()
{
    char c;
    while ((c = *lineptr) <= ' ') {
        if (c == 0) return 1;
        lineptr ++;
    }
    return 0;
}

static char getdelim ()
{
    if (ateol ()) return 0;
    return *(lineptr ++);
}

static void putdelim (char delim)
{
    if (delim != 0) -- lineptr;
}

static char *getname ()
{
    if (ateol ()) return NULL;
    char *p = lineptr;
    char c = *p;
    if (((c < 'A') || (c > 'Z')) && ((c < 'a') || (c > 'z')) && (c != '_')) return NULL;
    do c = *(++ p);
    while (((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')) || ((c >= '0') && (c <= '9')) || (c == '_'));
    int len = p - lineptr;
    char *name = malloc (len + 1);
    memcpy (name, lineptr, len);
    name[len] = 0;
    lineptr = p;
    return name;
}

static SVal getreal ()
{
    if (ateol ()) return NAN;
    char *p = lineptr;
    char c = *p;
    if (c == '-') c = *(++ p);
    if (c == '.') c = *(++ p);
    if ((c >= '0') && (c <= '9')) {
        return STRTOV (lineptr, &lineptr);
    }
    return NAN;
}

static CVal getcomplex ()
{
    CVal cval;
    cval.imag = NAN;
    cval.real = getreal ();
    if (! ISNAN (cval.real)) {
        char delim = getdelim ();
        if (delim != '@') {
            putdelim (delim);
            cval.imag = 0.0;
        } else {
            cval.imag = getreal ();
        }
    }
    return cval;
}

// parse branchname (posnodename,negnodename) ...
//  fills in matrix->nodelist, matrix->branchlist, branch values
void Branch::parse ()
{
    char const *bn = getname ();
    if (bn == NULL) throw "expecting branch name";
    this->branchname = bn;

    char delim = getdelim ();
    if (delim != '(') throw "expecting '(' before plus node name";
    char *pnn = getname ();
    if (pnn == NULL) throw "expecting plus node name";
    this->plusnode = findormakenamednode (pnn);
    watchwrite (&this->plusnode);
    assert (this->plusnode != NULL);
    delim = getdelim ();
    if (delim != ',') throw "expecting ',' after plus node name";
    char *mnn = getname ();
    if (mnn == NULL) throw "expecting minus node name";
    this->minusnode = findormakenamednode (mnn);
    assert (this->minusnode != NULL);
    delim = getdelim ();
    if (delim != ')') throw "expecting ')' after minus node name";

    this->parsevalues ();

    if (! ateol ()) throw "expecting end-of-line";

    assert (this->plusnode != NULL);
    assert (this->minusnode != NULL);
    watchwrite (NULL);

    this->nextbranch = matrix->branchlist;
    matrix->branchlist = this;
}

static Node *findormakenamednode (char *name)
{
    Node **lnode, *node;
    for (lnode = &matrix->nodelist; (node = *lnode) != NULL; lnode = &node->nextnode) {
        if (strcmp (node->nodename, name) == 0) {
            free (name);
            return node;
        }
    }
    node = new Node;
    node->nextnode = NULL;
    node->nodename = name;
    *lnode = node;
    return node;
}

static Node *findnodebyname (char const *name)
{
    Node *node;
    for (node = matrix->nodelist; node != NULL; node = node->nextnode) {
        if (strcmp (node->nodename, name) == 0) return node;
    }
    char *msg;
    asprintf (&msg, "undefined node '%s", name);
    throw msg;
}

static Branch *findbranchbyname (char const *name)
{
    Branch *branch;
    for (branch = matrix->branchlist; branch != NULL; branch = branch->nextbranch) {
        if (strcmp (branch->branchname, name) == 0) return branch;
    }
    char *msg;
    asprintf (&msg, "undefined branch '%s", name);
    throw msg;
}

void Capacitor::parsevalues ()
{
    this->farads = getreal ();
    if (ISNAN (this->farads)) throw "expecting capacitor farads";
}

void Current::parsevalues ()
{
    this->amps = getcomplex ();
    if (ISNAN (this->amps.real)) throw "expecting current amps";
}

void CurDepOnCurrent::parsevalues ()
{
    this->ratio = getcomplex ();
    if (ISNAN (this->ratio.real)) throw "expecting current ratio";
    this->depbranchname = getname ();
    if (this->depbranchname == NULL) throw "expecting dependent branch name";
}

void CurDepOnVoltage::parsevalues ()
{
    this->ratio = getcomplex ();
    if (ISNAN (this->ratio.real)) throw "expecting amps/volts ratio";
    this->depposnodename = getname ();
    if (this->depposnodename == NULL) throw "expecting dependent positive node name";
    this->depnegnodename = getname ();
    if (this->depnegnodename == NULL) throw "expecting dependent negative node name";
}

void Inductor::parsevalues ()
{
    this->henries = getreal ();
    if (ISNAN (this->henries)) throw "expecting inductor henries";
}

void Resistor::parsevalues ()
{
    this->ohms = getreal ();
    if (ISNAN (this->ohms)) throw "expecting resistor ohms";
}

void Voltage::parsevalues ()
{
    this->volts = getcomplex ();
    if (ISNAN (this->volts.real)) throw "expecting voltage volts";
}

void VoltDepOnCurrent::parsevalues ()
{
    this->ratio = getcomplex ();
    if (ISNAN (this->ratio.real)) throw "expecting volts/amps ratio";
    this->depbranchname = getname ();
    if (this->depbranchname == NULL) throw "expecting dependent branch name";
}

void VoltDepOnVoltage::parsevalues ()
{
    this->ratio = getcomplex ();
    if (ISNAN (this->ratio.real)) throw "expecting voltage ratio";
    this->depposnodename = getname ();
    if (this->depposnodename == NULL) throw "expecting dependent positive node name";
    this->depnegnodename = getname ();
    if (this->depnegnodename == NULL) throw "expecting dependent negative node name";
}

///////////////////////
//  do computations  //
///////////////////////

static void acanalysis ()
{
    SVal start = getreal ();
    SVal stop  = getreal ();
    SVal step  = getreal ();

    if (ISNAN (step)) {
        throw "missing start, stop, step frequencies";
    }

    for (SVal freq = start; freq <= stop; freq += step) {
        womega = freq * (SVal) (2.0 * M_PI);

        printf ("\nresults freq "FMT":\n", freq);

        try {
            matrix->setup ();
            //printf ("\nbefore:\n");
            //matrix->print ();
            matrix->solve ();
            //printf ("\nafter:\n");
            //matrix->print ();
        } catch (char const *msg) {
            fprintf (stderr, "error: %s\n", msg);
            matrix->print ();
            return;
        }

        // 0x2220 = 0xE2 0x88 0xA0 = angle symbol https://www.compart.com/en/unicode/U+2220

        printf ("\n  voltages:\n\n");
        for (Node *node = matrix->nodelist; node != NULL; node = node->nextnode) {
            if (node->nodenum > 0) {
                CVal v = matrix->rowslist[node->nodenum-1]->getconstantcolumn ();
                SVal a = v.arg () * (SVal) (180.0 / M_PI);
                printf ("    v_%03u  %s = "FMT" + j "FMT" = "FMT" \342\210\240 "FMT"\302\260\n", node->nodenum, node->nodename, v.real, v.imag, v.abs (), a);
            }
        }
        printf ("\n  currents:\n\n");
        for (Branch *branch = matrix->branchlist; branch != NULL; branch = branch->nextbranch) {
            CVal i = matrix->rowslist[matrix->numnodes+branch->branchnum-2]->getconstantcolumn ();
            SVal a = i.arg () * (SVal) (180.0 / M_PI);
            printf ("    i_%03u  %s = "FMT" + j "FMT" = "FMT" \342\210\240 "FMT"\302\260\n", branch->branchnum, branch->branchname, i.real, i.imag, i.abs (), a);
        }
        printf ("\n");

        matrix->reset ();
    }
}

/////////////////////////////
//  Branches (components)  //
/////////////////////////////

Branch::Branch ();
Capacitor::Capacitor ();
Current::Current ();
CurDepOnCurrent::CurDepOnCurrent ();
CurDepOnVoltage::CurDepOnVoltage ();
Inductor::Inductor ();
Resistor::Resistor ();
Voltage::Voltage ();
VoltDepOnCurrent::VoltDepOnCurrent ();
VoltDepOnVoltage::VoltDepOnVoltage ();

// add capacitor to matrix
//  plusnode  = resistor takes current from plus node
//  minusnode = resistor gives current to minus node
//  farads    = how many farads
void Capacitor::addtomatrix (Row *row)
{
    // r = 1 / (j * w * c) = - j / (w * c)
    // v = i * r

    // v = vp - vm
    // i = ib

    //  1 * vp - 1 * vm - R * ib = 0
    // -1 * vp + 1 * vm + R * ib = 0
    row->addtonodecolumnr (this->plusnode, -1);
    row->addtonodecolumnr (this->minusnode, 1);
    row->addtobranchcolumni (this, -1.0 / (womega * this->farads));
}

// add current source to matrix
//  plusnode  = positive end of current source
//  minusnode = negative end of current source
//  amps      = how many amps from minusnode to plusnode
void Current::addtomatrix (Row *row)
{
    // current in the branch is this many amps
    //  1 * ib = amps
    row->addtobranchcolumnr (this, 1);
    row->addtoconstantcolumn (this->amps);
}

void CurDepOnCurrent::addtomatrix (Row *row)
{
    // ibranch = ratio * idepend
    //  -1 * ibanch + ratio * idepend = 0
    row->addtobranchcolumnr (this, -1);
    row->addtobranchcolumn (findbranchbyname (this->depbranchname), this->ratio);
}

void CurDepOnVoltage::addtomatrix (Row *row)
{
    // ibranch = ratio * (iposvolts - inegvolts)
    //  -1 * ibanch + ratio * iposvolts - ratio * inegvolts = 0
    row->addtobranchcolumnr (this, -1);
    row->addtonodecolumn (findnodebyname (this->depposnodename), this->ratio);
    row->addtonodecolumn (findnodebyname (this->depnegnodename), this->ratio.neg ());
}

// add inductor to matrix
//  plusnode  = resistor takes current from plus node
//  minusnode = resistor gives current to minus node
//  henries   = how many henries
void Inductor::addtomatrix (Row *row)
{
    // r = j * w * l
    // v = i * r

    // v = vp - vm
    // i = ib

    //  1 * vp - 1 * vm - R * ib = 0
    // -1 * vp + 1 * vm + R * ib = 0
    row->addtonodecolumnr (this->plusnode, -1);
    row->addtonodecolumnr (this->minusnode, 1);
    row->addtobranchcolumni (this, womega * this->henries);
}

// add resistor to matrix
//  plusnode  = resistor takes current from plus node
//  minusnode = resistor gives current to minus node
//  ohms      = how many ohms
void Resistor::addtomatrix (Row *row)
{
    // v = i * r

    // v = vp - vm
    // i = ib

    //  1 * vp - 1 * vm - R * ib = 0
    // -1 * vp + 1 * vm + R * ib = 0
    row->addtonodecolumnr (this->plusnode, -1);
    row->addtonodecolumnr (this->minusnode, 1);
    row->addtobranchcolumnr (this, this->ohms);
}

// add voltage source to matrix
//  plusnode  = positive end of voltage source
//  minusnode = negative end of voltage source
//  volts     = how many volts from minusnode to plusnode
void Voltage::addtomatrix (Row *row)
{
    // vp - vm = V
    row->addtonodecolumnr (this->plusnode, 1);
    row->addtonodecolumnr (this->minusnode, -1);
    row->addtoconstantcolumn (this->volts);
}

void VoltDepOnCurrent::addtomatrix (Row *row)
{
    // vp - vm = ratio * idepend
    // - vp + vm + ratio * idepend = 0
    row->addtonodecolumnr (this->plusnode, -1);
    row->addtonodecolumnr (this->minusnode, 1);
    row->addtobranchcolumn (findbranchbyname (this->depbranchname), this->ratio);
}

void VoltDepOnVoltage::addtomatrix (Row *row)
{
    // vp - vm = ratio * (iposvolts - inegvolts)
    // - vp + vm + ratio * iposvolts - ratio * inegvolts = 0
    row->addtonodecolumnr (this->plusnode, -1);
    row->addtonodecolumnr (this->minusnode, 1);
    row->addtonodecolumn (findnodebyname (this->depposnodename), this->ratio);
    row->addtonodecolumn (findnodebyname (this->depnegnodename), this->ratio.neg ());
}

//////////////
//  Matrix  //
//////////////

// matrix:
//  vnode1  vnode2  ...  ibranch1  ibranch2  ibranch3  ...  =  constant
//  ------  ------  ---  --------  --------  --------  ---     --------
//  row for currents going into node0 = ground
//  row for currents going into node1
//  row for currents going into node2
//    .
//    .
//    .
//  row for branch1
//  row for branch2
//  row for branch3
//    .
//    .
//    .

Matrix::Matrix ()
{
    this->numbranches = 0;
    this->rowslist = NULL;
    this->branchlist = NULL;

    this->numnodes = 1;
    this->nodelist = new Node;
    this->nodelist->nextnode = NULL;
    this->nodelist->nodename = "ground";
}

Matrix::~Matrix ()
{
    this->reset ();
}

Row *Matrix::getnoderow (Node *node)
{
    assert (node->nodenum < this->numnodes);
    return this->rowslist[node->nodenum];
}

Row *Matrix::getbranchrow (Branch *branch)
{
    assert ((branch->branchnum > 0) && (branch->branchnum <= this->numbranches));
    return this->rowslist[this->numnodes+branch->branchnum-1];
}

// all branches and nodes have been added to the matrix, fill in matrix with component values
void Matrix::setup ()
{
    // number all the branches on the list starting at one
    this->numbranches = 0;
    for (Branch *branch = this->branchlist; branch != NULL; branch = branch->nextbranch) {
        branch->branchnum = ++ this->numbranches;
    }

    // number all the nodes on the list starting at zero
    // node 0 is "ground", all other voltages computed relative to ground
    this->numnodes = 0;
    for (Node *node = this->nodelist; node != NULL; node = node->nextnode) {
        if (this->numnodes == 0) {
            assert (strcmp (node->nodename, "ground") == 0);
        }
        node->nodenum = this->numnodes ++;
    }

    // allocate matrix memory
    Number numrows = this->numnodes + this->numbranches;
    this->rowslist = malloc (numrows * sizeof *this->rowslist);
    for (Number i = numrows; i > 0;) {
        this->rowslist[--i] = new Row->init (this);
    }

    // fill in matrix with component values
    for (Branch *branch = this->branchlist; branch != NULL; branch = branch->nextbranch) {

        // compute current through branch
        Row *row = this->getbranchrow (branch);
        branch->addtomatrix (row);

        // all currents entering a node = all currents leaving a node
        this->getnoderow (branch->plusnode)->addtobranchcolumnr (branch, 1);
        this->getnoderow (branch->minusnode)->addtobranchcolumnr (branch, -1);
    }
}

// solve matrix to the form:
//   1  0  0  k1
//   0  1  0  k2
//   0  0  1  k3
//   0  0  0   0
void Matrix::solve ()
{
    // number of rows = number of cols
    Number numrows = this->numnodes + this->numbranches;

    // get all major diagonal values 1.0 except for last row
    // get all values below major diagonal 0.0
    for (Number row = 1; row < numrows; row ++) {

        // point to the row being processed
        Row *rowptr = this->rowslist[row-1];

        // pivot = abs (value from row on major diagonal)
        SVal pivav = rowptr->columns[row-1]->abs ();

        // find row below this one that has larger value in same column as this one's pivot
        // swap this one with it so we get the largest value on the major diagonal
        // this keeps us from using zero or very small values
        for (Number r = row; ++ r <= numrows;) {
            Row *rptr = this->rowslist[r-1];
            SVal pav = rptr->columns[row-1]->abs ();
            if (pav > pivav) {
                this->rowslist[r-1] = rowptr;
                this->rowslist[row-1] = rowptr = rptr;
                pivav = pav;
            }
        }

        if (pivav == 0.0) {
            char *err;
            char const *obj = NULL;
            char const *of  = NULL;
            if (row <= this->numnodes) {
                for (Node *node = this->nodelist; node != NULL; node = node->nextnode) {
                    if (node->nodenum == row) {
                        of  = "voltage of";
                        obj = node->nodename;
                        break;
                    }
                }
            } else {
                Number bn = row - this->numnodes;
                for (Branch *branch = this->branchlist; branch != NULL; branch = branch->nextbranch) {
                    if (branch->branchnum == bn) {
                        of  = "current through";
                        obj = branch->branchname;
                        break;
                    }
                }
            }
            asprintf (&err, "missing pivot value row/col %u, cannot compute %s %s", row, of, obj);
            throw (char const *) err;
        }

        // normalize row such that pivot is 1.0
        CVal pivot = rowptr->columns[row-1];
        for (Number col = row; col <= numrows; col ++) {
            CVal *p = &rowptr->columns[col-1];
            p->diveq (pivot);
        }

        // subtract a multiple of this row from all rows below it such that those rows
        // get zero in this row's major diagonal column
        for (Number r = row; ++ r <= numrows;) {
            Row *rptr = this->rowslist[r-1];
            CVal piv = rptr->columns[row-1];
            CVal *q = rowptr->columns;
            CVal *p = rptr->columns;
            for (Number col = numrows; col > 0; -- col) {
                p->subeq (q->mul (piv));
                p ++;
                q ++;
            }
        }
    }

    // get all values above major diagonal 0.0
    for (Number row = numrows; row > 1;) {
        Row *rowptr = this->rowslist[--row-1];
        for (Number r = row; r > 1;) {
            Row *rptr = this->rowslist[--r-1];
            CVal piv = rptr->columns[row-1];
            CVal *q = rowptr->columns;
            CVal *p = rptr->columns;
            for (Number col = numrows; col > 0; -- col) {
                p->subeq (q->mul (piv));
                p ++;
                q ++;
            }
        }
    }
}

// print matrix for debugging
void Matrix::print ()
{
    for (Number vn = 0; ++ vn < this->numnodes;) {
        printf ("              v_%03u             ", vn);
    }
    printf ("  ");
    for (Number in = 0; ++ in <= this->numbranches;) {
        printf ("              i_%03u             ", in);
    }
    printf ("                 =\n");

    Number numrows = this->numnodes + this->numbranches;
    for (Number row = 1; row <= numrows; row ++) {
        Row *rowptr = this->rowslist[row-1];
        CVal *p = rowptr->columns;
        for (Number col = 0; ++ col <= numrows;) {
            if ((col == this->numnodes) || (col == numrows)) printf ("  ");
            printf (" [ %11.4f + j %11.4f ]", p->real, p->imag);
            p ++;
        }
        printf ("\n");
    }
}

// free off memory allocated by setup()
void Matrix::reset ()
{
    if (this->rowslist != NULL) {
        Number numrows = this->numnodes + this->numbranches;
        for (Number i = 0; i < numrows; i ++) {
            delete this->rowslist[i];
        }
        free (this->rowslist);
        this->rowslist = NULL;
    }
}

///////////
//  Row  //
///////////

Row::Row ();

//  1..nodecols-1   1..branchcols
//  <nodecolumns>  <branchcolumns>  <constcolumn>

Row *Row::init (Matrix *matrix)
{
    this->matrix = matrix;
    this->columns = calloc (matrix->numnodes + matrix->numbranches, sizeof *this->columns);
    return this;
}

Row::~Row ()
{
    free (this->columns);
    this->columns = NULL;
}

void Row::addtonodecolumn (Node *node, CVal val)
{
    assert (node->nodenum < this->matrix->numnodes);
    if (node->nodenum > 0) {
        this->columns[node->nodenum-1].addeq (val);
    }
}

void Row::addtonodecolumnr (Node *node, SVal val)
{
    assert (node->nodenum < this->matrix->numnodes);
    if (node->nodenum > 0) {
        this->columns[node->nodenum-1].real += val;
    }
}

void Row::addtobranchcolumn (Branch *branch, CVal val)
{
    assert ((branch->branchnum > 0) && (branch->branchnum <= this->matrix->numbranches));
    this->columns[this->matrix->numnodes+branch->branchnum-2].addeq (val);
}

void Row::addtobranchcolumnr (Branch *branch, SVal val)
{
    assert ((branch->branchnum > 0) && (branch->branchnum <= this->matrix->numbranches));
    this->columns[this->matrix->numnodes+branch->branchnum-2].real += val;
}

void Row::addtobranchcolumni (Branch *branch, SVal val)
{
    assert ((branch->branchnum > 0) && (branch->branchnum <= this->matrix->numbranches));
    this->columns[this->matrix->numnodes+branch->branchnum-2].imag += val;
}

void Row::addtoconstantcolumn (CVal val)
{
    this->columns[this->matrix->numnodes+this->matrix->numbranches-1].addeq (val);
}

CVal Row::getconstantcolumn ()
{
    return this->columns[this->matrix->numnodes+this->matrix->numbranches-1];
}
