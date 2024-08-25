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

// VC8 small computer handbook PDP-8/I, 1970, p87

// can emulate VC8/E or VC8/I
// envars:
//   vc8type=e or =i to select which (required)
//   vc8pms=<persistence milliseconds> (optional, default 500)
//   vc8size=<x and y size in pixels> (optional, default 1024)

#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>

#include "iodevvc8.h"
#include "rdcyc.h"

#define MAXBUFFPTS 65536
#define DEFPERSISTMS 500    // how long non-storage points last
#define MINPERSISTMS 10     // ...smaller and the up/down buttons won't work
#define MAXPERSISTMS 60000  // ...larger and it will overflow 16-bit counter

#define XYSIZE 1024
#define DEFWINSIZE XYSIZE
#define MINWINSIZE 64
#define MAXWINSIZE XYSIZE
#define BORDERWIDTH 2

#define EF_DN 04000     // done executing last command
#define EF_WT 00040     // de-focus beam for writing
#define EF_ST 00020     // enable storage mode
#define EF_ER 00010     // erase storage screen (450ms)
#define EF_CO 00004     // 1=red; 0=green
#define EF_CH 00002     // display select
#define EF_IE 00001     // interrupt enable

#define EF_MASK 077

struct Button {
    Button (IODevVC8 *vc8);
    virtual ~Button ();

    virtual void draw (unsigned long color) = 0;
    bool clicked ();

protected:
    IODevVC8 *vc8;

    bool lastpressed;
    int xleft, xrite, ytop, ybot;
};

struct ClearButton : Button {
    ClearButton (IODevVC8 *vc8);
    void position ();
    virtual void draw (unsigned long color);
};

struct TriButton : Button {
    TriButton (IODevVC8 *vc8, bool upbut);
    void position ();
    virtual void draw (unsigned long color);

private:
    bool upbut;
    XSegment segs[3];
};

struct PersisButton {
    TriButton upbutton;
    TriButton dnbutton;

    PersisButton (IODevVC8 *vc8);
    void setpms (uint16_t pms);
    void draw (unsigned long color);

private:
    IODevVC8 *vc8;

    char pmsbuf[12];
    int pmslen;
};

IODevVC8 iodevvc8;

static IODevOps const iodevEops[] = {
    { 06050, "DILC (VC8/E) Logic Clear" },
    { 06051, "DICD (VC8/E) Clear Done flag" },
    { 06052, "DISD (VC8/E) Skip on Done" },
    { 06053, "DILX (VC8/E) Load X" },
    { 06054, "DILY (VC8/E) Load Y" },
    { 06055, "DIXY (VC8/E) Intensify at (X,Y)" },
    { 06056, "DILE (VC8/E) Load Enable/Status register" },
    { 06057, "DIRE (VC8/E) Read Enable/Status register" }
};

static IODevOps const iodevIops[] = {
    { 06051, "DCX (VC8/I) Clear X-Coordinate Buffer" },
    { 06053, "DXL (VC8/I) Load X-Coordinate Buffer" },
    { 06054, "DIX (VC8/I) Intensify" },
    { 06057, "DXS (VC8/I) X-Coordinate Sequence" },
    { 06061, "DCY (VC8/I) Clear Y-Coordinate Buffer" },
    { 06063, "DYL (VC8/I) Clear and Load Y-Coordinate BUffer" },
    { 06064, "DIY (VC8/I) Intensify" },
    { 06067, "DYS (VC8/I) Y-Coordinate Sequence" },
    { 06074, "DSB (VC8/I) Set Brightness Control to 0" },
    { 06075, "DSB (VC8/I) Set Brightness Control to 1" },
    { 06076, "DSB (VC8/I) Set Brightness Control to 2" },
    { 06077, "DSB (VC8/I) Set Brightness Control to 3" }
};

static int xioerror (Display *xdis);

