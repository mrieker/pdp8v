#!/bin/bash
#
#  Run chess program
#
#   $1 = (nothing) : run on tubes
#         -nohwlib : run using cycle-level simulator
#         -csrclib : run using gate-level simulator
#
exec ../driver/raspictl "$@" -startpc 0200 -binloader checkmo.bin
