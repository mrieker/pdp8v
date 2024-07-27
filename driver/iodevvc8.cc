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
// must set envar vc8type=e or =i to select which

#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>

#include "iodevvc8.h"

#define MAXBUFFPTS 65536
#define PERSISTMS 100       // how long non-storage points last

#define BUTTONDIAM 32
#define BORDERWIDTH 2
#define WINDOWWIDTH 1024
#define WINDOWHEIGHT 1024

#define EF_DN 04000     // done executing last command
#define EF_WT 00040     // de-focus beam for writing
#define EF_ST 00020     // enable storage mode
#define EF_ER 00010     // erase storage screen (450ms)
#define EF_CO 00004     // 1=red; 0=green
#define EF_CH 00002     // display select
#define EF_IE 00001     // interrupt enable

#define EF_MASK 077

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
    this->pms = PERSISTMS;
    char const *envpms = getenv ("vc8pms");
    if (envpms != NULL) {
        this->pms = atoi (envpms);
        if (this->pms > 2000) this->pms = 2000;
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
}

IODevVC8::~IODevVC8 ()
{
    this->ioreset ();
}

// reset the device
// - close window, kill thread
void IODevVC8::ioreset ()
{
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
                    this->xcobuf  = (input + WINDOWWIDTH / 2) % WINDOWWIDTH;
                    this->eflags |= EF_DN;
                    break;
                }

                // DILY (VC8/E) Load Y
                case 06054: {
                    this->ycobuf  = (input + WINDOWHEIGHT / 2) % WINDOWHEIGHT;
                    this->eflags |= EF_DN;
                    break;
                }

                // DIXY (VC8/E) Intensify at (X,Y)
                case 06055: {
                    this->insertpoint ();
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
                    if (opcode & 2) this->xcobuf |= input % WINDOWWIDTH;
                    if (opcode & 4) this->insertpoint ();
                    break;
                }

                case 06061 ... 06067: {
                    if (opcode & 1) this->ycobuf  = 0;
                    if (opcode & 2) this->ycobuf |= input % WINDOWHEIGHT;
                    if (opcode & 4) this->insertpoint ();
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
void IODevVC8::insertpoint ()
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
    this->buffer[ins].t = this->intens;
    this->insert = ins = (ins + 1) % MAXBUFFPTS;
    if (ins == this->remove) this->remove = (this->remove + 1) % MAXBUFFPTS;
}

