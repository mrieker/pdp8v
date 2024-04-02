MACH = $(shell uname -m)

default: applymap.$(MACH) archive.$(MACH) assemble.$(MACH) link.$(MACH) sizes.$(MACH) trace.$(MACH)

applymap.$(MACH): applymap.c
	cc -Wall -g -o applymap.$(MACH) applymap.c

archive.$(MACH): archive.c
	cc -Wall -g -o archive.$(MACH) archive.c

assemble.$(MACH): assemble.c
	cc -Wall -g -o assemble.$(MACH) assemble.c

link.$(MACH): link.c
	cc -Wall -g -o link.$(MACH) link.c

sizes.$(MACH): sizes.c
	cc -Wall -g -o sizes.$(MACH) sizes.c

trace.$(MACH): trace.c
	cc -Wall -g -o trace.$(MACH) trace.c
