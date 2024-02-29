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

#ifndef _TIME_H
#define _TIME_H

typedef __uint32_t time_t;

struct tm {
    int tm_sec;     // 00..59
    int tm_min;     // 00..59
    int tm_hour;    // 00..23
    int tm_mday;    // 01..31
    int tm_mon;     // 00..11
    int tm_year;    // 0=1900
    int tm_wday;    // 0=Sunday
    int tm_yday;    // 000..365
    int tm_isdst;
};

time_t time (time_t *tp);
tm *gmtime (time_t const *tp);
tm *gmtime_r (time_t const *tp, tm *tmbuf);
time_t timegm (tm const *tmbuf);

#endif
