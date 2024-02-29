#!/bin/bash
#
#  runs with DE0 board
#
#    maybe add -mintimes
#    maybe add -printstate
#
cd `dirname $0`
read x < ../de0/proc-synth.v
if [ "${x:0:3}" != "// " ]
then
    echo bad // modulename in "$x"
    exit 1
fi
modname=${x:3}
./loadmod.sh
exec ./testboard.armv6l -csource $modname -cpuhz 250000 "$@"
