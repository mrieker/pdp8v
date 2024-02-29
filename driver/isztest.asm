
; test worst-case isz
; ../asm/assemble isztest.asm isztest.obj > isztest.lis
; ../asm/link -o isztest.oct isztest.obj
; ./raspictl -mintimes isztest.oct
; - should keep looping forever

	. = 0
	.global	__boot
__boot:
	cla cll cma
	dca	sevens
	tad	accum
	iac
	dca	accum
	tad	accum
	jmpi	.+1
	.word	07776

sevens:	.word	.-.
accum:	.word	.-.

	. = 07776
	isz	sevens
	jmp	.

