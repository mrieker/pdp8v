#!/bin/bash
set -e
if [ dttest.oct -ot dttest.asm ]
then
    ../asm/assemble dttest.asm dttest.obj > dttest.lis
    ../asm/link -o dttest.oct dttest.obj > dttest.map
fi
exec ../driver/raspictl "$@" -script dttest.tcl
