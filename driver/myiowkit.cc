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

// my version of the IOWarrior library

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <map>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>

#include "myiowkit.h"

// https://stackoverflow.com/questions/4157687/using-char-as-a-key-in-stdmap
struct cmp_str
{
   bool operator () (char const *a, char const *b) const
   {
      return strcmp (a, b) < 0;
   }
};

#define USB_VENDOR_ID_CODEMERCS 1984
/* low speed iowarrior */
#define USB_DEVICE_ID_CODEMERCS_IOW40   0x1500
#define USB_DEVICE_ID_CODEMERCS_IOW24   0x1501
#define USB_DEVICE_ID_CODEMERCS_IOWPV1  0x1511
#define USB_DEVICE_ID_CODEMERCS_IOWPV2  0x1512
#define USB_DEVICE_ID_CODEMERCS_IOW24_SENSI 0x158A
/* full speed iowarrior */
#define USB_DEVICE_ID_CODEMERCS_IOW56   0x1503
#define USB_DEVICE_ID_CODEMERCS_IOW56_ALPHA 0x158B

#define USB_DEVICE_ID_CODEMERCS_IOW28   0x1504
#define USB_DEVICE_ID_CODEMERCS_IOW28L  0x1505
#define USB_DEVICE_ID_CODEMERCS_IOW100  0x1506
#define USB_DEVICE_ID_CODEMERCS_IOWTW   0x15FF

#define CODEMERCS_MAGIC_NUMBER  0xC0    // like COde Mercenaries

/* Define an ioctl command for getting more info on the device */
 
/* A struct for available device info which is used */
/* in the new ioctl IOW_GETINFO                     */
struct iowarrior_info {
    int32_t vendor;         // vendor id : supposed to be USB_VENDOR_ID_CODEMERCS in all cases
    int32_t product;        // product id : depends on type of chip (USB_DEVICE_ID_CODEMERCS_XXXXX)
    char serial[9];         // the serial number of our chip (if a serial-number is not available this is empty string)
    int32_t revision;       // revision number of the chip
    int32_t speed;          // USB-speed of the device (0=UNKNOWN, 1=LOW, 2=FULL 3=HIGH)
    int32_t power;          // power consumption of the device in mA
    int32_t if_num;         // the number of the endpoint
    int32_t packet_size;    // size of the data-packets on this interface
};

/* Define the ioctl commands for reading and writing data */
#define IOW_WRITE   _IOW(CODEMERCS_MAGIC_NUMBER, 1, long)
#define IOW_READ    _IOW(CODEMERCS_MAGIC_NUMBER, 2, long)

/* Get some device-information (product-id , serial-number etc.) in order to identify a chip. */
#define IOW_GETINFO _IOR(CODEMERCS_MAGIC_NUMBER, 3, struct iowarrior_info)

#define IOWKIT_MAX_PIPES 4

void IowKit::list (void (*each) (void *ctx, char const *dn, int pipe, char const *sn, int pid, int rev), void *ctx)
{
    struct dirent **namelist;
    int nents = scandir ("/dev/usb", &namelist, NULL, NULL);
    if (nents < 0) {
        if (errno == ENOENT) return;
        fprintf (stderr, "IowKit::list: error scanning /dev/usb: %m\n");
        abort ();
    }
    for (int i = 0; i < nents; i ++) {
        struct dirent *de = namelist[i];
        if (memcmp (de->d_name, "iowarrior", 9) == 0) {
            char devpath[10+strlen(de->d_name)];
            sprintf (devpath, "/dev/usb/%s", de->d_name);
            int fd = open (devpath, O_RDONLY);
            if (fd < 0) {
                fprintf (stderr, "IowKit::list: error opening %s: %m\n", devpath);
                abort ();
            }

            struct iowarrior_info info;
            if (ioctl (fd, IOW_GETINFO, &info) < 0) {
                fprintf (stderr, "IowKit::list: error getting %s info: %m\n", devpath);
                abort ();
            }
            ::close (fd);

            (*each) (ctx, devpath, info.if_num, info.serial, info.product, info.revision);
        }
        free (de);
    }
    free (namelist);
}

char const *IowKit::pidname (int pid)
{
    switch (pid) {
        case IOWKIT_PRODUCT_ID_IOW24: return "IO-Warrior24";
        case IOWKIT_PRODUCT_ID_IOWPV1: return "IO-Warrior24PV";
        case IOWKIT_PRODUCT_ID_IOW40: return "IO-Warrior40";
        case IOWKIT_PRODUCT_ID_IOW56: return "IO-Warrior56";
        case IOWKIT_PRODUCT_ID_IOW28: return "IO-Warrior28";
        case IOWKIT_PRODUCT_ID_IOW100: return "IO-Warrior100";
        default: return NULL;
    }
}

IowKit::IowKit ()
{
    fds   = new int[IOWKIT_MAX_PIPES];
    infos = new iowarrior_info[IOWKIT_MAX_PIPES];
    memset (fds, 0xFF, IOWKIT_MAX_PIPES * sizeof *fds);
    memset (infos, 0, IOWKIT_MAX_PIPES * sizeof *infos);
}

