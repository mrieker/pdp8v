#!/bin/bash
#
#  run life.asm program
#
#   $1 = (nothing) : run on tubes
#        -csrclib  : gate-level simulator (pc, raspi, zynq)
#        -nohwlib  : cycle-level simulator (pc, raspi, zynq)
#        -zynqlib  : tube-speed simulator (zynq only)
#
#  ./life.sh -csrclib < life-1.dat
#
cd `dirname $0`
set -e
if [ life.obj -ot life.asm ]
then
    ../asm/assemble life.asm life.obj > life.lis
fi
if [ life.oct -ot life.obj ]
then
    ../asm/link -o life.oct life.obj > life.map
fi
exec ../driver/raspictl "$@" -script life.tcl
