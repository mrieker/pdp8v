MACH = $(shell uname -m)

default: compare.$(MACH) focal12.oct focal12.ecd myfocal.oct

compare.$(MACH): compare.c
	cc -Wall -g -o compare.$(MACH) compare.c

errors.$(MACH): errors.c
	cc -Wall -g -o errors.$(MACH) errors.c

focal12.ecd: errors.$(MACH) focal12.lis
	./errors.$(MACH) < focal12.lis > focal12.ecd

focal12.oct: focal12.obj
	../../asm/link.x86_64 -o focal12.oct focal12.obj > /dev/null

focal12.obj: focal12.pal
	../../asm/assemble.x86_64 -pal focal12.pal focal12.obj > focal12.lis

myfocal.oct: myfocal.obj
	../../asm/link.x86_64 -o myfocal.oct myfocal.obj > /dev/null

myfocal.obj: myfocal.pal
	../../asm/assemble.x86_64 -pal myfocal.pal myfocal.obj > myfocal.lis

