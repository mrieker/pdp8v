#!/bin/bash
#
#  extended arithmetic
#  multiply/divide test
#  should halt at 0201 first then 0251 if good
#  just repeats halting at 0251 if good
#    takes only a couple seconds to run
#  prints detailed error if failure
#
#  export MAINDECOPTS='-cpuhz 1000000000 -mintimes -nohw'
#
export iodevptaperdr=
export iodevtty_cps=10
export iodevtty_debug=0
export iodevtty='-|-'
export switchregister=0
exec ../driver/raspictl -binloader -haltcont -haltprint -startpc 0200 $MAINDECOPTS bins/8E-D0MB.bin
