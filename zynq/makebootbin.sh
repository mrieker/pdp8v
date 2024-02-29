#!/bin/bash
#
#  create the BOOT.BIN file for pdp8v
#  assumes the .bit bitstream file is up-to-date
#
set -e -v
cd `dirname $0`
mydir=`pwd`
cd ~/MYIR_HW
source petadevel/settings.sh
source /tools/Xilinx/Vivado/2018.3/settings64.sh
cd petakernl/zturn
rm -f images/linux/BOOT.BIN
petalinux-package --boot --force --fsbl ./images/linux/zynq_fsbl.elf --fpga $mydir/pdp8v.runs/impl_1/myboard_wrapper.bit --u-boot
ls -l `pwd`/images/linux/BOOT.BIN
scp images/linux/BOOT.BIN root@192.168.1.19:/boot/BOOT.BIN

# ssh root@192.168.1.19 reboot
# ssh 192.168.1.19
# cd nfs/pdp8/driver
# ./raspictl -zynqlib [-paddles] -randmem -mintimes -quiet

