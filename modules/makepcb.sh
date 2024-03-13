#!/bin/bash -e
#
#  Make an unrouted PCB from whole.mod
#  Normally called from makefile that updates whole.mod
#
#  $1 = acl,alu,ma,pc,rpi,seq
#
cd `dirname $0`
case $1 in
    acl)   bh=140 ;;
    alu)   bh=170 ;;
    ma)    bh=110 ;;
    pc)    bh=140 ;;
    rpi)   bh=80  ;;
    seq)   bh=170 ;;
esac
lbl="${1^^} `date +%F`"
echo $lbl
rm -f $1.pcb $1.rep
../netgen/netgen.sh whole.mod -gen $1 -boardsize 180,$bh -net $1.net -pcbmap $1.map -pcblabel "$lbl" -pcb $1.pcb -report $1.rep
mkdir -p ../kicads/$1
mv -i $1.pcb ../kicads/$1/$1.kicad_pcb
mv -i $1.rep ../kicads/$1/$1.rep
