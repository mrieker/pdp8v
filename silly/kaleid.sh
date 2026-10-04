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
#   switchregister=04000  to draw full circle patters (else normal)
#   vc8pms= persistence in milliseconds (default 9000)
#   vc8size= initial window size (default 800)
#
cd `dirname $0`
if [ kaleid.oct -ot kaleid.pal ]
then
    ../asm/assemble -pal kaleid.pal kaleid.obj > kaleid.lis
    ../asm/link -o kaleid.oct kaleid.obj > kaleid.map
fi
if [ "$switchregister" == "" ]
then
    export switchregister=0
fi
if [ "$vc8pms" == "" ]
then
    export vc8pms=9000
fi
if [ "$vc8size" == "" ]
then
    export vc8size=800
fi
export vc8type=e
exec ../driver/raspictl "$@" kaleid.oct
