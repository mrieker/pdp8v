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

#include <time.h>

static void dateconv (__uint32_t day, tm *tmbuf);
static __uint16_t isleapyear (__uint16_t year);

tm *gmtime (time_t const *tp)
{
    static tm tmbuf;
    return gmtime_r (tp, &tmbuf);
}

tm *gmtime_r (time_t const *tp, tm *tmbuf)
{
    __uint32_t t = *tp;
    tmbuf->tm_sec  = t % 60;  t /= 60;
    tmbuf->tm_min  = t % 60;  t /= 60;
    tmbuf->tm_hour = t % 24;  t /= 24;
    tmbuf->tm_wday = (t + 4) % 7;   // 0=Sunday
    dateconv (t, tmbuf);
    return tmbuf;
}

time_t timegm (tm const *tmbuf)
{
    __uint16_t cent, dy, month, qcent, qmonth, year;
    __uint32_t day;

    year  = tmbuf->tm_year + 1900;      // years since 01-mar-0000
    month = tmbuf->tm_mon - 2;          // months since 01-mar
    if ((__int16_t) month < 0) {
                                        //    month    year-=   month
                                        //  -1..-12       1    11.. 0
                                        // -13..-24       2    11.. 0
        dy     = (11 - month) / 12;
        year  -= dy;
        month += dy * 12;
    }
    if (month >= 12) {
        dy     = month / 12;
        year  += dy;
        month -= dy * 12;               // subtract yourselves a bunch of twelves
    }

    cent  = year / 100;                 // centuries since 01-mar-0000
    qcent = cent / 4;                   // quad centuries since 01-mar-0000
    cent %= 4;
    year %= 100;

    day    = qcent * (365 * 400 + 24 * 4 + 1);          // days from 01-mar-0000 to 01-mar-0400,0800,1200,1600,2000,...
    day   +=  cent * (__uint32_t) (365 * 100 + 24);     // days from 01-mar-0000 to 01-mar-xx00
    day   +=  year * 365 + year / 4;                    // days from 01-mar-0000 to 01-mar-xxxx

    // ma ap ma ju ju  au se oc no de  ja fe
    // 31 30 31 30 31  31 30 31 30 31  31 28/29
    qmonth = month / 5;
    month %= 5;
    day   += qmonth * (31 + 30 + 31 + 30 + 31);
    day   +=  month * 31 - month / 2;

    day   += (__int32_t) (tmbuf->tm_mday - 1);

    day   -= 70 * 365 +   70 / 4                    // days since 1-Jan-1900 (1900 not a ly)
          +  300 * 365 +  300 / 4 - 2               // days since 1-Jan-1600
                                                    // (-2 because 1700,1800 not leapyears)
          + 1600 * 365 + 1600 / 4 - 12              // days since 1-Jan-0000
          -  31 -  29;                              // days since 1-Mar-0000 (0000 is a ly)

    return day * 86400 + tmbuf->tm_hour * 3600 + tmbuf->tm_min * 60 + tmbuf->tm_sec;
}

// input:
//  day = days since 1970-01-01 (0 for 1970-01-01)
// output:
//  tmbuf->tm_mday = day of month (01..31)
//  tmbuf->tm_mon  = month of year (00..11)
//  tmbuf->tm_year = year since 1900
static void dateconv (__uint32_t day, tm *tmbuf)
{
    __uint16_t qcent, cent, qyear, year, qmonth, bmonth, month, dic, diqy, diy;

    day +=   70 * 365 +   70 / 4                    // days since 1-Jan-1900 (1900 not a ly)
          +  300 * 365 +  300 / 4 - 2               // days since 1-Jan-1600
                                                    // (-2 because 1700,1800 not leapyears)
          + 1600 * 365 + 1600 / 4 - 12              // days since 1-Jan-0000
          -  31 -  29;                              // days since 1-Mar-0000 (0000 is a ly)

    qcent = (__uint16_t) (day / (400 * 365 + 400 / 4 - 3));
                                                    // quad centuries since 1-Mar-0000
    day %= 400 * 365 + 400 / 4 - 3;                 // days since 1-Mar in quad century
                                                    // (0 <= day <= 146096)

    if (day == 146096) {                            // 29-Feb-1600,2000,etc
        cent  =   3;
        qyear =  24;
        year  =   3;
        diy   = 365;
    } else {
        cent  = (__uint16_t) (day / (100 * 365 + 100 / 4 - 1));
        dic   = day % (100 * 365 + 100 / 4 - 1);    // day in century since 1-Mar-00
        qyear = dic / (4 * 365 + 4 / 4);            // quad years since 1-Mar-00
        diqy  = dic % (4 * 365 + 4 / 4);            // day in quadyear since 1-Mar-00
        year  = diqy / 365;                         // year in quadyear since 1-Mar-00
        if (year == 4) year = 3;
        diy   = diqy - year * 365;                  // day in year since 1-Mar
    }
    tmbuf->tm_yday = diy;

    year += ((qcent * 4 + cent) * 25 + qyear) * 4;  // years since 1-Mar-0000

    qmonth = diy / (31 + 30 + 31 + 30 + 31);        // quintmonth since 1-Mar
    diy   %= 31 + 30 + 31 + 30 + 31;                // day in quintmonth
    bmonth = diy / (31 + 30);                       // bimonth in quintmonth since 1-Mar
    diy   %= 31 + 30;                               // day in bimonth
    month  = diy / 31;                              // month in bimonth since 1-Mar
    tmbuf->tm_mday = diy % 31 + 1;                  // day in month

    month += qmonth * 5 + bmonth * 2;               // month since 1-Mar
    if (month >= 10) {
        year ++;
        tmbuf->tm_mon   = month - 10;
        tmbuf->tm_yday -= 31 + 30 + 31 + 30 + 31 + 31 + 30 + 31 + 30 + 31;
    } else {
        tmbuf->tm_mon   = month + 2;
        tmbuf->tm_yday += 31 + 28 + isleapyear (year);
    }

    tmbuf->tm_year = year - 1900;
}

static __uint16_t isleapyear (__uint16_t year)
{
    if (year % 4 != 0) return 0;
    year = (year / 4) % 100;
    return (year != 25) && (year != 50) && (year != 75);
}
