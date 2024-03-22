#!/bin/bash
#
#  Run raspictl GUI wrapper
#  Spawns raspictl in a thread
#
dir=`dirname $0`
unm=`uname -m`
export exename=$dir/gui.$unm
$dir/loadmod.sh
. $dir/iowsns.si
$dir/iowowner.sh
export CLASSPATH=$dir/GUI.jar
export LD_LIBRARY_PATH=$dir
exec java -Dunamem=`uname -m` GUI "$@"
