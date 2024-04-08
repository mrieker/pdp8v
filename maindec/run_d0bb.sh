#!/bin/bash
#
#  instruction test #2
#  runs indefinitely
#  - dings tty bell every five mins or so
#
exec ../driver/raspictl "$@" -script d0bb.tcl
