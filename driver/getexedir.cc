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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "miscdefs.h"

// get executeable directory into buf including trailing /
// also return machine name pointer including leading . (eg, ".x86_64") within buf
char *getexedir (char *buf, int buflen)
{
    int rc;
    char *env = getenv ("exename");
    if ((env != NULL) && (env[0] != 0)) {
        rc = strlen (env);
        if (rc >= buflen) {
            fprintf (stderr, "getexedir: envar exename longer than %d bytes\n", buflen - 1);
            ABORT ();
        }
        memcpy (buf, env, rc + 1);
    } else {
        rc = readlink ("/proc/self/exe", buf, buflen);
        if (rc < 0) {
            fprintf (stderr, "getexedir: error readink link /proc/self/exe: %m\n");
            ABORT ();
        }
        if (rc >= buflen) {
            fprintf (stderr, "getexedir: link /proc/self/exe longer than %d bytes\n", buflen - 1);
            ABORT ();
        }
        buf[rc] = 0;
    }

    char *lastslash = strrchr (buf, '/');
    if (lastslash == NULL) {
        fprintf (stderr, "getexedir: no slash in executable name %s\n", buf);
        ABORT ();
    }
    char *lastdot = strrchr (++ lastslash, '.');
    if (lastdot <= lastslash) {
        fprintf (stderr, "getexedir: missing or null machname in %s\n", buf);
        ABORT ();
    }
    *lastslash = 0;
    return lastdot;
}
