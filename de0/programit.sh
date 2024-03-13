#!/bin/bash
#
#  Run on PC with DE0 plugged into an USB port
#  Assumes the verilog has been compiled
#
set -v -x
q=~/intelFPGA_lite/20.1/quartus/bin
$q/quartus_pgm -l
$q/quartus_pgm -c USB-Blaster -a
$q/quartus_pgm -c USB-Blaster pdp8.cdf
