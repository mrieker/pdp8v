#!/bin/bash
#
#  Module tester using paddles
#
cd `dirname $0`
./loadmod.sh
. ./iowsns.si
. ./gpioudp.si
./iowowner.sh
dbg=''
if [ "$1" == "-gdb" ]
then
    dbg='gdb --args'
    shift
fi
exec $dbg ./manualboardtest.`uname -m` "$@"
