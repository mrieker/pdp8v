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
	dumpfile.o.$(MACH) \
	filesys.o.$(MACH) \
	fsuck.o.$(MACH) \

default: dumpdisk.$(MACH) initvol.$(MACH) listdisk.$(MACH) readdisk.$(MACH) test.$(MACH) test2.$(MACH) writedisk.$(MACH) nullboot.obj

dumpdisk.$(MACH): dumpdisk.o.$(MACH) $(OBJS)
	$(GPP) -o $@ $^

initvol.$(MACH): initvol.o.$(MACH) $(OBJS)
	$(GPP) -o $@ $^

listdisk.$(MACH): listdisk.o.$(MACH) $(OBJS)
	$(GPP) -o $@ $^

readdisk.$(MACH): readdisk.o.$(MACH) $(OBJS)
	$(GPP) -o $@ $^

test.$(MACH): test.o.$(MACH) $(OBJS)
	$(GPP) -o $@ $^

test2.$(MACH): test2.o.$(MACH) $(OBJS)
	$(GPP) -o $@ $^

writedisk.$(MACH): writedisk.o.$(MACH) $(OBJS)
	$(GPP) -o $@ $^

nullboot.obj: nullboot.asm
	../asm/assemble.x86_64 nullboot.asm nullboot.obj > nullboot.lis

%.o.$(MACH): %.cc *.h
	$(GPP) -DUNIPROC=$(UNIPROC) -c -o $@ $<

