
; ../asm/assemble dttest.asm dttest.obj > dttest.lis
; ../asm/link -o dttest.oct dttest.obj > dttest.map
; ../driver/raspictl -csrclib -script dttest.tcl

	ion   = 06001
	iof   = 06002
	ksf   = 06031
	kie   = 06035
	krb   = 06036
	tfl   = 06040
	tsf   = 06041
	tls   = 06046
	pioff = 06310	; printinstr off
	pion  = 06311	; printinstr on
	dtra  = 06761
	dtxa  = 06764
	dtla  = 06766
	dtsf  = 06771
	dtrb  = 06772
	idwc  = 07754	; 2s comp dma word count
	idca  = 07755	; dma address minus one

	. = 0
	.word	.-.
	jmpi	.+1
	.word	intserv

_0003:	.word	00003
_0007:	.word	00007
_0012:	.word	00012
_0015:	.word	00015
_0060:	.word	00060

ai10:	.word	.-.

_0204:	.word	00204
_0214:	.word	00214
_0224:	.word	00224
_0244:	.word	00244
_0314:	.word	00314
_0604:	.word	00604
_0614:	.word	00614
_0714:	.word	00714
_3000:	.word	03000
_4321:	.word	04321
_5076:	.word	05076
_5077:	.word	05077
_7000:	.word	07000
_7577:	.word	07577
_7774:	.word	07774
_7777:	.word	07777

driveno: .word	.-.			; drive number in top 3 bits

blockno:  .word	.-.			; block number wanted
randseed: .word	.-.			; random number seed

_blockseeds:  .word blockseeds		; table of random seed used to write a block
_fatal:       .word fatal		; print fatal error message
_getrand:     .word getrand		; get random uint12
_idwc:	      .word idwc		; points to dma transfer negative wordcount
_idca:	      .word idca		; points to dma transfer address word
_dmabuff:     .word dmabuff		; points to beginning of dma transfer buffer
_printoct:    .word printoct		; print number in octal
_printcrlf:   .word printcrlf		; print cr/lf
_printstrz:   .word printstrz		; print null-terminated string
_readrand:    .word readrand		; read block and verify random numbers
_rewind:      .word rewind		; rewind the tape
_scanbegblk:  .word scanbegblk
_waitfordone: .word waitfordone		; wait for any type of completion
_waitforeot:  .word waitforeot		; wait for end-of-tape
_waitsuccess: .word waitsuccess		; wait for success
_writerand:   .word writerand		; write random numbers to block

	. = 0200
	.global	__boot
__boot:
	cla cll			; disable tty interrupts
	kie
	tfl			; pretend tty is ready to type something

	osr			; get random seed from switch register
	dca	randseed
	tad	randseed	; it also doubles as drive number in <11:09>
	and	_7000
	dca	driveno

	jmsi	_rewind		; rewind tape to beginning

	dca	blockno		; write initial random data to whole tape
initloop:
	jmsi	_writerand	; write block with random data
	isz	blockno
	tad	blockno
	tad	_5076		; -2702
	sza cla
	jmp	initloop

readloop:
	jmsi	_getrand	; get random block to read
	dca	blockno
	tad	blockno		; make sure .lt. 02702
	cll
	tad	_5076
	szl cla
	jmp	readloop	; get another rand if not
	tad	_readloopm1
	jmsi	_printstrz
	tad	blockno
	jmsi	_printoct
	jmsi	_scanbegblk	; skip to beginning of block
	jmsi	_readrand	; read block & verify random numbers
	jmp	readloop	; try another

_readloopm1: .word .+1
	.asciz	"\r\nverify blockno="


	.align	00200

;
; rewind tape
;  input:
;   AC = zero
;   driveno = drive number in <11:09>
;   interrupts disabled
;  output:
;   tape rewound
;   AC = zero
;   interrupts disabled
;
rewind:	.word	.-.
	tad	driveno		; select drive
	tad	_0604		; 'rewind reverse'
	dtla			; start rewinding
	jmsi	_waitforeot	; wait for completion
	jmpi	rewind

;
; scan for beginning of a block going forward
;  input:
;   driveno = drive number in <11:09>
;   blockno = block number
;   interrupts disabled
;
scanbegblk: .word .-.
	cla cll
	tad	blockno
	sza cla
	jmp	sbb_nonzero
	jms	rewind
	jmp	sbb_readfwd
