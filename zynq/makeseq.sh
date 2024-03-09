#!/bin/bash
cd `dirname $0`
rm -f seq*.v
../netgen/netgen.sh ../modules/whole.mod -gen seqcirc -verilog seq-temp.v
sed 's/module proc/module seqcirc/g' seq-temp.v | \
sed 's/_ac_aluq/N_ac_aluq/g'  | \
sed 's/_ac_sc/N_ac_sc/g'      | \
sed 's/_alu_add/N_alu_add/g'  | \
sed 's/_alu_and/N_alu_and/g'  | \
sed 's/_alua_m1/N_alua_m1/g'  | \
sed 's/_alua_ma/N_alua_ma/g'  | \
sed 's/_alub_ac/N_alub_ac/g'  | \
sed 's/_alub_m1/N_alub_m1/g'  | \
sed 's/_alucout/N_alucout/g'  | \
sed 's/_grpa1q/N_grpa1q/g'    | \
sed 's/_dfrm/N_dfrm/g'        | \
sed 's/_jump/N_jump/g'        | \
sed 's/_intak/N_intak/g'      | \
sed 's/_ln_wrt/N_ln_wrt/g'    | \
sed 's/_ma_aluq/N_ma_aluq/g'  | \
sed 's/_maq/N_maq/g'          | \
sed 's/_mread/N_mread/g'      | \
sed 's/_mwrite/N_mwrite/g'    | \
sed 's/_pc_aluq/N_pc_aluq/g'  | \
sed 's/_pc_inc/N_pc_inc/g'    > seqcirc.v
