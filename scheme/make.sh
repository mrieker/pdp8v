#!/bin/bash
set -e -x
../asm/assemble.x86_64 scheme.asm scheme.obj > scheme.lis
../asm/link.x86_64 -o scheme.oct scheme.obj > scheme.map
