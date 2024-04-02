#!/bin/bash
set -e
../asm/assemble dttest.asm dttest.obj > dttest.lis
../asm/link -o dttest.oct dttest.obj > dttest.map
touch dttest.dt
exec ../driver/raspictl -csrclib -script dttest.tcl
