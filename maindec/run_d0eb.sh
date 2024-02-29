#!/bin/bash
#
#  random TAD test
#  runs indefinitely
#  types T<CR><LF> on tty every second or so
#
#  export MAINDECOPTS='-cpuhz 1000000000 -mintimes -nohw'
#
export iodevptaperdr=
export iodevtty_cps=10
export iodevtty_debug=0
export iodevtty='-|-'
export switchregister=0
exec ../driver/raspictl -binloader -haltstop -startpc 0200 $MAINDECOPTS bins/8E-D0EB.bin
