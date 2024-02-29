#!/bin/bash
#
#  instruction test #2
#  runs indefinitely
#
#  export MAINDECOPTS='-cpuhz 1000000000 -mintimes -nohw'
#  - dings teletype bell every second or so
#
#  export export MAINDECOPTS='-cpuhz 52000 -mintimes'
#  - dings tty bell every five mins or so
#
export iodevptaperdr=
export iodevtty_cps=10
export iodevtty_debug=0
export iodevtty='-|-'
export switchregister=0
exec ../driver/raspictl -binloader -haltstop -startpc 0200 $MAINDECOPTS bins/8E-D0BB.bin
