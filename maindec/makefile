MACH   = $(shell uname -m)
GPP    = g++ -O2 -g -Wall
ASM    = ../asm/assemble.$(MACH)
LNK    = ../asm/link.$(MACH)

ifeq ($(MACH),armv6l)
	HASTSC = 0
	UNIPROC = 1
else
	HASTSC = 1
	UNIPROC = 0
endif

default: bindump.$(MACH) last1000.$(MACH)

bindump.$(MACH): bindump.cc ../driver/disassemble.o.$(MACH)
	$(GPP) -o $@ $^

last1000.$(MACH): last1000.c
	$(GPP) -o $@ $^

