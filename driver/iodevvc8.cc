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

#define NSPERFADE (64*1024*1024)            // nanoseconds per fading level
#define NSPERPOINT (128*1024)               // fastest the processor can generate a point
#define FADELEVELS 32                       // total graylevels for fading
                                            // note: persistence = NSPERFADE * FADELEVELS
#define MAXBUFFPTS (NSPERFADE/NSPERPOINT*2) // max number of points processor can generate in NSPERFADE
#define MAXALLPTS (MAXBUFFPTS*FADELEVELS*2) // room for all points till they fade to black

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
            if (opcode & 2) this->xcobuf |= input & 03777;
            if (opcode & 4) this->insertpoint ();
            break;
        }

        case 06061 ... 06067: {
            if (opcode & 1) this->ycobuf  = 0;
            if (opcode & 2) this->ycobuf |= input & 03777;
            if (opcode & 4) this->insertpoint ();
            break;
        }

        case 06074:     // black
        case 06075:     // dk gray
        case 06076:     // lt gray
        case 06077: {   // white
            this->intens = (3 - (opcode & 3)) * FADELEVELS / 3;
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
    if (this->threadid == 0) {
        int rc = pthread_create (&this->threadid, NULL, threadwrap, this);
        if (rc != 0) ABORT ();
        this->fadetick = 0;
        this->buffer = (VC8Pt *) malloc (MAXBUFFPTS * sizeof *this->buffer);
        if (this->buffer == NULL) ABORT ();
    }

    // queue the point
    // remove oldest point if queue overflow
    int ins = this->insert;
    this->buffer[ins].x = this->xcobuf;
    this->buffer[ins].y = this->ycobuf;
    this->buffer[ins].t = this->fadetick - this->intens;
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
    Window xwin = XCreateSimpleWindow (xdis, xrootwin, 100, 100, 1024, 1024, 2, whitepixel, blackpixel);
    XMapRaised (xdis, xwin);
    XStoreName (xdis, xwin, "VC8");

    // set up to draw on the window
    Drawable drawable = xwin;
    XGCValues gcvalues;
    memset (&gcvalues, 0, sizeof gcvalues);
    gcvalues.foreground = whitepixel;
    gcvalues.background = blackpixel;
    GC xgc = XCreateGC (xdis, drawable, GCForeground | GCBackground, &gcvalues);

    // set up gray levels for fading things out
    //          0 = white
    // FADELEVELS = black
    Colormap cmap = XDefaultColormap (xdis, 0);
    unsigned long graylevels[FADELEVELS+1];
    for (int i = 0; i <= FADELEVELS; i ++) {
        XColor xcolor;
        memset (&xcolor, 0, sizeof xcolor);
        xcolor.red   = (FADELEVELS - i) * 65535 / FADELEVELS;
        xcolor.green = (FADELEVELS - i) * 65535 / FADELEVELS;
        xcolor.blue  = (FADELEVELS - i) * 65535 / FADELEVELS;
        xcolor.flags = DoRed | DoGreen | DoBlue;
        XAllocColor (xdis, cmap, &xcolor);
        graylevels[i] = xcolor.pixel;
    }

    // allocate buffer to hold all points
    VC8Pt *allpoints = (VC8Pt *) malloc (MAXALLPTS * sizeof *allpoints);
    if (allpoints == NULL) ABORT ();
    int allins = 0;
    int allrem = 0;

    // index in allpoints where a given xy is
    int *indices = (int *) malloc (1024*1024 * sizeof *indices);
    if (indices == NULL) ABORT ();
    memset (indices, -1, 1024*1024 * sizeof *indices);

    // get time for first fading update
    struct timespec nowts;
    if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) ABORT ();
    uint64_t nextfadens = ((nowts.tv_sec * 1000000000ULL + nowts.tv_nsec) / NSPERFADE + 1) * NSPERFADE;

    // repeat until ioreset() is called
    unsigned long foreground = whitepixel;
    pthread_mutex_lock (&this->lock);
    while (! this->resetting) {

        // wait for beginning of next fade tick or to be woken up for reset
        nowts.tv_sec  = nextfadens / 1000000000;
        nowts.tv_nsec = nextfadens % 1000000000;
        int rc = pthread_cond_timedwait (&this->cond, &this->lock, &nowts);
        if ((rc != 0) && (rc != ETIMEDOUT)) ABORT ();

        // copy points from processor while still locked
        int rem = this->remove;
        while (rem != this->insert) {
            allpoints[allins] = this->buffer[rem];
            rem = (rem + 1) % MAXBUFFPTS;
            int xy = allpoints[allins].y * 1024 + allpoints[allins].x;
            indices[xy] = allins;
            allins = (allins + 1) % MAXALLPTS;
            if (allrem == allins) {
                xy = allpoints[allrem].y * 1024 + allpoints[allrem].x;
                if (indices[xy] == allrem) indices[xy] = -1;
                allrem = (allrem + 1) % MAXALLPTS;
            }
        }
        this->remove = rem;

        // see how many fade tick intervals have passed
        if (clock_gettime (CLOCK_REALTIME, &nowts) < 0) ABORT ();
        uint64_t incfadetick = 0;
        uint64_t nowns = nowts.tv_sec * 1000000000ULL + nowts.tv_nsec;
        if (nowns > nextfadens) incfadetick = (nowns - nextfadens) / NSPERFADE;
        if (incfadetick > 0) {

            // account for a number of fade tick intervals
            this->fadetick += incfadetick;
            pthread_mutex_unlock (&this->lock);
            nextfadens += incfadetick * NSPERFADE;

            // fade existing points
            // if point has faded to black, remove from ring buffer

            for (int i = 0; i < MAXALLPTS;) {

                // get next point, break out if no more left
                int j = (allrem + i) % MAXALLPTS;
                if (j == allins) break;
                VC8Pt *p = &allpoints[j];

                // remove or skip if it has been superceded
                int xy = p->y * 1024 + p->x;
                if (indices[xy] != j) {
                    if (i == 0) allrem = (allrem + 1) % MAXALLPTS;
                    else i ++;
                    continue;
                }

                // it's current fade level is based on how old it is
                uint16_t fadelevl = this->fadetick - p->t;

                // if faded to black, draw it to remove from screen and remove from allpoints[]
                if (fadelevl >= FADELEVELS) {
                    fadelevl = FADELEVELS;
                    if (i == 0) allrem = (allrem + 1) % MAXALLPTS;
                    else i ++;
                }

                // otherwise, just draw it with whatever gray level it is at
                else i ++;

                // draw point
                if (foreground != graylevels[fadelevl]) {
                    foreground = graylevels[fadelevl];
                    XSetForeground (xdis, xgc, foreground);
                }
                XDrawPoint (xdis, drawable, xgc, p->x, p->y);
            }

            XFlush (xdis);

            pthread_mutex_lock (&this->lock);
        }
    }

    // ioreset(), unlock, close display and exit thread
    pthread_mutex_unlock (&this->lock);
    free (allpoints);
    free (indices);
    XCloseDisplay (xdis);
}