IODevVC8::IODevVC8 ()
{
    // get display type, 'e' for VC8/E, 'i' for VC8/I
    // if not defined, display is disabled
    this->type = VC8TypeN;
    char const *envtype = getenv ("vc8type");
    if (envtype == NULL) return;

    // maybe override default ephemeral persistence
    this->pms = DEFPERSISTMS;
    char const *envpms = getenv ("vc8pms");
    if (envpms != NULL) {
        int intpms = atoi (envpms);
        if (intpms < MINPERSISTMS) intpms = MINPERSISTMS;
        if (intpms > MAXPERSISTMS) intpms = MAXPERSISTMS;
        this->pms = intpms;
    }

    // decode display type
    switch (envtype[0]) {
        case 'E': case 'e': {
            this->type = VC8TypeE;
            opscount   = sizeof iodevEops / sizeof iodevEops[0];
            opsarray   = iodevEops;
            iodevname  = "vc8e";
            iodevs[05] = this;
            break;
        }
        case 'I': case 'i': {
            this->type = VC8TypeI;
            opscount   = sizeof iodevIops / sizeof iodevIops[0];
            opsarray   = iodevIops;
            iodevname  = "vc8i";
            iodevs[05] = this;
            iodevs[06] = this;
            iodevs[07] = this;
            break;
        }
        default: {
            fprintf (stderr, "IODevVC8::IODevVC8: bad vc8type %s - should be E or I\n", envtype);
            ABORT ();
        }
    }

    fprintf (stderr, "IODevVC8::IODevVC8: device type %s defined\n", iodevname);

    if (pthread_cond_init (&this->cond, NULL) != 0) ABORT ();
    if (pthread_mutex_init (&this->lock, NULL) != 0) ABORT ();

    this->resetting = false;
    this->buffer    = NULL;
    this->threadid  = 0;
    this->insert    = 0;
    this->remove    = 0;
    this->xcobuf    = 0;
    this->ycobuf    = 0;
    this->intens    = 3;
    this->eflags    = 0;
    this->waiting   = false;
    this->xdis      = NULL;
    this->xwin      = 0;
    this->xgc       = NULL;
    this->allpoints = NULL;
    this->indices   = NULL;
}

IODevVC8::~IODevVC8 ()
{
    this->ioreset ();
}

// reset the device
// - close window, kill thread
void IODevVC8::ioreset ()
{
    if (this->type == VC8TypeN) return;

    pthread_mutex_lock (&this->lock);

    if (this->resetting) {
        do pthread_cond_wait (&this->cond, &this->lock);
        while (this->resetting);
    } else {
        this->resetting = true;

        if (this->threadid != 0) {
            pthread_cond_broadcast (&this->cond);
            pthread_mutex_unlock (&this->lock);
            pthread_join (this->threadid, NULL);
            pthread_mutex_lock (&this->lock);
            this->threadid = 0;
            free (this->buffer);
            this->buffer = NULL;
        }

        switch (this->type) {
            case VC8TypeE: {
                this->eflags = 0;
                break;
            }
            case VC8TypeI: {
                this->eflags = EF_ST;
                this->intens = 3;
                break;
            }
            default: ABORT ();
        }

        this->resetting = false;
        pthread_cond_broadcast (&this->cond);
    }

    pthread_mutex_unlock (&this->lock);
}

// load/unload files
SCRet *IODevVC8::scriptcmd (int argc, char const *const *argv)
{
    return new SCRetErr ("unknown %s command %s", iodevname, argv[0]);
}

