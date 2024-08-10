#!/bin/bash
#  sets up tty to print here on stdout, just prints error messages
#  optionally run ../driver/tc08status on another screen
cd `dirname $0`
exec ../../driver/raspictl "$@" -script d3ra.tcl
