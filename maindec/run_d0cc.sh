#!/bin/bash
#
#  add instruction test
#  runs indefinitely
#  types SIMAD SIMROT FCT RANDOM on teletype after about 6 minutes
#
#  export MAINDECOPTS='-cpuhz 1000000000 -mintimes -nohw'
#
# p52/v1-43
#  field checking code
#
export iodevptaperdr=
export iodevtty_cps=10
export iodevtty_debug=0
export iodevtty='-|-'
export switchregister=0270
exec ../driver/raspictl -binloader -haltstop -startpc 0200 $MAINDECOPTS bins/8E-D0CC.bin
