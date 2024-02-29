#!/bin/bash
#
#  EAE instruction test #1
#  tests everything except multiply and divide
#  repeats forever
#    prints '<CR><LF>' after about 30 sec
#    prints '<CR><LF>KE8 1' after another 30 sec
#  if error, prints message then halts (SR = 5000)
#
#  export MAINDECOPTS='-cpuhz 1000000000 -mintimes -nohw'
#
export iodevptaperdr=
export iodevtty_cps=10
export iodevtty_debug=0
export iodevtty='-|-'
export switchregister=5000
exec ../driver/raspictl -binloader -haltstop -startpc 0200 $MAINDECOPTS bins/8E-D0LB.bin
