
; A-DEC-08-LBAA-D Binary Loader.pdf

; ../asm/assemble.x86_64 binloader.asm binloader.obj > binloader.lis
; ../asm/link.x86_64 -o binloader.oct binloader.obj > binloader.map

; ln -s 0 swreg.lnk
; export switchregister=swreg.lnk
; export iodevptaperdr=pdp-8.nz/dec-08-a2b1-pb.bin
; time ../driver/raspictl.x86_64 -haltstop binloader.oct
; ... takes about 45 sec

	rsf =     06011
	rfc =     06014
	rrb_rfc = 06016

	ksf =     06031		; p257: if keyboard flag set, increment PC
	kcc =     06032		; clear kb flag, clear accum, start reader
	krb =     06036		; p257: clear kb flag, load ac from input buffer, start next read

	cdf =     06201
	rdf =     06214

	.global	__boot

	. = 0
__boot:
	jmpi	beginptr
beginptr: .word	begin

	. = 07612
switch:	.word	0		; zero: not in rubout area; one: in rubout area
memtem:	.word	0
char:	.word	0
chksum:	.word	0
origin:	.word	0		; address for next data word from tape

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; read tape until frame (200) byte found
;  input:
;   AC = 0
;  output:
;   char = last byte read with <7> set
;   return+0: leader/trailer
;         +1: data or origin
;  scratch:
;   switch
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	. = 07626
begg:	.word	0
; skip over data between rubouts
begg_wrswitch:
	dca	switch		; set switch
begg_readchar:
	jms	read		; get a character
	tad	m376		; test for 377
	spa sna cla
	jmp	begg_notrubout	; no
	isz	switch		; yes: complement switch
	cma
	jmp	begg_wrswitch
begg_notrubout:
	tad	switch		; not rubout
	sza cla			; is switch set?
	jmp	begg_readchar	; yes: ignore

; non-rubout outside of rubout block
	tad	char		; no: test for code
	and	mask		; check top two bits
	tad	m200
				;  0000+7600 = 7600
				;  0100+7600 = 7700
				;  0200+7600 = 0000
				;  0300+7600 = 0100
	spa
	isz	begg		; was 0000 or 0100, data or origin
	spa sna cla
	jmpi	begg		; data, origin, or leader/trailer

	tad	char		; field setting
	and	fmask
	tad	change
	dca	memtem
	jmp	begg_readchar	; continue input

fmask:	.word	070
change:	iot	cdf		; change data field

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; read byte from either high-speed or low-speed paper tape reader
; save byte in 'char' and returns it in accum
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
read:	.word	0
	.word	0		; either jmp lor or jmp hir based on top switch
lor:	iot	ksf		; wait for flag
	jmp	.-1
	iot	krb		; read byte, clear top bits and start another read
	dca	char
	tad	char
	jmpi	read
hir:	iot	rsf
	jmp	.-1
	iot	rrb_rfc
	jmp	lor+3

mask:	.word	0300

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; end of tape
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
bend:	jms	assemb
	cia
	tad	chksum
m376:	hlt

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; loader starts here
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
begin:	iot	kcc		; start reading
	iot	rfc
	iot	rdf
	tad	change		; get 'change data field' instruction
	dca	memtem		; save field instruction
	cla osr			; read switch register
	sma cla
	tad	hiri		; top switch off: use hir
	tad	lori		;             on: use lor
	dca	read+1
skipleader:
	jms	begg		; read byte from tape
	jmp	skipleader	; ignore leader

go:	dca	chksum		; update checksum
	tad	memtem
	dca	memfld
	tad	char
	dca	word1		; save most significant bits
	jms	read		; read next byte from tape
	dca	word2		; save least significant bits
	jms	begg		; read next byte from tape
	jmp	bend		; trailer, end
	jms	assemb		; put a 12-bit word together from word1/word2
	snl			; see if data or address
	jmp	memfld		; - data
	dca	origin		; address, save address as origin
chex:	tad	word1		; add 6-bit words to checksum
	tad	word2
	tad	chksum
	jmp	go

memfld:	.word	0
	dcai	origin
	isz	origin
m200:	cla			; 07600
	jmp	chex

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; combine 6 bits from each of word1,word2 into accumulator
; also returns the tag bit in link
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
assemb:	.word	0
	tad	word1
	cll rtl
	rtl
	rtl
	tad	word2
	jmpi	assemb

lori:	jmp	lor
hiri:	.word	hir-lor
	.word	0

word2:	.word	0		; least significant bits

	. = 07776
word1:	.word	0		; most significant bits
	jmp	begin

