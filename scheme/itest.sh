#!/bin/bash
export iodevtty='-|-'
if [ "$1" != "" ]
then
    export iodevtty=$1
fi
export switchregister=0001
export iodevtty_cps=1000000
export iodevtty_debug=0
export iodevtty_doublcr=0
exec ../driver/raspictl -nohw -startpc 0200 scheme.oct
