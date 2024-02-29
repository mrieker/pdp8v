#!/bin/bash
# test tco by setting $1 = odd and even numbers
# takes about a minute to run with $1 = 150
export switchregister=0000
export iodevtty='|-'
export iodevtty_cps=1000
export iodevtty_debug=0
exec ../driver/raspictl -csrclib -startpc 0200 scheme.oct \
    "(define is-even? (lambda (n) (or  (=  n) (is-odd?  (- n 1)))))
     (define is-odd?  (lambda (n) (and (<> n) (is-even? (- n 1)))))
     \"is $1 even?\" (is-even? $1)
     \"is $1 odd?\"  (is-odd? $1)"
