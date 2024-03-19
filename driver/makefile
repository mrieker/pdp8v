CFLAGS ?= -O2 -Werror
MACH   := $(shell uname -m)
TCLINC := -I/usr/include/tcl8.6
GPP    := g++ $(CFLAGS) $(TCLINC) -std=c++11 -g -Wall -fPIC
ASM    := ../asm/assemble.$(MACH)
LNK    := ../asm/link.$(MACH)
LNKFLG := -ldl -lm -lpthread -lreadline -ltcl
RDCYC  := rdcyc-$(MACH).cc

ifeq ($(MACH),armv6l)
	UNIPROC := 1
else
	UNIPROC := 0
endif

ifeq ($(MACH),armv7l)
	LNKFLG := $(LNKFLG) -latomic
endif

LIBS = lib.a.$(MACH)

.PRECIOUS: csrcmod_%.cc csrcmod_%.so.$(MACH)

default: autoboardtest.$(MACH) gpioudpserver.$(MACH) loopedboardtest.$(MACH) \
	manualboardtest.$(MACH) raspictl.$(MACH) testboard.$(MACH) zynqtest.$(MACH) \
	GUI.jar libGUIRasPiCtl.$(MACH).so

lib.a.$(MACH): \
		abcd.o.$(MACH) \
		binloader.o.$(MACH) \
		controls.o.$(MACH) \
		csrclib.o.$(MACH) \
		disassemble.o.$(MACH) \
		extarith.o.$(MACH) \
		getexedir.o.$(MACH) \
		gpiolib.o.$(MACH) \
		iodevs.o.$(MACH) \
		iodevptape.o.$(MACH) \
		iodevrk8.o.$(MACH) \
		iodevrk8je.o.$(MACH) \
		iodevrtc.o.$(MACH) \
		iodevtty.o.$(MACH) \
		linc.o.$(MACH) \
		linkloader.o.$(MACH) \
		makepipelibmod.o.$(MACH) \
		memext.o.$(MACH) \
		memory.o.$(MACH) \
		myiowkit.o.$(MACH) \
		nohwlib.o.$(MACH) \
		physlib.o.$(MACH) \
		pindefs.o.$(MACH) \
		pipelib.o.$(MACH) \
		rdcyc.o.$(MACH) \
		readprompt.o.$(MACH) \
		rimloader.o.$(MACH) \
		scncall.o.$(MACH) \
		script.o.$(MACH) \
		shadow.o.$(MACH) \
		strprintf.o.$(MACH) \
		timedlib.o.$(MACH) \
		zynqlib.o.$(MACH)
	rm -f lib.a.$(MACH)
	ar rc $@ $^

clockit.$(MACH): clockit.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^

counter.$(MACH): counter.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^

exptest.$(MACH): exptest.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^

exptimingall.$(MACH): exptimingall.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^

exptimingdfrm.$(MACH): exptimingdfrm.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^

exptimingiak.$(MACH): exptimingiak.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^

exptimingioin.$(MACH): exptimingioin.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^

exptimingjmp.$(MACH): exptimingjmp.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^

exptimingmd0.$(MACH): exptimingmd0.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^

exptimingmd1.$(MACH): exptimingmd1.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^

exptimingmwt.$(MACH): exptimingmwt.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^

GUI.jar: GUI.java GUIRasPiCtl.h
	rm -f GUI.class GUI\$$*.class GUI.jar
	CLASSPATH=. javac GUI.java
	jar cf GUI.jar GUI*.class
	rm -f GUI.class GUI\$$*.class

iow56list.$(MACH): iow56list.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^

iow56testpair.$(MACH): iow56testpair.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^ -lpthread -lreadline

iowlist.$(MACH): iowlist.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^

iow100test.$(MACH): iow100test.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^

libGUIRasPiCtl.$(MACH).so: GUIRasPiCtl.cc GUIRasPiCtl.h raspictl.o.$(MACH) $(LIBS)
	$(GPP) -shared -o libGUIRasPiCtl.$(MACH).so -I/opt/jdk/include -I/opt/jdk/include/linux GUIRasPiCtl.cc raspictl.o.$(MACH) $(LIBS) $(LNKFLG)

GUIRasPiCtl.h: GUIRasPiCtl.java
	rm -f GUIRasPiCtl.class GUIRasPiCtl.h
	CLASSPATH=. javac -h . GUIRasPiCtl.java

autoboardtest.$(MACH): autoboardtest.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^ $(LNKFLG)

gpioudpserver.$(MACH): gpioudpserver.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^ $(LNKFLG)

loopedboardtest.$(MACH): loopedboardtest.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^ $(LNKFLG)

manualboardtest.$(MACH): manualboardtest.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^ $(LNKFLG)

maseqcountdn.$(MACH): maseqcountdn.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^ $(LNKFLG)

pc7676.$(MACH): pc7676.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^ $(LNKFLG)

raspictl.$(MACH): raspictl.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^ $(LNKFLG)

testboard.$(MACH): testboard.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^ $(LNKFLG)

zynqtest.$(MACH): zynqtest.o.$(MACH) $(LIBS)
	$(GPP) -o $@ $^ $(LNKFLG)

csrcmod_%.so.$(MACH): csrcmod_%.cc csrcmod.cc
	$(GPP) -shared -o $@ $^

csrcmod_%.cc: ../modules/whole.mod ../netgen/classes/NetGen.class
	../netgen/netgen.sh ../modules/whole.mod -gen $* -csource $@

../netgen/classes/NetGen.class:
	cd ../netgen ; make

../modules/whole.mod:
	cd ../modules ; make whole.mod

rdcyc.o.$(MACH): $(RDCYC) *.h
	$(GPP) -DUNIPROC=$(UNIPROC) -c -o $@ $<

%.o.$(MACH): %.cc *.h
	$(GPP) -DUNIPROC=$(UNIPROC) -c -o $@ $<

