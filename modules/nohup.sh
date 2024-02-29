#!/bin/bash
#
#  Run simulation in background
#
#  $1 = log file name
#
cd `dirname $0`
nohup ./simit.sh < /dev/null > $1 2>&1 &