sbb_nonzero:
	tad	_7777		; set up DMA word count
	dcai	_idwc
	tad	_dmabuff	; set up DMA memory addr
	dcai	_idca
	tad	driveno		; 'scan forward normal' to get next block number
	tad	_0214
	dtla
	jmsi	_waitfordone
	spa cla
	jmp	sbb_ateot
	tadi	_dmabuff	; get current block number
	cma iac			; ...negative
	tad	blockno		; get desired block number - current block number
	spa
	jmp	sbb_behindus
	sna			; if zero, ...
	jmpi	scanbegblk	; ... we're there

	; need to skip forward AC blocks
	cma iac			; make it negative
	dcai	_idwc		; set it up as DMA word count
	tad	driveno		; 'scan forward continuous' to skip that many blocks
	tad	_0314
	dtla
	jmsi	_waitsuccess	; wait for successful completion
	jmp	sbb_verify

	; am at end of tape, that's just like being at beginning of block 2702
sbb_ateot:
	tad	blockno		; get desired block number - 2702
	tad	_5076		; ... result should always be negative

	; need to skip backward -AC blocks
	; suppose blockno=11 and _dmabuff=14, AC = -3
sbb_behindus:
	tad	_7777		; have to skip one extra (skip to end of block 10 for the example)
	dcai	_idwc
	tad	driveno		; 'scan backward continuous' to skip that many blocks
	tad	_0714
	dtla
	jmsi	_waitsuccess	; wait for successful comletion

	; now skip forward one block, should be what we want
sbb_readfwd:
	tad	_7777		; set up DMA word count
	dcai	_idwc
	tad	driveno		; 'scan forward normal' to get next block number
	tad	_0214
	dtla
	jmsi	_waitsuccess

	; verify that we just skipped forward to beginning of the target block
sbb_verify:
	tadi	_dmabuff	; get current block number
	cma iac			; ...negative
	tad	blockno		; we should be exactly there now
	sza
	jmsi	_fatal		; - fatal if not
	jmpi	scanbegblk

;
; scan for end of a block going backward
;  input:
;   driveno = drive number in <11:09>
;   blockno = block number
;   interrupts disabled
;
scanendblk: .word .-.
	cla cll
	tad	blockno		; see if targeting 2701
	tad	_5077
	sza cla
	jmp	seb_noteot
	tad	driveno		; skip driver to end-of-tape
	tad	_0204
	dtla
	jmsi	_waitforeot	; we should hit end-of-tape
	jmp	seb_readrev
seb_noteot:
	tad	_7777		; set up DMA word count
	dcai	_idwc
	tad	_dmabuff	; set up DMA memory addr
	dcai	_idca
	tad	driveno		; 'scan reverse normal' to get next block number
	tad	_0614
	dtla
	jmsi	_waitfordone
	spa cla
	jmp	seb_atbot
	tad	blockno		; get desired block number
	cma iac			; ...negative
	tadi	_dmabuff	; get current block number - desired block number
	spa
	jmp	seb_aheadus
	sna			; if zero, ...
	jmpi	scanendblk	; ... we're there

	; need to skip backward AC blocks
	cma iac			; make it negative
	dcai	_idwc		; set it up as DMA word count
	tad	driveno		; 'scan reverse continuous' to skip that many blocks
	tad	_0714
	dtla
	jmsi	_waitsuccess	; wait for successful completion
	jmp	seb_verify

	; am at beginning of tape, that's just like being at end of block 7777
seb_atbot:
	tad	blockno		; get 7777 - desired block number
	cma			; ... result should always be negative

	; need to skip forward -AC blocks
	; suppose blockno=14 and _dmabuff=11, AC = -3
seb_aheadus:
	tad	_7777		; have to skip one extra (skip to beg of block 15 for the example)
	dcai	_idwc
	tad	driveno		; 'scan forward continuous' to skip that many blocks
	tad	_0314
	dtla
	jmsi	_waitsuccess	; wait for successful comletion

	; now skip backward one block, should be what we want
seb_readrev:
	tad	_7777		; set up DMA word count
	dcai	_idwc
	tad	driveno		; 'scan reverse normal' to get prior block number
	tad	_0614
	dtla
	jmsi	_waitsuccess

	; verify that we just skipped backward to end of the target block
seb_verify:
	tadi	_dmabuff	; get current block number
	cma iac			; ...negative
	tad	blockno		; we should be exactly there now
	sza
	jmsi	_fatal		; - fatal if not
	jmpi	scanendblk


	.align	00200

;
; wait for success status - fatal if something else
;  output:
;   AC = zero
;
waitsuccess: .word .-.
	jms	waitfordone
	tad	_7777
	sza
	jmsi	_fatal
	jmpi	waitsuccess

;
; wait for end-of-tape status - fatal if something else
;  output:
;   AC = zero
;
waitforeot: .word .-.
	jms	waitfordone
	tad	_3000
	sza
	jmsi	_fatal
	jmpi	waitforeot