// perform i/o instruction
uint16_t IODevVC8::ioinstr (uint16_t opcode, uint16_t input)
{
    pthread_mutex_lock (&this->lock);

    switch (this->type) {

        // VC8/E
        case VC8TypeE: {
            switch (opcode) {

                // DILC (VC8/E) Logic Clear
                case 06050: {
                    this->eflags = 0;
                    break;
                }

                // DICD (VC8/E) Clear Done flag
                case 06051: {
                    this->eflags &= ~ EF_DN;
                    break;
                }

                // DISD (VC8/E) Skip on Done
                case 06052: {
                    if (this->eflags & EF_DN) input |= IO_SKIP;
                    else skipoptwait (opcode, &this->lock, &this->waiting);
                    break;
                }

                // DILX (VC8/E) Load X
                case 06053: {
                    this->xcobuf  = (input + XYSIZE / 2) % XYSIZE;
                    this->eflags |= EF_DN;
                    break;
                }

                // DILY (VC8/E) Load Y
                case 06054: {
                    this->ycobuf  = (input + XYSIZE / 2) % XYSIZE;
                    this->eflags |= EF_DN;
                    break;
                }

                // DIXY (VC8/E) Intensify at (X,Y)
                case 06055: {
                    this->insertpoint (this->eflags & EF_CO ? 5 : 4);
                    break;
                }

                // DILE (VC8/E) Load Enable/Status register
                case 06056: {
                    this->eflags = (this->eflags & EF_DN) | (input & EF_MASK);
                    this->wakethread ();
                    input &= IO_LINK;
                    break;
                }

                // DIRE (VC8/E) Read Enable/Status register
                case 06057: {
                    input = (input & IO_LINK) | this->eflags;
                    break;
                }

                default: {
                    input = UNSUPIO;
                    break;
                }
            }
            this->updintreq ();
            break;
        }

        // VC8/I
        case VC8TypeI: {
            switch (opcode) {
                case 06051 ... 06057: {
                    if (opcode & 1) this->xcobuf  = 0;
                    if (opcode & 2) this->xcobuf |= input % XYSIZE;
                    if (opcode & 4) this->insertpoint (this->intens);
                    break;
                }

                case 06061 ... 06067: {
                    if (opcode & 1) this->ycobuf  = 0;
                    if (opcode & 2) this->ycobuf |= input % XYSIZE;
                    if (opcode & 4) this->insertpoint (this->intens);
                    break;
                }

                case 06074:     // dk gray
                case 06075:     // gray
                case 06076:     // lt gray
                case 06077: {   // white
                    this->intens = opcode & 3;
                    break;
                }

                default: {
                    input = UNSUPIO;
                    break;
                }
            }
            break;
        }

        default: {
            input = UNSUPIO;
            break;
        }
    }

    pthread_mutex_unlock (&this->lock);
    return input;
}

// queue point to thread to display it
// start thread if not already running to open window
void IODevVC8::insertpoint (uint16_t t)
{
    // start thread if this is first point ever
    // otherwise, just wake the thread to process point
    if (this->threadid == 0) {
        this->wakethread ();
    } else if ((this->insert + MAXBUFFPTS - this->remove) % MAXBUFFPTS >= MAXBUFFPTS / 2) {
        pthread_cond_broadcast (&this->cond);
    }

    // queue the point
    // remove oldest point if queue overflow
    int ins = this->insert;
    this->buffer[ins].x = this->xcobuf;
    this->buffer[ins].y = this->ycobuf;
    this->buffer[ins].t = t;
    this->insert = ins = (ins + 1) % MAXBUFFPTS;
    if (ins == this->remove) this->remove = (this->remove + 1) % MAXBUFFPTS;
}

// wake thread or start it
void IODevVC8::wakethread ()
{
    if (this->threadid == 0) {
        int rc = createthread (&this->threadid, threadwrap, this);
        if (rc != 0) ABORT ();
        this->buffer = (VC8Pt *) malloc (MAXBUFFPTS * sizeof *this->buffer);
        if (this->buffer == NULL) ABORT ();
    } else {
        pthread_cond_broadcast (&this->cond);
    }
}

// thread what does the display I/O
void *IODevVC8::threadwrap (void *zhis)
{
    ((IODevVC8 *)zhis)->thread ();
    return NULL;
}

