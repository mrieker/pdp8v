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

// lists out serial numbers for all found io-warrior boards plugged into the USB

// as root:
//  ./iowlist.`uname -m` 

#include <stdio.h>

#include "myiowkit.h"

static void listeach (void *ctx, char const *dn, int pipe, char const *sn, int pid, int rev)
{
    char const *name = IowKit::pidname (pid);
    if (name == NULL) name = "?";
    printf (" devname=%s pipeno=%d serialno=%s productid=%X=%s revision=%X\n", dn, pipe, sn, pid, name, rev);
}

int main (int argc, char **argv)
{
    IowKit::list (listeach, NULL);
    return 0;
}
