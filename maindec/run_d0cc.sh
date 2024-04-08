#!/bin/bash
#
#  add instruction test
#  runs indefinitely
#
#  with -nohwlib (15,000,000 cps), types SIMAD SIMROT FCT RANDOM
#       on teletype after about 5 minutes (4,000,000,000 cycles)
#
#  with -zynqlib  SIMAD  after  3 mins
#                 SIMROT after 25 mins
#                 FCT    after 29 mins
#                 RANDOM after 40 mins
#         and **** EXTENDED BANKS message at 40 mins
#
# p52/v1-43
#  field checking code
#
exec ../driver/raspictl "$@" -script d0cc.tcl
