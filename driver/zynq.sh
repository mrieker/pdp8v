#!/bin/bash
#
#  test zynq implementation
#
#  $1 = -mintimes
#       -printinstr
#       -printstate
#
exec ./raspictl -zynqlib -randmem -quiet "$@"
