#!/bin/bash
exec ../driver/raspictl "$@" -script test.tcl '(load "test.scm")'
