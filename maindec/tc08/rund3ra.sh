#!/bin/bash
#  sets up tty to print here on stdout, just prints error messages
#  optionally run ../driver/tc08status on another screen
exec ../../driver/raspictl -nohwlib -script d3ra.tcl
