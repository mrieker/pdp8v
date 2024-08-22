#!/bin/bash
#
#  random ISZ test
#  runs indefinitely
#  babbles <CR><LF>FC<RUB> on tty every pass every second or so
#  doesn't ever seem to test incrementing 7777->0000
#
mkdir -p objs
if [ objs/d0fc-d.obj -ot d0fc-d.asm ]
then
    ../asm/assemble d0fc-d.asm objs/d0fc-d.obj > objs/d0fc-d.lis
fi
if [ objs/d0fc-d.oct -ot objs/d0fc-d.obj ]
then
    ../asm/link -o objs/d0fc-d.oct objs/d0fc-d.obj > objs/d0fc-d.map
fi
exec ../driver/raspictl "$@" -script d0fc.tcl
