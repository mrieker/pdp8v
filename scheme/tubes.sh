#!/bin/bash
#
#  run scheme on the tubes
#  assumes it is running on raspi plugged into tubes
#
flags=
export iodevtty='-|-'
if [ "$1" != "" ]
then
    flags=' -mintimes'
    export iodevtty=$1
fi
export switchregister=0010
export iodevtty_cps=120
export iodevtty_debug=0
export iodevtty_doublcr=0
exec ../driver/raspictl $flags -startpc 0200 scheme.oct
