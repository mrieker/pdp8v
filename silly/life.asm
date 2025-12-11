
; adapted from LIFE.BA
;  1 REM LIFE CLARK BAKER 3/72 C.O.G.
;  2  REMARKABLY TRANSLATED TO OS8 BASIC BY KAY R. FISHER ...DEC

; $ ../asm/assemble life.asm life.obj > life.lis
; $ ../asm/link -o life.oct life.obj > life.map
;
; $ ../driver/raspictl -csrclib -skipopt life.oct
; CSrcLib::open: modules acl alu ma pc rpi seq
; CSrcLib::halfcycle: nloops=2
; IODevTTY::ioinstr: 03 connecting to stdin/stdout
;
; Enter start pattern then control-D:
; **  * * ** * ***
;   * * * * ** *
;  ** * ** ** *
;   **    ** **  *
; ctrl-D

	ksf = 06031	; skip if keyboard ready
	krb = 06036	; read keyboard char

	tfl = 06040	; pretend the printer is ready to accept a character
	tsf = 06041	; skip if printer ready
	tls = 06046	; start printing char

	.global	__boot

	. = 0010

ai10:	.word	0
ai11:	.word	0

_0001:	.word	00001
_0002:	.word	00002
_0003:	.word	00003
_0011:	.word	00011
_0012:	.word	00012
_0015:	.word	00015
_0023:	.word	00023
_0040:	.word	00040
_0060:	.word	00060
_0072:	.word	00072
_0076:	.word	00076
_0177:	.word	00177
_1440:	.word	01440
_5000:	.word	05000
_6030:	.word	06030
_7500:	.word	07500
_7634:	.word	07634
_7700:	.word	07700
_7702:	.word	07702
_7740:	.word	07740
_7750:	.word	07750
_7752:	.word	07752
_7766:	.word	07766
_7775:	.word	07775
_7776:	.word	07776
_7777:	.word	07777

_msg001:	.word	msg001
_msg002:	.word	msg002
_msg003:	.word	msg003
_msg004:	.word	msg004
_a:		.word	a
_am1:		.word	a-1
_d:		.word	d
_dm1:		.word	d-1
_dumpit:	.word	dumpit
_prstrz:	.word	prstrz
_prdcml:	.word	prdcml
_prchar:	.word	prchar
_prcrlf:	.word	prcrlf
_stepit:	.word	stepit

array:	.word	0
c:	.word	0
ctr:	.word	0
ctri:	.word	0
ctrj:	.word	0
ctrx:	.word	0
ctry:	.word	0
g:	.word	0
i:	.word	0
j:	.word	0
l:	.word	0
m:	.word	0
p:	.word	0
ptr:	.word	0
x:	.word	0
y:	.word	0

	. = 0200

__boot:
	cll cla
	tfl

	tad	_5000		; clear arrays
	dca	ctr
	tad	_am1
	dca	ai10
	tad	_dm1
	dca	ai11
clrloop:
	dcai	ai10
	dcai	ai11
	isz	ctr
	jmp	clrloop

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;; read initial contents from keyboard ;;
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	tad	_msg003		; display prompt
	jmsi	_prstrz

	dca	l		; no longest line read in yet
	dca	p		; no occupied positions yet
	dca	x		; no lines read in yet
readloop1:
	tad	x		; print line number
	jmsi	_prdcml
	tad	_0076		; '>'
	jmsi	_prchar
	dca	y		; no chars in line read yet
readloop2:
	tad	y		; see off end of line
	tad	_7700
	sma cla
	jmp	endline		; if so, end the line
	tad	x		; point to position in array
	bsw
	tad	y
	tad	_d
	dca	ptr
readkbchr:
	ksf			; read kb char
	jmp	.-1
	krb
	and	_0177		; mask out top bit
	tad	_7740		; check for control-vs-space-vs-printable
	spa
	jmp	ctrlchr
	sza			; space gets stored as a zero
	cla iac			; any printable gets stored as a one
	dcai	ptr		; store char in array
	tadi	ptr		; increment initial population
	tad	p
	dca	p
	isz	y		; one more char in line
	tadi	ptr		; echo printable char
	sza cla
	tad	_0012		;   splat - space
	tad	_0040		;   space
	jmsi	_prchar
	jmp	readloop2	; read another char
ctrlchr:
	tad	_0023		; 040-015 ie check for return
	sza
	jmp	notcr
endline:
	jmsi	_prcrlf		; end of line, print CR/LF
	tad	l		; see if longest line so far
	cma iac
	tad	y
	sna cla
	jmp	notlongest
	tad	y		; if so, save longest line length
	dca	l
notlongest:
	isz	x		; increment line number
	tad	x		; see if off end of array
	tad	_7750
	spa cla
	jmp	readloop1	; if not, read in next line
	jmp	endofinput	; if so, end of all input
