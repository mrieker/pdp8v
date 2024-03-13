#!/bin/bash
#
#  create/update the <board>circ.v files from the <board>.mod files
#  no need for rpi board, it is built into backplane.v
#
cd `dirname $0`
set -e -x
make -C ../modules whole.mod
for mod in acl alu ma pc seq
do
    if [ ${mod}circ.v -ot ../modules/whole.mod ]
    then
        rm -f ${mod}circ.v
        ../netgen/netgen.sh ../modules/whole.mod -gen ${mod}circ -verilog ${mod}circ.v
    fi
done
