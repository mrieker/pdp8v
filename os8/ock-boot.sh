#!/bin/bash
#
#  Run OS/8 with 3 disks
#
#   $1 =          : run on tubes
#   $1 = -csrclib : gate-level PC simulation
#        -nohwlib : cycle-level PC simulation
#        -zynqlib : gate-level FPGA simulation
#
#  Downloads bootable OS/8 to rkab0.rk05 (saves backup copy in ock.rk05)
#  Creates empty rkab1.rk05 and rkab2.rk05
#
cd `dirname $0`
set -e
if [ rk8jeboot.oct -ot rk8jeboot.asm ]
then
    make -C ../asm assemble.`uname -m`
    make -C ../asm link.`uname -m`
    ../asm/assemble rk8jeboot.asm rk8jeboot.obj > rk8jeboot.lis
    ../asm/link -o rk8jeboot.oct rk8jeboot.obj > /dev/null
fi
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
if [ ! -f v3d-src.rk05 ]
then
    rm -f v3d-src.rk05.temp
    wget -O v3d-src.rk05.temp https://tangentsoft.com/pidp8i/uv/v3d-src.rk05
    chmod a-w v3d-src.rk05.temp
    mv v3d-src.rk05.temp v3d-src.rk05
fi
if [ ! -f rkab2.rk05 ]
then
    cat v3d-src.rk05 > rkab2.rk05
fi
exec ../driver/raspictl "$@" -script ock-boot.tcl
