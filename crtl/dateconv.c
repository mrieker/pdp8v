
#include <stdio.h>
#include <time.h>

#ifndef __HODE__
static __uint64_t getnowns ()
{
    __uint32_t nowts = time (NULL);
    return (__uint64_t) nowts * (__uint64_t) 1000000000 + 987654321;
}
#endif

static void dateconv (__uint32_t day, __uint16_t *dom_r, __uint16_t *month_r, __uint16_t *year_r);

int main (int argc, char **argv)
{
    __uint64_t nowns  = getnowns ();
    __uint32_t nowsec = nowns /     1000000000;
    __uint32_t nowday = nowns / 86400000000000;
    __uint16_t nowdom, nowmon, nowyear;
    dateconv (nowday, &nowdom, &nowmon, &nowyear);
    printf ("%llu -> %lu -> %lu -> %04u-%02u-%02u\n", nowns, nowsec, nowday, nowyear, nowmon, nowdom);

    return 0;
}

// input:
//  day = days since 1970-01-01 (0 for 1970-01-01)
// output:
//  *dom_r   = day of month (01..31)
//  *month_r = month of year (01..12)
//  *year_r  = full year number
static void dateconv (__uint32_t day, __uint16_t *dom_r, __uint16_t *month_r, __uint16_t *year_r)
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

    year += ((qcent * 4 + cent) * 25 + qyear) * 4;
                                                    // years since 1-Mar-0000

    qmonth = diy / (31 + 30 + 31 + 30 + 31);        // quintmonth since 1-Mar
    diy   %= 31 + 30 + 31 + 30 + 31;                // day in quintmonth
    bmonth = diy / (31 + 30);                       // bimonth in quintmonth since 1-Mar
    diy   %= 31 + 30;                               // day in bimonth
    month  = diy / 31;                              // month in bimonth since 1-Mar
    diy   %= 31;                                    // day in month

    month += qmonth * 5 + bmonth * 2;               // month since 1-Mar
    if (month >= 10) {
        month -= 12;
        year ++;
    }

    *dom_r   = diy + 1;
    *month_r = month + 3;
    *year_r  = year;
}
