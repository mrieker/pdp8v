#!/bin/bash
#  telnet into this host port 12303 with another screen after this starts
#  optionally run ../driver/dtapestatus on another screen
exec ../../driver/raspictl -csrclib -script d3eb.tcl
