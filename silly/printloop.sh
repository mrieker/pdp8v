#!/bin/bash
cd `dirname $0`
../driver/raspictl "$@" -mintimes -script printloop.tcl '(load "printloop.scm")'
