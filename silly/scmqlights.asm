
; shift '1' right-to-left through LINK:SC:MQ combination to test PiDP panel lights

; ../asm/assemble scmqlights.asm scmqlights.obj > scmqlights.lis
; ../asm/link -o scmqlights.oct scmqlights.obj > scmqlights.map

; ../driver/raspictl -pidp -nohwlib scmqlights.oct

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
	.word	.-.		; interrupt service...
	dca	saveac		; save accumulator
	clsa			; clear clock interrupt request
	cla
	tad	saveac		; restore accumulator
	ion			; return after the hlt
	jmpi	0

saveac:	.word	.-.

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
	cla cll iac
	dca	mqval
	cma
	dca	scval

loop:
	cla
	tad	mqval
	mql			; AC -> MQ; 0 -> AC
	scl			; ~scval<04:00> -> SC
scval:	.word	.-.

	hlt

	tad	mqval		; ral ~scval:mqval:link
	ral
	dca	mqval
	tad	scval
	cml ral
	dca	scval
	tad	scval
	cma bsw
	ral

	jmp	loop

intena:	.word	05300		;    <11> = interrupt enable
				; <10:09> = 01  : repeat
				; <08:06> = 010 : mS ticks
period:	.word	-1000		; interrupt every 1000mS

mqval:	.word	.-.

