#!/bin/bash
#
#  extended arithmetic mode B indirect operand test
#  it should prompt on tty for highest field to test
#  enter '7' then press <CR>
#  should print '<CR><LF>KE8 EME' every 30 seconds or so
#
cd `dirname $0`
exec ../driver/raspictl "$@" -script dhkea.tcl
