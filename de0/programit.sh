#!/bin/bash
#
#  Run on homepc with DE0 plugged into an USB port
#  Assumes the verilog has been compiled
#
set -v -x
r=`realpath $0`
cd `dirname $r`
if [ `hostname` != homepc ]
then
    ssh 192.168.1.7 $r
else
    q=~/intelFPGA_lite/20.1/quartus/bin
    $q/quartus_pgm -l
    $q/quartus_pgm -c USB-Blaster -a
    $q/quartus_pgm -c USB-Blaster pdp8.cdf
fi
