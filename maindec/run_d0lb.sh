#!/bin/bash
#
#  EAE instruction test #1
#  tests everything except multiply and divide
#  repeats forever
#    prints '<CR><LF>' after about 30 sec
#    prints '<CR><LF>KE8 1' after another 30 sec
#  if error, prints message then halts (SR = 5000)
#
cd `dirname $0`
exec ../driver/raspictl "$@" -script d0lb.tcl