IowKit::~IowKit ()
{
    this->close ();
    delete[] fds;
    delete[] infos;
    fds   = NULL;
    infos = NULL;
}

bool IowKit::openbysn (char const *sn)
{
    struct dirent **namelist;
    int nents = scandir ("/dev/usb", &namelist, NULL, NULL);
    if (nents < 0) {
        if (errno == ENOENT) return false;
        fprintf (stderr, "IowKit::openbysn: error scanning /dev/usb: %m\n");
        abort ();
    }
    int nfound = 0;
    for (int i = 0; i < nents; i ++) {
        struct dirent *de = namelist[i];
        if (memcmp (de->d_name, "iowarrior", 9) == 0) {
            char devpath[10+strlen(de->d_name)];
            sprintf (devpath, "/dev/usb/%s", de->d_name);
            int fd = open (devpath, O_RDWR);
            if (fd < 0) {
                if (errno == EBUSY) goto itsbusy;   // already open, such as by another openbysn()
                fprintf (stderr, "IowKit::openbysn: error opening %s: %m\n", devpath);
                abort ();
            }

            struct iowarrior_info info;
            if (ioctl (fd, IOW_GETINFO, &info) < 0) {
                fprintf (stderr, "IowKit::openbysn: error getting %s info: %m\n", devpath);
                abort ();
            }
            if ((info.if_num < 0) || (info.if_num >= IOWKIT_MAX_PIPES)) {
                fprintf (stderr, "IowKit::openbysn: bad if_num on %s: %d\n", devpath, info.if_num);
                abort ();
            }
            if (info.serial[0] == 0) {
                fprintf (stderr, "IowKit::openbysn: null serial number on %s\n", devpath);
                abort ();
            }
            if (strcmp (info.serial, sn) == 0) {
                this->infos[info.if_num] = info;
                this->fds[info.if_num] = fd;
                nfound ++;
            } else {
                ::close (fd);
            }
        }
    itsbusy:;
        free (de);
    }
    free (namelist);
    return nfound > 0;
}

void IowKit::close ()
{
    for (int i = 0; i < IOWKIT_MAX_PIPES; i ++) {
        if (this->fds[i] >= 0) {
            ::close (this->fds[i]);
            this->fds[i] = -1;
        }
    }
}

char const *IowKit::getsn ()
{
    return this->infos[0].serial;
}

int IowKit::getpid ()
{
    return this->infos[0].product;
}

int IowKit::getrev ()
{
    return this->infos[0].revision;
}

void IowKit::read (int numPipe, void *buffer, int length)
{
    int extra = -1;
    switch (numPipe) {
        case IOW_PIPE_IO_PINS:
            extra = 1;
            break;
        case IOW_PIPE_SPECIAL_MODE:
        case IOW_PIPE_ADC_MODE:
        case IOW_PIPE_I2C_MODE:
            extra = 0;
            break;
    }
    if ((extra < 0) || (this->fds[numPipe] < 0)) {
        fprintf (stderr, "IowKit::read: bad pipe %d\n", numPipe);
        abort ();
    }

    int siz = this->infos[numPipe].packet_size;
    if (length != siz + extra) {
        fprintf (stderr, "IowKit::read: bad length %d not %d\n", length, siz + extra);
        abort ();
    }

    int rc = ::read (this->fds[numPipe], ((char *)buffer) + extra, siz);
    if (rc < 0) {
        fprintf (stderr, "IowKit::read: error reading %s pipe %d: %m\n", this->infos[numPipe].serial, numPipe);
        abort ();
    }
    if (rc != siz) {
        fprintf (stderr, "IowKit::read: only read %d of %d from %s pipe %d\n", rc, siz, this->infos[numPipe].serial, numPipe);
        abort ();
    }
}

void IowKit::write (int numPipe, void const *buffer, int length)
{
    int extra = -1;
    switch (numPipe) {
        case IOW_PIPE_IO_PINS:
            extra = 1;
            break;
        case IOW_PIPE_SPECIAL_MODE:
        case IOW_PIPE_ADC_MODE:
        case IOW_PIPE_I2C_MODE:
            extra = 0;
            break;
    }
    if ((extra < 0) || (this->fds[numPipe] < 0)) {
        fprintf (stderr, "IowKit::write: bad pipe %d\n", numPipe);
        abort ();
    }

    int siz = this->infos[numPipe].packet_size;
    if (length != siz + extra) {
        fprintf (stderr, "IowKit::write: bad length %d not %d\n", length, siz + extra);
        abort ();
    }

    int rc = ::write (this->fds[numPipe], ((char const *)buffer) + extra, siz);
    if (rc < 0) {
        fprintf (stderr, "IowKit::write: error writing %s pipe %d: %m\n", this->infos[numPipe].serial, numPipe);
        abort ();
    }
    if (rc != siz) {
        fprintf (stderr, "IowKit::write: only wrote %d of %d to %s pipe %d\n", rc, siz, this->infos[numPipe].serial, numPipe);
        abort ();
    }
}