notcr:
	tad	_0011		; 015-004 ie check for control-D
	sza cla
	jmp	readkbchr	; if not, ignore it and read another char
endofinput:

	; d = filled in with data at 0,0
	; x = number of rows
	; y = number of columns

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;; center the D array into the A array ;;
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	tad	_dm1		; point to collected data
	dca	ai10
	tad	x		; point to a[12-x/2][32-y/2]
	and	_7776
	bsw
	tad	y
	cll rar
	cma iac
	tad	_1440
	tad	_am1
	dca	ai11
	tad	_5000
	dca	ctr
offsetloop:
	tadi	ai10
	dcai	ai11
	isz	ctr
	jmp	offsetloop

	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	;; copy A array to D array ;;
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	tad	_am1
	dca	ai10
	tad	_dm1
	dca	ai11
	tad	_5000
	dca	ctr
copyloop:
	tadi	ai10
	dcai	ai11
	isz	ctr
	jmp	copyloop

	; A = array used for current generation computation, stepped once per loop
	; D = array stepped twice per loop to scout ahead for duplicates
	;     stops stepping once a duplicate with A is found
	;     when that duplicate is found yet again, the program halts

	dca	m		; no match yet
	dca	g		; generation 0
	jmpi	_bigloop

_bigloop:	.word	bigloop

	.align	00200

	;; print current A array contents ;;
	;; see if A array matches D array ;;
	;; halt if 3 such matches
	;; - first match happens at gen 0 cuz D was initialized from A
	;; - second match happens when the first duplicate is found
	;; - third match happens when the whole loop has been printed
	;; less than 2 matches, step D twice to scout ahead for loops
	;; step A once
	;; repeat

bigloop:
	tad	_msg001		; print generation number
	jmsi	_prstrz
	tad	g
	jmsi	_prdcml
	tad	_msg002		; print population count
	jmsi	_prstrz
	tad	p
	jmsi	_prdcml
	jmsi	_prcrlf
	jmsi	_prcrlf

	tad	_a		; print out array
	dca	array
	jmsi	_dumpit

	tad	_5000		; compare two arrays for match
	dca	ctr
	tad	_am1
	dca	ai10
	tad	_dm1
	dca	ai11
cmploop:
	tadi	ai10
	cma iac
	tadi	ai11
	sza cla
	jmp	nomatch
	isz	ctr
	jmp	cmploop

	isz	m		; match, increment counter
	cla cll
	tad	m		; see if third match
	tad	_7775
	snl cla
	jmp	nomatch

	tad	_msg004		; if so, whole loop printed so we're done
	jmsi	_prstrz
	hlt

nomatch:
	isz	g		; increment generation
	nop

	tad	m		; see if any match so far
	cll rar
	sza cla
	jmp	leavedalone	; if so, leave duplicate array as is

	tad	_d		; step duplicate array
	dca	array
	jmsi	_stepit		; ...twice
	jmsi	_stepit		; ...to scout ahead for duplicate
leavedalone:

	tad	_a		; always step main array
	dca	array
	jmsi	_stepit

	jmp	bigloop

	.align	00200

;; step array to next state
;;  input:
;;   array = points to A or D array
;;  output:
;;   array updated
;;   p = population count
;;  scratch:
;;   ai10, ai11, c, ctri, ctrj, ctrx, ctry, i, j, ptr, x, y
stepit:	.word	0
	cla iac			; start scanning on row 1 (leave row 0 alone)
	dca	x
	tad	_7752		; set up to scan 22 rows (leave rows 0, 23 alone)
	dca	ctrx
stepit_topx:
	cla iac			; start scanning in col 1 (leave col 0 alone)
	dca	y
	tad	_7702		; set up to scan 62 cols (leave cols 0, 63 alone)
	dca	ctry
stepit_topy:
	dca	c
	cma			; point I to row before the one being scanned
	tad	x
	dca	i
	tad	_7775		; sub-scan 3 rows:  x-1, x, x+1
	dca	ctri
stepit_topi:
	cma			; point J to col before the one being scanned
	tad	y
	dca	j
	tad	_7775		; sub-scan 3 cols:  y-1, y, y+1
	dca	ctrj
stepit_topj:
	tad	i		; point to sub-scan location
	bsw
	tad	j
	tad	array
	dca	ptr
	cma			; AC = -1
	tadi	ptr		; AC = array[i][j] - 1
	cll rar
	sna cla
	isz	c		; count number of array[i][j] that are 1 or 2
	isz	j
	isz	ctrj
	jmp	stepit_topj
	isz	i
	isz	ctri
	jmp	stepit_topi

	tad	x		; point to main scan location
	bsw
	tad	y
	tad	array
	dca	ptr
	tadi	ptr		; read array[x][y]
	sza cla
	jmp	stepit_nonzelem
	tad	c		; array[x][y]==0, read c
	tad	_7775
	sza cla
	jmp	stepit_nxty
	tad	_0003		; array[x][y] == 0 and c == 3, set array[x][y] = 3
	dcai	ptr
	jmp	stepit_nxty
