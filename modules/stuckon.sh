#!/bin/bash
#
#  Generate stuckon.mod which is a copy of proc.mod with stuckon envars applied
#  Then it runs the simulator on stuckon.mod
#
set -e
#export stuckon_Q_0__dfrm_seq_0_0_2=1    # seq.rep: J.0._dfrm/seqcirc[0:0]2  J89  16.9, 13.1
#export stuckon_DFF_malo_ma_0_f=1        #  ma.rep: J.f.0.DFF/malo/macirc    J83  12.9,  8.6
../netgen/netgen.sh whole.mod -gen proc -myverilog stuckon.mod 2>&1 | grep -v ' not in place$'
export pipesim_modfile=stuckon.mod
export switchregister=random
exec ./pipesim.sh -quiet -randmem -printinstr -printstate -cyclelimit 100000
