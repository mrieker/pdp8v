; pdp-8e isz test

	iof = 06002
	ion = 06001
	tcf = 06042
	tls = 06046
	tsf = 06041

; constants and variables

	.global	__boot

	. = 0
__boot:
	.word	0
	jmp	1		; peripheral interrupt
frmloc:	.word	2		; isz test instruction location
limlo:	.word	3		; low limit test area
	.word	0
	.word	0
limhi:	.word	-07576		; high limit test area
asuc:	.word	suc
msk7:	.word	00007	;AI	; octal conversion mask
work:	.word	0	;AI	; ir0
work1:	.word	0	;AI	; ir1
m377:	.word	-0377	;AI	;
num:	.word	03607	;AI	; the random number location
three:	.word	3	;AI

isz1:	iszi	toloc	;AI	; moving isz
jmp1:	jmp	back	;AI	;   test instruction
jmp2:	jmp	bakbrn		;     group
toloc:	.word	0		; location to be isz'd
patrn:	.word	0		; starting isz pattern
befor:	.word	0		; failing pattern before failing isz
after:	.word	0		; predicted results of each isz
k4:	.word	4		; switch register masks
k0400:	.word	0400
k0200:	.word	0200
k0100:	.word	0100
note:	.word	0		; 7's=error with no skip
print:	.word	inf1-1		; 0's=error with skip
aerr1:	.word	err1
aerr2:	.word	err2
apdr:	.word	pdr
itadnm:	tad	num
atfclf:	.word	tfclf

; sr0(0) = halt after error printout
; sr1(1) = no printouts
; sr3[1] = hold from constant
; sr4(1) = hold to constant
; sr5(1) = hold pattern constant
; sr9(0) = do one isz only
; sr11(1) = do test part 2

; program start
start:	jmsi	.+1
	.word	patch
	and	three

; PAGE 1-1

	sza cla			; skip if part 1
	jmpi	k0400		; goto part 2
	tad	itadnm
	dca	ranum+1

	; check for fixed pattern
chek1:	las
	and	k0100
	sza			; SR5 OFF = generate random pattern
	jmp	chek2		; SR5 ON = leave pattern as is

	; select the pattern
selpat:	jms	ranum
	dca	patrn

	; check for fixed to
chek2:	las
	and	k0200
	sza cla			; SR4 OFF = generate random 'to'
	jmp	chek3		; SR4 ON = leave 'to' as is

	; select the to location
selto:	jms	ranum
	dca	toloc
	tad	toloc
	jms	limtst

	; check for fixed from
chek3:	las
	and	k0400
	sza cla			; SR3 OFF = generate random 'from'
	jmp	plcint		; SR3 ON = leave 'from' as is

	; select the from location
selfrm:	jms	ranum
	dca	frmloc
	tad	frmloc
	jms	limtst

	; place from instructions
plcint:	cla cma
	tad	frmloc
	dca	work
	tad	isz1
	dcai	work
	tad	jmp1
	dcai	work
	tad	jmp2
	dcai	work

	; deposit pattern in to location
	tad	patrn
	dcai	toloc

; PAGE 1-2

	; store predicted isz result
	tad	patrn
	dca	befor
lup1:	tad	befor
	iac
	dca	after
	jmpi	asuc

	;  ISZ ...
	;  jmp back
	;  jmp bakbrn

	; return for no skip condition
back:	las
	ral
	spa cla
	jmp	las1		; SR1 ON: ????????
	tadi	toloc		; SR1 OFF: ????????????????
	cia
	tad	after
	sza cla
	jmpi	aerr1		; error in isz operation
	tadi	toloc
	sna cla
	jmpi	aerr1		; error in isz skip detection
las1:	las
	and	k4
	sna cla			; skip if not one isz (sr9)
	jmp	chek1		; SR9 OFF: 
	iac			; SR9 ON: 
	tad	befor
	jmp	lup1-1

	; return for skip condition
bakbrn:	las
	ral
	spa cla
	jmp	chek1		; SR1 ON: ??????????
	tadi	toloc		; SR1 OFF: ??????????
	sza cla			; skip if to location ok
	jmpi	aerr2		; error in isz location
	jmp	chek1

	; test high-low limits

