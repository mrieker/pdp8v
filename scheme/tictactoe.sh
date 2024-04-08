#!/bin/bash
cd `dirname $0`
exec ../driver/raspictl "$@" -script tictactoe.tcl