void IODevVC8::thread ()
{
    // get initial window size
    this->winsize = DEFWINSIZE;
    char const *envsize = getenv ("vc8size");
    if (envsize != NULL) {
        this->winsize = atoi (envsize);
        if (this->winsize < MINWINSIZE) this->winsize = MINWINSIZE;
        if (this->winsize > MAXWINSIZE) this->winsize = MAXWINSIZE;
    }

    // open connection to XServer
    this->xdis = XOpenDisplay (NULL);
    if (this->xdis == NULL) {
        fprintf (stderr, "IODevVC8::thread: error opening X display\n");
        ABORT ();
    }

    // set up handler to be called when window closed
    XSetIOErrorHandler (xioerror);

    // create and display a window
    this->blackpixel = XBlackPixel (this->xdis, 0);
    this->whitepixel = XWhitePixel (this->xdis, 0);
    Window xrootwin = XDefaultRootWindow (this->xdis);
    this->xwin = XCreateSimpleWindow (this->xdis, xrootwin, 100, 100,
        this->winsize + BORDERWIDTH * 2, this->winsize + BORDERWIDTH * 2, 0, this->whitepixel, this->blackpixel);
    XMapRaised (this->xdis, this->xwin);

    char const *typname;
    switch (this->type) {
        case VC8TypeE: typname = "VC8/E"; break;
        case VC8TypeI: typname = "VC8/I"; break;
        default: ABORT ();
    }
    char hostname[256];
    gethostname (hostname, sizeof hostname);
    hostname[255] = 0;
    char windowname[8+256+12];
    snprintf (windowname, sizeof windowname, "%s (%s:%d)", typname, hostname, (int) getpid ());
    XStoreName (this->xdis, this->xwin, windowname);

    // set up to draw on the window
    XGCValues gcvalues;
    memset (&gcvalues, 0, sizeof gcvalues);
    gcvalues.foreground = this->whitepixel;
    gcvalues.background = this->blackpixel;
    this->xgc = XCreateGC (this->xdis, this->xwin, GCForeground | GCBackground, &gcvalues);

    // set up gray levels for the intensities
    Colormap cmap = XDefaultColormap (this->xdis, 0);
    for (int i = 0; i < 4; i ++) {
        XColor xcolor;
        memset (&xcolor, 0, sizeof xcolor);
        xcolor.red   = (i + 1) * 65535 / 4;
        xcolor.green = (i + 1) * 65535 / 4;
        xcolor.blue  = (i + 1) * 65535 / 4;
        xcolor.flags = DoRed | DoGreen | DoBlue;
        XAllocColor (this->xdis, cmap, &xcolor);
        this->graylevels[i] = xcolor.pixel;
    }
    unsigned long graypixel = this->graylevels[2];

    // set up red and green colors
    unsigned long greenpix, magenpix, redpixel;
    {
        XColor xcolor;
        memset (&xcolor, 0, sizeof xcolor);
        xcolor.red   = 65535;
        xcolor.flags = DoRed | DoGreen | DoBlue;
        XAllocColor (this->xdis, cmap, &xcolor);
        this->graylevels[5] = redpixel = xcolor.pixel;

        xcolor.blue  = 65535;
        XAllocColor (this->xdis, cmap, &xcolor);
        magenpix = xcolor.pixel;

        xcolor.red   = 0;
        xcolor.blue  = 0;
        xcolor.green = 65535;
        XAllocColor (this->xdis, cmap, &xcolor);
        this->graylevels[4] = greenpix = xcolor.pixel;
    }

    // allocate buffer to hold all points
    this->allpoints = (VC8Pt *) malloc (XYSIZE * XYSIZE * sizeof *this->allpoints);
    if (this->allpoints == NULL) ABORT ();

    uint16_t *timemss = (uint16_t *) malloc (XYSIZE * XYSIZE * sizeof *timemss);
    if (timemss == NULL) ABORT ();

    this->indices = (int *) malloc (MAXWINSIZE * MAXWINSIZE * sizeof *this->indices);
    if (this->indices == NULL) ABORT ();
    memset (this->indices, -1, MAXWINSIZE * MAXWINSIZE * sizeof *this->indices);

    // buttons
    ClearButton clearbutton (this);
    PersisButton persisbutton (this);
    persisbutton.setpms (this->pms);

    // repeat until ioreset() is called
    char wsizbuf[12] = { 0 };
    uint16_t wsiztim = 0;
    uint32_t lastsec = 0;
    uint32_t counter = 0;
    int allins = 0;
    int allrem = 0;
    uint16_t oldeflags = 0;
    this->foreground = this->whitepixel;
    pthread_mutex_lock (&this->lock);
    while (! this->resetting) {

        // wait for points queued or reset
        // max of time for next persistance expiration
        struct timespec ts;
        if (clock_gettime (CLOCK_REALTIME, &ts) < 0) ABORT ();
        ts.tv_nsec += 50000000;
        if (ts.tv_nsec >= 1000000000) {
            ts.tv_nsec -= 1000000000;
            ts.tv_sec ++;
        }
        int rc = pthread_cond_timedwait (&this->cond, &this->lock, &ts);
        if ((rc != 0) && (rc != ETIMEDOUT)) ABORT ();

        // copy points from processor while still locked
        if (clock_gettime (CLOCK_REALTIME, &ts) < 0) ABORT ();
        uint16_t timems = ((ts.tv_sec * 1000000000ULL) + ts.tv_nsec) / 1000000;
        int allnew = allins;
        int rem = this->remove;
        while (rem != this->insert) {
            VC8Pt *p = &this->buffer[rem];              // get point queued by processor
            rem = (rem + 1) % MAXBUFFPTS;
            int mxy = this->mappedxy (p);               // this pixel is occupied by this point now
            this->indices[mxy] = allins;
            int xy = p->y * XYSIZE + p->x;              // this is latest occurrence of the x,y
            timemss[xy] = timems;                       // remember when we got it
            this->allpoints[allins] = *p;               // copy to end of all points ring
            allins = (allins + 1) % MAXBUFFPTS;
            if (allrem == allins) {
                allrem = (allrem + 1) % MAXBUFFPTS;     // ring overflowed, remove oldest point
            }
            counter ++;
        }
        this->remove = rem;

        uint16_t neweflags = this->eflags;
        pthread_mutex_unlock (&this->lock);

        // allpoints:  ...  allrem  ...  allnew  ...  allins  ...
        //                  <old points> <new points>

        // maybe erase everything
        if (~ oldeflags & neweflags & EF_ER) {
            XClearWindow (this->xdis, this->xwin);
            XFlush (this->xdis);
            usleep (450000);
            pthread_mutex_lock (&this->lock);
            this->eflags |= EF_DN;
            this->updintreq ();
            pthread_mutex_unlock (&this->lock);
            allins = allnew = allrem = 0;
            this->insert = this->remove = 0;
        }

        // get window size
        XWindowAttributes winattrs;
        if (XGetWindowAttributes (this->xdis, this->xwin, &winattrs) == 0) ABORT ();
        int newwinxsize = winattrs.width  - BORDERWIDTH * 2;
        int newwinysize = winattrs.height - BORDERWIDTH * 2;
        int newwinsize  = (newwinxsize < newwinysize) ? newwinxsize : newwinysize;
        if (newwinsize < MINWINSIZE) newwinsize = MINWINSIZE;
        if (newwinsize > MAXWINSIZE) newwinsize = MAXWINSIZE;

        // if window size changed, clear window then re-draw old points at new scaling
        if (this->winsize != newwinsize) {
            this->winsize = newwinsize;
            XClearWindow (this->xdis, this->xwin);

            // refill indices to indicate where the latest point can be found
            memset (this->indices, -1, MAXWINSIZE * MAXWINSIZE * sizeof *this->indices);
            for (int refill = allrem; refill != allins; refill = (refill + 1) % (XYSIZE * XYSIZE)) {
                int mxy = this->mappedxy (&this->allpoints[refill]);
                this->indices[mxy] = refill;
            }

            // redraw the oldest points
            for (int redraw = allrem; redraw != allnew; redraw = (redraw + 1) % (XYSIZE * XYSIZE)) {
                this->drawpt (redraw, false);
            }

            // set new positions of buttons
            clearbutton.position ();
            persisbutton.upbutton.position ();
            persisbutton.dnbutton.position ();

            // set up to display new window size
            sprintf (wsizbuf, "%d", this->winsize);
            wsiztim = timems;
        }

        // ephemeral mode: erase old points what have timed out
        if (! (neweflags & EF_ST)) {
            while (allrem != allnew) {

                // get next point, break out if no more left
                VC8Pt *p = &allpoints[allrem];

                // see if it has timed out or has been superceded
                // draw in black and remove if so
                int xy = p->y * XYSIZE + p->x;
                int mxy = this->mappedxy (p);
                uint16_t dtms = timems - timemss[xy];
                if ((this->indices[mxy] == allrem) && (dtms < this->pms)) break;

                this->drawpt (allrem, true);

                // in either case, remove timed-out entry from ring
                allrem = (allrem + 1) % (XYSIZE * XYSIZE);
            }
        }

        // draw new points passed to us from processor
        while (allnew != allins) {
            this->drawpt (allnew, false);
            allnew = (allnew + 1) % (XYSIZE * XYSIZE);
        }

        // draw border box
        this->setfg (graypixel);
        for (int i = 0; i < BORDERWIDTH; i ++) {
            XDrawRectangle (this->xdis, this->xwin, this->xgc, i, i, this->winsize + BORDERWIDTH * 2 - 1 - i * 2, this->winsize + BORDERWIDTH * 2 - 1 - i * 2);
        }

        // storage mode: draw CLEAR button
        // ephemeral mode: draw persistance UP/DOWN buttons
        if (neweflags & EF_ST) {
            if (! (oldeflags & EF_ST)) {
                persisbutton.draw (this->blackpixel);
            }
            clearbutton.draw (magenpix);
        } else {
            if (oldeflags & EF_ST) {
                clearbutton.draw (this->blackpixel);
            }
            persisbutton.draw (magenpix);
        }

        // maybe show new window size
        if (wsizbuf[0] != 0) {
            bool dead = (timems - wsiztim > 1000);
            this->setfg (dead ? this->blackpixel : magenpix);
            XDrawString (this->xdis, this->xwin, this->xgc, BORDERWIDTH * 2, BORDERWIDTH * 2 + 10, wsizbuf, strlen (wsizbuf));
            if (dead) wsizbuf[0] = 0;
        }

        XFlush (this->xdis);

        // maybe print counts once per minute
        uint32_t thissec = ts.tv_sec;
        if (lastsec / 60 != thissec / 60) {
            if ((lastsec != 0) && getmintimes ()) {
                uint32_t pps = counter / (thissec - lastsec);
                if (pps != 0) fprintf (stderr, "IODevVC8::thread: %u point%s per second\n", pps, (pps == 1 ? "" : "s"));
            }
            lastsec = thissec;
            counter = 0;
        }

        // storage mode: check for clear button clicked
        if (neweflags & EF_ST) {
            if (clearbutton.clicked ()) XClearWindow (this->xdis, this->xwin);
        } else {
            int16_t delta = 0;
            if (persisbutton.upbutton.clicked ()) {
                delta += this->pms / 4;
            }
            if (persisbutton.dnbutton.clicked ()) {
                delta -= this->pms / 4;
            }
            if (delta != 0) {
                persisbutton.draw (this->blackpixel);
                this->pms += delta;
                if (this->pms < MINPERSISTMS) this->pms = MINPERSISTMS;
                if (this->pms > MAXPERSISTMS) this->pms = MAXPERSISTMS;
                persisbutton.setpms (this->pms);
            }
        }

        oldeflags = neweflags;

        pthread_mutex_lock (&this->lock);
    }

    // ioreset(), unlock, close display and exit thread
    pthread_mutex_unlock (&this->lock);
    free (this->allpoints);
    free (timemss);
    free (this->indices);
    XCloseDisplay (this->xdis);
    this->xdis = NULL;
    this->xwin = 0;
    this->xgc = NULL;
    this->allpoints = NULL;
    this->indices = NULL;
}