limtst:	.word	0
	spa
	jmp	.+5
	tad	limlo
	sma cla
	jmpi	limtst
	jmp	ranum+1
	tad	limhi
	sma cla
	jmp	ranum+1
	jmpi	limtst

; PAGE 1-3

	; random number generator
ranum:	.word	0
	tad	num
	cll ral
	szl
	tad	three
	dca	num
	tad	num		; ac = new random number
	jmpi	ranum

k1000:	.word	01000
kp:	.word	0

	. = 0200
	jmp	start

	; error routine 1
err1:	tad	skpdat+6
	dca	skpdat
	cma
	dca	note
	jmp	kpgo

	; error routine 2
err2:	tad	skpdat-1
	dca	skpdat
kpgo:	tad	frmloc
	dca	work
	tad	a3
	jms	setup

	tad	toloc
	dca	work
	tad	a4
	jms	setup

	tad	patrn
	dca	work
	tad	a5
	jms	setup
	tad	befor
	dca	work
	tad	a6
	jms	setup

	tadi	toloc
	dca	work
	tad	a7
	jms	setup

	; tty point routine
tty:	iot	iof
	tad	print
	dca	work
	tadi	work

; PAGE 1-4

	iot	tls
	iot	tsf
	jmp	.-1
	tad	m377
	sza cla
	jmp	tty+3
	iot	tcf
	iot	ion
	las
	sma cla			; SR0 ON: keep going
	hlt			; SR0 OFF: halt after error (sr0)

	tad	note
	sna cla
	jmp	chek1
	dca	note
	jmp	las1		; return to no skip routine

	; error print out line 1
inf1:	.word	0306		; F  from (instruction location)
	.word	0240		; SPACE
indata:	.word	0		; X
	.word	0		; X
	.word	0		; X
	.word	0		; X
	.word	0240		; SPACE
	.word	0240		; SPACE
	.word	0324		; T  to (operand address)
	.word	0240		; SPACE
ondata:	.word	0		; X  address
	.word	0		; X
	.word	0		; X
	.word	0		; X
	.word	0215		; CR
	.word	0212		; LF
	.word	0215		; CR
	.word	0215		; CR

	; error printout line 2
	.word	0317		; O  operand (starting count)
	.word	0240		; SPACE
stdata:	.word	0		; X  pattern
	.word	0		; X
	.word	0		; X
	.word	0		; X
	.word	0240		; SPACE
	.word	0240		; SPACE
	.word	0306		; F  failing count
	.word	0240		; SPACE
fldata:	.word	0		; X  pattern before failing isz
	.word	0		; X
	.word	0		; X
	.word	0		; X
	.word	0240		; SPACE

; PAGE 1-5

	.word	0240		; SPACE
	.word	0322		; R  result after failure
	.word	0240		; SPACE

rsdata:	.word	0		; X  pattern after failing isz
	.word	0		; X
	.word	0		; X
	.word	0		; X
	.word	0240		; SPACE
	.word	0240		; SPACE
skpdat:	.word	0316		; N  no
	.word	0323		; S  skip
	.word	0215		; CR
	.word	0212		; LF
	.word	0212		; LF
	.word	0377		; RUBOUT
	.word	0316		; N
	.word	0323		; S

setup:	.word	0
	dca	work1
	tad	work
	rtl
	rtl
	jms	morsu
	rtr
	rtr
	rtr
	jms	morsu
	rtr
	rar
	jms	morsu
	jms	morsu
	cla
	jmpi	setup
morsu:	.word	0
	and	msk7
	tad	tw6
	dcai	work1
	tad	work
	jmpi	morsu

	; page 1 constants
a3:	.word	indata-1
a4:	.word	ondata-1
a5:	.word	stdata-1
a6:	.word	fldata-1
a7:	.word	rsdata-1
tw6:	.word	0260

	; part 2 initialization routine
	. = 0400
	tad	limlo

; PAGE 1-6

	cia
	dca	from		; low limit to from
	tad	limlo
	cma
	dca	to
	tad	a0
	dca	patcyc
	tad	inst1
	dca	ranum+1
	jmp	chek1		; go to page 0 start

	; path decision routine
