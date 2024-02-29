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

// read with prompt using readline

#include <stdio.h>
#include <stdlib.h>

#include <readline/history.h>
#include <readline/readline.h>

#include "readprompt.h"

char const *readprompt (char const *prompt)
{
    char *expansion, *line;
    int result;

    static char *lastline = NULL;
    static int initted = 0;

    if (!initted) {
        using_history ();
        stifle_history (500);
        history_expansion_char = 0;
        rl_bind_key ('\t', rl_insert);
        initted = 1;
    }

readit:
    line = readline (prompt);
    if (line == NULL) return NULL;
    if ((line[0] != 0) && ((lastline == NULL) || (strcmp (line, lastline) != 0))) {
        result = history_expand (line, &expansion);
        if (result != 0) {
            fprintf (stderr, "%s\n", expansion);
        }
        if ((result < 0) || (result == 2)) {
            free (expansion);
            goto readit;
        }
        add_history (expansion);
        free (line);
        line = expansion;
    }
    if (lastline != NULL) free (lastline);
    lastline = line;
    return lastline;
}
