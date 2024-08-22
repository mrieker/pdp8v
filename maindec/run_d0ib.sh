#!/bin/bash
#
#  random JMP-JMS test
#  runs indefinitely
#  rings bell on tty every second or two
#
#  export MAINDECOPTS='-cpuhz 1000000000 -mintimes -nohw'
#
cd `dirname $0`
exec ../driver/raspictl "$@" -script d0ib.tcl