void IODevVC8::drawpt (int i, bool erase)
{
    // don't bother drawing if superceded
    VC8Pt *pt = &this->allpoints[i];
    int mxy = this->mappedxy (pt);
    if (this->indices[mxy] == i) {

        // latest of this xy, draw point
        this->setfg (erase ? this->blackpixel : this->graylevels[pt->t]);
        int mx = pt->x * this->winsize / XYSIZE;
        int my = pt->y * this->winsize / XYSIZE;
        XDrawPoint (this->xdis, this->xwin, this->xgc, mx + BORDERWIDTH, my + BORDERWIDTH);

        // if erasing, point no longer in allpoints
        if (erase) this->indices[mxy] = -1;
    }
}

int IODevVC8::mappedxy (VC8Pt const *pt)
{
    int mx = pt->x * this->winsize / XYSIZE;
    int my = pt->y * this->winsize / XYSIZE;
    return my * MAXWINSIZE + mx;
}

void IODevVC8::setfg (unsigned long color)
{
    if (this->foreground != color) {
        this->foreground = color;
        XSetForeground (this->xdis, this->xgc, this->foreground);
    }
}

void IODevVC8::updintreq ()
{
    if ((this->eflags & (EF_DN | EF_IE)) == (EF_DN | EF_IE)) {
        setintreqmask (IRQ_VC8);
    } else {
        clrintreqmask (IRQ_VC8, this->waiting && (this->eflags & EF_DN));
    }
}

