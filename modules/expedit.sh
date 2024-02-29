#!/bin/bash
#
#  Edit the exp.map file
#
cd `dirname $0`
lbl="EXP `date +%F`"
echo $lbl
rm -f exp.pcb.tmp exp.rep.tmp exp.map.new
../netgen/netgen.sh exp.mod -gen exp -boardsize 140,70 -pcbmap exp.map -pcblabel "$lbl" -pcb exp.pcb.tmp -report exp.rep.tmp -mapedit exp.map.new
mv -i exp.map.new exp.map
