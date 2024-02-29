#!/bin/bash
#
#  Run simulation via raspictl
#  Compares whole.mod to shadow.cc
#
#  ./pipesim.sh -randmem -printinstr
#      ...or whatever options
#
cd `dirname $0`
exec ../driver/raspictl -sim whole.mod "$@"
