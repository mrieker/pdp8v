#!/bin/bash
#
#  run something on the DE0
#  assumes DE0 connected to this raspi
#  ...and has been programmed with de0.v & proc-manual.v
#
#   ./run_de0.sh -cpuhz 1000000000 -mintimes -quiet -randmem
#   ./run_de0.sh -binloader -cpuhz 1000000000 -mintimes -startpc 0200 ../maindec/bins/8E-D0BB.bin
#
dd=`dirname $0`
export iodevptaperdr=
export iodevtty_cps=10
export iodevtty_debug=1
export iodevtty=iodevtty.`hostname`
export switchregister=0
exec $dd/../driver/raspictl "$@"
