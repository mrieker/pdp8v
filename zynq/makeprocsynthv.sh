#!/bin/bash
cd `dirname $0`
cd ../modules
make ../de0/proc-synth.v
cd ../zynq
if [ proc-synth.v -ot ../de0/proc-synth.v ]
then
    set -x
    sed 's/_DFRM/DFRM_N/g' ../de0/proc-synth.v | \
        sed 's/_JUMP/JUMP_N/g' | \
        sed 's/_INTAK/INTAK_N/g' | \
        sed 's/_MDL/MDL_N/g' | \
        sed 's/_MD/MD_N/g' | \
        sed 's/_MREAD/MREAD_N/g' | \
        sed 's/_MWRITE/MWRITE_N/g' > proc-synth.v
fi