// gets called when window closed
// if we don't provide this, sometimes get segfaults when closing
static int xioerror (Display *xdis)
{
    fprintf (stderr, "IODevVC8::xioerror: X server IO error\n");
    exit (1);
}

/////////////////////////////
//  Common to all Buttons  //
/////////////////////////////

Button::Button (IODevVC8 *vc8)
{
    this->vc8   = vc8;
    lastpressed = false;
}

Button::~Button ()
{ }

bool Button::clicked ()
{
    Window root, child;
    int root_x, root_y, win_x, win_y;
    unsigned int mask;
    Bool ok = XQueryPointer (vc8->xdis, vc8->xwin, &root, &child, &root_x, &root_y, &win_x, &win_y, &mask);
    if (ok && (win_x > xleft) && (win_x < xrite) && (win_y > ytop) && (win_y < ybot)) {
        // mouse inside button area, if pressed, remember that
        if (mask & Button1Mask) {
            lastpressed = true;
        } else if (lastpressed) {
            // mouse button released in button area after being pressed in button area
            lastpressed = false;
            return true;
        }
    } else {
        // mouse outside button area, pretend button was released
        lastpressed = false;
    }
    return false;
}

/////////////////////////////////////////
//  Circle Button for Clearing Screen  //
/////////////////////////////////////////

