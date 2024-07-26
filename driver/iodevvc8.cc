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

#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>

#include "iodevvc8.h"

#define MAXBUFFPTS 65536

#define BUTTONDIAM 32
#define BORDERWIDTH 2
#define WINDOWWIDTH 1024
#define WINDOWHEIGHT 1024

IODevVC8 iodevvc8;

static IODevOps const iodevops[] = {
    { 06051, "DCX (VC8) Clear X-Coordinate Buffer" },
    { 06053, "DXL (VC8) Load X-Coordinate Buffer" },
    { 06054, "DIX (VC8) Intensify" },
    { 06057, "DXS (VC8) X-Coordinate Sequence" },
    { 06061, "DCY (VC8) Clear Y-Coordinate Buffer" },
    { 06063, "DYL (VC8) Clear and Load Y-Coordinate BUffer" },
    { 06064, "DIY (VC8) Intensify" },
    { 06067, "DYS (VC8) Y-Coordinate Sequence" },
    { 06074, "DSB (VC8) Set Brightness Control to 0" },
    { 06075, "DSB (VC8) Set Brightness Control to 1" },
    { 06076, "DSB (VC8) Set Brightness Control to 2" },
    { 06077, "DSB (VC8) Set Brightness Control to 3" }
};

IODevVC8::IODevVC8 ()
{
    opscount = sizeof iodevops / sizeof iodevops[0];
    opsarray = iodevops;

    iodevname = "vc8";

    if (pthread_cond_init (&this->cond, NULL) != 0) ABORT ();
    if (pthread_mutex_init (&this->lock, NULL) != 0) ABORT ();

    this->resetting = false;
    this->buffer    = NULL;
    this->threadid  = 0;
    this->insert    = 0;
    this->remove    = 0;
    this->xcobuf    = 0;
    this->ycobuf    = 0;
    this->intens    = 0;
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

        this->resetting = false;
        pthread_cond_broadcast (&this->cond);
    }

    pthread_mutex_unlock (&this->lock);
}

// load/unload files
SCRet *IODevVC8::scriptcmd (int argc, char const *const *argv)
{
    return new SCRetErr ("unknown display command %s", argv[0]);
}

// perform i/o instruction
uint16_t IODevVC8::ioinstr (uint16_t opcode, uint16_t input)
{
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
    return input;
}

// queue point to thread to display it
// start thread if not already running to open window
void IODevVC8::insertpoint ()
{
    pthread_mutex_lock (&this->lock);

    // start thread if this is first point ever
    // otherwise, just wake the thread to process point
    if (this->threadid == 0) {
        int rc = pthread_create (&this->threadid, NULL, threadwrap, this);
        if (rc != 0) ABORT ();
        this->buffer = (VC8Pt *) malloc (MAXBUFFPTS * sizeof *this->buffer);
        if (this->buffer == NULL) ABORT ();
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

    pthread_mutex_unlock (&this->lock);
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

    // create and display a window
    unsigned long blackpixel = XBlackPixel (xdis, 0);
    unsigned long whitepixel = XWhitePixel (xdis, 0);
    Window xrootwin = XDefaultRootWindow (xdis);
    Window xwin = XCreateSimpleWindow (xdis, xrootwin, 100, 100, WINDOWWIDTH + BORDERWIDTH * 2, WINDOWHEIGHT + BORDERWIDTH * 2, 0, whitepixel, blackpixel);
    XMapRaised (xdis, xwin);
    XStoreName (xdis, xwin, "VC8");

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
    VC8Pt *allpoints = (VC8Pt *) malloc (MAXBUFFPTS * sizeof *allpoints);
    if (allpoints == NULL) ABORT ();

    // repeat until ioreset() is called
    bool lastbuttonpressed = false;
    unsigned long foreground = whitepixel;
    pthread_mutex_lock (&this->lock);
    while (! this->resetting) {

        // wait for points queued or reset
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
        int allins = 0;
        int allrem = 0;
        int rem = this->remove;
        while (rem != this->insert) {
            allpoints[allins] = this->buffer[rem];
            rem = (rem + 1) % MAXBUFFPTS;
            allins = (allins + 1) % MAXBUFFPTS;
            if (allrem == allins) {
                allrem = (allrem + 1) % MAXBUFFPTS;
            }
        }
        this->remove = rem;

        pthread_mutex_unlock (&this->lock);

        // draw points passed to us from processor
        for (int i = 0; i < MAXBUFFPTS; i ++) {

            // get next point, break out if no more left
            int j = (allrem + i) % MAXBUFFPTS;
            if (j == allins) break;
            VC8Pt *p = &allpoints[j];

            // draw point
            if (foreground != graylevels[p->t]) {
                foreground = graylevels[p->t];
                XSetForeground (xdis, xgc, foreground);
            }
            XDrawPoint (xdis, drawable, xgc, p->x + BORDERWIDTH, p->y + BORDERWIDTH);
        }

        // draw border box
        if (foreground != whitepixel) {
            foreground = whitepixel;
            XSetForeground (xdis, xgc, foreground);
        }
        for (int i = 0; i < BORDERWIDTH; i ++) {
            XDrawRectangle (xdis, drawable, xgc, i, i, WINDOWWIDTH + BORDERWIDTH * 2 - 1 - i * 2, WINDOWHEIGHT + BORDERWIDTH * 2 - 1 - i * 2);
        }

        // draw red CLEAR button
        if (foreground != redpixel) {
            foreground = redpixel;
            XSetForeground (xdis, xgc, foreground);
        }
        XDrawArc (xdis, drawable, xgc, WINDOWWIDTH + BORDERWIDTH - BUTTONDIAM, WINDOWHEIGHT + BORDERWIDTH - BUTTONDIAM, BUTTONDIAM - 1, BUTTONDIAM - 1, 0, 360 * 64);
        XDrawString (xdis, drawable, xgc, WINDOWWIDTH + BORDERWIDTH + 1 - BUTTONDIAM, WINDOWHEIGHT + BORDERWIDTH + 4 - BUTTONDIAM / 2, "CLEAR", 5);

        XFlush (xdis);

        // check for clear button clicked
        Window root, child;
        int root_x, root_y, win_x, win_y;
        unsigned int mask;
        Bool ok = XQueryPointer (xdis, xwin, &root, &child, &root_x, &root_y, &win_x, &win_y, &mask);
        if (ok && (win_x > WINDOWWIDTH + BORDERWIDTH - BUTTONDIAM) && (win_x < WINDOWWIDTH + BORDERWIDTH) &&
                (win_y > WINDOWHEIGHT + BORDERWIDTH - BUTTONDIAM) && (win_y < WINDOWHEIGHT + BORDERWIDTH)) {
            // mouse inside button area, if pressed, remember that
            if (mask & Button1Mask) {
                lastbuttonpressed = true;
            } else if (lastbuttonpressed) {
                // mouse button released in button area after being pressed in button area, clear screen
                XClearWindow (xdis, xwin);
                lastbuttonpressed = false;
            }
        } else {
            // mouse outside button area, pretend button was released
            lastbuttonpressed = false;
        }

        pthread_mutex_lock (&this->lock);
    }

    // ioreset(), unlock, close display and exit thread
    pthread_mutex_unlock (&this->lock);
    free (allpoints);
    XCloseDisplay (xdis);
}
