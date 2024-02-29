#!/bin/bash
#
#  Make an unrouted PCB from whole.mod
#  Normally called from makefile that updates whole.mod
#
#  $1 = acl,alu,ma,pc,rpi,seq
#
cd `dirname $0`
make whole.mod
case $1 in
    acl) bh=170 ;;
    alu) bh=170 ;;
    ma)  bh=110 ;;
    pc)  bh=140 ;;
    rpi) bh=80  ;;
    seq) bh=170 ;;
esac
lbl="${1^^} `date +%F`"
echo $lbl
rm -f $1.pcb.tmp $1.rep.tmp $1.map.new
../netgen/netgen.sh whole.mod -gen $1 -boardsize 180,$bh -pcbmap $1.map -pcblabel "$lbl" -pcb $1.pcb.tmp -report $1.rep.tmp -mapedit $1.map.new
mv -i $1.map.new $1.map
