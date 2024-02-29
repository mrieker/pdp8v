#!/bin/bash
#
#  feed random opcodes and operands to DE0 for testing
#
cd `dirname $0`
cpuhz=52000 exec ./run_de0.sh -mintimes -quiet -randmem
