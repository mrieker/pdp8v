MACH = $(shell uname -m)

default: applymap.$(MACH) archive.$(MACH) assemble.$(MACH) link.$(MACH) octtobin.$(MACH) sizes.$(MACH) trace.$(MACH)

applymap.$(MACH): applymap.c
	cc -Wall -g -o applymap.$(MACH) applymap.c

archive.$(MACH): archive.c
	cc -Wall -g -o archive.$(MACH) archive.c

assemble.$(MACH): assemble.c
	cc -Wall -g -o assemble.$(MACH) assemble.c

link.$(MACH): link.c
	cc -Wall -g -o link.$(MACH) link.c

octtobin.$(MACH): octtobin.c
	cc -Wall -g -o octtobin.$(MACH) octtobin.c

sizes.$(MACH): sizes.c
	cc -Wall -g -o sizes.$(MACH) sizes.c

trace.$(MACH): trace.c
	cc -Wall -g -o trace.$(MACH) trace.c
