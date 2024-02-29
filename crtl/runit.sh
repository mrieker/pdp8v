#!/bin/bash
#
#  Run the given .hex file via simulated Hode CPU
#  Can run on raspi or x86 system
#
#   ./runit.sh [<options to raspictl> ...] <hexfile> [<args passed to hexfile's main()> ...]
#
d=`dirname $0`
uname=`uname -m`
exec $d/../driver/raspictl.$uname -nohw "$@"
