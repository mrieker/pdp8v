#!/bin/bash
#
#  random AND test
#  runs indefinitely
#  prints <CR><LF>A on tty every second or so
#
#  export MAINDECOPTS='-cpuhz 1000000000 -mintimes -nohw'
#
#  run on tubes:
#    export addhz=17000
#    export MAINDECOPTS='-cpuhz 22000 -mintimes'
#  prints <CR><LF>A every 5 mins or so
#
export iodevptaperdr=
export iodevtty_cps=10
export iodevtty_debug=0
export iodevtty='-|-'
export switchregister=0
exec ../driver/raspictl -binloader -haltstop -startpc 0200 $MAINDECOPTS bins/8E-D0DB.bin
