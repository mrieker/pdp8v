#!/bin/bash
#
#  random ISZ test
#  runs indefinitely
#  babbles <CR><LF>FC<RUB> on tty every pass every second or so
#  doesn't ever seem to test incrementing 7777->0000
#
#  export MAINDECOPTS='-cpuhz 1000000000 -mintimes -nohw'
#
mkdir -p objs
if [ objs/d0fc-d.obj -ot d0fc-d.asm ]
then
    ../asm/assemble.x86_64 d0fc-d.asm objs/d0fc-d.obj > objs/d0fc-d.lis
fi
if [ objs/d0fc-d.oct -ot objs/d0fc-d.obj ]
then
    ../asm/link.x86_64 -o objs/d0fc-d.oct objs/d0fc-d.obj > objs/d0fc-d.map
fi
export iodevptaperdr=
export iodevtty_cps=10
export iodevtty_debug=0
export iodevtty='-|-'
export switchregister=0
exec ../driver/raspictl -haltstop -startpc 0200 $MAINDECOPTS objs/d0fc-d.oct
