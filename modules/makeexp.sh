#!/bin/bash
#
#  Make an unrouted experiment PCB from exp.mod
#  Normally called from makefile
#
cd `dirname $0`
lbl="EXP `date +%F`"
echo $lbl
rm -f exp.pcb exp.rep
../netgen/netgen.sh exp.mod -gen exp -boardsize 140,70 -pcbmap exp.map -pcblabel "$lbl" -pcb exp.pcb -report exp.rep
mv -i exp.pcb ../kicads/exp/exp.kicad_pcb
