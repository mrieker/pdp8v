#!/bin/bash
#
#  Compile and run kaleidoscope
#
#   $1 =          : tubes
#   $1 = -nohwlib : fast PC/raspi/pidp simulator
#   $1 = -zynqlib : zynq simulator
#
#  Other options:
#
#   -mintimes : print instruction and point plotting rates once per minute
#   -pidp : update PiDP8 panel leds
#
#  Optional envars:
#
#   vc8pms= persistence in milliseconds (default 500)
#   vc8size= initial window size (default 1024)
#
cd `dirname $0`
if [ kaleid.oct -ot kaleid.pal ]
then
    ../asm/assemble -pal kaleid.pal kaleid.obj > kaleid.lis
    ../asm/link -o kaleid.oct kaleid.obj > kaleid.map
fi
vc8type=e exec ../driver/raspictl "$@" kaleid.oct
