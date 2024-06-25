#!/bin/bash
#
#  Start BASIC running
#
#   ./runpt.sh              - run on tubes
#   ./runpt.sh -csrclib     - run on gate-level PC simulator
#   ./runpt.sh -nohwlib     - run on cycle-level PC simulator
#   ./runpt.sh -zynqlib     - run on zynq simulator
#
exec ../driver/raspictl "$@" -script runpt.tcl