pdr:	tad	ranum
	cia
	tad	gfrom
	sna cla			; skip if not requesting from
	jmp	frut		; go to from address routine

	tad	ranum
	cia
	tad	gto
	sna cla			; skip if not requesting to
	jmp	torut		; go to address routine
	jmp	prut		; go to pattern routine

	; select pattern and other things
prut:	tadi	patcyc
	dca	patt
	tad	patt
	sna			; no skip if end of pattern table
	jmp	.+6
	cla iac
	tad	patcyc
	dca	patcyc
	tad	patt
	jmpi	ranum		; return, ac = new pattern

	tad	ak7776
	dca	patcyc		; restore start address of patt table
	iac
	tad	to
	dca	to		; increment to
	tad	to
	cia
	tad	from
	sza cla			; skip if to = from
	jmp	.+4
	tad	to
	tad	three
	dca	to		; skip around from
	tad	to
	sma
	jmp	gout

; PAGE 1-7

	tad	limhi
	spa cla			; skip if end test are
	jmp	gout
	cla iac
	tad	from
	dca	from		; advance from
	tad	limlo
	cia
	dca	to		; reset to address
	tad	from
	tad	limhi
	spa cla
	jmp	gout
	jmp	0400
gout:	cla
	tad	patt
	jmpi	ranum

	; select to routine
torut:	tad	to
	jmpi	ranum

	; select from routine
frut:	tad	from
	jmpi	ranum

	; page 3 constants
gfrom:	.word	selfrm+1	; stored return address when
				; random from is requested
gto:	.word	selto+1		; stored return address when
				; random to is requested
gpat:	.word	selpat+1	; stored return address when
				; random pattern is requested
from:	.word	0		; current from address
to:	.word	0		; current to address
patt:	.word	0		; current pattern
patcyc:	.word	0		; current pattern address
inst1:	jmpi	apdr
k7776:	.word	07776
	.word	07775
	.word	07773
	.word	07767
	.word	07757
	.word	07737
	.word	07677
	.word	07577
	.word	07377
	.word	06777
	.word	05777
	.word	03777
	.word	00001
	.word	00003
	.word	00007
	.word	00017

; PAGE 1-8

	.word	00037
	.word	00077
	.word	00177
	.word	00377
	.word	00777
	.word	01777
k3777:	.word	03777
	.word	0
ak7776:	.word	k7776
a0:	.word	k3777+1

suc:	tad	ct
	iac
	dca	ct
	tad	ct
	sza cla
	jmpi	atfclf
	tad	kp
	tad	k1000
	dca	kp
	tad	kp
	sza cla
	jmpi	atfclf
	iot	iof
	tad	inf2		; set up string to print
	dca	work
	jmpi	.+1		; jump to print routine, return via atfclf
	.word	07602
	.word	0215		; CR
	.word	0212		; LF
	.word	0306		; 'F'
	.word	0303		; 'C'
	.word	0377		; rubout = end of string
ct:	.word	0
inf2:	.word	0567

	. = 0600

	; check for to-from conflict
tfclf:	tad	toloc
	cia
	tad	frmloc
	sna
	jmp	chek2
	iac
	sna
	jmp	chek2
	iac
	sna cla
	jmp	chek2

; PAGE 1-9

	jmpi	frmloc		; jumps to the ISZ being tested
				;  the ISZ then jumps to 'back' if no skip
				;  ... or 'bakbrn' if it skipped

patch:	.word	0		; restore then go away
	dca	0
	tad	x
	dca	1
	tad	x1
	dca	2
	tad	x2
	dca	3
	tad	x3
	dca	start
	tad	x4
	dca	start+1
	iot	ion
	jmpi	patch

x:	.word	07402
x1:	.word	0
x2:	.word	07157
x3:	iot	ion
x4:	las

	. = 07602
	tadi	work		; print string starting at 'work'
	iot	tls
	iot	tsf
	jmp	.-1
	tad	m377		; ends with a rubout
	sza cla
	jmp	.-6		; keep printing if not rubout
	jmp	ovr

	. = 07617
ovr:	iot	tcf		; turn printer off
	iot	ion		; enable interrupts
	jmpi	atfclf

