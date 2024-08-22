#!/bin/bash
#
#  extended arithmetic
#  multiply/divide test
#  should halt at 0201 first then 0251 if good
#  just repeats halting at 0251 if good
#    takes only a couple seconds to run
#  prints detailed error if failure
#
cd `dirname $0`
exec ../driver/raspictl "$@" -script d0mb.tcl
