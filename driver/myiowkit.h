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

#ifndef _MYIOWKIT_H
#define _MYIOWKIT_H

#include <stdint.h>

// product ids
#define IOWKIT_PRODUCT_ID_IOW40  0x1500
#define IOWKIT_PRODUCT_ID_IOW24  0x1501
#define IOWKIT_PRODUCT_ID_IOW24_SENSI  0x158A
#define IOWKIT_PRODUCT_ID_IOWPV1 0x1511
#define IOWKIT_PRODUCT_ID_IOWPV2 0x1512
#define IOWKIT_PRODUCT_ID_IOW56  0x1503
#define IOWKIT_PRODUCT_ID_IOW56_ALPHA  0x158B
#define IOWKIT_PRODUCT_ID_IOW28     0x1504
#define IOWKIT_PRODUCT_ID_IOW28L    0x1505
#define IOWKIT_PRODUCT_ID_IOW100    0x1506

// pipe names
#define IOW_PIPE_IO_PINS      0
#define IOW_PIPE_SPECIAL_MODE 1
#define IOW_PIPE_I2C_MODE       2   //only IOW28, IOW100 (not IOW28L)
#define IOW_PIPE_ADC_MODE       3   //only IOW28, IOW100 (not IOW28L)

// first IO-Warrior revision with serial numbers
#define IOW_NON_LEGACY_REVISION 0x1010

typedef struct _IOWKIT_REPORT
 {
  uint8_t ReportID;
  uint8_t Bytes[4];
 }
  IOWKIT_REPORT, *PIOWKIT_REPORT;

typedef struct _IOWKIT40_IO_REPORT
 {
  uint8_t ReportID;
  uint8_t Bytes[4];
 }
  IOWKIT40_IO_REPORT, *PIOWKIT40_IO_REPORT;

typedef struct _IOWKIT24_IO_REPORT
 {
  uint8_t ReportID;
  uint8_t Bytes[2];
 }
  IOWKIT24_IO_REPORT, *PIOWKIT24_IO_REPORT;

typedef struct _IOWKIT_SPECIAL_REPORT
 {
  uint8_t ReportID;
  uint8_t Bytes[7];
 }
  IOWKIT_SPECIAL_REPORT, *PIOWKIT_SPECIAL_REPORT;

typedef struct _IOWKIT56_IO_REPORT
 {
  uint8_t ReportID;
  uint8_t Bytes[7];
 }
  IOWKIT56_IO_REPORT, *PIOWKIT56_IO_REPORT;

typedef struct _IOWKIT56_SPECIAL_REPORT
 {
  uint8_t ReportID;
  uint8_t Bytes[63];
 }
  IOWKIT56_SPECIAL_REPORT, *PIOWKIT56_SPECIAL_REPORT;

typedef struct _IOWKIT28_IO_REPORT
{
    uint8_t ReportID;
    uint8_t Bytes[4];
}
IOWKIT28_IO_REPORT, *PIOWKIT28_IO_REPORT;

typedef struct _IOWKIT28_SPECIAL_REPORT
{
    uint8_t ReportID;
    uint8_t Bytes[63];
}
IOWKIT28_SPECIAL_REPORT, *PIOWKIT28_SPECIAL_REPORT;

typedef struct _IOWKIT100_IO_REPORT
{
    uint8_t ReportID;
    uint8_t Bytes[12];
}
IOWKIT100_IO_REPORT, *PIOWKIT100_IO_REPORT;

typedef struct _IOWKIT100_SPECIAL_REPORT
{
    uint8_t ReportID;
    uint8_t Bytes[63];
}
IOWKIT100_SPECIAL_REPORT, *PIOWKIT100_SPECIAL_REPORT;

typedef struct IowKit
{
    static void list (void (*each) (void *ctx, char const *dn, int pipe, char const *sn, int pid, int rev), void *ctx);
    static char const *pidname (int pid);

    IowKit ();
    ~IowKit ();

    bool openbysn (char const *sn);
    void close ();

    char const *getsn ();
    int getpid ();
    int getrev ();

    void read (int numPipe, void *buffer, int length);
    void write (int numPipe, void const *buffer, int length);

private:
    int *fds;
    struct iowarrior_info *infos;
} *IOWKIT_HANDLE;

#endif
