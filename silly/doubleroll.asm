
; roll AC / PC lights
; reverse direction every SR'th steps

; ./doubleroll.sh                 on the tubes
; ./doubleroll.sh -csrclib        gui simulator
; ./doubleroll.sh -pidp -nohwlib  on pidp8 panel

	hasextar = 1	; 0=no extarith boards installed
			; 1=has extarith boards installed

	ion  = 06001	; enable interrupts

	cdf0 = 06201
	cdf1 = 06211
	cif0 = 06202
	rib  = 06234

	clze = 06130	; (RTC) clear the given enable bits
	clsk = 06131	; (RTC) skip if requesting interrupt
	clde = 06132	; (RTC) set the given enable bits
	clab = 06133	; (RTC) set buffer and counter to given value
	clen = 06134	; (RTC) read enable register
	clsa = 06135	; (RTC) read status register and reset it
	clba = 06136	; (RTC) read buffer register
	clca = 06137	; (RTC) read counter into buffer and read buffer

	pioff = 06310
	pion  = 06311
	wfi   = 06313	; wait for interrupt request

	. = 00000
	.word	.-.
	jmpi	0

	. = 00002
h0003:	wfi
	jmp	finish3

	. = 00005
h0006:	wfi
	jmsi	_step		; PC=0006; AC=3000
	.word	h0014,01400	; up -> 0014; 1400
	.word	h0003,06000	; dn -> 0003; 6000
	.word	h0006,03000	; hold

	. = 00013
h0014:	wfi
	jmsi	_step		; PC=0014; AC=1400
	.word	h0030,00600
	.word	h0006,03000
	.word	h0014,01400

	. = 00027
h0030:	wfi
	jmsi	_step		; PC=0030; AC=0600
	.word	h0060,00300
	.word	h0014,01400
	.word	h0030,00600

	. = 00057
h0060:	wfi
	jmsi	_step		; PC=0060; AC=0300
	.word	h0140,00140
	.word	h0030,00600
	.word	h0060,00300

	. = 00137
h0140:	wfi
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

_0010:	.word	00010
_0020:	.word	00020
_0070:	.word	00070
_0077:	.word	00077
_0100:	.word	00100
_2001:	.word	02001
_4000:	.word	04000
_4001:	.word	04001
_6201:	.word	06201
_6202:	.word	06202
_7776:	.word	07776
_7777:	.word	07777

	. = 00277
h0300:	wfi
	jmsi	_step		; PC=0300; AC=0060
	.word	h0600,00030
	.word	h0140,00140
	.word	h0300,00060

	. = 00577
h0600:	wfi
	jmsi	_step		; PC=0600; AC=0030
	.word	h1400,00014
	.word	h0300,00060
	.word	h0600,00030

	. = 01377
h1400:	wfi
	jmsi	_step		; PC=1400; AC=0014
	.word	h3000,00006
	.word	h0600,00030
	.word	h1400,00014

	. = 02777
h3000:	wfi
	jmsi	_step		; PC=3000; AC=0006
	.word	h6000,00003
	.word	h1400,00014
	.word	h3000,00006

	. = 05777
h6000:	wfi
	jmsi	_step		; PC=6000; AC=0003
	.word	h4001,04001
	.word	h3000,00006
	.word	h6000,00003

	. = 04000
h4001:	wfi
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
	jmpi	_4001		; start at 4001

copyaddr: .word	.-.
intena:	.word	05400		;    <11> = interrupt enable
				; <10:09> = 01  : repeat
				; <08:06> = 100 : 100uS ticks
period:	.word	-667		; interrupt every 66700uS (15Hz)

; step PC and AC
;   jmsi   _step
;   .word  pcleft,acright
;   .word  pcright,acleft
;   .word  pchold,achold
; interrupts disabled
; saveddf,if = possibly one bit set
;      df,if = zero

step:	.word	.-.
	clsa			; clear clock interrupt request

	cla cll			; save data & instr fields
	rib			; AC = 000000iiiddd
	ral
	rtl			; AC = 000iiiddd000
	dca	savdfif
	tad	savdfif
	bsw			; AC = ddd000000iii
	tad	savdfif		; AC = dddiiidddiii
	and	_0077		; AC = 000000dddiii
	dca	savdfif

.if hasextar
	cla sca			; read SC into AC
.else
	tad	fakesc
.endif
	rar			; put SC<00> in link
	snl cla
	jmp	countup

	; rotating AC rightward
	; rotating PC leftward
	; decrementing MQ down to 0
countdn:
.if hasextar
	cla mqa			; read MQ (count) into AC
.else
	tad	fakemq
.endif
	sna
	jmp	countdnend	; zero, switch to counting up
	tad	_7777		; more to go, decrement count
.if hasextar
	mql			; store new count into MQ, then clear AC
.else
	dca	fakemq
.endif
	;  dfif        pc
	; 000000  110000000000  what it was when called
	; 000001  100000000001  what it will be on return
	; 000001  000000000011  what it will be step after that
	tadi	step		; get what new PC will be
	tad	_4000		; see if it will be 4000 (pausing at 4001)
	sza cla
	jmp	loadit
	tad	savdfif		; if so, rotate DFIF left one bit
	cll
	sna
	cml
	ral
	dca	savdfif
	jmp	loadit

	; change to counting up
countdnend:
.if hasextar
	scl			; set SC = 20
	.word	07757
.else
	tad	_0020
	dca	fakesc
.endif

	; - rotating AC leftward
	;   rotating PC rightward
	;   incrementing MQ up to SR
countup:
.if hasextar
	cla mqa			; read MQ (count) into AC
.else
	tad	fakemq
.endif
	cll cma			; get complement count
	dca	comcount
	cla osr			; read limit from SR into AC
	tad	comcount	; compute limit-count-1
	iac			; compute limit-count
	snl cla
	jmp	countupend
.if hasextar
	cla mqa			; read MQ (count) into AC
.else
	tad	fakemq
.endif
	iac			; increment count
.if hasextar
	mql			; store new count into MQ, then clear AC
.else
	dca	fakemq
.endif
	isz	step
	isz	step
	;  dfif        pc
	; 000001  100000000001  what it was when called
	; 000000  110000000000  what it will be on return
	; 000000  011000000000  what it will be step after that
	tadi	step		; get what new PC will be
	tad	_2001		; see if it will be 5777 (pausing at 6000)
	sza cla
	jmp	loadit
	tad	savdfif		; if so, rotate DFIF right one bit
	cll bsw
	sna
	cml
	rar
	bsw
	dca	savdfif

	; step = points to newpc, newac words
	; savdfif = holds new df,if registers
loadit:
	tadi	step		; get new PC
	dca	newpc
	isz	step		; get new AC
	tadi	step
	dca	newac

	tad	savdfif		; load up new DF
	and	_0070
	tad	_6201
	dca	.+1
	.word	.-.
	tad	savdfif		; load up new IF
	ral
	rtl
	and	_0070
	tad	_6202
	dca	.+1
	.word	.-.
	tad	newac		; load up new AC
	ion			; allow RTC interrupt
	jmpi	newpc		; load up new PC

	; change to counting down
countupend:
.if hasextar
	scl			; set SC = 01
	.word	07776
.else
	iac
	dca	fakesc
.endif
	jmp	countdn

comcount: .word	.-.
newac:	  .word	.-.
newpc:    .word	.-.
savdfif:  .word	.-.		; <05:03> = DF; <02:00> = IF

.if hasextar ^ 1
fakemq:	.word	.-.
fakesc:	.word	.-.
.endif

