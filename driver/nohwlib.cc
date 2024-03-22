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

// simulate processor via shadow
// no hardware needed

#include <time.h>

#include "gpiolib.h"
#include "shadow.h"

NohwLib::NohwLib (Shadow *shadow)
{
    libname = "nohwlib";

    this->shadow = shadow;
}

void NohwLib::open ()
{ }

void NohwLib::close ()
{ }

void NohwLib::halfcycle ()
{ }

uint32_t NohwLib::readgpio ()
{
    uint32_t value = shadow->readgpio ((gpiowritten & G_IRQ) != 0);
    value = (value & ~ G_OUTS) | (gpiowritten & G_OUTS);
    if (! (value & G_DENA)) {
        value = (value & ~ (G_LINK | G_DATA)) | (gpiowritten & (G_LINK | G_DATA));
    }
    return value;
}

void NohwLib::writegpio (bool wdata, uint32_t value)
{
    gpiowritten = value | (wdata ? G_QENA : G_DENA);
}

bool NohwLib::haspads ()
{
    return false;
}

void NohwLib::readpads (uint32_t *pinss)
{
    ABORT ();
}

void NohwLib::writepads (uint32_t const *masks, uint32_t const *pinss)
{
    ABORT ();
}
