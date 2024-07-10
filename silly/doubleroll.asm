
; roll AC / PC lights
; reverse direction every SR'th steps

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
	jmp	intserv

	. = 00002
h0003:	hlt
	jmp	finish3

	. = 00005
h0006:	hlt
	jmsi	_step		; PC=0006; AC=3000
	.word	h0014,01400	; up -> 0014; 1400
	.word	h0003,06000	; dn -> 0003; 6000
	.word	h0006,03000	; hold

	. = 00013
h0014:	hlt
	jmsi	_step		; PC=0014; AC=1400
	.word	h0030,00600
	.word	h0006,03000
	.word	h0014,01400

	. = 00027
h0030:	hlt
	jmsi	_step		; PC=0030; AC=0600
	.word	h0060,00300
	.word	h0014,01400
	.word	h0030,00600

	. = 00057
h0060:	hlt
	jmsi	_step		; PC=0060; AC=0300
	.word	h0140,00140
	.word	h0030,00600
	.word	h0060,00300

; reset counter for next interrupt
intserv:			; interrupt service...
	dca	saveac		; save accumulator
	clsa			; clear clock interrupt request
	cla
	tad	saveac		; restore accumulator
	ion			; return after the hlt
	jmpi	0

saveac:	.word	.-.

	. = 00137
h0140:	hlt
	jmsi	_step		; PC=0140; AC=0140
	.word	h0300,00060
	.word	h0060,00300
	.word	h0140,00140

finish3:
	jmsi	_step		; PC=0003; AC=6000
	.word	h0006,03000
	.word	h4001,04001
	.word	h0003,06000

_step:	.word	step

	. = 00277
h0300:	hlt
	jmsi	_step		; PC=0300; AC=0060
	.word	h0600,00030
	.word	h0140,00140
	.word	h0300,00060

	. = 00577
h0600:	hlt
	jmsi	_step		; PC=0600; AC=0030
	.word	h1400,00014
	.word	h0300,00060
	.word	h0600,00030

	. = 01377
h1400:	hlt
	jmsi	_step		; PC=1400; AC=0014
	.word	h3000,00006
	.word	h0600,00030

	. = 02777
h3000:	hlt
	jmsi	_step		; PC=3000; AC=0006
	.word	h6000,00003
	.word	h1400,00014
	.word	h3000,00006

	. = 05777
h6000:	hlt
	jmsi	_step		; PC=6000; AC=0003
	.word	h4001,04001
	.word	h3000,00006
	.word	h6000,00003

	. = 04000
h4001:	hlt
	jmsi	_step		; PC=4001; AC=4001
	.word	h0003,06000
	.word	h6000,00003
	.word	h4001,04001

	. = 02000
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
	tad	_4001		; put 4001 in AC
	jmpi	_4001		; put 4001 in PC

_4001:	.word	04001

intena:	.word	05400		;    <11> = interrupt enable
				; <10:09> = 01  : repeat
				; <08:06> = 100 : 100uS ticks
period:	.word	-667		; interrupt every 66700uS

; step PC and AC
;  jmsi   _step
;  .word  pcup,acup
;  .word  pcdn,acdn
;  .word  pchl,achl
step:	.word	.-.
	cla osr			; read switches
	cll cma iac		; ...negative
	sna
	jmp	stephold	; - zero: hold in place
	tad	count		; compute count-switches
	snl cla
	jmp	stepinc
	tad	direc		; count >= SR, flip direction
	cma
	dca	direc
	dca	count		; reset count
stepinc:
	isz	count		; count < SR, increment
	tad	count		; display count in pidp's MQ leds
	mql			; AC -> MQ; 0 -> AC
	tad	direc
	and	_0021
	iac
	and	_0021
	cma
	dca	.+2
	scl
	.word	.-.
	tad	direc		; get direction
	sma cla
	jmp	stepup		; pos - pc goes up
stepdn:
	isz	step		; neg - pc goes down
	isz	step
stepup:
	tadi	step
	dca	steppc
	isz	step
	tadi	step
	jmpi	steppc
stephold:
	isz	step		; SR=0: hold in place
	isz	step
	jmp	stepdn

count:	.word	.-.
direc:	.word	.-.

steppc:	.word	.-.

_0021:	.word	00021

