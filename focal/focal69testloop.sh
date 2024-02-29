#!/bin/bash
mach=`uname -m`
if [ focal69test.$mach -ot focal69test.c ]
then
    cc -o focal69test.$mach focal69test.c
fi
while true
do
    date
    ./focal69test.$mach
    date
    sleep 2
done
