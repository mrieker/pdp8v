#!/bin/bash
#
#  random JMP-JMS test
#  runs indefinitely
#  prints <CR><LF>JC on tty every second or two
#
cd `dirname $0`
exec ../driver/raspictl "$@" -script d0jc.tcl
