
default: \
	../kicads/acl/acl.kicad_pcb \
	../kicads/alu/alu.kicad_pcb \
	../kicads/ma/ma.kicad_pcb \
	../kicads/pc/pc.kicad_pcb \
	../kicads/rpi/rpi.kicad_pcb \
	../kicads/seq/seq.kicad_pcb \
	StuckOnsExtract.class

clean:
	rm -f ../kicads/acl/acl.kicad_pcb \
		../kicads/alu/alu.kicad_pcb \
		../kicads/ma/ma.kicad_pcb \
		../kicads/pc/pc.kicad_pcb \
		../kicads/rpi/rpi.kicad_pcb \
		../kicads/seq/seq.kicad_pcb \
		StuckOnsExtract*.class

#
#  Check fanout, get overall parts count
#
fanout: whole.mod
	rm -f proc.rep
	echo EXCEEDS > proc.rep
	../netgen/netgen.sh whole.mod -gen proc -report proc.rep 2>&1 | grep -v 'position 0.0,0.0 collision' | grep -v ' not in place'
	@if ! grep EXCEEDS proc.rep ; then echo ======== FANOUT OK ; fi
	head proc.rep

parts: whole.mod
	rm -f *.tmp
	../netgen/netgen.sh whole.mod -gen acl -report acl.tmp 2>&1 | grep -v 'position 0.0,0.0 collision' | grep -v ' not in place'
	../netgen/netgen.sh whole.mod -gen alu -report alu.tmp 2>&1 | grep -v 'position 0.0,0.0 collision' | grep -v ' not in place'
	../netgen/netgen.sh whole.mod -gen ma -report  ma.tmp  2>&1 | grep -v 'position 0.0,0.0 collision' | grep -v ' not in place'
	../netgen/netgen.sh whole.mod -gen pc -report  pc.tmp  2>&1 | grep -v 'position 0.0,0.0 collision' | grep -v ' not in place'
	../netgen/netgen.sh whole.mod -gen seq -report seq.tmp 2>&1 | grep -v 'position 0.0,0.0 collision' | grep -v ' not in place'
	head -7 *.tmp
	rm -f *.tmp

#
#  Make unrouted PCBs
#
../kicads/acl/acl.kicad_pcb: whole.mod acl.map
	./makepcb.sh acl

../kicads/alu/alu.kicad_pcb: whole.mod alu.map
	./makepcb.sh alu

../kicads/ma/ma.kicad_pcb: whole.mod ma.map
	./makepcb.sh ma

../kicads/pc/pc.kicad_pcb: whole.mod pc.map
	./makepcb.sh pc

../kicads/rpi/rpi.kicad_pcb: whole.mod rpi.map
	./makepcb.sh rpi

../kicads/seq/seq.kicad_pcb: whole.mod seq.map
	./makepcb.sh seq

#
#  make de0 verilog file
#  use alternatives to proc like proc_seq, proc_ma_seq, etc for board subsets (see proc.mod for subsets)
#  de0 needs to be compiled with qvartvs afterward
#
../de0/proc-synth.v: whole.mod
	../netgen/netgen.sh whole.mod -gen proc -verilog ../de0/proc-synth.v

#
#  make C-source file equivalent for use by driver CSrcLib class
#  use alternatives to proc like proc_seq, proc_ma_seq, etc for board subsets (see proc.mod for subsets)
#  testboard or whatever needs to be compiled afterward
#
../driver/csrcstate.cc: whole.mod
	../netgen/netgen.sh whole.mod -gen proc -csource ../driver/csrcstate.cc 2>&1 | grep -v 'not in place'

#
#  create whole.mod from the individual module files
#
whole.mod: acl.mod alu.mod cons.inc ma.mod pc.mod proc.mod rpi.mod seq.mod $(wildcard testpads_*.mod)
	rm -f whole.c whole.mod
	cat acl.mod alu.mod ma.mod pc.mod rpi.mod seq.mod proc.mod $(wildcard testpads_*.mod) > whole.c
	gcc -E -o whole.mod whole.c
	rm -f whole.c

#
# Compile StuckOnExtract class
#
StuckOnsExtract.class: StuckOnsExtract.java
	rm -f StuckOnsExtract*.class
	CLASSPATH=. javac StuckOnsExtract.java

