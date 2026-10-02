
; roll AC / PC lights
; reverse direction every SR'th steps

; ../asm/assemble doubleroll.asm doubleroll.obj > doubleroll.lis
; ../asm/link -o doubleroll.oct doubleroll.obj > doubleroll.map

; ../driver/raspictl doubleroll.oct                on the tubes
; ../driver/guiraspictl -csrclib doubleroll.oct    gui simulator

	ion  = 06001

	cdf0 = 06201
	cdf1 = 06211
	cif0 = 06202
	rdf  = 06214
	rif  = 06224

	clze = 06130	; (RTC) clear the given enable bits
	clsk = 06131	; (RTC) skip if requesting interrupt
	clde = 06132	; (RTC) set the given enable bits
	clab = 06133	; (RTC) set buffer and counter to given value
	clen = 06134	; (RTC) read enable register
	clsa = 06135	; (RTC) read status register and reset it
	clba = 06136	; (RTC) read buffer register
	clca = 06137	; (RTC) read counter into buffer and read buffer

	. = 00002
h0003:	clsk
	jmp	.-1
	jmp	finish3

	. = 00005
h0006:	clsk
	jmp	.-1
	jmsi	_step		; PC=0006; AC=3000
	.word	h0014,01400	; up -> 0014; 1400
	.word	h0003,06000	; dn -> 0003; 6000
	.word	h0006,03000	; hold

	. = 00013
h0014:	clsk
	jmp	.-1
	jmsi	_step		; PC=0014; AC=1400
	.word	h0030,00600
	.word	h0006,03000
	.word	h0014,01400

	. = 00027
h0030:	clsk
	jmp	.-1
	jmsi	_step		; PC=0030; AC=0600
	.word	h0060,00300
	.word	h0014,01400
	.word	h0030,00600

	. = 00057
h0060:	clsk
	jmp	.-1
	jmsi	_step		; PC=0060; AC=0300
	.word	h0140,00140
	.word	h0030,00600
	.word	h0060,00300

	. = 00137
h0140:	clsk
	jmp	.-1
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
h0300:	clsk
	jmp	.-1
	jmsi	_step		; PC=0300; AC=0060
	.word	h0600,00030
	.word	h0140,00140
	.word	h0300,00060

	. = 00577
h0600:	clsk
	jmpi	_h0600
	jmsi	_step		; PC=0600; AC=0030
	.word	h1400,00014
	.word	h0300,00060
_h0600:	.word	h0600,00030

	. = 01377
h1400:	clsk
	jmpi	_h1400
	jmsi	_step		; PC=1400; AC=0014
	.word	h3000,00006
	.word	h0600,00030
_h1400:	.word	h1400,00014

	. = 02777
h3000:	clsk
	jmpi	_h3000
	jmsi	_step		; PC=3000; AC=0006
	.word	h6000,00003
	.word	h1400,00014
_h3000:	.word	h3000,00006

	. = 05777
h6000:	clsk
	jmpi	_h6000
	jmsi	_step		; PC=6000; AC=0003
	.word	h4001,04001
	.word	h3000,00006
_h6000:	.word	h6000,00003

	. = 04000
h4001:	clsk
	jmp	.-1
	jmsi	_step		; PC=4001; AC=4001
	.word	h0003,06000
	.word	h6000,00003
	.word	h4001,04001

	. = 02000
	.global	__boot
__boot:

	; copy whole program to fields 1..7
loopfield:
	tadi	copyaddr	; read word from source field
dstcdf:	cdf1			; change to dest field
	dcai	copyaddr	; write word to dest field
	cdf0			; back to source field
	isz	copyaddr	; on to next word
	jmp	loopfield
	tad	dstcdf		; on to next data field
	tad	_0010
	dca	dstcdf
	tad	dstcdf
	and	_0100
	sna cla
	jmp	loopfield

	cla cma
	clze			; clear all RTC enable bits
	cla
	tad	period		; set RTC buffer and counter register
	clab
	cla
	tad	intena		; set RTC enable bits
	clde
	cla cll
	tad	_4001		; put 4001 in AC
	jmpi	_4001		; put 4001 in PC

_0010:	.word	00010
_0100:	.word	00100
_4001:	.word	04001

copyaddr: .word	.-.
intena:	.word	05400		;    <11> = interrupt enable
				; <10:09> = 01  : repeat
				; <08:06> = 100 : 100uS ticks
period:	.word	-667		; interrupt every 66700uS

; step PC and AC
;  jmsi   _step
;  .word  pcleft,acright
;  .word  pcright,acleft
;  .word  pchold,achold

step:	.word	.-.
	clsa			; clear clock interrupt request

	cla cll			; save data & instr fields
	rif
	rtr
	rar
	rdf
	dca	olddfif

	tad	_6201		; set DF = IF
	rif
	dca	.+1
	cif0

	cla sca			; read SC into AC
	rar			; put SC<00> in link
	snl
	jmp	countup

	; rotating AC rightward
	; rotating PC leftward
	; decrementing MQ down to 0
countdn:
	cla mqa			; read MQ (count) into AC
	sna
	jmp	countdnend	; zero, switch to counting up
	tad	_7777		; more to go, decrement count
	mql			; store new count into MQ, then clear AC
	tadi	step		; get what new PC will be
	tad	_4000		; see if it will be 4000
	sza cla
	jmp	loadit
	tad	olddfif		; if so, rotate DFIF left one bit
	cll
	sna
	cml
	ral
	dca	olddfif
	jmp	loadit

	; change to counting up
countdnend:
	scl			; set SC = 20
	.word	07757

	; - rotating AC leftward
	;   rotating PC rightward
	;   incrementing MQ up to SR
countup:
	cla mqa			; read MQ (count) into AC
	cll cma			; get complement count
	dca	comcount
	cla osr			; read limit from SR into AC
	tad	comcount	; compute limit-count-1
	iac			; compute limit-count
	snl
	jmp	countupend
	cla mqa			; read MQ (count) into AC
	iac			; increment count
	mql			; store new count into MQ, then clear AC
	isz	step
	isz	step
	tadi	step		; get what new PC will be
	tad	_4000		; see if it will be 4000
	sza cla
	jmp	loadit
	tad	olddfif		; if so, rotate DFIF right one bit
	cll bsw
	sna
	cml
	rar
	bsw
	dca	olddfif

	; step = points to newpc, newac words
	; olddfif = hold new df,if registers
loadit:
	tadi	step		; get new PC
	dca	newpc
	isz	step		; get new AC
	tadi	step
	dca	newac

	tad	olddfif		; load up new DF
	and	_0070
	tad	_6201
	dca	.+1
	.word	.-.
	tad	olddfif		; load up new IF
	ral
	rtl
	and	_0070
	tad	_6202
	dca	.+1
	.word	.-.
	tad	newac		; load up new AC
	jmpi	newpc		; load up new PC

	; change to counting down
countupend:
	scl			; set SC = 01
	.word	07776
	jmp	countdn

_0070:	.word	00070
_4000:	.word	04000
_6201:	.word	06201
_6202:	.word	06202
_7777:	.word	07777

comcount: .word	.-.
newac:	  .word	.-.
newpc:    .word	.-.
olddfif:  .word	.-.

