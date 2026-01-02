#!/bin/bash
if [ lightroll.oct -ot lightroll.pal ]
then
    ../asm/assemble -pal lightroll.pal lightroll.obj > lightroll.lis
    ../asm/link -o lightroll.oct lightroll.obj > lightroll.map
fi
exec ../driver/raspictl "$@" -skipopt -mintimes lightroll.oct
