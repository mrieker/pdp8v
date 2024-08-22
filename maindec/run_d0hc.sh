#!/bin/bash
#
#  random JMP test
#  runs indefinitely
#  prints <CR><LF>HC on tty every couple seconds
#
cd `dirname $0`
exec ../driver/raspictl "$@" -script d0hc.tcl
