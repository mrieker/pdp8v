#!/bin/bash
#
#  instruction test #1
#  halts once at 0146 then runs indefinitely
#
#  export MAINDECOPTS='-cpuhz 1000000000 -mintimes -nohw'
#  - dings tty bell every second or so
#
#  export export MAINDECOPTS='-cpuhz 52000 -mintimes'
#  - dings tty bell every five mins or so
#
export iodevptaperdr=
export iodevtty_cps=10
export iodevtty_debug=1
export iodevtty='-|-'
export switchregister=7777
exec ../driver/raspictl -binloader -haltcont -haltprint -startpc 0200 $MAINDECOPTS bins/8E-D0AB.bin
