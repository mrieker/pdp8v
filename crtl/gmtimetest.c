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

// run on linux and hode, compare the output

#include <stdio.h>
#include <time.h>

int main (int argc, char **argv)
{
    time_t tmbin = 0;
    do {
        struct tm *tmbuf = gmtime (&tmbin);
        time_t     rebin = timegm (tmbuf);
        unsigned int tmday = tmbin / 86400;
        unsigned int reday = rebin / 86400;
        printf ("%04d-%02d-%02d %02d %03d  %5u %5u %5d\n",
                    tmbuf->tm_year + 1900, tmbuf->tm_mon + 1, tmbuf->tm_mday,
                    tmbuf->tm_wday, tmbuf->tm_yday,
                    tmday, reday, reday - tmday);
        tmbin += 86400;
    } while ((__uint32_t)tmbin >= 86400);
    return 0;
}
