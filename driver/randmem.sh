#!/bin/bash
#
#  run randmem test in background
#  supplies random opcodes and data to the cpu for testing
#  runs on raspi plugged into rasboard
#  assumes whole cpu is available with all boards
#  paddles are optional
#
cd `dirname $0`
if [ "$1" == "" ]
then
    ./loadmod.sh
    nohup ./`basename $0` loop < /dev/null >> randmem.log 2>&1 &
    exit
fi
if [ "$1" == "loop" ]
then
    cpuhz=472000
    ##for ((cpuhz=480000; $cpuhz != 468000; cpuhz=$cpuhz-1000))
    ##do
        date >> randmem-$cpuhz.log
        ./raspictl.armv7l -randmem -mintimes -cpuhz $cpuhz >> randmem-$cpuhz.log 2>&1
        date >> randmem-$cpuhz.log
        sleep 3
    ##done
    exit
fi
