#!/bin/bash

function compit
{
    while read cname
    do
        sname=${cname/.c/.s}
        set -x -e
        ./compiler.x86_64 $cname -s $sname > /dev/null
        set +x
    done
}

ls test*.c | compit
