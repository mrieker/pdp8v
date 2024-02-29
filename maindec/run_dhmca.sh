#!/bin/bash
#
#  memory extension and time share control test
#  rings tty bell at end of every pass
#
#  export MAINDECOPTS='-cpuhz 1000000000 -mintimes -nohw'
#    rings tty bell 2 or 3 times a minute
#
#  export MAINDECOPTS='-cpuhz 52000 -mintimes'
#
export iodevptaperdr=
export iodevtty_cps=10
export iodevtty_debug=1
export iodevtty='-|-'
export switchregister=0007
exec ../driver/raspictl -binloader -haltstop -haltprint -infloopstop -startpc 0200 $MAINDECOPTS bins/MAINDEC-08-DHMCA-b-pb.bin
