#!/bin/bash
set -e -x

unamem=`uname -m`
cc -std=c99 -o trigtest.$unamem trigtest.c -lm
./trigtest.$unamem | sed 's/-nan/ nan/g' > tt.x86

rm -f x.c x.hode.o x.hode.s
ln -s trigtest.c x.c
make x.hex
./runit.sh x.hex tt.x86 > tt.hode
grep ERROR tt.hode
