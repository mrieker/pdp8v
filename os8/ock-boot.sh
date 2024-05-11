#!/bin/bash
#
#  Run OS/8 with 3 disks
#
#   $1 =          : run on tubes
#   $1 = -csrclib : various simulators
#        -nohwlib
#        -zynqlib
#
cd `dirname $0`
set -e
if [ ! -f ock.rk05 ]
then
    rm -f ock.rk05.temp
    wget -O ock.rk05.temp https://tangentsoft.com/pidp8i/uv/ock.rk05
    chmod a-w ock.rk05.temp
    mv ock.rk05.temp ock.rk05
fi
if [ ! -f rkab0.rk05 ]
then
    cat ock.rk05 > rkab0.rk05
fi
exec ../driver/raspictl "$@" -script ock-boot.tcl
