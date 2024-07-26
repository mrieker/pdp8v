
; draw a graphics box on VC8

; ../asm/assemble drawbox.asm drawbox.obj > drawbox.lis
; ../asm/link -o drawbox.oct drawbox.obj > drawbox.map
; ../driver/raspictl drawbox.oct

	clze = 06130
	clde = 06132
	clab = 06133
	clsa = 06135
	clca = 06137

	. = 00020

negedgelen:	.word	.-.	; negative of edge length
posstartpt:	.word	.-.	; positive start co-ord
posendpoint:	.word	.-.	; positive end co-ord

minus1:		.word	  -1
centerpoint:	.word	 512	; screen centerpoint
halfinitsize:	.word	 400	; initial box half size
posstep:	.word	   8	; step this size each time

intensity:	.word	   0
plus3:		.word	   3
p06074:		.word	06074
p00300:		.word	00300

counter: .word	.-.

	. = 00200
	.global	__boot
__boot:
	iot	06077		; intensity = max

startover:
	cll cla
	tad	halfinitsize
	cia ral
	dca	negedgelen
	tad	halfinitsize
	cia
	tad	centerpoint
	dca	posstartpt
	jms	setintens	; skip an intensity step

drawbox:
	tad	negedgelen
	cia
	tad	posstartpt
	dca	posendpoint

	tad	posstartpt
	iot	06053		; load X
	cla
	tad	negedgelen
	dca	counter
	jms	setintens
	tad	posstartpt
leftloop:
	iot	06067		; load Y and draw
	iac
	isz	counter
	jmp	leftloop

	cla
	tad	negedgelen
	dca	counter
	jms	setintens
	tad	posstartpt
botloop:
	iot	06057		; load X and draw
	iac
	isz	counter
	jmp	botloop

	cla
	tad	negedgelen
	dca	counter
	jms	setintens
	tad	posendpoint
riteloop:
	tad	minus1
	iot	06067		; load Y and draw
	isz	counter
	jmp	riteloop

	cla
	tad	negedgelen
	dca	counter
	jms	setintens
	tad	posendpoint
toploop:
	tad	minus1
	iot	06057		; load X and draw
	isz	counter
	jmp	toploop

	cll cla
	jms	setintens	; skip an intensity step

	tad	negedgelen	; make smaller boxes next
	tad	posstep
	tad	posstep
	szl
	jmp	pauseabit
	dca	negedgelen
	tad	posstartpt
	tad	posstep
	dca	posstartpt
	jmp	drawbox

pauseabit:
	cla cma			; turn RTC off
	clze
	clsa			; clear status register
	cla			; run RTC at 1KHz
	tad	p00300
	clde
	cla			; clear counter
	clab
pauseloop:
	clsa			; read status register
	sma cla
	jmp	pauseloop	; repeat until counted up
	jmp	startover	; counted 4.096 second wait, resume drawing

; increment intensity level
setintens: .word .-.
	cll cla iac		; increment intensity
	tad	intensity
	and	plus3
	dca	intensity
	tad	intensity
	tad	p06074
	dca	.+1		;   06074..06077
	nop
	jmpi	setintens

