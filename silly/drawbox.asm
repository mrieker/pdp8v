
; draw a graphics box on VC8

; ../asm/assemble drawbox.asm drawbox.obj > drawbox.lis
; ../asm/link -o drawbox.oct drawbox.obj > drawbox.map
; ../driver/raspictl drawbox.oct

	. = 00020

negedgelen:	.word	.-.	; negative of edge length
posstartpt:	.word	.-.	; positive start co-ord
posendpoint:	.word	.-.	; positive end co-ord

minus1:		.word	  -1
centerpoint:	.word	 512	; screen centerpoint
halfinitsize:	.word	 400	; initial box half size
posstep:	.word	   8	; step this size each time

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
	tad	posstartpt
leftloop:
	iot	06067		; load Y and draw
	iac
	isz	counter
	jmp	leftloop

	cla
	tad	negedgelen
	dca	counter
	tad	posstartpt
botloop:
	iot	06057		; load X and draw
	iac
	isz	counter
	jmp	botloop

	cla
	tad	negedgelen
	dca	counter
	tad	posendpoint
riteloop:
	tad	minus1
	iot	06067		; load Y and draw
	isz	counter
	jmp	riteloop

	cla
	tad	negedgelen
	dca	counter
	tad	posendpoint
toploop:
	tad	minus1
	iot	06057		; load X and draw
	isz	counter
	jmp	toploop

	cll cla
	tad	negedgelen	; make smaller boxes next
	tad	posstep
	tad	posstep
	szl
	jmp	startover
	dca	negedgelen
	tad	posstartpt
	tad	posstep
	dca	posstartpt
	jmp	drawbox

