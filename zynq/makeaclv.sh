#!/bin/bash
cd `dirname $0`
rm -f acl*.v
../netgen/netgen.sh ../modules/whole.mod -gen aclcirc -verilog acl-temp.v
sed 's/module proc/module aclcirc/g' acl-temp.v | \
sed 's/_aluq/N_aluq/g'               | \
sed 's/_acN_aluq/N_ac_aluq/g'        | \
sed 's/_ac_sc/N_ac_sc/g'             | \
sed 's/_grpa1q/N_grpa1q/g'           | \
sed 's/_ln_wrt/N_ln_wrt/g'           | \
sed 's/_lnq/N_lnq/g'                 | \
sed 's/_maq/N_maq/g'                 | \
sed 's/_newlink/N_newlink/g'         > aclcirc.v
