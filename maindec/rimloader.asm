
; small-computer-handbook-1972.pdf p 107 v 2-15

	.psect	lowspeed
	.align	128

	bopl = .
	. = . + 00156

	iot	06032		; 06032
	iot	06031		; 06031
	jmp	bopl+0157	; 05357
	iot	06036		; 06036
	cll rtl			; 07106
	rtl			; 07006
	s_ma			; 07510
	jmp	bopl+0157	; 05357
	rtl			; 07006
	iot	06031		; 06031
	jmp	bopl+0167	; 05367
	iot	06034		; 06034
	snl			; 07420
	dcai	bopl+0176	; 03776
	dca	bopl+0176	; 03376
	jmp	bopl+0156	; 05356

	.word	00000

	.psect	highspeed
	.align	128

	boph = .
	. = . + 00156

	iot	06014		; 06014
	iot	06011		; 06011
	jmp	boph+0157	; 05357
	iot	06016		; 06016
	cll rtl			; 07106
	rtl			; 07006
	s_ma			; 07510
	jmp	boph+0174	; 05374
	rtl			; 07006
	iot	06011		; 06011
	jmp	boph+0167	; 05367
	iot	06016		; 06016
	snl			; 07420
	dcai	boph+0176	; 03776
	dca	boph+0176	; 03376
	jmp	boph+0157	; 05357

	.word	00000

