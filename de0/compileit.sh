#!/bin/bash -v
#
#  Run on homepc
#
set -v -x
r=`realpath $0`
cd `dirname $r`
if [ `hostname` != homepc ]
then
    ssh 192.168.1.7 $r
else
    q=~/intelFPGA_lite/20.1/quartus/bin
    $q/quartus_map --read_settings_files=on --write_settings_files=off pdp8 -c pdp8
    $q/quartus_fit --read_settings_files=off --write_settings_files=off pdp8 -c pdp8
    $q/quartus_asm --read_settings_files=off --write_settings_files=off pdp8 -c pdp8
    $q/quartus_sta pdp8 -c pdp8
fi
