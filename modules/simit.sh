#!/bin/bash
#
#  Run simulation
#  Compares whole.mod to proc.sim
#
cd `dirname $0`
date
make whole.mod
../netgen/netgen.sh whole.mod -sim proc.sim
date
