#!/bin/bash
#
#  random AND test
#  runs indefinitely
#
#  prints <CR><LF>A every 5 mins or so
#
#  ./run_d0db.sh           # run on tubes
#  ./run_d0db.sh -csrclib  # run on PC simulator
#  ./run_d0db.sh -zynqlib  # run on zynq
#
exec ../driver/raspictl "$@" -script d0db.tcl
