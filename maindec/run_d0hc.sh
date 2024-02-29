#!/bin/bash
#
#  random JMP test
#  runs indefinitely
#  prints <CR><LF>HC on tty every couple seconds
#
#  export MAINDECOPTS='-cpuhz 1000000000 -mintimes -nohw'
#
export iodevptaperdr=
export iodevtty_cps=10
export iodevtty_debug=0
export iodevtty='-|-'
export switchregister=0
exec ../driver/raspictl -binloader -haltstop -startpc 0200 $MAINDECOPTS bins/8E-D0HC.bin
