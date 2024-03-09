#!/bin/bash
cd `dirname $0`
rm -f alu*.v
../netgen/netgen.sh ../modules/whole.mod -gen alucirc -verilog alu-temp.v
sed 's/module proc/module alucirc/g' alu-temp.v | \
sed 's/_alu_add/N_alu_add/g'         | \
sed 's/_alu_and/N_alu_and/g'         | \
sed 's/_alua_m1/N_alua_m1/g'         | \
sed 's/_alua_ma/N_alua_ma/g'         | \
sed 's/_alub_ac/N_alub_ac/g'         | \
sed 's/_alub_m1/N_alub_m1/g'         | \
sed 's/_alucout/N_alucout/g'         | \
sed 's/_aluq/N_aluq/g'               | \
sed 's/_grpa1q/N_grpa1q/g'           | \
sed 's/_lnq/N_lnq/g'                 | \
sed 's/_maq/N_maq/g'                 | \
sed 's/_newlink/N_newlink/g'         > alucirc.v
