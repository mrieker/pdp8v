#!/bin/bash
#
#  Generate stuckon.mod which is a copy of proc.mod with stuckon envars applied
#  Then it runs the simulator on stuckon.mod
#
#  nohup ./stuckongen.sh < /dev/null > stuckongen.log 2>&1 &
#
cd `dirname $0`

function processlist
{
    while read line
    do
        if [ "$line" == "${line/reset/}" ]
        then
            echo ""
            date
            echo "$line"
            varname="${line%% # *}"
            echo "$line" > stuckongen/$varname.out
            export stuckon_$varname=1
            ./stuckon.sh >> stuckongen/$varname.out 2>&1
            unset stuckon_$varname
            echo ""
        else
            echo "$line - skipping"
        fi
    done
}

rm -rf stuckongen
mkdir stuckongen
rm -f stuckon.mod
../netgen/netgen.sh whole.mod -gen proc -myverilog stuckon.mod 2>&1 | grep -v ' not in place$'
export CLASSPATH=.
java StuckOnsExtract < stuckon.mod | sort | processlist
