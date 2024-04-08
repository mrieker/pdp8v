#!/bin/bash
#
#  random TAD test
#  runs indefinitely
#  types T<CR><LF> on tty every 5-6 minutes
#
exec ../driver/raspictl "$@" -script d0eb.tcl
