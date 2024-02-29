#!/bin/bash -e
mydir=`dirname $0`
uname=`uname -m`
incdir=$mydir/../crtl/include
first=$1
shift
set -x
gcc -nostdlib -D__HODE__ -E -I$incdir $first.c "$@" > $first.hode.i
$mydir/compiler.$uname $first.hode.i -s $first.hode.s > /dev/null
$mydir/../asm/assemble.$uname $first.hode.s $first.hode.o > $first.lis
