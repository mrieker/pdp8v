MACH = $(shell uname -m)

CC  = ../cc/compile.sh
AR  = ../asm/archive.$(MACH)
ASM = ../asm/assemble.$(MACH)
LNK = ../asm/link.$(MACH)

LIB = library/crtl.hode.a

default: assemble.hex calc.hex circus.hex printpi.hex

assemble.hex: ../asm/assemble.hode.o $(LIB)
	$(LNK) -o assemble.hex ../asm/assemble.hode.o $(LIB) > assemble.map

../asm/assemble.hode.o: ../asm/assemble.c
	$(CC) ../asm/assemble

calc.hex: calc.hode.o $(LIB)
	$(LNK) -o calc.hex calc.hode.o $(LIB) > calc.map

calc.hode.o: calc.c
	$(CC) calc

circus.hex: circus.hode.o $(LIB)
	$(LNK) -o circus.hex circus.hode.o $(LIB) > circus.map

circus.hode.o: circus.c
	$(CC) circus

printpi.hex: printpi.hode.o $(LIB)
	$(LNK) -o printpi.hex printpi.hode.o $(LIB) > printpi.map

printpi.hode.o: printpi.c
	$(CC) printpi

x.hex:	x.hode.o $(LIB)
	$(LNK) -o x.hex x.hode.o $(LIB) > x.map

x.hode.o: x.c
	$(CC) x

sltest.hex: sltest.hode.o $(LIB)
	$(LNK) -o sltest.hex sltest.hode.o $(LIB) > sltest.map

sltest.hode.o: sltest.c
	$(CC) sltest

