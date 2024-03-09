#!/bin/bash
cd `dirname $0`
rm -f pc*.v
../netgen/netgen.sh ../modules/whole.mod -gen pccirc -verilog pc-temp.v
sed 's/module proc/module pccirc/g' pc-temp.v | \
sed 's/_aluq/N_aluq/g'               | \
sed 's/_pcN_aluq/N_pc_aluq/g'        | \
sed 's/_pc_inc/N_pc_inc/g'           > pccirc.v
