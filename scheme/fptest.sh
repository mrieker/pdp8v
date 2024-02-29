#!/bin/bash
export switchregister=0010
export iodevtty='|-'
export iodevtty_cps=1000000
export iodevtty_debug=0
exec ../driver/raspictl -nohw -startpc 0200 scheme.oct '(load "fpadd.scm")'
