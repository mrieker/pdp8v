#!/bin/bash
#
#  random DCA test
#  runs indefinitely
#  rings bell on tty every pass every second or so
#
exec ../driver/raspictl "$@" -script d0gc.tcl
