#!/bin/bash
cd `dirname $0`
if [ doubleroll.oct -ot doubleroll.pal ]
then
    ../asm/assemble -pal doubleroll.pal doubleroll.obj > doubleroll.lis
    ../asm/link -o doubleroll.oct doubleroll.obj > doubleroll.map
fi
if [ "$swithregister" == "" ]
then
    export switchregister=0140
fi
exec ../driver/raspictl "$@" -mintimes doubleroll.oct
