MACH = $(shell uname -m)

CC  = ../../cc/compile.sh
AR  = ../../asm/archive.$(MACH)
ASM = ../../asm/assemble.$(MACH)
LNK = ../../asm/link.$(MACH)

LIBA = \
	__memcpy.hode.o \
	__memcpy_4w.hode.o \
	__memcpy_4w_rev.hode.o \
	__memcpy_w.hode.o \
	__memcpy_ww.hode.o \
	and_QQQ.hode.o \
	boot.hode.o \
	close.hode.o \
	cmp_QQ.hode.o \
	com_QQ.hode.o \
	div_LLL.hode.o \
	div_LLW.hode.o \
	div_QQL.hode.o \
	div_QQQ.hode.o \
	div_QQW.hode.o \
	div_www.hode.o \
	div_wwW.hode.o \
	div_WWW.hode.o \
	endtags.hode.o \
	exit.hode.o \
	fabs.hode.o \
	fabsf.hode.o \
	fpnativ.hode.o \
	fpraspi.hode.o \
	frexp.hode.o \
	frexpf.hode.o \
	getcmdarg.hode.o \
	getenv.hode.o \
	getnowns.hode.o \
	isfinite.hode.o \
	isfinitef.hode.o \
	isinf.hode.o \
	isinff.hode.o \
	isnan.hode.o \
	isnanf.hode.o \
	ldexp.hode.o \
	ldexpf.hode.o \
	memchr.hode.o \
	memcmp.hode.o \
	memcpy.hode.o \
	memmove.hode.o \
	memset.hode.o \
	mod_WQW.hode.o \
	mod_www.hode.o \
	mod_wwW.hode.o \
	mod_WWW.hode.o \
	mul_LLL.hode.o \
	mul_llw.hode.o \
	mul_LLW.hode.o \
	mul_LWL.hode.o \
	mul_www.hode.o \
	mul_wwW.hode.o \
	mul_WWW.hode.o \
	neg_QQ.hode.o \
	open.hode.o \
	printinstr.hode.o \
	rdps.hode.o \
	read.hode.o \
	readline.hode.o \
	setrosize.hode.o \
	setstklim.hode.o \
	setttyecho.hode.o \
	shl_LLW.hode.o \
	shl_QQW.hode.o \
	shl_WWW.hode.o \
	shr_LLW.hode.o \
	shr_QQW.hode.o \
	shr_wwW.hode.o \
	shr_WWW.hode.o \
	strcasecmp.hode.o \
	strchr.hode.o \
	strcmp.hode.o \
	strcpy.hode.o \
	strlen.hode.o \
	strncasecmp.hode.o \
	sub_QQQ.hode.o \
	throw.hode.o \
	udiv.hode.o \
	umul.hode.o \
	unlink.hode.o \
	watchwrite.hode.o \
	write.hode.o \
	wrps.hode.o \

LIBC = \
	abort.hode.o \
	asprintf.hode.o \
	assertfail.hode.o \
	ass_putbuf.hode.o \
	ass_putchr.hode.o \
	ass_putdbl.hode.o \
	ass_putflt.hode.o \
	ass_putint.hode.o \
	ass_putstr.hode.o \
	ass_putuint.hode.o \
	ass_putuinthex.hode.o \
	complexd.hode.o \
	complexf.hode.o \
	dprintf.hode.o \
	fdfile.hode.o \
	fprintf.hode.o \
	gmtime.hode.o \
	malloc.hode.o \
	$(patsubst math_%.c,mathd%.hode.o,$(wildcard math_*.c)) \
	$(patsubst math_%.c,mathf%.hode.o,$(wildcard math_*.c)) \
	qsort.hode.o \
	sprintf.hode.o \
	start.hode.o \
	stdio.hode.o \
	strdup.hode.o \
	strerror.hode.o \
	strtod.hode.o \
	strtof.hode.o \
	strtol.hode.o \
	strtoul.hode.o \
	time.hode.o \
	vfprintf.hode.o \
	vfprintf_fp.hode.o \

crtl.hode.a: $(LIBA) $(LIBC)
	$(AR) -o $@ $^

complex%.hode.o: complex%.c complex_.c
	$(CC) $(subst .c,,$<)

mathd%.hode.o: mathd.c math_%.c
	cat $^ > $(patsubst %.hode.o,%.c,$@)
	$(CC) $(patsubst %.hode.o,%,$@)
	rm $(patsubst %.hode.o,%.c,$@)

mathf%.hode.o: mathf.c math_%.c
	cat $^ > $(patsubst %.hode.o,%.c,$@)
	$(CC) $(patsubst %.hode.o,%,$@)
	rm $(patsubst %.hode.o,%.c,$@)

%.hode.o: %.asm
	$(ASM) $< $@ > $(patsubst %.asm,%.lis,$<)

%.hode.o: %.c
	$(CC) $(patsubst %.c,%,$<)

