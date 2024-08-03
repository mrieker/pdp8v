#!/bin/bash
#  sets up tty to print here on stdout, but doesn't print anything
#  optionally run ../driver/tc08status on another screen
exec ../../driver/raspictl -nohwlib -script d3eb.tcl