// wake thread or start it
void IODevVC8::wakethread ()
{
    if (this->threadid == 0) {
        int rc = pthread_create (&this->threadid, NULL, threadwrap, this);
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
    // open connection to XServer
    Display *xdis = XOpenDisplay (NULL);
    if (xdis == NULL) {
        fprintf (stderr, "IODevVC8::thread: error opening X display\n");
        ABORT ();
    }

    // set up handler to be called when window closed
    XSetIOErrorHandler (xioerror);

    // create and display a window
    unsigned long blackpixel = XBlackPixel (xdis, 0);
    unsigned long whitepixel = XWhitePixel (xdis, 0);
    Window xrootwin = XDefaultRootWindow (xdis);
    Window xwin = XCreateSimpleWindow (xdis, xrootwin, 100, 100, WINDOWWIDTH + BORDERWIDTH * 2, WINDOWHEIGHT + BORDERWIDTH * 2, 0, whitepixel, blackpixel);
    XMapRaised (xdis, xwin);

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
    XStoreName (xdis, xwin, windowname);

    // set up to draw on the window
    Drawable drawable = xwin;
    XGCValues gcvalues;
    memset (&gcvalues, 0, sizeof gcvalues);
    gcvalues.foreground = whitepixel;
    gcvalues.background = blackpixel;
    GC xgc = XCreateGC (xdis, drawable, GCForeground | GCBackground, &gcvalues);

    // set up gray levels for the intensities
    Colormap cmap = XDefaultColormap (xdis, 0);
    unsigned long graylevels[4];
    for (int i = 0; i < 4; i ++) {
        XColor xcolor;
        memset (&xcolor, 0, sizeof xcolor);
        xcolor.red   = (i + 1) * 65535 / 4;
        xcolor.green = (i + 1) * 65535 / 4;
        xcolor.blue  = (i + 1) * 65535 / 4;
        xcolor.flags = DoRed | DoGreen | DoBlue;
        XAllocColor (xdis, cmap, &xcolor);
        graylevels[i] = xcolor.pixel;
    }

    // set up red color for clear button
    unsigned long redpixel;
    {
        XColor xcolor;
        memset (&xcolor, 0, sizeof xcolor);
        xcolor.red   = 65535;
        xcolor.flags = DoRed | DoGreen | DoBlue;
        XAllocColor (xdis, cmap, &xcolor);
        redpixel = xcolor.pixel;
    }

    // allocate buffer to hold all points
    VC8Pt *allpoints = (VC8Pt *) malloc (WINDOWWIDTH * WINDOWHEIGHT * sizeof *allpoints);
    if (allpoints == NULL) ABORT ();

    uint16_t *timemss = (uint16_t *) malloc (WINDOWWIDTH * WINDOWHEIGHT * sizeof *timemss);
    if (timemss == NULL) ABORT ();

    int *indices = (int *) malloc (WINDOWWIDTH * WINDOWHEIGHT * sizeof *indices);
    if (indices == NULL) ABORT ();
    memset (indices, -1, WINDOWWIDTH * WINDOWHEIGHT * sizeof *indices);

    // repeat until ioreset() is called
    bool lastclearpressed = false;
    int allins = 0;
    int allrem = 0;
    uint16_t oldeflags = 0;
    unsigned long foreground = whitepixel;
    pthread_mutex_lock (&this->lock);
    while (! this->resetting) {

        // wait for points queued or reset
        // max of time for next persistance expiration
        struct timespec ts;
        if (clock_gettime (CLOCK_REALTIME, &ts) < 0) ABORT ();
        ts.tv_nsec += 1000000 * (this->pms > 50 ? 50 : this->pms) / 4;
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
            int xy = p->y * WINDOWWIDTH + p->x;         // this is latest occurrence of the x,y
            indices[xy] = allins;
            timemss[xy] = timems;                       // remember when we got it
            allpoints[allins] = *p;                     // copy to end of all points ring
            allins = (allins + 1) % MAXBUFFPTS;
            if (allrem == allins) {
                allrem = (allrem + 1) % MAXBUFFPTS;     // ring overflowed, remove oldest point
            }
        }
        this->remove = rem;

        uint16_t neweflags = this->eflags;
        pthread_mutex_unlock (&this->lock);

        // allpoints:  ...  allrem  ...  allnew  ...  allins  ...
        //                  <old points> <new points>

        // maybe erase everything
        if (~ oldeflags & neweflags & EF_ER) {
            XClearWindow (xdis, xwin);
            XFlush (xdis);
            usleep (450000);
            pthread_mutex_lock (&this->lock);
            this->eflags |= EF_DN;
            this->updintreq ();
            pthread_mutex_unlock (&this->lock);
            allins = allnew = allrem = 0;
            this->insert = this->remove = 0;
        }

        oldeflags = neweflags;

        // ephemeral mode: erase old points what have timed out
        if (! (neweflags & EF_ST)) {
            while (allrem != allnew) {

                // get next point, break out if no more left
                VC8Pt *p = &allpoints[allrem];

                // see if it has timed out
                // draw in black and remove if so
                int xy = p->y * WINDOWWIDTH + p->x;
                uint16_t dtms = timems - timemss[xy];
                if (dtms < this->pms) break;

                // if it has been superceded by a point later on in ring, don't erase it
                if (indices[xy] == allrem) {
                    indices[xy] = -1;
                    if (foreground != blackpixel) {
                        foreground = blackpixel;
                        XSetForeground (xdis, xgc, foreground);
                    }
                    XDrawPoint (xdis, drawable, xgc, p->x + BORDERWIDTH, p->y + BORDERWIDTH);
                }

                // in either case, remove timed-out entry from ring
                allrem = (allrem + 1) % (WINDOWWIDTH * WINDOWHEIGHT);
            }
        }

        // draw new points passed to us from processor
        while (allnew != allins) {

            // get new point
            VC8Pt *p = &allpoints[allnew];

            // don't bother drawing if superceded
            int xy = p->y * WINDOWWIDTH + p->x;
            if (indices[xy] == allnew) {

                // latest of this xy, draw point
                if (foreground != graylevels[p->t]) {
                    foreground = graylevels[p->t];
                    XSetForeground (xdis, xgc, foreground);
                }
                XDrawPoint (xdis, drawable, xgc, p->x + BORDERWIDTH, p->y + BORDERWIDTH);
            }

            // on to next point
            allnew = (allnew + 1) % (WINDOWWIDTH * WINDOWHEIGHT);
        }

        // draw border box
        if (foreground != whitepixel) {
            foreground = whitepixel;
            XSetForeground (xdis, xgc, foreground);
        }
        for (int i = 0; i < BORDERWIDTH; i ++) {
            XDrawRectangle (xdis, drawable, xgc, i, i, WINDOWWIDTH + BORDERWIDTH * 2 - 1 - i * 2, WINDOWHEIGHT + BORDERWIDTH * 2 - 1 - i * 2);
        }

        // storage mode: draw red CLEAR button
        if (neweflags & EF_ST) {
            if (foreground != redpixel) {
                foreground = redpixel;
                XSetForeground (xdis, xgc, foreground);
            }
            XDrawArc (xdis, drawable, xgc, WINDOWWIDTH + BORDERWIDTH - BUTTONDIAM, WINDOWHEIGHT + BORDERWIDTH - BUTTONDIAM, BUTTONDIAM - 1, BUTTONDIAM - 1, 0, 360 * 64);
            XDrawString (xdis, drawable, xgc, WINDOWWIDTH + BORDERWIDTH + 1 - BUTTONDIAM, WINDOWHEIGHT + BORDERWIDTH + 4 - BUTTONDIAM / 2, "CLEAR", 5);
        }

        XFlush (xdis);

        // storage mode: check for clear button clicked
        if (neweflags & EF_ST) {
            Window root, child;
            int root_x, root_y, win_x, win_y;
            unsigned int mask;
            Bool ok = XQueryPointer (xdis, xwin, &root, &child, &root_x, &root_y, &win_x, &win_y, &mask);
            if (ok && (win_x > WINDOWWIDTH + BORDERWIDTH - BUTTONDIAM) && (win_x < WINDOWWIDTH + BORDERWIDTH) &&
                    (win_y > WINDOWHEIGHT + BORDERWIDTH - BUTTONDIAM) && (win_y < WINDOWHEIGHT + BORDERWIDTH)) {
                // mouse inside button area, if pressed, remember that
                if (mask & Button1Mask) {
                    lastclearpressed = true;
                } else if (lastclearpressed) {
                    // mouse button released in button area after being pressed in button area, clear screen
                    XClearWindow (xdis, xwin);
                    lastclearpressed = false;
                }
            } else {
                // mouse outside button area, pretend button was released
                lastclearpressed = false;
            }
        }

        pthread_mutex_lock (&this->lock);
    }

    // ioreset(), unlock, close display and exit thread
    pthread_mutex_unlock (&this->lock);
    free (allpoints);
    free (timemss);
    free (indices);
    XCloseDisplay (xdis);
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
