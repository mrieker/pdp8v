#!/bin/bash
#
#  Run autoboardtest with all board combinations
#  Runs each combo for 10 seconds
#  Intended to run on simulators (csrclib,zynqlib) only
#  ...cuz you would have to plug/unplug tube boards quickly
#
#  $1... = options to pass to autoboardtest
#           -csrclib : run on PC for testing autoboardtest itself
#           -pipelib : run on PC using tcl-based simulator
#           -zynqlib : run on zynq
#          plus other options such as -verbose -page
#
for ((i = 1 ; i < 64 ; i ++))
do
    g=$(($i^($i>>1)))
    echo = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
    boards=""
    if ((g & 32)) ; then boards="$boards acl" ; fi
    if ((g & 16)) ; then boards="$boards alu" ; fi
    if ((g &  8)) ; then boards="$boards ma"  ; fi
    if ((g &  4)) ; then boards="$boards pc"  ; fi
    if ((g &  2)) ; then boards="$boards rpi" ; fi
    if ((g &  1)) ; then boards="$boards seq" ; fi
    echo = = = = $i$boards
    sleep 1
    ./autoboardtest "$@"$boards & < /dev/null
    abtpid=$!
    sleep 5
    kill -INT $abtpid
    wait
    echo = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =
done
