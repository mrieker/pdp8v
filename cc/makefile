
MACH = $(shell uname -m)
GCC  = g++ -std=c++11 -Wall -g

INCS = $(wildcard *.h)

OBJS = \
	allocate.$(MACH).o \
	assemble.$(MACH).o \
	binop.$(MACH).o \
	compiler.$(MACH).o \
	decl.$(MACH).o \
	error.$(MACH).o \
	expr.$(MACH).o \
	generate.$(MACH).o \
	locn.$(MACH).o \
	mach.$(MACH).o \
	numconst.$(MACH).o \
	opcode.$(MACH).o \
	optmachs.$(MACH).o \
	optprims.$(MACH).o \
	parsedecl.$(MACH).o \
	parsefunc.$(MACH).o \
	prim.$(MACH).o \
	scope.$(MACH).o \
	structtype.$(MACH).o \
	stmt.$(MACH).o \
	token.$(MACH).o \
	unop.$(MACH).o \

default: compiler.$(MACH)

clean:
	rm -f *.$(MACH) *.$(MACH).o

compiler.$(MACH): $(OBJS)
	$(GCC) -o $@ $^

%.$(MACH).o: %.cc $(INCS)
	$(GCC) -c -o $@ $<