#define BUTTONDIAM 32

ClearButton::ClearButton (IODevVC8 *vc8)
    : Button (vc8)
{
    position ();
}

void ClearButton::position ()
{
    xleft = vc8->winsize + BORDERWIDTH - BUTTONDIAM;
    xrite = vc8->winsize + BORDERWIDTH;
    ytop  = vc8->winsize + BORDERWIDTH - BUTTONDIAM;
    ybot  = vc8->winsize + BORDERWIDTH;
}

void ClearButton::draw (unsigned long color)
{
    vc8->setfg (color);
    XDrawArc (vc8->xdis, vc8->xwin, vc8->xgc, vc8->winsize + BORDERWIDTH - BUTTONDIAM, vc8->winsize + BORDERWIDTH - BUTTONDIAM, BUTTONDIAM - 1, BUTTONDIAM - 1, 0, 360 * 64);
    XDrawString (vc8->xdis, vc8->xwin, vc8->xgc, vc8->winsize + BORDERWIDTH + 1 - BUTTONDIAM, vc8->winsize + BORDERWIDTH + 4 - BUTTONDIAM / 2, "CLEAR", 5);
}

#undef BUTTONDIAM

////////////////////////////////////////////////
//  Triangle Button for Altering Persistence  //
////////////////////////////////////////////////

#define BUTTONDIAM 48

TriButton::TriButton (IODevVC8 *vc8, bool upbut)
    : Button (vc8)
{
    this->upbut = upbut;
    position ();
}

