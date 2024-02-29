#!/bin/bash
#
#  Run FOCAL interpreter
#
cd `dirname $0`
make
export iodevrtc_factor=1
export iodevtty=/dev/pts/2
exec ../../driver/raspictl -cpuhz 1000000000 -csrclib -linc -startpc 0200 focal12.oct
