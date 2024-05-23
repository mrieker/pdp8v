CFLAGS ?= -O2 -Werror
MACH   := $(shell uname -m)
TCLINC := -I/usr/include/tcl8.6
GPP    := g++ $(CFLAGS) $(TCLINC) -std=c++11 -g -Wall -fPIC
ASM    := ../asm/assemble.$(MACH)
LNK    := ../asm/link.$(MACH)
LNKFLG := -ldl -lm -lpthread -lreadline -lrt -ltcl
RDCYC  := rdcyc-$(MACH).cc

ifeq ($(MACH),armv6l)
	UNIPROC := 1
else
	UNIPROC := 0
endif

ifeq ($(MACH),armv7l)
	LNKFLG := $(LNKFLG) -latomic
endif

LIBS = lib.$(MACH).a

.PRECIOUS: csrcmod_%.cc csrcmod_%.so.$(MACH)

default: autoboardtest.$(MACH) dtapestatus.$(MACH) gpioudpserver.$(MACH) \
	raspictl.$(MACH) tclboardtest.$(MACH) testboard.$(MACH) zynqtest.$(MACH) \
	GUI.jar libGUIRasPiCtl.$(MACH).so

lib.$(MACH).a: \
		abcd.$(MACH).o \
		binloader.$(MACH).o \
		controls.$(MACH).o \
		csrclib.$(MACH).o \
		disassemble.$(MACH).o \
		dyndis.$(MACH).o \
		extarith.$(MACH).o \
		getexedir.$(MACH).o \
		gpiolib.$(MACH).o \
		iodevs.$(MACH).o \
		iodevdtape.$(MACH).o \
		iodevptape.$(MACH).o \
		iodevrk8.$(MACH).o \
		iodevrk8je.$(MACH).o \
		iodevrtc.$(MACH).o \
		iodevtty.$(MACH).o \
		linc.$(MACH).o \
		linkloader.$(MACH).o \
		makepipelibmod.$(MACH).o \
		memext.$(MACH).o \
		memory.$(MACH).o \
		myiowkit.$(MACH).o \
		nohwlib.$(MACH).o \
		physlib.$(MACH).o \
		pindefs.$(MACH).o \
		pipelib.$(MACH).o \
		rdcyc.$(MACH).o \
		readprompt.$(MACH).o \
		rimloader.$(MACH).o \
		scncall.$(MACH).o \
		script.$(MACH).o \
		shadow.$(MACH).o \
		strprintf.$(MACH).o \
		timedlib.$(MACH).o \
		zynqlib.$(MACH).o
	rm -f lib.$(MACH).a
	ar rc $@ $^

clockit.$(MACH): clockit.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^

counter.$(MACH): counter.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^

exptest.$(MACH): exptest.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^

exptimingall.$(MACH): exptimingall.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^

exptimingdfrm.$(MACH): exptimingdfrm.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^

exptimingiak.$(MACH): exptimingiak.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^

exptimingioin.$(MACH): exptimingioin.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^

exptimingjmp.$(MACH): exptimingjmp.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^

exptimingmd0.$(MACH): exptimingmd0.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^

exptimingmd1.$(MACH): exptimingmd1.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^

exptimingmwt.$(MACH): exptimingmwt.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^

GUI.jar: GUI.java GUIRasPiCtl.h
	rm -f GUI.class GUI\$$*.class GUI.jar
	CLASSPATH=. javac GUI.java
	jar cf GUI.jar GUI*.class
	rm -f GUI.class GUI\$$*.class

iow56list.$(MACH): iow56list.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^

iow56testpair.$(MACH): iow56testpair.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^ -lpthread -lreadline

iowlist.$(MACH): iowlist.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^

iow100test.$(MACH): iow100test.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^

libGUIRasPiCtl.$(MACH).so: GUIRasPiCtl.cc GUIRasPiCtl.h raspictl.$(MACH).o $(LIBS)
	$(GPP) -shared -o libGUIRasPiCtl.$(MACH).so -I/opt/jdk/include -I/opt/jdk/include/linux GUIRasPiCtl.cc raspictl.$(MACH).o $(LIBS) $(LNKFLG)

GUIRasPiCtl.h: GUIRasPiCtl.java
	rm -f GUIRasPiCtl.class GUIRasPiCtl.h
	CLASSPATH=. javac -h . GUIRasPiCtl.java

autoboardtest.$(MACH): autoboardtest.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^ $(LNKFLG)

dtapestatus.$(MACH): dtapestatus.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^ $(LNKFLG)

gpioudpserver.$(MACH): gpioudpserver.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^ $(LNKFLG)

raspictl.$(MACH): raspictl.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^ $(LNKFLG)

tclboardtest.$(MACH): tclboardtest.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^ $(LNKFLG)

testboard.$(MACH): testboard.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^ $(LNKFLG)

zynqtest.$(MACH): zynqtest.$(MACH).o $(LIBS)
	$(GPP) -o $@ $^ $(LNKFLG)

csrcmod_%.so.$(MACH): csrcmod_%.cc csrcmod.cc
	$(GPP) -shared -o $@ $^

csrcmod_%.cc: ../modules/whole.mod ../netgen/classes/NetGen.class
	../netgen/netgen.sh ../modules/whole.mod -gen $* -csource $@

../netgen/classes/NetGen.class:
	cd ../netgen ; make

../modules/whole.mod:
	cd ../modules ; make whole.mod

rdcyc.$(MACH).o: $(RDCYC) *.h
	$(GPP) -DUNIPROC=$(UNIPROC) -c -o $@ $<

%.$(MACH).o: %.cc *.h
	$(GPP) -DUNIPROC=$(UNIPROC) -c -o $@ $<

