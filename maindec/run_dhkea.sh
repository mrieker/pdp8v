#!/bin/bash
#
#  extended arithmetic mode B indirect operand test
#  it should prompt on tty for highest field to test
#  enter '7' then press <CR>
#  should print '<CR><LF>KE8 EME' every 30 seconds or so
#
#  export MAINDECOPTS='-cpuhz 1000000000 -mintimes -nohw'
#
export iodevptaperdr=
export iodevtty_cps=10
export iodevtty_debug=1
export iodevtty=iodevtty.`hostname`
export switchregister=0
exec ../driver/raspictl -binloader -haltstop -startpc 0200 $MAINDECOPTS bins/08-DHKEA.bin
