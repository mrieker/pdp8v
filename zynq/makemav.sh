#!/bin/bash
cd `dirname $0`
rm -f ma*.v
../netgen/netgen.sh ../modules/whole.mod -gen macirc -verilog ma-temp.v
sed 's/module proc/module macirc/g' ma-temp.v | \
sed 's/_aluq/N_aluq/g'               | \
sed 's/_maN_aluq/N_ma_aluq/g'        | \
sed 's/_maq/N_maq/g'                 > macirc.v