stepit_nonzelem:
	tad	c		; AC = c
	tad	_7775		; AC = c - 3
	cll rar			; AC = (c - 3) / 2
	sna cla
	jmp	stepit_nxty
	tad	_0002		; array[x][y] != 0 and c != 3 and c != 4, set array[x][y] = 2
	dcai	ptr
stepit_nxty:
	isz	y		; increment column number
	isz	ctry
	jmp	stepit_topy	; repeat if more columns in row
	isz	x		; increment row number
	isz	ctrx
	jmp	stepit_topx	; repeat if more rows

	tad	_5000		; count population of whole resultant array
	dca	ctr
	tad	array
	dca	ptr
	dca	p
stepit_count:
	tadi	ptr		; also and each element with 0001
	and	_0001
	dcai	ptr
	tadi	ptr
	tad	p
	dca	p
	isz	ptr
	isz	ctr
	jmp	stepit_count
	jmpi	stepit

	.align	00200

;; print array
;;  input:
;;   array = which array to print (A or D)
;;  output:
;;   array printed
;;   AC = zero
;;  scratch:
;;   ai10, ctrx, x, ctry
dumpit:	.word	0
	cla cma			; point before first word of array
	tad	array
	dca	ai10
	dca	x		; clear row number
	tad	_7750		; total number of rows to print
	dca	ctrx
dumpit_topx:
	tad	x		; print line number
	jmsi	_prdcml
	tad	_0072		; ':'
	jmsi	_prchar
	isz	x
	tad	_7700		; columns in a row
	dca	ctry
dumpit_topy:
	tadi	ai10
	sza cla
	tad	_0012		; star - space
	tad	_0040		; space
	jmsi	_prchar
	isz	ctry
	jmp	dumpit_topy
	jmsi	_prcrlf
	isz	ctrx
	jmp	dumpit_topx
	jmpi	dumpit

;; print .asciz message pointed to by AC
;; clear AC on return
;; scratch: ai10
prstrz:	.word	0
	tad	_7777
	dca	ai10
prstrz_loop:
	tadi	ai10
	sna
	jmpi	prstrz
	jms	prchar
	jmp	prstrz_loop

;; print decimal number in AC then clear AC
prdcml:	.word	0
	dca	prdcml_val	; save value to be printed
	tad	_6030		; print 1000s
	jms	prdcml_digit
	tad	_7634		; print 100s
	jms	prdcml_digit
	tad	_7766		; print 10s
	jms	prdcml_digit
	tad	prdcml_val	; print 1s
	tad	_0060
	jms	prchar
	jmpi	prdcml

;; - print single decimal digit
;;  input:
;;   prdcml_val = value to print
;;   AC = negative divisor for decimal digit
;;  output:
;;   prdcml_val = remainder (lt divisor)
;;   AC = zero
prdcml_digit: .word 0
	dca	prdcml_div	; save negative divisor
	tad	_0060		; set up a '0' digit
	dca	prdcml_chr
	tad	prdcml_val	; get value to print
prdcml_loop:
	cll
	tad	prdcml_div	; see if value ge divisor
	snl
	jmp	prdcml_done	; if not, done looping
	isz	prdcml_chr	; if so, increment digit
	jmp	prdcml_loop	; repeat subtraction
prdcml_done:
	cma iac			; value lt divisor, undo subtraction
	tad	prdcml_div
	cma iac
	dca	prdcml_val	; save remainder
	tad	prdcml_chr	; print digit
	jms	prchar
	jmpi	prdcml_digit

prdcml_val:	.word	0
prdcml_div:	.word	0	; divisor for digit being decoded
prdcml_chr:	.word	0	; digit decoded

;; print CR/LF then clear AC
prcrlf:	.word	0
	tad	_0015
	jms	prchar
	tad	_0012
	jms	prchar
	jmpi	prcrlf

;; print char in AC then clear AC
prchar:	.word	0
	tsf
	jmp	.-1
	tls
	cla
	jmpi	prchar


;msg001: .asciz	"\033[H\033[JGeneration "
msg001:	.asciz	"\r\nGeneration "
msg002:	.asciz	" population "
msg003:	.asciz	"\r\nEnter start pattern then control-D:\r\n"
msg004:	.asciz	"Stable population -- All done\r\n\r"

a:	.blkw	03000		; 24 rows, 64 columns
d:	.blkw	03000

