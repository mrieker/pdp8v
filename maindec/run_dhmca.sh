#!/bin/bash
#
#  memory extension and time share control test
#  rings tty bell at end of every pass
#
#  export MAINDECOPTS='-cpuhz 1000000000 -mintimes -nohw'
#    rings tty bell 2 or 3 times a minute
#
cd `dirname $0`
exec ../driver/raspictl "$@" -script dhmca.tcl
