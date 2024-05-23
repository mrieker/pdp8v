MACH   = $(shell uname -m)
GPP    = g++ -g -O2 -std=c++11 -Wall
ASM    = ../asm/assemble.$(MACH)
LNK    = ../asm/link.$(MACH)

ifeq ($(MACH),armv6l)
	HASTSC = 0
	UNIPROC = 1
else
	HASTSC = 1
	UNIPROC = 0
endif

OBJS = \
	dumpfile.$(MACH).o \
	filesys.$(MACH).o \
	fsuck.$(MACH).o \

default: dumpdisk.$(MACH) initvol.$(MACH) listdisk.$(MACH) readdisk.$(MACH) test.$(MACH) test2.$(MACH) writedisk.$(MACH) nullboot.obj

dumpdisk.$(MACH): dumpdisk.$(MACH).o $(OBJS)
	$(GPP) -o $@ $^

initvol.$(MACH): initvol.$(MACH).o $(OBJS)
	$(GPP) -o $@ $^

listdisk.$(MACH): listdisk.$(MACH).o $(OBJS)
	$(GPP) -o $@ $^

readdisk.$(MACH): readdisk.$(MACH).o $(OBJS)
	$(GPP) -o $@ $^

test.$(MACH): test.$(MACH).o $(OBJS)
	$(GPP) -o $@ $^

test2.$(MACH): test2.$(MACH).o $(OBJS)
	$(GPP) -o $@ $^

writedisk.$(MACH): writedisk.$(MACH).o $(OBJS)
	$(GPP) -o $@ $^

nullboot.obj: nullboot.asm
	../asm/assemble.x86_64 nullboot.asm nullboot.obj > nullboot.lis

%.$(MACH).o: %.cc *.h
	$(GPP) -DUNIPROC=$(UNIPROC) -c -o $@ $<

