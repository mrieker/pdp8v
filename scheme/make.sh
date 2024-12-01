#!/bin/bash
cd `dirname $0`
set -e -x
../asm/assemble.x86_64 scheme.asm scheme.obj > scheme.lis
../asm/link.x86_64 -o scheme.oct scheme.obj > scheme.map
../asm/octtobin.x86_64 < scheme.oct > scheme.bin
