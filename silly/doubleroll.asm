
; roll AC / PC lights

; ../asm/assemble doubleroll.asm doubleroll.obj > doubleroll.lis
; ../asm/link -o doubleroll.oct doubleroll.obj > doubleroll.map

; ../driver/raspictl doubleroll.oct                on the tubes
; ../driver/guiraspictl -csrclib doubleroll.oct    gui simulator

	ion  = 06001

	clze = 06130	; (RTC) clear the given enable bits
	clsk = 06131	; (RTC) skip if requesting interrupt
	clde = 06132	; (RTC) set the given enable bits
	clab = 06133	; (RTC) set buffer and counter to given value
	clen = 06134	; (RTC) read enable register
	clsa = 06135	; (RTC) read status register and reset it
	clba = 06136	; (RTC) read buffer register
	clca = 06137	; (RTC) read counter into buffer and read buffer


	. = 00000
	.word	.-.
	jmpi	_intserv

	. = 00002
h0003:	hlt
	rar		; 6000 -> 3000
	jmp	h0006

	. = 00005
h0006:	hlt
	rar		; 3000 -> 1400
	jmp	h0014

	. = 00013
h0014:	hlt
	rar		; 1400 -> 0600
	jmp	h0030

	. = 00027
h0030:	hlt
	rar		; 0600 -> 0300
	jmp	h0060

	. = 00057
h0060:	hlt
	rar		; 0300 -> 0140
	jmp	h0140

_intserv: .word	intserv

	. = 00137
h0140:	hlt
	rar		; 0140 -> 0060
	jmpi	.+1
	.word	h0300

	. = 00277
h0300:	hlt
	rar		; 0060 -> 0030
	jmpi	.+1
	.word	h0600

	. = 00577
h0600:	hlt
	rar		; 0030 -> 0014
	jmpi	.+1
	.word	h1400

	. = 01377
h1400:	hlt
	rar		; 0014 -> 0006
	jmpi	.+1
	.word	h3000

	. = 02777
h3000:	hlt
	rar		; 0006 -> 0003
	jmpi	.+1
	.word	h6000

	. = 05777
h6000:	hlt
	tad	_3776	; 0003 -> 4001
	jmpi	.+1
	.word	h4001

_3776:	.word	03776

	. = 04000
h4001:	hlt
	tad	_1777	; 4001 -> 6000
	jmp	h0003

_1777:	.word	01777


	. = 01000
	.global	__boot
__boot:
	cla cma
	clze			; clear all RTC enable bits
	cla
	tad	period		; set RTC buffer and counter register
	clab
	cla
	tad	intena		; set RTC enable bits
	clde
	ion
	cla cll
	tad	_6000		; put 6000 in AC
	jmp	h0003		; go wait for interrupt

intserv:			; interrupt service...
	dca	saveac		; save accumulator
	clsa			; clear clock interrupt request
	cla
	tad	saveac		; restore accumulator
	ion			; return after the hlt
	jmpi	0

_6000:	.word	06000

saveac:	.word	.-.

intena:	.word	05400		;    <11> = interrupt enable
				; <10:09> = 01  : repeat
				; <08:06> = 100 : 100uS ticks
period:	.word	-667		; interrupt every 66700uS