void TriButton::position ()
{
    xleft = vc8->winsize + BORDERWIDTH - BUTTONDIAM;
    xrite = vc8->winsize + BORDERWIDTH;
    if (this->upbut) {
        ytop = vc8->winsize + BORDERWIDTH - BUTTONDIAM     - BUTTONDIAM / 2;
        ybot = vc8->winsize + BORDERWIDTH - BUTTONDIAM / 2 - BUTTONDIAM / 2;
        segs[0].x1 = vc8->winsize + BORDERWIDTH - BUTTONDIAM;
        segs[0].y1 = vc8->winsize + BORDERWIDTH - BUTTONDIAM / 2 - BUTTONDIAM / 2;
        segs[0].x2 = vc8->winsize + BORDERWIDTH - 1;
        segs[0].y2 = vc8->winsize + BORDERWIDTH - BUTTONDIAM / 2 - BUTTONDIAM / 2;
        segs[1].x1 = vc8->winsize + BORDERWIDTH - 1;
        segs[1].y1 = vc8->winsize + BORDERWIDTH - BUTTONDIAM / 2 - BUTTONDIAM / 2;
        segs[1].x2 = vc8->winsize + BORDERWIDTH - BUTTONDIAM / 2;
        segs[1].y2 = vc8->winsize + BORDERWIDTH - BUTTONDIAM     - BUTTONDIAM / 2;
        segs[2].x1 = vc8->winsize + BORDERWIDTH - BUTTONDIAM / 2;
        segs[2].y1 = vc8->winsize + BORDERWIDTH - BUTTONDIAM     - BUTTONDIAM / 2;
        segs[2].x2 = vc8->winsize + BORDERWIDTH - BUTTONDIAM;
        segs[2].y2 = vc8->winsize + BORDERWIDTH - BUTTONDIAM / 2 - BUTTONDIAM / 2;
    } else {
        ytop = vc8->winsize + BORDERWIDTH - BUTTONDIAM / 2;
        ybot = vc8->winsize + BORDERWIDTH;
        segs[0].x1 = vc8->winsize + BORDERWIDTH - BUTTONDIAM;
        segs[0].y1 = vc8->winsize + BORDERWIDTH - BUTTONDIAM / 2;
        segs[0].x2 = vc8->winsize + BORDERWIDTH - 1;
        segs[0].y2 = vc8->winsize + BORDERWIDTH - BUTTONDIAM / 2;
        segs[1].x1 = vc8->winsize + BORDERWIDTH - 1;
        segs[1].y1 = vc8->winsize + BORDERWIDTH - BUTTONDIAM / 2;
        segs[1].x2 = vc8->winsize + BORDERWIDTH - BUTTONDIAM / 2;
        segs[1].y2 = vc8->winsize + BORDERWIDTH - 1;
        segs[2].x1 = vc8->winsize + BORDERWIDTH - BUTTONDIAM / 2;
        segs[2].y1 = vc8->winsize + BORDERWIDTH - 1;
        segs[2].x2 = vc8->winsize + BORDERWIDTH - BUTTONDIAM;
        segs[2].y2 = vc8->winsize + BORDERWIDTH - BUTTONDIAM / 2;
    }
}

void TriButton::draw (unsigned long color)
{
    vc8->setfg (color);
    XDrawSegments (vc8->xdis, vc8->xwin, vc8->xgc, segs, 3);
}

//////////////////////////
//  Persistance Button  //
//////////////////////////

PersisButton::PersisButton (IODevVC8 *vc8)
    : upbutton (vc8, true),
      dnbutton (vc8, false)
{
    this->vc8 = vc8;

    pmslen = 0;
}

void PersisButton::setpms (uint16_t pms)
{
    pmslen = snprintf (pmsbuf, sizeof pmsbuf, "%u ms", pms);
}

void PersisButton::draw (unsigned long color)
{
    upbutton.draw (color);
    dnbutton.draw (color);
    XDrawString (vc8->xdis, vc8->xwin, vc8->xgc, vc8->winsize + BORDERWIDTH + 1 - BUTTONDIAM, vc8->winsize + BORDERWIDTH + 4 - BUTTONDIAM * 3 / 4, pmsbuf, pmslen);
}

#undef BUTTONDIAM
