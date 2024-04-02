#!/bin/bash
#
#  create/update the <board>circ_nto.v files from the <board>circ.v files
#  no need for rpi board, it does not have any tubes
#
cd `dirname $0`
set -e -x
for mod in acl alu ma pc seq
do
    if [ ${mod}circ_nto.v -ot ${mod}circ.v ]
    then
        rm -f ${mod}circ_nto.v
        java AddNTO < ${mod}circ.v > ${mod}circ_nto.v
    fi
done
