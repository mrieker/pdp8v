
	tfl  = 06040	; set tt flag pretending it is ready to accept
	tsf  = 06041	; skip if tt ready to accept
	tls  = 06046	; clear tt flag, start printing tt char

	.global	volbl

	. = 0

	iot	tfl
	cla
loop:
	iot	tsf
	jmp	loop
	tad @	ptr
	iot	tls
	sza cla
	jmp	loop
hang:
	hlt
	jmp	hang

ptr:	.word	msg-1
msg:	.ascii	"\r\nnon-bootable volume "
volbl:	.asciz	"\r\n"