;
; wait for operation to complete
;  input:
;   interrupts disabled
;   status A = some command with interrupts enabled
;  output:
;   interrupts disabled
;   AC = status B
;
waitfordone: .word .-.
wfd_loop:
	cla			; read status register B
	dtrb
	sza
	jmpi	waitfordone
	ion			; busy, enable interrupts
	hlt
wfd_halt:
	iof
	jmp	wfd_loop

__wfd_halt: .word -wfd_halt

;
; interrupt service -
;  prints status B and clears GO bit
;
intserv:
	dtsf			; check for tape op complete
	jmp	intret
	dca	intacsave
	tad	0		; see if interrupted waitfordone's HLT instruction
	tad	__wfd_halt
	sna cla
	jmp	wfd_loop	; if so, just jump back to it without enabling interrupts
	dtra			; if not, read status A
	and	_0204		; get existing GO,IE bits
	tad	_0003		; leave status B bits alone
	dtxa			; clear GO,IE, leave status B alone
	tad	intacsave
intret:
	ion
	jmpi	0

intacsave: .word .-.


	.align	00200
;
; write random data to block
;  input:
;   AC = zero
;   blockno = block number
;   randseed = random seed
;
writerand: .word .-.

	; save random seed that will be used to fill block
	tad	_blockseeds
	tad	blockno
	dca	wr_temp1
	tad	randseed
	dcai	wr_temp1

	; fill DMA buffer with random numbers
	tad	_7577
	dca	wr_temp1
	tad	_dmabuff
	dca	ai10
wr_fill:
	jmsi	_getrand
	dcai	ai10
	isz	wr_temp1
	jmp	wr_fill

	; set up DMA buffer descriptor
	tad	_7577
	dcai	_idwc
	tad	_dmabuff
	dcai	_idca

	; start writing
	tad	driveno
	tad	_0244
	dtla

	; wait for success
	jmsi	_waitsuccess
	jmpi	writerand

wr_temp1: .word	.-.

;
; read block and check random data
;  input:
;   AC = zero
;   blockno = block number
;   randseed = random seed
;
readrand: .word .-.

	; get random seed that was used to fill block
	tad	_blockseeds
	tad	blockno
	dca	rr_temp1
	tadi	rr_temp1
	dca	randseed

	; set up DMA buffer descriptor
	tad	_7577
	dcai	_idwc
	tad	_dmabuff
	dcai	_idca

	; start reading
	tad	driveno
	tad	_0224
	dtla

	; wait for success
	jmsi	_waitsuccess

	; verify DMA buffer random numbers
	tad	_7577
	dca	rr_temp1
	tad	_dmabuff
	dca	ai10
rr_check:
	jmsi	_getrand
	cma iac
	tadi	ai10
	sza
	jmsi	_fatal
	isz	rr_temp1
	jmp	rr_check
	jmpi	readrand

rr_temp1: .word	.-.


	.align	00200

;
; get 12-bit random number
;  input:
;   randseed = starting point
;  output:
;   randseed = updated
;   AC = random number
;
getrand: .word .-.
	cla cll
	tad	randseed
	tad	_4321
	szl
	iac
	dca	randseed
	tad	randseed
	jmpi	getrand


	.align	0200

printoct: .word	.-.
	dca	poval
	tad	_7774
	dca	pocnt
poloop:
	tad	poval
	rtl
	rtl
	and	_0007
	tad	_0060
	jms	printac
	tad	poval
	rtl
	ral
	dca	poval
	isz	pocnt
	jmp	poloop
	jmpi	printoct

pocnt:	.word	.-.
poval:	.word	.-.

printcrlf: .word .-.
	tad	_0015
	jms	printac
	tad	_0012
	jms	printac
	jmpi	printcrlf

printstrz: .word .-.
	dca	poval
printstrloop:
	tadi	poval
	sna
	jmpi	printstrz
	jms	printac
	isz	poval
	jmp	printstrloop

printac: .word	.-.
	tsf
	jmp	.-1
	tls
	cla
	jmpi	printac

; ------------------------------------------------------------------------------

;
; fatal error -
;
fatal:	.word	.-.
	dca	fatalac
	tad	_fatalm1
	jmsi	_printstrz
	tad	fatal
	jmsi	_printoct
	tad	_fatalm2
	jmsi	_printstrz
	tad	fatalac
	jmsi	_printoct
	hlt
	jmp	.-1

fatalac: .word	.-.

_fatalm1: .word	.+1
	.asciz	"\r\nFATAL ERROR at "
_fatalm2: .word	.+1
	.asciz	" AC="

; ------------------------------------------------------------------------------

dmabuff: .blkw	130

blockseeds: .blkw 1474

