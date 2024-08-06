#!/bin/bash
#
#  instruction test #1
#  outputs a bell about every 6-7 minutes
#
#  ./run_d0ab.sh           # run on tubes
#  ./run_d0ab.sh -csrclib  # run on PC
#  ./run_d0ab.sh -zynqlib  # run on zynq
#
cd `dirname $0`
exec ../driver/raspictl "$@" -script d0ab.tcl
