#!/bin/bash
#
#  Run OS/8 from dectape
#
#   $1 =          : run on tubes
#   $1 = -csrclib : gate-level PC simulation
#        -nohwlib : cycle-level PC simulation
#        -zynqlib : gate-level FPGA simulation
#
#  Downloads bootable OS/8 to AL-4711C-BA.tu56
#  Drive DTA1 is available along with RKA0,RKB0,RKA1,RKB1
#
cd `dirname $0`
set -e
if [ tc08boot.oct -ot tc08boot.asm ]
then
    make -C ../asm assemble.`uname -m`
    make -C ../asm link.`uname -m`
    ../asm/assemble tc08boot.asm tc08boot.obj > tc08boot.lis
    ../asm/link -o tc08boot.oct tc08boot.obj > /dev/null
fi
if [ ! -f AL-4711C-BA.tu56 ]
then
    rm -f AL-4711C-BA.tu56.temp
    wget -O AL-4711C-BA.tu56.temp https://www.pdp8online.com/ftp/images/misc_dectapes/AL-4711C-BA.tu56
    chmod a-w AL-4711C-BA.tu56.temp
    mv AL-4711C-BA.tu56.temp AL-4711C-BA.tu56
fi
if [ ! -f dta0.tu56 ]
then
    cat AL-4711C-BA.tu56 > dta0.dt56
fi
exec ../driver/raspictl "$@" -script tc08-boot.tcl
