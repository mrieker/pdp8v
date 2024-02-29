#!/bin/bash
#
#  Run the given .hex file on the Hode CPU hardware boardset
#  Assumes it is running on raspi plugged into Hode boardset
#  Does not require any of the A,C,I,D paddles
#
#   ./runhw.sh [<options to raspictl> ...] <hexfile> [<args passed to hexfile's main()> ...]
#
d=`dirname $0`
$d/../driver/loadmod.sh
uname=`uname -m`
exec $d/../driver/raspictl.$uname "$@"
