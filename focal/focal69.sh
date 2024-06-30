#!/bin/bash
#
#  run focal-69 interpreter
#  initial prompt is a single '*'
#
#  ./focal69.sh                              - run on tubes
#  ./focal69.sh [-csrclib/-nohwlib/-zynqlib] - run on various simulators
#
#   *1.10 ASK "HOW MUCH BORROW? ",PRIN
#   *1.20 ASK "HOW MANY YEARS? ",TERM
#   *1.30 FOR RATE=4.0,.5,10;DO 2.0
#   *1.40 QUIT
#   *2.10 SET INT=PRIN*(RATE/100)*TERM
#   *2.20 TYPE "RATE",RATE,"  ","INTEREST",INT,!
#   *GO
#   HOW MUCH BORROW? :1000
#   HOW MANY YEARS? :5
#   RATE=    4.0000  INTEREST=  200.0000
#   RATE=    4.5000  INTEREST=  225.0000
#   RATE=    5.0000  INTEREST=  250.0000
#   RATE=    5.5000  INTEREST=  275.0000
#   RATE=    6.0000  INTEREST=  300.0000
#   RATE=    6.5000  INTEREST=  325.0000
#   RATE=    7.0000  INTEREST=  350.0000
#   RATE=    7.5000  INTEREST=  375.0000
#   RATE=    8.0000  INTEREST=  400.0000
#   RATE=    8.5000  INTEREST=  425.0000
#   RATE=    9.0000  INTEREST=  450.0000
#   RATE=    9.5000  INTEREST=  475.0000
#   RATE=   10.0000  INTEREST=  500.0000
#   *
#
exec ../driver/raspictl "$@" -script focal69.tcl
