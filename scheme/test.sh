#!/bin/bash
exec ../driver/raspictl -nohwlib -script test.tcl '(load "test.scm")'
