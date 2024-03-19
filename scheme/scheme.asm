;;    Copyright (C) Mike Rieker, Beverly, MA USA
;;    www.outerworldapps.com
;;
;;    This program is free software; you can redistribute it and/or modify
;;    it under the terms of the GNU General Public License as published by
;;    the Free Software Foundation; version 2 of the License.
;;
;;    This program is distributed in the hope that it will be useful,
;;    but WITHOUT ANY WARRANTY; without even the implied warranty of
;;    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;;    GNU General Public License for more details.
;;
;;    EXPECT it to FAIL when someone's HeALTh or PROpeRTy is at RISk.
;;
;;    You should have received a copy of the GNU General Public License
;;    along with this program; if not, write to the Free Software
;;    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
;;
;;    http://www.gnu.org/licenses/gpl-2.0.html

	OP_ECHOLD = 1	; echo load file lines to output
	OP_GCTEST = 0	; gc test before each malloc
	OP_MAKNAM = 0	; print makenament symbol name
	OP_MALLPR = 0	; print mallocs and frees
	OP_MEMCHK = 1	; memck before & after garbcoll
	OP_POISON = 0	; poison freed memory blocks
	OP_TTWAIT = 0	; wait after each char printed
	OP_XARITH = 1	; extended arithmetic

	SR_VTRUBS = 00001 ;<00> vt-style rubouts

	ion   = 06001	; interrupts on
	iof   = 06002	; interrupts off
	gtf   = 06004	; get flags
	rtf   = 06005	; restore flags

	ksf   = 06031	; skip if kb char to be read
	kcc   = 06032	; clear kb flag, clear acc, start reader
	krs   = 06034	; read kb char but don't clear flag
	kie   = 06035	; keyboard and printer interrupt enable
	krb   = 06036	; read kb char and clear flag

	tfl   = 06040	; set tt flag pretending it is ready to accept
	tsf   = 06041	; skip if tt ready to accept
	tcf   = 06042	; clear tt flag
	tpc   = 06044	; start printing tt char
	tls   = 06046	; clear tt flag, start printing tt char

	clze  = 06130	; clear the given enable bits
	clsk  = 06131	; skip if requesting interrupt
	clde  = 06132	; set the given enable bits
	clab  = 06133	; set buffer and counter to given value
	clen  = 06134	; read enable register
	clsa  = 06135	; read status register and reset it
	clba  = 06136	; read buffer register
	clca  = 06137	; read counter into buffer and read buffer

	cdf_0 = 06201	; select data frame 0
	cif_0 = 06202	; select instruction frame 0
	cdif0 = 06203	; select data & instruction frame 0
	cdf_1 = 06211	; select data frame 1
	cif_1 = 06212	; select instruction frame 1
	cdif1 = 06213	; select data & instruction frame 1
	rdf   = 06214	; read (or) data frame into <05:03>
	rmf   = 06244	; restore memory frame

	sys6  = 06306	; system call, clear link if no error, else set link and put errno in ac
	pioff = 06310	; printinstr off
	pion  = 06311	; printinstr on
	hcf   = 06312	; dump registers and memory then exit

	SCN_EXIT        = 0
	SCN_PRINTLN6    = 1
	SCN_PRINTLN8    = 2
	SCN_PRINTLN12   = 3
	SCN_INTREQ      = 4
	SCN_CLOSE       = 5
	SCN_GETNOWNS    = 6
	SCN_OPEN6       = 7
	SCN_OPEN8       = 8
	SCN_OPEN12      = 9
	SCN_READ6       = 10
	SCN_READ8       = 11
	SCN_READ12      = 12
	SCN_WRITE6      = 13
	SCN_WRITE8      = 14
	SCN_WRITE12     = 15
	SCN_IRQATNS     = 16
	SCN_INTACK      = 17
	SCN_UNLINK6     = 18
	SCN_UNLINK8     = 19
	SCN_UNLINK12    = 20
	SCN_GETCMDARG6  = 21
	SCN_GETCMDARG8  = 22
	SCN_GETCMDARG12 = 23
	SCN_ADD_DDD     = 24
	SCN_ADD_FFF     = 25
	SCN_SUB_DDD     = 26
	SCN_SUB_FFF     = 27
	SCN_MUL_DDD     = 28
	SCN_MUL_FFF     = 29
	SCN_DIV_DDD     = 30
	SCN_DIV_FFF     = 31
	SCN_CMP_DD      = 32
	SCN_CMP_FF      = 33
	SCN_CVT_FP      = 34
	SCN_UDIV        = 35
	SCN_UMUL        = 36
	SCN_PRINTINSTR  = 37
	SCN_NEG_DD      = 38
	SCN_NEG_FF      = 39
	SCN_WATCHWRITE  = 40
	SCN_GETENV6     = 41
	SCN_GETENV8     = 42
	SCN_GETENV12    = 43
	SCN_SETTTYECHO  = 44
	SCN_IRQATTS     = 45
	SCN_READLN6     = 46
	SCN_READLN8     = 47
	SCN_READLN12    = 48

	O_RDONLY   = 00000
	O_WRONLY   = 00001
	O_RDWR     = 00002
	O_CREAT    = 00100
	O_EXCL     = 00200
	O_NOCTTY   = 00400
	O_TRUNC    = 01000
	O_APPEND   = 02000
	O_NONBLOCK = 04000



	. = 00000
	.word	.-.
	jmpi	.+1
	.word	intserv

_0002:	.word	00002
_0003:	.word	00003
_0004:	.word	00004
_0005:	.word	00005
_0006:	.word	00006

	. = 00010
ai10:	.word	.-.
ai11:	.word	.-.
ai12:	.word	.-.
ai13:	.word	.-.
ai14:	.word	.-.
ai15:	.word	.-.
ai16:	.word	.-.
ai17:	.word	.-.

_0007:	.word	00007
_0010:	.word	00010
_0012:	.word	00012
_0015:	.word	00015
_0016:	.word	00016
_0017:	.word	00017
_0037:	.word	00037
_0040:	.word	00040
_0042:	.word	00042
_0043:	.word	00043
_0050:	.word	00050
_0051:	.word	00051
_0055:	.word	00055
_0060:	.word	00060
_0070:	.word	00070
_0072:	.word	00072
_0077:	.word	00077
_0106:	.word	00106
_0125:	.word	00125
_0137:	.word	00137
_0140:	.word	00140
_0177:	.word	00177
_0377:	.word	00377
_1777:	.word	01777
_3777:	.word	03777
_4000:	.word	04000
_6201:	.word	06201
_7640:	.word	07640
_7641:	.word	07641
_7644:	.word	07644
_7706:	.word	07706
_7735:	.word	07735
_7736:	.word	07736
_7740:	.word	07740
_7750:	.word	07750
_7757:	.word	07757
_7763:	.word	07763
_7764:	.word	07764
_7765:	.word	07765
_7766:	.word	07766
_7770:	.word	07770
_7771:	.word	07771
_7772:	.word	07772
_7773:	.word	07773
_7774:	.word	07774
_7775:	.word	07775
_7776:	.word	07776
_7777:	.word	07777

_access:	.word	access
_accai10:	.word	accai10
_addsyment:	.word	addsyment
_allrestok:	.word	allrestok
_checklast:	.word	checklast
_checkmiss:	.word	checkmiss
_evalerr:	.word	evalerr
_evalret:	.word	evalret
_evaluate:	.word	evaluate
_falsetok:	.word	falsetok|(falsetok/4096)
_fatal:		.word	fatal
_free:		.word	free
_garbcoll:	.word	garbcoll
_kbbuff:	.word	kbbuff
_kbread:	.word	kbread
_l2malloc:	.word	l2malloc
_malloc:	.word	malloc
_memck:		.word	memck
_nextelem:	.word	nextelem
_nexteval:	.word	nexteval
_nextint:	.word	nextint
_nextstr:	.word	nextstr
_popcp:		.word	popcp
_popda:		.word	popda
_pushcp:	.word	pushcp
_pushda:	.word	pushda
_retfalse:	.word	retfalse
_retnull:	.word	retnull
_retsint12:	.word	retsint12
_tokprint:	.word	tokprint
_ttprint12z:	.word	ttprint12z
_ttprintac:	.word	ttprintac
_ttprintcrlf:	.word	ttprintcrlf
_ttprintint:	.word	ttprintint
_ttprintoct4:	.word	ttprintoct4
_ttprintwait:	.word	ttprintwait
_udiv12:        .word	udiv12
_udiv24:        .word	udiv24
_umul24:        .word	umul24
_wfint:		.word	wfint

umul_mcand: .blkw 2		; multiplicand on input, scratch on output
umul_prod:  .blkw 2		; product added to input value
udiv_dvquo: .blkw 2		; dividend on input, quotient on output
udiv_remdr: .blkw 2		; remainder on output

addsymfrm: .blkw 1		; frame symbol is added to
addsymnam: .blkw 1		; name node being added
addsymval: .blkw 1		; value for the symbol
evlnext:   .blkw 1		; next argument token to function
evaltok:   .blkw 1		; token being evaluated or result of evaluation
mall2szm2: .blkw 1		; log2(size+1)-2 being mallocd/freed
symframe:  .blkw 1		; innermost symbol frame that is currently active
tknext:    .blkw 1		; pointer to next char to be processed
tkspos:    .blkw 2		; source position (little endian)
tokfirst:  .blkw 1		; list element of first token parsed
toklast:   .blkw 1		; list element of last token parsed
toklists:  .blkw 1		; stack of lists being built
toknparn:  .blkw 1		; number of lists being built



	.align	00200

;   initialize
; closeloop: <= jump here on error or control-C
;   close all load files
; mainloop:
;   clear token parsing, clear stack, reset to top-level symbol frame
; readloop:
;   if readlnbuffd >= 0
;     read line from file
;     if not eof
;       null terminate line
;       maybe echo line to output
;       goto maintoke
;     readeof:
;       close load file
;       goto readloop
;   readnofile:
;     if hascmdline, goto dosysexit
;     try to get command line
;     if successful
;       set hascmdline
;       goto maintoke
;     nocmdline:
;       read line from keyboard
; maintoke:
;   tokenize line
; evalloop:
;   if tokfirst not null,
;     pop from tokfirst
;     evaluate
;     print result
;     goto evalloop
;   goto mainloop

	.global	__boot
__boot:
	initcif
	jmpi	_initialize
initret:

closeloop:			; <= jump here on error or control-C
	jmsi	_closeload	; close all open load files
	sma cla
	jmp	closeloop

mainloop:
	dca	tokfirst	; nothing being tokenized
	dca	toklast
	dca	toklists
	dca	toknparn
	jmsi	_resetstack	; reset stack
	tad	topsymframe	; reset symbol frame to top-level static frame
	dca	symframe

readloop:
	kbbuffcdf
	tadi	_readlnbuffdb	; maybe reading from load file
	spa cla
	jmp	readnofile
	tad	_readlnbuff
	sys6
	szl*sna
	jmp	readloadeof
	tad	_kbbuffm1	; null terminate
	dca	ai10
	dcai	ai10
	isz	lineno
	skp
	isz	lineno
.if OP_ECHOLD
	jms	printprompt	; echo load file line
	tad	_0072		; - print a ':'
	jmsi	_ttprintac
	kbbuffcdf		; - print keyboard buffer with \n on end
	tad	_kbbuffm1
	iac
	jmsi	_ttprint12z
.endif
	jmp	maintoke	; tokenize the line

readloadeof:
	jmsi	_closeload	; end of load file, close it
	jmp	readloop	; read line from outer file or keyboard

readnofile:
	tad	hascmdline	; don't do command line twice
	sza cla
	jmp	dosysexit
	tad	_getcmdarg	; maybe passed string on raspictl command line
	sys6
	szl
	jmp	nocmdline
	isz	hascmdline
	jmp	maintoke
nocmdline:
	cla			; no command line arg, read from keyboard
	isz	lineno
	skp
	isz	lineno
	jms	printprompt
	tad	_0137		; print an '_'
	jmsi	_ttprintac
	jmsi	_kbread		; read line from keyboard

maintoke:
	cla			; tokenize the line
	tad	lineno
	dca	tkspos+1
	jmsi	_tokenize
	tad	toknparn	; if open parentheses, read continuation line
	sza cla
	jmp	readloop

evalloop:
	tad	tokfirst	; see if anything more to evaluate
	sna
	jmp	mainloop
	jmsi	_accai10	; access list element memory
	tadi	ai10		;L_NEXT=0 : get pointer to next element
	dca	tokfirst
	tadi	ai10		;L_TOKN=1 : get pointer to token to evaluate
	jmsi	_evaluate	; evaluate the token
.if OP_GCTEST
	dca	mainresult
	jmsi	_garbcoll	; garbage collect
	tad	mainresult
.endif
	jmsi	_tokprint	; print evaluated token value
	jmp	evalloop	; repeat for all tokens parsed

dosysexit:
	jmsi	_ttprintwait	; - command line test, exit
	cdf_0
	tad	_sysexit
	sys6
	jmp	dosysexit

; print prompt string <lineno>[.<nparens>]
printprompt: .word .-.
	tad	lineno		; - print line number
	jmsi	_ttprintoct4
	tad	toknparn	; see if any unclosed parens
	sna cla
	jmpi	printprompt
	tad	_0056		; print a '.'
	jmsi	_ttprintac
	tad	toknparn	; print number of unclosed parens
	jmsi	_ttprintoct4
	jmpi	printprompt

_0056:	.word	00056
_initialize: .word initialize
_tokenize:   .word tokenize

_closeload:    .word closeload
_kbbuffm1:     .word kbbuff-1
_getcmdarg:    .word getcmdarg
_readlnbuff:   .word readlnbuff
_readlnbuffdb: .word readlnbuffd
_resetstack:   .word resetstack

_sysexit: .word	.+1
	.word	SCN_EXIT
	.word	0

hascmdline:  .word 0
lineno:	     .word 0
mainresult:  .word .-.
topsymframe: .word .-.



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (& integers ...)
f_amper:
	tad	faaccum+0	; save in case of recursion
	jmsi	_pushda
	tad	faaccum+1
	jmsi	_pushda
	cma			; clear accumulator
	dca	faaccum+0
	cma
faloop:
	dca	faaccum+1
	jmsi	_nextint	; get next int argument
	sza cla
	jmp	fadone
	tadi	ai10
	and	faaccum+0	; and with accumulator
	dca	faaccum+0
	tadi	ai10
	and	faaccum+1
	jmp	faloop
fadone:
	tad	_0005		;T_VALU=3
	jmsi	_allrestok	; allocate result token
		.word	TT_INT
	tad	faaccum+0	; T_VALU = 24-bit signed integer
	dcai	ai10
	tad	faaccum+1
	dcai	ai10
	jmsi	_popda		; restore outer accumulator value
	dca	faaccum+1
	jmsi	_popda
	dca	faaccum+0
	jmpi	_evalret	; return this summation token in evaltok

faaccum: .blkw	2

f_tilde:
	tad	faaccum+0	; save in case of recursion
	jmsi	_pushda
	tad	faaccum+1
	jmsi	_pushda
	jmsi	_checkmiss
	jmsi	_nextint	; get integer argument
	tadi	ai10
	cma
	dca	faaccum+0
	tadi	ai10
	cma
	dca	faaccum+1
	jmsi	_checklast
	jmp	fadone

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (| integers ...)
f_orbar:
	tad	faaccum+0	; save in case of recursion
	jmsi	_pushda
	tad	faaccum+1
	jmsi	_pushda
	cma			; clear accumulator
	dca	faaccum+0
	cma
foloop:
	dca	faaccum+1
	jmsi	_nextint	; get next int argument
	sza cla
	jmp	fodone
	tadi	ai10
	cma			; or with accumulator
	and	faaccum+0
	dca	faaccum+0
	tadi	ai10
	cma
	and	faaccum+1
	jmp	foloop
fodone:
	tad	faaccum+0
	cma
	dca	faaccum+0
	tad	faaccum+1
	cma
	dca	faaccum+1
	jmp	fadone

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; tokens
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	T_TYPE = 0
		TT_NUL = 0	; invisible no value
		TT_INT = 1	; T_VALU = 24-bit int (little endian)
		TT_LIS = 2	; T_VALU = points to first list element
		TT_STR = 3	; T_VALU = 12-bit counted string
		TT_SYM = 4	; T_VALU[0] = frame index; [1] = sym index; [2] = name entry composite pointer
		TT_FUN = 5	; T_VALU[0] = entrypoint; T_VALU[1...] = varies depending on T_VALU[0] function
		TT_BOO = 6	; T_VALU = 0: #False; = 1: #True
		TT_CHR = 7	; T_VALU = 12-bit character
	T_SPOS = 1		; 24-bit source position
	T_VALU = 3		; depends on T_TYPE

	; list elements

	L_NEXT = 0		; next in list or null
	L_TOKN = 1		; value token (never null)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

tkbeg:	.word	kbbuff		; input string pointer

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; tokenize string just read in from keyboard or load file
;  input:
;   tkbeg = address in kbbuff to start at
;           points to null-terminated 12-bit ascii characters
;   kbbuffcdf = cdf instr for tkbeg
;   tkspos = source position
;   toklast = last token parsed or 0 if none yet
;  output:
;   ac = zero
;   tokfirst,toklast = updated
;  scratch:
;   ai10, ai11, ai12
tokenize: .word .-.
	cla
	tad	tkbeg
	dca	tknext
tok_loop:
	kbbuffcdf
tok_loop2:
	cla
	tadi	tknext		; get next char from kb buffer
	sna
	jmpi	tokenize	; done if end of input
	isz	tknext
	tad	_7740		; -040 skip whitespace
	spa*sna
	jmp	tok_loop2
	tad	_7776		; 040-042 check for '"'
	sna
	jmpi	_tok_quote
	tad	_7777		; 042-043 check for '#'
	sna
	jmpi	_tok_hash
	tad	_7773		; 043-050 check for '('
	sna
	jmpi	_tok_oparen
	tad	_7777		; 050-051 check for ')'
	sna
	jmpi	_tok_cparen
	tad	_7774		; 051-055 check for '-'
	sna
	jmp	tok_hyphen
	tad	_7775		; 051-060 check for digits 0..9
	spa
	jmpi	_tok_symbol
	tad	_7766		; 060-072
	spa
	jmpi	_tok_digit
	tad	_7777		; 072-073 check for ';'
	sna
	jmpi	tokenize	; all done if comment
	jmpi	_tok_symbol	; otherwise it is a symbol

	; hyphen, if followed a digit, it is a negative number
	;                   otherwise, it is a symbol
tok_hyphen:
	tadi	tknext		; get next char, maybe 0..9 060..071
	tad	_7706		; now 7766..7777
	cll
	tad	_0012		; now 0000..0011
	szl
	jmpi	_tok_digit
	jmpi	_tok_symbol

_tok_cparen: .word tok_cparen
_tok_digit:  .word tok_digit
_tok_hash:   .word tok_hash
_tok_oparen: .word tok_oparen
_tok_quote:  .word tok_quote
_tok_symbol: .word tok_symbol

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; check memory integrity
memck:	.word	.-.
	cif_1
	jmpi	.+1
	.word	memck0
memck0ret:
	jmpi	memck



	.align	00200

	; parse character token
	;  #\c

tok_hash:
	tadi	tknext
	tad	_7644		; -0134 : check for '\'
	sza cla
	jmp	tok_symbol
	isz	tknext
	tadi	tknext		; get char following '\'
	sna
	jmpi	_tok_loopb
	isz	tknext
	dca	tksymlen	; save as token character
	tad	tksymlen
	tad	_7735		; -043 check for '#'
	sza cla
	jmp	tokhashstore
	tadi	tknext		; get char following 2nd '#'
	sna
	jmpi	_tok_loopb
	isz	tknext
	and	_0077		; map 100..177 to 000..077; leave 000..077 as is
				; eg, #" -> "   ## -> #   #@ -> NUL
				;     #J -> LF  #M -> CR  #[ -> ESC
	dca	tksymlen
tokhashstore:
	tad	_0004		;T_VALU=3 : room for 12-bit char
	jmsi	_tokappend
		.word	TT_CHR
	tad	tksymlen	; copy 22-bit char to token
	dcai	ai10
	jmpi	_tok_loopb

	; parse symbol token

tok_symbol:
	cla			; initialize name string length
	dca	tksymlen
tok_symcount:
	isz	tksymlen
	tadi	tknext		; get next character
	tad	_7740		; -040 end-of-line or control characters or space
	spa*sna
	jmp	tok_symcounted
	tad	_7776		; 040-042 check for '"'
	sna
	jmp	tok_symcounted
	tad	_7771		; 042-051 check for '('
	sna
	jmp	tok_symcounted
	tad	_7777		; 051-052 check for ')'
	sna
	jmp	tok_symcounted
	tad	_7757		; 052-073 check for ';'
	sna cla
	jmp	tok_symcounted
	isz	tknext
	jmp	tok_symcount
tok_symcounted:

	; tknext = points to terminating character
	; tksymlen = number of characters of symbol

	cla
	tad	tksymlen	; back tknext up to beginning of symbol
	cma iac
	tad	tknext		; ... so tokappend will have correct source location
	dca	tknext

	cma			; point to just before string
	tad	tknext
	dca	ai11
	tad	tksymlen	; get string length
	jmsi	_makenament	; make nametable entry
	dca	tksymnam	; save that as the symbol name

	tad	_0006		;T_VALU=3 : allocate symbol token
	jmsi	_tokappend	; append to token list
		.word	TT_SYM
	dcai	ai10		; T_VALU[0] = frame index (0=unknown, filled in with first lookup)
	dcai	ai10		; T_VALU[1] = entry index (0=unknown, filled in with first lookup)
	tad	tksymnam	; T_VALU[2] = name pointer
	dcai	ai10
	tad	tknext		; point to char after symbol
	tad	tksymlen
	dca	tknext
	jmpi	_tok_loopb	; process next token in input string

tksymlen:    .word .-.
tksymnam:    .word .-.
_tokappend:  .word tokappend
_tok_loopb:  .word tok_loop
_makenament: .word makenament

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; clear symbol frame/index links
;  input:
;   ac = composite pointer to token
;  output:
;   ac = zero
;  scratch:
;   ai10,df
clearsyms: .word .-.
	jmsi	_accai10	; access token memory
	tadi	ai10		;T_TYPE=0 : get token type
	isz	ai10		;T_VALU=3 : point just before type
	isz	ai10
	tad	_7774		;TT_SYM=4 : check for symbol
	sza
	jmp	csnotsym
	dcai	ai10		; clear out symbol frame and index
	dcai	ai10
	jmpi	clearsyms
csnotsym:
	tad	_0002		;TT_LIS=2 : check for list
	sza cla
	jmpi	clearsyms
	tad	clearsyms	; if so, save return address
	jmsi	_pushda
	tad	cstoken
	jmsi	_pushcp
	tadi	ai10		; point to first list element
csloop:
	sna			; done if end of list
	jmp	csdone
	jmsi	_accai10	; access list rlement
	tadi	ai10		;L_NEXT=0 : get pointer to next list element (or null)
	dca	cstoken
	tadi	ai10		;L_TOKN=1 : get pointer to token
	jms	clearsyms	; clear its symbols
	tad	cstoken		; process next in list
	jmp	csloop
csdone:
	jmsi	_popcp
	dca	cstoken
	jmsi	_popda
	dca	clearsyms
	jmpi	clearsyms

cstoken: .word	.-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; gc has a block to free, ac = composite pointer to block
free1:
	jmsi	_free		; free the block
	cif_1			; back to the collector
	jmpi	.+1
	.word	free1ret



	.align	00200

	; parse quoted string
	; tknext = points just past opening quote

tok_quote:
	cla cma
	tad	tknext		; point to first quote
	dca	ai11		; -> use this to read from buffer
	tad	ai11
	dca	ai12		; -> use this to write to buffer (after processing escapes)
tok_quotecount:
	tadi	ai11		; get character from input
	sna			; check for end of input
	jmp	tok_quoteend
	tad	_7736		; -042 check for ending quote
	sna
	jmp	tok_quoteend
	tad	_7777		; 042-043 check for '#' (escape)
	sna
	jmp	tok_quoteesc
	tad	_0043		; restore original char
tok_quotesave:
	dcai	ai12		; store in output overlaying input
	jmp	tok_quotecount
tok_quoteesc:
	tadi	ai11		; '#', get next char
	sna			; check for end of input
	jmp	tok_quoteend
	and	_0077		; map 100..177 to 000..077; leave 000..077 as is
				; eg, #" -> "   ## -> #   #@ -> NUL
				;     #J -> LF  #M -> CR  #[ -> ESC
	jmp	tok_quotesave

	; tknext = points just past opening quote
	; ai11 = points to terminating char in buffer (null or quote)
	; ai12 = points to last char of string in buffer with escapes processed

tok_quoteend:
	tad	tknext		; compute number of chars in string
	cma iac
	iac
	tad	ai12
	dca	tkstrlen
	cla cma			; point ai12 just before first char of de-escaped strong
	tad	tknext
	dca	ai12
	tad	ai11		; point tknext to start of remaining chars in input buffer
	dca	tknext
	tadi	tknext
	sza cla			; don't skip over null terminator
	isz	tknext		; skip over quote terminator

	; ai12 = points just before where de-escaped string is
	; tknext = points just past the closing quote
	; tkstrlen = number of de-escaped chars pointed to by ai12

	tad	tkstrlen	; get string length
	tad	_0004		;T_VALU=3 : add room for token header including T_VALU[0] to hold char count
	jmsi	_tokappendb	; allocate new token and append to output
		.word	TT_STR
	tad	tkstrlen	; save char count in T_VALU
	dcai	ai10		; packed char words follow char count word

	tad	_6201		; set up cdf to access token
	rdf
	dca	tok_quotetokcdf

	; ai10 = points to T_VALU entry in token = where to write packed char words

	tad	tkstrlen	; flip tkstrlen for isz
	sna
	jmpi	_tok_loop	; - zero, all done
	cma iac
	dca	tkstrlen
tok_quotepkloop:
	kbbuffcdf
	tadi	ai12		; get char
tok_quotetokcdf: .word .-.	; access token memory
	dcai	ai10		; store char
	isz	tkstrlen
	jmp	tok_quotepkloop
	jmpi	_tok_loop

_tokappendb: .word tokappend
_tok_loop: .word tok_loop
tkstrlen: .word .-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (strcmp str1 str2) => integer
; returns < 0: str1 < str2; = 0: str1 = str2; > 0: str1 > str2
f_strcmp:
	tad	scnedlen
	jmsi	_pushda
	tad	scnedptr
	jmsi	_pushda

	jmsi	_checkmiss
	jmsi	_nextstr	; get needle string
	tadi	ai10
	dca	scnedlen	; save needle length
	tad	ai10		; save needle pointer
	dca	scnedptr
	tad	_6201		; save needle data frame
	rdf
	dca	scnedcdf
	tad	evaltok		; save needle token so it can't get gc'd
	jmsi	_pushcp
	jmsi	_checkmiss
	jmsi	_nextstr	; get haystack string
	tadi	ai10
	dca	schaylen	; save haystack length
	tad	_6201		; save haystack data frame
	rdf
	dcai	schaycdf
	jmsi	_checklast	; no more arguments allowed
	jmsi	_popcp		; pop off saved needle token
	cla			; ...now that we don't do any more mallocs

	tad	schaylen	; compute needle length - haystack length
	cma iac
	tad	scnedlen
	dca	cmpdiff

	tad	cmpdiff		; get minimum of the two lengths
	spa cla
	tad	cmpdiff
	tad	schaylen
	sna
	jmp	strcmpdonez
	cma iac			; negative for isz
	dca	scminlen

	tad	scnedptr
	dca	ai11

	; cmpdiff = strlen(str1) - strlen(str2)
	; ai10 = points to str2
	; ai11 = points to str1

strcmploop:
schaycdf: .word .-.
	tadi	ai10		; read character from str2
	cma iac
scnedcdf: .word .-.
	tadi	ai11		; read character from str1
	sza
	jmp	strcmpdone	; stop if chars different
	isz	scminlen	; repeat if chars the same
	jmp	strcmploop
	jmp	strcmpdonez
strcmpdone:
	dca	cmpdiff		; chars different, save difference
strcmpdonez:
	jmsi	_popda		; restore recursion values
	dca	scnedptr
	jmsi	_popda
	dca	scnedlen
	tad	cmpdiff		; return difference
	jmpi	_retsint12

schaylen: .word .-.
scminlen: .word	.-.
scnedlen: .word	.-.
scnedptr: .word	.-.
cmpdiff:  .word	.-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (char integer)
f_char:
	jmsi	_checkmiss
	jmsi	_nextint
	tadi	ai10
	jmpi	.+1
	.word	retchar



	.align	00200

	; handle open paren by creating a list

tok_oparen:
	tad	_0006		;T_VALU=3 : room for pointer to list and two extras
	jmsi	_tokappendc
		.word	TT_LIS
	dcai	ai10		; clear T_VALU[0], ie, first element of list cuz we don't have it yet
	tad	toklists	; save outer list elements pointer in T_VALU[1]
	dcai	ai10
	tad	tokfirst	; save beginning of outer list elements pointer in T_VALU[2]
	dcai	ai10
	tad	toklast		; this list is now the list being built
	dca	toklists
	dca	tokfirst	; this list doesn't have any elements yet
	dca	toklast
	isz	toknparn
	jmpi	toloop

	; handle close paren by closing the list

tok_cparen:
	tad	toklists	; get list being closed
	sna
	jmp	tcexcess
	dca	toklast		; it is what next parsed token will be appended to
	tad	toklast		; access list element of list token being closed
	jmsi	_access
	dca	ai10		;L_TOKN=1 : get pointer to TT_LIS token created by tok_oparen
	tadi	ai10
	jmsi	_access		; access the token
	tad	_0002		;T_VALU=3 : point to just before value area
	dca	ai10
	tad	tokfirst	; save pointer to first list element in T_VALU[0]
	dcai	ai10
	tadi	ai10		; restore outer list elements pointer from T_VALU[1]
	dca	toklists
	tadi	ai10		; restore first list element pointer from T_VALU[2]
	dca	tokfirst
	cma
	tad	toknparn
	dca	toknparn
	jmpi	toloop

tcexcess:
	dca	evaltok
	tad	_tcxmsg
	jmpi	_evalerr

toloop:	.word	tok_loop
_tcxmsg: .word	tcxmsg

	; parse integer token

tok_digit:
	cla			; set up ai11 to point just before string
	tad	tknext
	tad	_7776
	dca	ai10
	cma			; convert until hit non-digit
	jms	strtoint	; convert string to integer
	cla
	tad	ai10		; update buffer pointer to terminating char
	dca	tknext
	tad	_0005		;T_VALU=3 : room for two-word integer
	jmsi	_tokappendc
		.word	TT_INT
	tad	umul_prod+0	; copy two-word integer to token
	dcai	ai10
	tad	umul_prod+1
	dcai	ai10
	jmpi	toloop

_tokappendc: .word tokappend

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; convert string to integer
;  input:
;   ac = max num chars to convert
;   df,ai10 = points just before first char
;  output:
;   ac = neg num unconverted chars (0 means all chars converted)
;   umul_prod = resultant signed integer
;   df,ai10 = points to terminating char (unless ac=0, then it points to char before)
strtoint: .word .-.
	cma			; save 1s comp of num chars to process
	dca	tdcount
	dca	umul_prod+0	; zero output result to begin with
	dca	umul_prod+1
	dca	tdnegfl		; assume no minus sign
	tad	_0012		; assume decimal base
	dca	tdbase
	jms	tdgetch		; see if first char is a '-'
	tad	_7723		; -055 : '-'
	sza
	jmp	tdnotneg
	cma			; if so, set neg flag
	dca	tdnegfl
	jms	tdgetch		; ... and get a new char
	jmp	tdckoctal
tdnotneg:
	tad	_0055		; not a '-', restore char
tdckoctal:
	tad	_7720		; check for leading zero
	sna
	jmp	tduseoctal
	tad	_0060		; no leading zero, restore leading digit
	jmp	tdgotch
tduseoctal:
	tad	_0010		; leading zero, change to octal base
	dca	tdbase
tdloop:
	jms	tdgetch		; get next char from input
tdgotch:
	tad	_7706		; -072 check for digits 0..9
	cll
	tad	_0012		; only 0..9 should flip the link bit
	snl
	jmp	tddone
	dca	tddigit
	tad	umul_prod+0	; get ready to multiply accumulated value by 10
	dca	umul_mcand+0
	tad	umul_prod+1
	dca	umul_mcand+1
	tad	tddigit		; insert digit into bottom of accumulated value
	dca	umul_prod+0
	dca	umul_prod+1
	tad	tdbase
	jmsi	_umul24
	jmp	tdloop		; try to get another digit
tddone:
	isz	tdnegfl		; see if '-' on front of number
	jmp	tdstore
	cla cll			; if so, negate the integer
	tad	umul_prod+0
	cma iac
	dca	umul_prod+0
	tad	umul_prod+1
	cma
	szl
	iac
	dca	umul_prod+1
tdstore:
	cla
	tad	tdcount		; return neg num chars left over
	jmpi	strtoint

tdgetch: .word	.-.		; get next char from input
	isz	tdcount		; make sure there is a char to get
	skp
	jmp	tddone		; stop if all used up
	tadi	ai10		; get the character
	jmpi	tdgetch		; process it

_7720:	.word	07720
_7723:	.word	07723
tddigit: .word	.-.
tdnegfl: .word	.-.
tdcount: .word	.-.
tdbase:  .word	.-.



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; allocate and append token to end of token list
;  input:
;   ac = number of words for token
;   toklast = composite pointer to link element (or null if this is first element)
;   tknext = points to beginning of token string in tkbeg buffer
;   tkspos = source position of beginning of tkbeg buffer
;   retadr+0 = token type TT_whatever
;  output:
;   ac = 0
;   df = maps the token
;   ai10 = just before T_VALU in the token
;   token = composite pointer to new token
;   toklast = composite pointer to new list element
tokappend: .word .-.
	jmsi	_malloc		; allocate token
	dca	token		; save composite pointer
	cla iac			;L_SIZE=2 : allocate a list element struct
	jmsi	_l2malloc
	dca	listel		; save composite pointer

	tad	toklast		; link to last token's list element
	sna
	jmp	tka_first
	jmsi	_access		; access last token's list element
	dca	tkatemp1
	tad	listel
	dcai	tkatemp1	;L_NEXT=0 : set to new list element
	jmp	tka_last
tka_first:
	tad	listel		; - no last token, this is the first
	dca	tokfirst
tka_last:
	tad	listel		; save new list element as the last element in the list so far
	dca	toklast
	tad	listel		; access new list element memory
	jmsi	_accai10
	dcai	ai10		;L_NEXT=0 : set link to next elment = null cuz this is the new last element
	tad	token		;L_TOKN=1 : set link to this element's token
	dcai	ai10
	cdf_0
	tadi	_tkbeg		; get offset of token in the current source line
	cll cml cma iac
	tad	tknext		; get token's position in source
	tad	tkspos+0
	dca	toksrcpos+0
	ral
	tad	tkspos+1
	dca	toksrcpos+1
	tadi	tokappend	; get token type
	dca	tkatype
	isz	tokappend
	tad	token
	jmsi	_accai10	; access token memory
	tad	tkatype		;T_TYPE=0 : set to given type
	dcai	ai10
	tad	toksrcpos+0	;T_SPOS=1 : set to token's position in source
	dcai	ai10
	tad	toksrcpos+1
	dcai	ai10
	jmpi	tokappend	;T_VALU=3 : follows T_SPOS

listel:	.word	.-.
token:	.word	.-.
toksrcpos: .blkw 2
tkatype: .word	.-.
tkatemp1: .word	.-.
_tkbeg:	.word	tkbeg

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; access memory
;  input:
;   ac<11:03> = address (8-word aligned)
;   ac<02:00> = data frame
;  output:
;   data frame set
;   ac<11:03> = address (8-word aligned)
;   ac<02:00> = cleared
access:	.word .-.
	dca	access_temp
	tad	access_temp
	ral
	rtl
	and	_0070
	tad	_6201	; cdf_0
	dca	.+1
	.word .-.
	tad	access_temp
	and	_7770
	jmpi	access

access_temp:	.word .-.

; same, except point ai10 just before first word
; returns with ac = zero
accai10: .word	.-.
	dca	access_temp
	tad	access_temp
	ral
	rtl
	and	_0070
	tad	_6201	; cdf_0
	dca	.+1
	.word .-.
	tad	access_temp
	and	_7770
	tad	_7777
	dca	ai10
	jmpi	accai10

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (_ integers ...)
; exclusive-or integers
f_under:
	tad	fxaccum+0	; save in case of recursion
	jmsi	_pushda
	tad	fxaccum+1
	jmsi	_pushda
	dca	fxaccum+0	; clear accumulator
	dca	fxaccum+1
fcarloop:
	jmsi	_nextint	; get next int argument
	sza cla
	jmp	fcardone
	tadi	ai10
	dca	fyaccum+0
	tadi	ai10
	dca	fyaccum+1

	tad	fxaccum+0	; xor into fxaccum
	and	fyaccum+0
	cma iac
	cll ral
	tad	fxaccum+0
	tad	fyaccum+0
	dca	fxaccum+0
	tad	fxaccum+1
	and	fyaccum+1
	cma iac
	cll ral
	tad	fxaccum+1
	tad	fyaccum+1
	dca	fxaccum+1
	jmp	fcarloop
fcardone:
	tad	_0005		;T_VALU=3
	jmsi	_allrestok	; allocate result token
		.word	TT_INT
	tad	fxaccum+0	; T_VALU = 24-bit signed integer
	dcai	ai10
	tad	fxaccum+1
	dcai	ai10
	jmsi	_popda		; restore outer accumulator value
	dca	fxaccum+1
	jmsi	_popda
	dca	fxaccum+0
	jmpi	_evalret	; return this summation token in evaltok

fxaccum: .blkw	2
fyaccum: .blkw	2



	.align	00200

	; name nodes linked to nametable

	N_NXT = 0	; composite pointer to next name entry
	N_LEN = 1	; length of string (characters)
	N_STR = 2	; first pair of characters

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; make name table entry for the given symbol
; use existing entry if name already in table
;  input:
;   ac = name string length (assumed gt 0)
;   df,ai11 = points just before 12-bit ascii string
;  output:
;   ac = name entry composite pointer
;  scratch:
;   ai10, ai11, ai12, df
makenament: .word .-.
	dca	namelen		; save name string length
	tad	_6201		; set up cdf to access string
	rdf
	dca	makenameincdf

	cla iac
	tad	namelen		; make negative length of char pairs + 1 for length word
	cll cml cma rar
	dca	negcmpwds	;  1 -> -2; 2 -> -2; 3 -> -3; 4 -> -3; 5 -> -4; 6 -> -4; 7 -> -5; 8 -> -5; ...

	; create a name node in case we need it

	tad	namelen		; malloc name entry
	cll iac rar
	tad	_0002		;N_STR=2
	jmsi	_malloc
	dca	namecpt		; save composite pointer
	tad	namecpt		; access its memory
	jmsi	_access
	dca	ai12
	tad	ai12		;N_NXT=0
	dca	namenxtptr	; save pointer to N_NXT word
	tad	namelen		; save name string length
	dcai	ai12		;N_LEN=1
	tad	_6201		; save name node data frame
	rdf
	dca	makenamecdf1
	tad	namelen
makenamepackloop:
	tad	_7776		; subtract 2 from name length
	dca	namelen		; - gos neg if just 1 char left
makenameincdf: .word .-.
	jms	tok_symget6bit	; convert first char to six-bit code
	bsw			; save in nametmp<11:06>
	dca	nameword
	tad	namelen		; see if there is second char
	sma cla			; if not, use zeroes
	jms	tok_symget6bit	; if so, convert it to sixbit in ac<05:00>
	tad	nameword	; merge with first char
makenamecdf1: .word .-.
	dcai	ai12		;N_STR=2 : store pair in name node
	tad	namelen		; see if any more to do
	sma+sza
	jmp	makenamepackloop

	; search existing names for duplicate

	cla
	tad	makenamecdf1
	dca	makenamecdf2
	tad	nametable
makenamefindloop:
	sna
	jmp	makenamenotfound
	dca	namefindcpt	; save composite pointer to entry being checked
	tad	namefindcpt
	jmsi	_accai10	; access entry memory
	tadi	ai10		;N_NXT=0 : save pointer to next entry
	dca	namefindnext
	tad	_6201		; set up name entry data frame
	rdf
	dca	makenamefindcdf
	tad	negcmpwds	; get neg num words to compare
	dca	namelen		; (odd char is always zero-padded)
	tad	namenxtptr	;N_NXT=0,N_LEN=1,N_STR=2 : point just before name length, string
	dca	ai12
makenamecmploop:
makenamefindcdf: .word .-.	; access existing entry memory
	tadi	ai10		; read existing entry word
	cma iac			; negate for compare
makenamecdf2: .word .-.		; access new node memory
	tadi	ai12		; see if it matches
	sza cla
	jmp	makenamefindnext ; if not, check next table entry
	isz	namelen		; if so, see if more words to check
	jmp	makenamecmploop

	tad	namecpt		; a name already exists, free off new one and use old as is
	jmsi	_free
	tad	namefindcpt
	jmpi	makenament

makenamefindnext:
	tad	namefindnext	; this entry diesn't match, try next entry
	jmp	makenamefindloop

makenamenotfound:
	tad	nametable	; not found, link new entry on front of table
	dcai	namenxtptr
	tad	namecpt
	dca	nametable
.if OP_MAKNAM
	cdf_1
	tad	_mnemsg1
	jmsi	_ttprint12z
	tad	namecpt
	jmsi	_ttprintoct4
	tad	_0040
	jmsi	_ttprintac
	tad	namecpt
	jmsi	_printnameb
	jmsi	_ttprintcrlf
.endif
	tad	namecpt		; return new entry
	jmpi	makenament

namecpt:      .word .-.
namefindcpt:  .word .-.
namefindnext: .word .-.
namelen:      .word .-.
namenxtptr:   .word .-.
nametable:    .word .-.
nameword:     .word .-.
negcmpwds:    .word .-.

.if OP_MAKNAM
_mnemsg1:     .word mnemsg1
_printnameb:  .word printname
.endif

makenament1:
	jms	makenament
	cif_1
	jmpi	.+1
	.word	makenament1ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; convert 12-bit ascii char in kbbuff to 6-bit
;  input:
;   ac = zero
;   ai11 = points just before word to get ascii char from
;          assumed to be in range 0040..0177
;   df = frame for ai11
;  output:
;   ac = character converted to 0000..0077
;        space 0040 + 7640 = 7700        = 7700 & 0077 = 0000
;          '0' 0060 + 7640 = 7720        = 7720 & 0077 = 0020
;          '@' 0100 + 7640 = 7740        = 7740 & 0077 = 0040
;	   'A' 0101 + 7640 = 7741        = 7741 & 0077 = 0041
;	   '_' 0137 + 7640 = 7777        = 7777 & 0077 = 0077
;          '`' 0140 + 7640 = 0000 + 0040 = 0040 & 0077 = 0040
;	   'a' 0141 + 7640 = 0001 + 0040 = 0041 & 0077 = 0041
;          '~' 0176 + 7640 = 0036 + 0040 = 0076 & 0077 = 0076
;   ai11 = incremented
tok_symget6bit: .word .-.
	tadi	ai11		; get char in range 0040..0177
	tad	_7640		; convert to range 7700..0037
	sma
	tad	_0040		; fold 0000..0037 down to 7740..7777
	and	_0077		; convert to range 0000..0077
	jmpi	tok_symget6bit



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; evaluate token
;  input:
;   ac = composite pointer to token
;        TT_INT : returned as is
;        TT_LIS : function called, returns function's return value
;        TT_STR : returned as is
;        TT_SYM : symbol's value looked up
;        TT_FUN : returned as is
;        TT_BOO : returned as is
;        TT_CHR : returned as is
;  output:
;   ac = evaltok = composite pointer to evaluated token
;  scratch:
;   ai10, ai11, ai12, df
evaluate: .word .-.
	sna
	jmsi	_fatal		; should never be null
	dca	evaltok
evaltco:
	jmsi	_ckctrlc
	tad	evaltok		; access token memory
	jmsi	_access
	dca	evtemp1		;T_TYPE=0 : get token type
	tadi	evtemp1
	tad	_7770		; 7 types
	cll
	tad	_0007		; 7 types
	snl
	jmsi	_fatal		; T_TYPE is not 1..7
	tad	_evalcase
	dca	evtemp1
	jmpi	evtemp1

_evalcase: .word .+1
	jmp	evalint		; TT_INT = 1  T_VALU = 24-bit int (little endian)
	jmp	evallis		; TT_LIS = 2  T_VALU = points to first element
	jmp	evalstr		; TT_STR = 3  T_VALU = 12-bit counted
	jmp	evalsym		; TT_SYM = 4  T_VALU = 6-bit counted
	jmp	evalfun		; TT_FUN = 5  T_VALU = machine code entrypoint
	jmp	evalboo		; TT_BOO = 6  T_VALU = 0:false; 1:true
	jmp	evalchr		; TT_CHR = 7  T_VALU = 12-bit character

	; booleans, characters, integers, strings and functions evaluate to themselves
evalint:
evalstr:
evalfun:
evalboo:
evalchr:
	tad	evaltok
	jmpi	evaluate

	; symbols do a lookup in the symbol table
evalsym:
	tad	evaltok
	jmsi	_symlookup	; look for symbol in table
	sna
	jmp	evalsymudf
	dca	evaltok		; found, return value token
	tad	evaltok
	jmpi	evaluate
evalsymudf:
	tad	_evalmsg1	; undefined symbol

	; print error message
	;  ac = address of error message in df 1
	;  evaltok = token to show error location in source
evalerr:
	cdf_1
	jmsi	_ttprint12z
	jmsi	_ttprintcrlf
	tad	evaltok		; print token
	sza
	jmsi	_tokprint
	jmpi	_closeloopb	; back to keyboard input

_ckctrlc:    .word ckctrlc
_closeloopb: .word closeloop
_symlookup:  .word symlookup

	; lists do a function call to get the value
	;  ac = zero
	;  evaltok = composite pointer to TT_LIS token
evallis:
	tad	evaluate	;*TCO* save from recursion
	jmsi	_pushda		;*TCO* code way below marked *TCO* depends on this push order
	tad	evlnext
	jmsi	_pushcp
	tad	evllist
	jmsi	_pushcp
	tad	evaltok		; save list token pointer in recursion-guarded variable
	dca	evllist
	tad	evllist		; access list token
	jmsi	_access
	tad	_0003		;T_VALU=3
	dca	evtemp1
	tadi	evtemp1		; get composite pointer to first list element
	sna
	jmp	evlerr1
	jmsi	_accai10	; access the first list element
	tadi	ai10
	dca	evlnext		;L_NEXT=0 : save pointer to second element in list
	tadi	ai10		;L_TOKN=1 : get presumed function token
	jms	evaluate	; ...or something that evaluates to a function
	jmsi	_accai10	; access the presumed TT_FUN token
	tadi	ai10		;T_TYPE=0 : make sure it is a function
	tad	_7773		;TT_FUN=5
	sza
	jmp	evlerr2
	isz	ai10
	isz	ai10
	tadi	ai10		;T_VALU=3 : get function entrypoint
	dca	evtemp1
	jmpi	evtemp1		; call function
				;  success: jumps to evalret with result value token in evaltok
				;    error: jumps to evalerr with error message in ac and token in evaltok
evalret:
	dca	evtemp1		; save tco-style flag (zero: normal; else: tcostyle)
	jmsi	_popcp		; restore recursion
	dca	evllist
	jmsi	_popcp
	dca	evlnext
	jmsi	_popda
	dca	evaluate
	tad	evtemp1		; test tco-style flag
	sza cla
	jmp	evaltco		; tco, evaltok is not evaluated yet
	tad	evaltok		; normal, return value is in evaltok
	jmpi	evaluate
evlerr1:
	tad	_evlmsg1	; empty list doesn't have function
	jmp	evalerr
evlerr2:
	jmsi	_popcp		; first list element isn't a function
	dca	evaltok
	tad	_evlmsg2
	jmp	evalerr

_evalmsg1: .word evalmsg1
_evlmsg1:  .word evlmsg1
_evlmsg2:  .word evlmsg2

evtemp1: .word	.-.
evframe: .word	.-.
evllist: .word	.-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; return signed integer in ac
retsint12:
	dca	evtemp1		; save signed value
	tad	_0005		;T_TYPE=3
	jmsi	_allrestok	; allocate integer token
		.word	TT_INT
	tad	evtemp1		; get low word being returned
	dcai	ai10		; store in token
	tad	evtemp1		; get low word again
	spa cla
	cma			; sign extend to second word
	dcai	ai10		; store in token
	jmp	evalret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (halt integercode)
f_halt:
	jmsi	_nextint
	sna cla
	tadi	ai10
	dca	_sysex+2
	jmsi	_ttprintwait
	cdf_0
	tad	_sysex
	sys6
haltloop:
	cla
	tad	_sysex+2
	iof
	hlt
	jmp	haltloop

_sysex:	.word	.+1
	.word	SCN_EXIT
	.word	.-.



	.align	00200

	SF_OUTER   = 0
	SF_ENTRIES = 1
	SF_SIZE    = 2

	SE_NEXT  = 0
	SE_NAME1 = 1
	SE_VALU1 = 2
	SE_NAME2 = 3
	SE_VALU2 = 4
	SE_NAME3 = 5
	SE_VALU3 = 6
	SE_SIZE  = 7

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; symbol table lookup
;  input:
;   ac = composite pointer to symbol name token (TT_SYM)
;   symframe = symbol frame to start searching in
;  output:
;   ac = 0: symbol not found
;     else: composite pointer to value token
;  scratch:
;   df, ai10, ai11, ai12
symlookup: .word .-.
	dca	slutoken	; save symbol name being looked up
	tad	slutoken	; access symbol name memory
	jmsi	_access
	tad	_0002		;T_VALU=3
	dca	ai10
	tadi	ai10		;T_VALU[0] = symbol frame index
	sza
	jmp	sluknown

	isz	ai10		;T_VALU[1] = skip over entry index
	tadi	ai10		;T_VALU[2] = get name entry composite pointer
	cma iac			; save negative for comparing
	dca	slunegname

	; this is the first time looking for this symbol
	; scan symbol frames and entries for matching name
	; count frames and entries as we scan
	;  then save to symbol for quick lookup next time

	; slunegname = name being looked up
	; symframe   = frames to search

	dca	slufrcnt	; reset frame counter
	tad	symframe	; start at current symbol table frame
slufrmloop:
	; ac = composite pointer to symbol frame node (SF_...) to check
	sna			; stop if no more frames to look at
	jmpi	symlookup	; - return with ac = zero
	jmsi	_accai10	; access the symbol table frame
	tadi	ai10		;SF_OUTER=0 : save pointer to next outer frame (or null)
	dca	sluframe
	isz	slufrcnt	; increment frame counter
	dca	sluencnt	; reset entry counter
	tadi	ai10		;SF_ENTRIES=1 : get pointer to first entry for the frame
sluentloop:
	; ac = composite pointer to symbol entry node (SE_...) to check
	sna			; go on to next out frame if no more entries in this frame
	jmp	sluentdone
	jmsi	_accai10	; access the symbol table entry
	tadi	ai10		;SE_NEXT=0 : save pointer to next entry in this frame
	dca	sluentry
	tad	_7775		; three symbols per entry
	dca	slucmpcount
slucmploop:
	; ac   = zero
	; df   = accesses symbol entry node (SE_...)
	; ai10 = just before SE_NAMEn of the symbol entry node
	isz	sluencnt	; increment entry counter
	tadi	ai10		;SE_NAME1=1,SE_NAME2=3,SE_NAME3=5 : get name entry
	sna
	jmp	sluentdone	; - null, no more symbols in this frame
	tad	slunegname	; see if name entry matches what we're looking for
	sna cla
	jmp	slufound	; if so, we're done looking
	; ac   = zero
	; df   = accesses symbol entry node (SE_...)
	; ai10 = just before SE_VALUn of the symbol entry node
	isz	ai10		;SE_VALU1=2,SE_VALU2=4,SE_VALU3=6 : no match, skip over value
	isz	slucmpcount	; see if more names to search in this entry (1, 2 or 3)
	jmp	slucmploop
	tad	sluentry	; no more names in entry, check next entry
	jmp	sluentloop
sluentdone:
	tad	sluframe	; no more entries in frame, check next outer frame
	jmp	slufrmloop
slufound:
	tadi	ai10		;SE_VALU1=2,SE_VALU2=4,SE_VALU3=6 : symbol found, return corresponding value token from SE_VALU1,2,3
	dca	sluentry	; save value token found
	tad	slutoken	; access symbol name token we looked up
	jmsi	_access
	tad	_0002		;T_VALU=3 : access value area
	dca	ai10
	tad	slufrcnt	; T_VALU[0] = frame count
	dcai	ai10
	tad	sluencnt	; T_VALU[1] = entry count
	dcai	ai10
	tad	sluentry	; return value token found
	jmpi	symlookup

	; symbol has been looked up before
	;  T_VALU[0] = index of frame (1 based) starting at symframe
	;  T_VALU[1] = entry number (1 based) within that frame
	; assumes symbols are never removed and new ones only inserted on end
	; ... but does allow value to be mutated

	; ac = symbol frame index (1 = symframe, 2 = next outer, ...)
	; df,ai10 = symbol entry index (1 = first entry, 2 = second entry, ...)
sluknown:
	cma iac			; save negative frame count
	dca	slufrcnt
	tadi	ai10		; save positive entry count
	dca	sluencnt
	tad	symframe	; step through frames
sluknownframeloop:
	sna
	jmsi	_fatal		; - should not run out of frames
	jmsi	_accai10	; access frame memory
	isz	slufrcnt	; see if this is the right frame
	skp
	jmp	sluknownframedone
	tadi	ai10		;SF_OUTER=0 : get outer frame
	jmp	sluknownframeloop
sluknownframedone:
	isz	ai10		;SF_ENTRIES=1 : get composite pointer to first symbol entry node
	nop
	tadi	ai10
sluknownentryloop:
	; ac = composite pointer to SE_... node
	; sluencnt = entry counter
	;   1 = use SE_VALU1 entry of ac node
	;   2 = use SE_VALU2 entry of ac node
	;   3 = use SE_VALU3 entry of ac node
	;   4 = use SE_VALU1 entry of next node
	;     ...
	sna
	jmsi	_fatal		; - should not run out of entries
	jmsi	_accai10	; access entry memory
	tad	sluencnt	; see if at least 3 more entries to step over
	tad	_7775
	spa*sna			; (skip if pos and nonz)
	jmp	sluknownentrydone
	dca	sluencnt	; more, decrement count
	tadi	ai10		;SE_NEXT=0 : get next entry in the frame
	jmp	sluknownentryloop
sluknownentrydone:
	; df,ai10 = points just before SE_... node (ie, offset -1)
	; ac = entry index
	;   -2 = use SE_VALU1 entry of df,ai10 node
	;   -1 = use SE_VALU2 entry of df,ai10 node
	;    0 = use SE_VALU3 entry of df,ai10 node
	cll ral
	;   -4 = use SE_VALU1 entry of df,ai10 node
	;   -2 = use SE_VALU2 entry of df,ai10 node
	;    0 = use SE_VALU3 entry of df,ai10 node
	tad	_0007
	;    3 = use SE_VALU1 entry of df,ai10 node
	;    5 = use SE_VALU2 entry of df,ai10 node
	;    7 = use SE_VALU3 entry of df,ai10 node
	tad	ai10		; get pointer value entry
	dca	sluencnt	; read the value entry
	tadi	sluencnt
	jmpi	symlookup

slucmpcount: .word .-.		; number of symbols in entry to serach
slucmpvalu:  .word .-.		; corresponding value token for symbol
slufrcnt:    .word .-.		; frame number it was found in (starts at 1)
sluencnt:    .word .-.		; entry number it was found in (starts at 1)
sluframe:    .word .-.		; next outer frame to search
sluentry:    .word .-.		; next entry in frame to search
slunegname:  .word .-.		; negative name entry pointer we're looking for
slutoken:    .word .-.		; symbol token we are looking for



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (<< inta intb)
; (>> inta intb)
f__shr:
	tad	_7777
f__shl:
	jmsi	_pushda		; save right-shift flag
	jmsi	_checkmiss
	jmsi	_nextint	; get next integer argument
	tadi	ai10
	dca	udiv_remdr+0
	tadi	ai10
	dca	udiv_remdr+1
	jmsi	_checkmiss
	jmsi	_nextint	; get next integer argument
	jmsi	_popda
	dca	shxright	; save right-shift flag
	tadi	ai10		; mask the shift count
	and	_0037
	sna
	jmp	shxdone		; zero, leave number as is
	cma iac
	dca	shxcount	; save negative count
	isz	shxright
	jmp	shlloop
shrloop:
	tad	udiv_remdr+1	; right shift, copy sign bit
	ral
	cla
	tad	udiv_remdr+1	; shift right, sign extend
	rar
	dca	udiv_remdr+1
	tad	udiv_remdr+0
	rar
	dca	udiv_remdr+0
	isz	shxcount	; repeat if more shifting
	jmp	shrloop
	jmp	shxdone
shlloop:
	tad	udiv_remdr+0	; shift left
	cll ral
	dca	udiv_remdr+0
	tad	udiv_remdr+1
	ral
	dca	udiv_remdr+1
	isz	shxcount	; repeat if more shifting
	jmp	shlloop
shxdone:
	tad	_0005		;T_VALU=3
	jmsi	_allrestok	; allocate result token
		.word	TT_INT
	tad	udiv_remdr+0	; return remainder
	dcai	ai10
	tad	udiv_remdr+1
	dcai	ai10
	jmpi	_evalret

shxright: .word	.-.
shxcount: .word	.-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (umullo/umulhi a b) => integer
; unsigned multiply
f_umulhi:
	iac
f_umullo:
	jmsi	_pushda		; 0000=>low; 0001=>high
	jmsi	_checkmiss
	jmsi	_nextint	; get integer a
	tadi	ai10
	jmsi	_pushda
	tadi	ai10
	jmsi	_pushda
	jmsi	_checkmiss
	jmsi	_nextint	; get integer b
	jmsi	_checklast	; make sure no more args
	jmsi	_popda		; put integer a where umullohi_1 can get it
	dca	umullohival+1
	jmsi	_popda
	dca	umullohival+0
	jmsi	_popda		; get hi/lo flag
	cll rar			; => put in link; zero accumulator
	tad	ai10		; point to where b value is
	cif_1
	jmpi	.+1
	.word	umullohi_1
umullohi1ret:
	tad	_0005		;T_VALU=3
	jmsi	_allrestok	; allocate result token
		.word	TT_INT
	tad	umullohival+0	; return low order value
	dcai	ai10
	tad	umullohival+1
	dcai	ai10
	jmpi	_evalret

umullohival: .blkw 2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; get next string argument
;  input:
;   ac = zero
;  output:
;   ac = zero: success
;              nextstrtok = string token composite pointer
;              df,ai10 = just before T_VALU
;        else: no more args
;  prints error and exits if non-string arg
nextstr: .word	.-.
	tad	nextstr
	jmsi	_pushda
	jmsi	_nexteval	; get and evaluate next token in the list
	sna
	jmp	nsdone
	dca	nextstrtok	; save composite pointer
	tad	nextstrtok
	jmsi	_accai10
	tadi	ai10		;T_TYPE=0 : make sure it is a string
	tad	_7775		;TT_INT=3
	sza cla
	jmp	nserr1
	isz	ai10		;T_VALU=3
	isz	ai10
	jmsi	_popda
	dca	nextstr
	jmpi	nextstr
nsdone:
	jmsi	_popda
	dca	nextstr
	cma
	jmpi	nextstr

nserr1:
	tad	_nsmsg1
	jmpi	_evalerr

nextstrtok: .word .-.

_nsmsg1: .word	nsmsg1



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; BUILT-IN FUNCTIONS
; These are called by evaluate when passed a TT_LIS token
; The first element of the list must evaluate to a function (TT_FUN) value
;  input:
;   ac = zero
;   df,ai10 = points to T_VALU of function (TT_FUN) token
;             T_VALU[0] = machine code address, eg, f_define
;             T_VALU[1..] = varies depending on T_VALU[0] function
;   evllist = composite pointer to list token being evaluated
;   evlnext = composite pointer to second list entry if any, zero if not
;  output:
;   jump to evalret on success
;     ac = zero: normal; else: tco-style
;     evaltok = function result value token
;       normal: evaltok is already evaluated
;       tco-style: evaltok is not evaluated
;   jump to evalerr on failure
;     ac = errormsg
;     evaltok = errant token indicating where in source code the error is
;  scratch:
;   ai10, ai11, ai12, df, evaluate, evllist, evlnext
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (car list)
; access first element of list
f_car:
	jms	singlelist	; get single list argument
	sna
	jmp	carerr3		; - error if list is empty
	jmsi	_access		; access first list element
	dca	ai10		;L_TOKN=1 : get corresponding token
	tadi	ai10
	dca	evaltok		; that is our result
	jmpi	_evalret
carerr3:
	tad	_carmsg3
	jmpi	_evalerr

_carmsg3: .word	carmsg3

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (cdr list)
; access rest of list
f_cdr:
	jms	singlelist	; get single list argument
	sna
	jmp	carerr3		; - error if list is empty
	jmsi	_access		; access first list element
	dca	cdrlist		;L_NEXT=0 : get corresponding token
	tadi	cdrlist
	dca	cdrlist
	tad	_0004		;T_VALU=3
	jmsi	_allrestok	; allocate list token
		.word	TT_LIS
	tad	cdrlist
	dcai	ai10
	jmpi	_evalret

cdrlist: .word	.-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (length list)
; length of list
f_length:
	jms	singlelist	; get single list argument
	dca	cdrlist
	dca	lengthcount
	tad	cdrlist
lengthloop:
	sna
	jmp	lengthdone
	isz	lengthcount
	jmsi	_access		; access first list element
	dca	cdrlist		;L_NEXT=0 : get pointer to next element in list
	tadi	cdrlist
	jmp	lengthloop
lengthdone:
	tad	_0005		;T_VALU=3
	jmsi	_allrestok	; allocate result token
		.word	TT_INT
	tad	lengthcount	; fill in T_VALU with 24-bit length
	dcai	ai10
	dcai	ai10
	jmpi	_evalret	; return length token in evaltok

lengthcount: .word .-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (stackroom)
; how many words available on stack
f_stackroom:
	cdf_0
	tadi	_stkcpptrc
	cma iac
	tadi	_stkdaptrc
	dca	lengthcount
	jmp	lengthdone

_stkcpptrc: .word stkcpptr
_stkdaptrc: .word stkdaptr

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (heaproom)
; (heaproom-gc)
; get number of 8-word blocks on free list
f_heaproom_gc:
	jmsi	_garbcoll
f_heaproom:
	dca	lengthcount	; initialize length
	tad	malwds8m1d	; start with 8-word blocks
	dca	ai10
	iac
heaproomloop1:
	dca	heaproominc	; save word size in 8-word blocks
	malwdscdf
	tadi	ai10		; get pointer to first free n-word block
heaproomloop2:
	sna
	jmp	heaproomdone2	; no more of this size, on to next size
	jmsi	_access		; access the free block
	dca	heaproomptr	; save pointer
	tad	lengthcount	; increment length for the free block
	tad	heaproominc
	dca	lengthcount
	tadi	heaproomptr	; get pointer to next free block of this size
	jmp	heaproomloop2
heaproomdone2:
	tad	heaproominc	; next list has double the size
	cll ral
	and	_1777		; stop if we just did the 4K-size list
	sza
	jmp	heaproomloop1
	jmp	lengthdone

malwds8m1d: .word malwds8-1
heaproominc: .word .-.
heaproomptr: .word .-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (null? list)
; #t if list is empty, #f otherwise
f_null_q:
	jms	singlelist	; get single list argument
retaciszero:
	sna cla
	tad	_0010		;truetok-falsetok=0010 : no elements, set to return #t token
	tad	_falsetok	; return #f token if non-zero
	dca	evaltok
	jmpi	_evalret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; get single list argument
;  input:
;   ac = zero
;  output:
;   ac = points to first list element (or zero if list is empty)
;   df,ai10 = points to T_VALUE field of argument token (already checked to be TT_LIS)
singlelist: .word .-.
	tad	singlelist	; save from recursion
	jmsi	_pushda
	jmsi	_checkmiss
	jmsi	_nexteval	; get and evaluate the next argument token
	jmsi	_accai10	; access the argument token
	tadi	ai10		;T_TYPE=0 : make sure it is an list
	tad	_7776		;TT_LIS=2
	sza cla
	jmp	carerr2
	jms	checklast
	jmsi	_popda
	dca	singlelist
	isz	ai10
	isz	ai10
	tadi	ai10		;T_VALU=3 : get pointer to first list element
	jmpi	singlelist

carerr2:
	tad	_carmsg2
	jmpi	_evalerr

_carmsg2: .word	carmsg2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; make sure there are no more arguments
;  input:
;   ac = zero
;  output:
;   ac = zero
;   df = preserved
;  prints error message and exits if extra args
checklast: .word .-.
	tad	evlnext
	sna
	jmpi	checklast
	dca	evaltok
	tad	_carmsg4
	jmpi	_evalerr
_carmsg4: .word	carmsg4

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; make sure there is at least one more argument
;  input:
;   ac = zero
;  output:
;   ac = zero
;  prints error message and exits if no more args
checkmiss: .word .-.
	tad	evlnext
	sza cla
	jmpi	checkmiss
	tad	_carmsg5
	jmpi	_evalerr
_carmsg5: .word	carmsg5

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; print CR/LF
ttprintcrlf1:
	jmsi	_ttprintcrlf
	cif_1
	jmpi	.+1
	.word	ttprintcrlf1ret



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (cons values... [list])
;   construct a list from values and optional list on the end
; (list values...)
;   construct a list from values, don't check for list on the end
f_cons:
	cma
f_list:
	dca	listflag	; 0000=list; 7777=cons
	jms	makelist
	jmpi	_evalret	; all done

; make a list from remaining arguments
;  input:
;   ac = 0000: last arg is a value
;        7777: if last arg is a list, prepend this list to it
;  output:
;   ac = zero
;   evaltok = list token
;  scratch:
;   df, ai10
makelist: .word	.-.
	tad	consresbeg	; save for recursion
	jmsi	_pushcp
	tad	consresend
	jmsi	_pushcp
	tad	conslastok
	jmsi	_pushcp
	tad	listflag
	jmsi	_pushda
	dca	consresbeg	; start out with an empty list
	dca	consresend
	dca	conslastok
consloop:
	jmsi	_nexteval	; get and evaluate next token in the list
	sna
	jmp	consdone
	dca	constoken
	tad	conslastok	; there is another token, append the previous token if any
	sza cla
	jms	consapplast
	tad	constoken	; then this one becomes the previous
	dca	conslastok
	jmp	consloop
consdone:
	jmsi	_popda
	dca	listflag
	; conslastok = last cons argument value token (or null if none)
	; consresbeg = list entry for first value token (or null if none so far)
	; consresend = last entry for next-to-last token (or null if none so far)
	; listflag = 0000: always treat conslastok as a value
	;            7777: if conslastok is a list, append its elements
	tad	conslastok	; access the last token
	sna
	jmp	consret		; - null, cons has no arguments, return an empty list
	jmsi	_accai10
	tadi	ai10		;T_TYPE=0 : see if it is an list
	isz	ai10
	isz	ai10
	and	listflag	; if (list...), always treat last value as non-list
	tad	_7776		;TT_LIS=2
	sna cla
	jmp	constolist
	jms	consapplast	; not a list, append as a value on the end
	; consresbeg = list entry for first value token (or null if none so far)
	jmp	consret
constolist:
	; conslastok = last cons argument value token - known to be a list
	; consresbeg = list entry for first value token (or null if none so far)
	; consresend = last entry for next-to-last token (or null if none so far)
	tadi	ai10		;T_VALU=3 : get pointer to list that goes on the end
	dca	consapple	; link the given list onto end of constructed list
	jms	consappentry
consret:
	; consresbeg = list entry for first value token (or null if empty list)
	tad	_0004		;T_VALU=3 : allocate a list token for the result
	jmsi	_allrestok
		.word	TT_LIS
	tad	consresbeg	; set link to first list element
	dcai	ai10
	jmsi	_popcp		; restore recursion
	dca	conslastok
	jmsi	_popcp
	dca	consresend
	jmsi	_popcp
	dca	consresbeg
	jmpi	makelist

listflag: .word	.-.	; 0000=list; 7777=cons
consresbeg: .word .-.	; composite pointer to first element of list being built
consresend: .word .-.	; composite pointer to last element of list being built
conslastok: .word .-.	; composite pointer to last value token argument to cons
constoken:  .word .-.	; composite pointer to value token

; append the last token as a value to the list being built
;  input:
;   conslastok = value token to append to list
;   consresbeg = beginning of cons list being built
;   consresend = end of cons list being built
;  output:
;   ac = 0
;   consresbeg,consregend = updated to include conslastok
;  scratch:
;   df
consapplast: .word .-.
	cla iac			; allocate a list entry node
	jmsi	_l2malloc	;L_SIZE=2
	dca	consapple
	tad	consapple
	jmsi	_accai10
	dcai	ai10		;L_NEXT=0 : zero link to next element
	tad	conslastok	;L_TOKN=1 : link to value token
	dcai	ai10
	jms	consappentry
	jmpi	consapplast

; append list element to the list being built
;  input:
;   consapple  = list element to append to list
;   consresbeg = beginning of cons list being built
;   consresend = end of cons list being built
;  output:
;   ac = 0
;   consresbeg,consregend = updated to include conslastok
;  scratch:
;   df
consappentry: .word .-.
	tad	consresend	; get previous end link element
	sza
	jmp	consappentrylink
	tad	consapple	; this is the first value being cons'd
	dca	consresbeg	; - set the beginning element to this one
	jmp	consappentrydone
consappentrylink:
	jmsi	_access		; access previous list element
	dca	consresend
	tad	consapple	; write the new one to its L_NEXT
	dcai	consresend	;L_NEXT=0 : link the new one on to the last one
consappentrydone:
	tad	consapple	; the new one is now the end of the list
	dca	consresend	; - set the end element to this one
	jmpi	consappentry

consapple:    .word .-.		; composite pointer to list element to append to list being built

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (prompt values ...)
; display values without any formatting then read from keyboard
f_prompt:
	jmsi	_nexteval	; get prompt string token
	sna
	jmp	pmtdone
	jmsi	_tokdisplayb	; print it
		.word	ttprintac
	jmp	f_prompt
pmtdone:
	jmsi	_kbread		; read line from keyboard
	tad	_kbbuff		; calc number of chars read
	cma iac
	cdf_0
	tadi	_kbnext
	dca	pmtstrlen
	tad	pmtstrlen
	tad	_0004		;T_VALU=3
	jmsi	_allrestok	; allocate result token
		.word	TT_STR
	tad	pmtstrlen	; T_VALU[0] = number of chars
	dcai	ai10
	tad	pmtstrlen	; return if num chars zero
	sna
	jmpi	_evalret
	cma iac			; save neg num chars for isz
	dca	pmtstrlen
	cma			; point just before kbbuff
	tad	_kbbuff
	dca	ai11
	tad	_6201		; get df for string result token
	rdf
	dca	pmtcdf
pmtloop:
	kbbuffcdf		; copy kbbuff to result token
	tadi	ai11
pmtcdf:	.word	.-.
	dcai	ai10
	isz	pmtstrlen
	jmp	pmtloop
	jmpi	_evalret	; all done

_kbnext: .word	kbnext
_tokdisplayb: .word tokdisplay
pmtstrlen: .word .-.



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (load "filename")
f_load:
	jmsi	_checkmiss
	jmsi	_nextstr	; get the single string argument
	jmsi	_checklast	; make sure no more args
	tadi	ai10		; get negative string length in chars
	cma iac
	dca	loadfnlen
	tad	loadfnlen	; make sure it fits in kbbuff
	sna
	jmp	loaderr1	; - null filename
	tad	_kbbufflen
	spa*sna cla
	jmp	loaderr3
	tad	_6201		; get string data frame
	rdf
	dca	loadfncdf
	tad	_kbbuffm1b	; point to kb buffer
	dca	ai11
loadfnloop:
loadfncdf: .word .-.
	tadi	ai10		; get character from string token
	kbbuffcdf
	dcai	ai11		; put character in kbbuff
	isz	loadfnlen
	jmp	loadfnloop
	dcai	ai11		; null terminate
	tadi	_readlnbuffd	; save old file descriptor (or -1 for kb)
	dca	oldloadfd
	tad	_openfile	; try to open that file
	sys6
	szl
	jmp	loaderr4
	dcai	_readlnbuffd	; set up to read next line from load file
	cdf_0
	tadi	_lineno		; save old line number
	dca	oldloadln
	dcai	_lineno		; reset line number in new load file
	iac			;LS_SIZE=3
	jmsi	_l2malloc	; stack the old file descriptor
	dca	newloadstk
	tad	newloadstk
	jmsi	_accai10
	tad	loadstack	;LS_OUTER=0 : outer load stack element
	dcai	ai10
	tad	oldloadfd	;LS_OLDFD=1 : outer load file descriptor
	dcai	ai10
	tad	oldloadln	;LS_OLDLN=2 : outer load file line number
	dcai	ai10
	tad	newloadstk	; push old load information
	dca	loadstack
	jmpi	_retnull	; return null token
loaderr1:
	tad	_loadmsg1
	jmpi	_evalerr
loaderr3:
	tad	_loadmsg3
	jmpi	_evalerr
loaderr4:
	cla
	tad	_loadmsg4
	jmpi	_evalerr

	; close the load file
	; returns ac<0: nothing closed
	;         else: something closed
closeload: .word .-.
	cdf_1
	cla
	tadi	_readlnbuffd	; get fd load file was opened on (or -1 if not open)
	spa
	jmpi	closeload
	dcai	_closefilefd
	tad	_closefile	; close that file
	sys6
	cla cma
	dcai	_closefilefd
	tad	loadstack	; pop previous load file
	sna
	jmsi	_fatal
	jmsi	_accai10
	tadi	ai10		;LS_OUTER=0 : restore next outer load file (or null)
	dca	loadstack
	tadi	ai10		;LS_OLDFD=1 : restore next outer file des (or -1 for kb)
	dca	oldloadfd
	tadi	ai10		;LS_OLDLN=2 : restore next outer line number
	dca	oldloadln
	kbbuffcdf
	tad	oldloadfd
	dcai	_readlnbuffd
	cdf_0
	tad	oldloadln
	dcai	_lineno
	jmpi	closeload

loadfnlen:    .word .-.		; filename length counter
loadstack:    .word .-.		; existing load stack
oldloadfd:    .word .-.		; old fd being loaded from (or -1 if keyboard)
oldloadln:    .word .-.		; old line number being read from
newloadstk:   .word .-.		; newly allocated load stack element

_closefile:   .word closefile
_closefilefd: .word closefilefd
_kbbuffm1b:   .word kbbuff-1
_lineno:      .word lineno
_mainloop:    .word mainloop
_kbbufflen:   .word kbbufflen
_openfile:    .word openfile
_readlnbuffd: .word readlnbuffd
_mkkbbufflen: .word -kbbufflen

_loadmsg1: .word loadmsg1
_loadmsg3: .word loadmsg3
_loadmsg4: .word loadmsg4



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; numeric comparison
; second operand defaults to zero
; ac=4:eq ac=2:gt ac=1:lt
f__ge:	iac
f__le:	iac
f__eq:	iac
f__ne:	iac
f__gt:	iac
f__lt:	iac
	jmsi	_pushda

	; get A value with top bit flipped
	jmsi	_checkmiss
	jmsi	_nextint
	tadi	ai10
	jmsi	_pushda
	tadi	ai10
	tad	_4000
	jmsi	_pushda

	; point to B value (or null if missing)
	jmsi	_nextint
	sza cla
	dca	ai10

	jmsi	_checklast

	; restore from stack now that we won't call eval any more
	jmsi	_popda
	dca	cmpinta+1
	jmsi	_popda
	dca	cmpinta+0
	jmsi	_popda
	dca	cmpcond

	; compute A+~B where A,B have their top bits flipped
	tad	ai10		; b defaults to zero
	sza cla
	tadi	ai10		; ac=b<11:00>
	cll cma			; l=0 ac=~b<11:00>
	tad	cmpinta+0	; l=carry0 ac=a<11:00>+~b<11:00>
	dca	cmpinta+0

	tad	ai10		; b defaults to zero
	sza cla
	tadi	ai10		; l=carry0 ac=b<23:12>
	ral			; l=b<23> ac=b<22:12>,carry0
	cma			; l=b<23> ac=~b<22:12>,~carry0
	rar			; l=~carry0 ac=b<23>,~b<22:12>
	cml			; l=carry0 ac=b<23>,~b<22:12>
	szl
	cll iac			; l=carry1 ac=(b<23>,~b<22:12>)+carry0
	tad	cmpinta+1	; l=carry1 ac=(b<23>,~b<22:12>)+carry0+(~a<23>,a<22:12>)

	; see what result is
	;  A=B  =>  l=0, ac=7777, cmpinta+0=7777
	;  A>B  =>  l=1
	;  A<B  =>  otherwise
	and	cmpinta+0	; see if both words of A+~B are 7777
	iac			; ... indicating A=B
	sna cla			; if not, leave link alone and set AC=0000
	cll cml iac		; if so, set L=1,AC=0001
	ral			; ac=3:eq ac=1:gt ac=0:lt
	iac			; ac=4:eq ac=2:gt ac=1:lt

	; see if it matches what caller wants
	and	cmpcond
	sza cla
	tad	_0010		;truetok-falsetok=0010
	tad	_falsetok
	dca	evaltok
	jmpi	_evalret

cmpcond: .word	.-.
cmpinta: .blkw	2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (if condval trueval falseval)
f_if:
	jmsi	_checkmiss
	jmsi	_nextbool	; evaluate condval
	sna cla
	jmsi	_nextelem	; false, skip over and don't evaluate true value
	cla
	jmsi	_checkmiss
	jmsi	_nextelem	; get true or false value token and don't eval just yet
	jmpi	_evalret	; evaluate and return, tco style

_nextbool: .word nextbool

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (lambda (paramnames ...) funcbody)
; (lambda varargsparamname funcbody)
; create a TT_FUN token
;  T_VALU[0] = userfunc or userfuncva
;  T_VALU[1] = current symbol frame
;  T_VALU[2] = paramnames... list entry or varargsparamname token
;  T_VALU[3] = unevaluated funcbody token
f_lambda:
	jmsi	_checkmiss
	jmsi	_nextelem	; get (paramnames ...) token
	jmsi	_accai10	; access token memory
	tadi	ai10		;T_TYPE=0 : get token type
	tad	_7776		;TT_LIS=2 : make sure it is a list
	sza
	jmp	lamnotlist
	isz	ai10
	isz	ai10
	tadi	ai10		;T_VALU=3 : get first list element pointer from T_VALU[0]
	dca	lamparams
	tad	_userfunc
	jmp	lamgetbody
lamnotlist:
	tad	_7776		;TT_SYM=4 : maybe it is varargs paramname
	sza cla
	jmp	lamerr2
	tad	evaltok
	dca	lamparams
	tad	_userfuncva
lamgetbody:
	dca	lamhandlr
	jmsi	_checkmiss
	jmsi	_nextelem	; get funcbody token
	dca	lamfbody
	jmsi	_checklast	; there shouldn't be another token

	tad	symframe
	dca	lamsymfrm
	tad	_0007		;T_VALU=3
	jmsi	_allrestok	; allocate and access TT_FUN token
		.word	TT_FUN
	tad	lamhandlr	; T_VALU[0] = userfunc or userfuncva
	dcai	ai10
	tad	lamsymfrm	; T_VALU[1] = current symbol frame
	dcai	ai10
	tad	lamparams	; T_VALU[2] = paramnames first list entry (not the list token)
	dcai	ai10		;             or varargsparamname token
	tad	lamfbody	; T_VALU[3] = unevaluated function body token
	dcai	ai10
	jmpi	_evalret	; return the TT_FUN token composite pointer in evaltok

lamparams: .word .-.
lamfbody:  .word .-.
lamsymfrm: .word .-.
lamhandlr: .word .-.

lamerr2:
	tad	_lammsg2
	jmpi	_evalerr

_lammsg2: .word	lammsg2

_userfunc:   .word userfunc
_userfuncva: .word userfuncva



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; short-circuiting and/or

f_and:
andloop:
	jms	tcoiflast	; if last argument, return its value as our value
	jms	nextbool	; not the last, evaluate the next token
	spa
	jmp	rettrue		; no more args, the and is true
	sza cla
	jmp	andloop		; this arg true, keep evaluating
retfalse:
	cla
	tad	_falsetok
	dca	evaltok
	jmpi	_evalret

f_or:
orloop:
	jms	tcoiflast	; if last argument, return its value as our value
	jms	nextbool	; not the last, evaluate the next token
	spa
	jmp	retfalse	; no more args, the or is false
	sna cla
	jmp	orloop		; this arg is false, keep evaluating
rettrue:
	cla
	tad	_truetok
	dca	evaltok
	jmpi	_evalret

f_begin:
	jms	tcoiflast	; if last argument, return its value as our value
	jmsi	_nexteval	; not the last, evaluate the next token of any type
	sza cla			; discard the value
	jmp	f_begin		; repeat if more arguments
	jmp	retfalse	; no args at all, return #F

f_not:
	jmsi	_checkmiss
	jms	nextbool
	jmp	.+1
	.word	retaciszero

_truetok: .word	truetok|(truetok/4096)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; if the next argument is the last argument to a function,
; return the value of the argument as the value of the function tco style
;  input:
;   evlnext = points to argument list element to be processed
;  output:
;   returns with ac = zero if either list is empty or more than one
;   jumps to evalret with the one argument if it is the last one
tcoiflast: .word .-.
	tad	evlnext		; point to list element of argument
	sna
	jmpi	tcoiflast	; nothing there, normal processing
	jmsi	_accai10	; access list element memory
	tadi	ai10		;L_NEXT=0
	sza cla			; check link to next argument list element
	jmpi	tcoiflast	; non-zero, this one isn't the last
	tadi	ai10		;L_TOKN=1
	dca	evaltok		; this is the last, get pointer to last argument token
	iac			; evaluate it tco style
	jmpi	_evalret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; get next boolean argument
; returns:
;  0000 = false
;  0001 = true
;  7777 = no more args
nextbool: .word .-.
	tad	nextbool
	jmsi	_pushda
	jmsi	_nexteval	; get and evaluate next token in the list
	sna
	jmp	nbdone
	jmsi	_accai10
	tadi	ai10		;T_TYPE=0 : make sure it is an boolean
	tad	_7772		;TT_BOO=6
	sza cla
	jmp	orerr1
	jmsi	_popda
	dca	nextbool
	isz	ai10
	isz	ai10
	tadi	ai10		;T_VALU=3 : get 0 or 1
	jmpi	nextbool
nbdone:
	jmsi	_popda
	dca	nextbool
	cma
	jmpi	nextbool

orerr1:
	tad	_ormsg1
	jmpi	_evalerr

_ormsg1: .word	ormsg1

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (define varname value)
;   T_VALU[0] = f_define
; make entry in symbol table setting varname to value
; return value is null
; to define a function, value should be (lambda (paramnames ...) funcbody)
f_define:
	jmsi	_checkmiss
	jmsi	_nextelem	; get varname token
	jmsi	_symtok2nament	; get corresponding name entry
	jmsi	_pushcp
	jmsi	_checkmiss
	jmsi	_nexteval	; get and evaluate value token
	dca	addsymval
	jmsi	_checklast	; make sure there are no more tokens after value
	jmsi	_popcp
	dca	addsymnam	; add symbol table entry
	tad	symframe
	dca	addsymfrm
	jmsi	_addsyment
	jmpi	_retnull	; return value is the null token

_symtok2nament: .word symtok2nament

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (int?/list?/string?/lambda?/bool?/char? value)
f_char_q:			;TT_CHR=7
	iac
f_bool_q:			;TT_BOO=6
	iac
f_lambda_q:			;TT_FUN=5
	tad	_0002
f_string_q:			;TT_STR=3
	iac
f_list_q:			;TT_LIS=2
	iac
f_int_q:			;TT_INT=1
	cma
	jmsi	_pushda		; save neg type to test for
	jmsi	_checkmiss	; make sure we have an argument
	jmsi	_nexteval	; evaluate next argument
	jmsi	_access		; access argument memory
	dca	typredptr	;T_TYPE=0 : save pointer to type
	jmsi	_checklast	; make sure no more arguments
	jmsi	_popda		; get neg type to test for
	tadi	typredptr	; add value type
	jmpi	.+1		; return true if AC=zero, false otherwise
	.word	retaciszero

typredptr: .word .-.



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (display values ...)
; display values without any formatting
f_display:
	jmsi	_nexteval	; get next token
	sna
	jmp	retnull		; all done
	jms	tokdisplay
		.word	ttprintac
	jmp	f_display
retnull:
	tad	_nulltok
	dca	evaltok
	jmpi	_evalret

_nulltok: .word	nulltok|(nulltok/4096)

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; display token - just the bare value
;  input:
;   ac = composite pointer to token
;   retadr+0 = address of character print function
;  output:
;   ac = zero
;  scratch:
;   df, ai10, udiv_...
tokdisplay: .word .-.
	dca	tokdisptok
	cdf_0			; save pointer to print function
	tadi	tokdisplay
	dca	tokdispac
	isz	tokdisplay
	tad	tokdisptok
	jmsi	_accai10	; access token memory
	tadi	ai10		;T_TYPE=0 : get token type
	tad	_7770
	cll
	tad	_0010
	snl
	jmp	tokdispret
	isz	ai10		;T_VALU=3 : skip to just before value
	isz	ai10
	tad	tokdisptbl	; jump to entry in table
	dca	tokdisptmp
	jmpi	tokdisptmp
tokdispret:
	cla
	jmpi	tokdisplay

tokdisptok: .word .-.
tokdispac:  .word .-.

tokdisptbl: .word .+1
	jmp	tokdispret	; TT_NUL = 0  null display
	jmp	tokdispint	; TT_INT = 1  T_VALU = 24-bit int (little endian)
	jmp	tokdisplis	; TT_LIS = 2  T_VALU = points to first element
	jmp	tokdispstr	; TT_STR = 3  T_VALU = 12-bit counted
	jmp	tokdispsymb	; TT_SYM = 4  T_VALU = 6-bit counted
	jmp	tokdispret	; TT_FUN = 5  T_VALU = machine code entrypoint
	jmp	tokdispboo	; TT_BOO = 6  T_VALU = 0:false; 1:true
	jmp	tokdispchr	; TT_CHR = 7  T_VALU = 12-bit character

tokdispsymb: jmpi .+1
	.word	tokdispsym

tokdispint:
	tadi	ai10		; get 24-bit integer value
	dca	udiv_dvquo+0
	tadi	ai10
	sma
	jmp	tokdispintpos
	cma
	dca	udiv_dvquo+1
	tad	_0055		; negative display minus
	jmsi	tokdispac
	tad	udiv_dvquo+0	; negate the value
	cll cma iac
	dca	udiv_dvquo+0
	ral
	tad	udiv_dvquo+1
tokdispintpos:
	dca	udiv_dvquo+1
	jmsi	_pushda		; push null terminator on stack
tokdisppush:
	tad	_0012		; squeeze a digit out the bottom
	jmsi	_udiv24
	tad	udiv_remdr	; convert digit to ascii
	tad	_0060
	jmsi	_pushda		; push on stack
	tad	udiv_dvquo+0	; see if more number to process
	sza cla
	jmp	tokdisppush
	tad	udiv_dvquo+1
	sza cla
	jmp	tokdisppush
tokdisppop:
	jmsi	_popda		; pop digit from stack
	sna
	jmp	tokdispret	; done if null
	jmsi	tokdispac	; otherwise, print and repeat
	jmp	tokdisppop

tokdispchr:
	tadi	ai10		; get 12-bit character
	jmsi	tokdispac	; printit
	jmp	tokdispret

tokdisplis:
	tad	tokdisplisnext	; save from recursion
	jmsi	_pushcp
	tad	tokdisplay
	jmsi	_pushda
	tad	tokdisplisac
	jmsi	_pushda
	tad	tokdispac
	dca	tokdisplisac
	tadi	ai10		; pointer to first list element
tokdisplisloop:
	sna
	jmp	tokdisplisdone
	jmsi	_accai10	; access list element memory
	tadi	ai10		;L_NEXT=0
	dca	tokdisplisnext
	tadi	ai10		;L_TOKN=1
	jms	tokdisplay	; display the entry token
tokdisplisac:	.word	.-.
	tad	tokdisplisnext	; on to next element if any
	jmp	tokdisplisloop
tokdisplisdone:
	jmsi	_popda		; restore recursion
	dca	tokdisplisac
	jmsi	_popda
	dca	tokdisplay
	jmsi	_popcp
	dca	tokdisplisnext
	jmp	tokdispret	; all done

tokdispstr:
	tadi	ai10		; number of chars in string
	sna
	jmp	tokdispret	; null string, all done
	cma iac			; save negative length for isz
	dca	tokdispstrlen
tokdispstrloop:
	tadi	ai10		; get character
	jmsi	tokdispac	; print it
	isz	tokdispstrlen	; check for more
	jmp	tokdispstrloop	; repeat if so
	jmp	tokdispret	; done if not

tokdispboo:
	tad	_0043		; print a '#'
	jmsi	tokdispac
	tadi	ai10		; get true/false flag
	sza cla
	tad	_0016		; print 'T'
	tad	_0106		; print 'F'
	jmsi	tokdispac
	jmp	tokdispret

tokdisplisnext:   .word .-.
tokdispstrlen:    .word .-.
tokdisptmp:       .word .-.



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (+ integers ...)
f_plus:
	tad	fpaccum+0	; save in case of recursion
	jmsi	_pushda
	tad	fpaccum+1
	jmsi	_pushda
	dca	fpaccum+0	; clear accumulator
fploop:
	dca	fpaccum+1
	jms	nextint		; get next int argument
	sza cla
	jmp	fpmdone
	cll
	tad	fpaccum+0	; add to accumulator
	tadi	ai10
	dca	fpaccum+0
	ral
	tad	fpaccum+1
	tadi	ai10
	jmp	fploop
fpmdone:
	tad	_0005		;T_VALU=3
	jmsi	_allrestok	; allocate result token
		.word	TT_INT
	tad	fpaccum+0	; T_VALU = 24-bit signed integer
	dcai	ai10
	tad	fpaccum+1
	dcai	ai10
	jmsi	_popda		; restore outer accumulator value
	dca	fpaccum+1
	jmsi	_popda
	dca	fpaccum+0
	jmpi	_evalret	; return this summation token in evaltok

fpaccum: .blkw	2

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (- integers ...)
;   (-)        =>  0
;   (- a)      =>  - a
;   (- a b)    =>  a - b
;   (- a b c)  =>  - a + b - c
f_minus:
	tad	fpaccum+0	; save in case of recursion
	jmsi	_pushda
	tad	fpaccum+1
	jmsi	_pushda
	dca	fpaccum+0	; clear accumulator
fmloop:
	dca	fpaccum+1
	jms	nextint		; get next int argument
	sza cla
	jmp	fpmdone
	cll
	tad	fpaccum+0	; add to accumulator
	tadi	ai10
	cma			; then complement accumulator
	dca	fpaccum+0
	ral
	tad	fpaccum+1
	tadi	ai10
	cma
	isz	fpaccum+0	; then increment accumulator
	jmp	fmloop
	iac
	jmp	fmloop

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (* integers ...)
f_times:
	tad	fpaccum+0	; save in case of recursion
	jmsi	_pushda
	tad	fpaccum+1
	jmsi	_pushda
	iac			; init accumulator = 1
	dca	fpaccum+0
	dca	fpaccum+1
ftloop:
	jms	nextint		; get next int argument
	sza cla
	jmp	fpmdone
	tadi	ai10		; save as multiplier
	dca	lomultplr

	dca	umul_prod+0	; high_prod = low_fpaccum * (high_multiplier + 4000)
	dca	umul_prod+1
	tad	fpaccum+0
	dca	umul_mcand+0
	dca	umul_mcand+1
	tadi	ai10		; (high_multiplier)
	tad	_4000
	jmsi	_umul24
	tad	umul_prod+0
	dca	umul_prod+1
	dca	umul_prod+0	; low_prod = 0

	tad	fpaccum+0	; prod += (fpaccum + 40000000) * (low_multiplier)
	dca	umul_mcand+0
	tad	fpaccum+1
	tad	_4000
	dca	umul_mcand+1
	tad	lomultplr	; get multiplier low word
	jmsi	_umul24		; multiply by that

	tad	lomultplr	; maybe flip product sign bit
	tad	fpaccum+0
	rar
	cla rar
	tad	umul_prod+1	; save the other bits as is
	dca	fpaccum+1
	tad	umul_prod+0
	dca	fpaccum+0

	jmp	ftloop

lomultplr: .word .-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; get next integer argument
;  input:
;   ac = zero
;  output:
;   ac = zero: success
;              df,ai10 = just before T_VALU
;        else: no more args
;  prints error and exits if non-int arg
nextint: .word	.-.
	tad	nextint
	jmsi	_pushda
	jmsi	_nexteval	; get and evaluate next token in the list
	sna
	jmp	nidone
	jmsi	_accai10
	tadi	ai10		;T_TYPE=0 : make sure it is an integer
	tad	_7777		;TT_INT=1
	sza cla
	jmp	nierr1
	isz	ai10		;T_VALU=3
	isz	ai10
	jmsi	_popda
	dca	nextint
	jmpi	nextint
nidone:
	jmsi	_popda
	dca	nextint
	cma
	jmpi	nextint

nierr1:
	tad	_nimsg1
	jmpi	_evalerr

_nimsg1: .word	nimsg1

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (fatal integercode)
f_fatal:
	jmsi	_nextint
	sna cla
	tadi	ai10
	jmsi	_fatal

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; print token
tokprint1:
	jmsi	_tokprint
	cif_1
	jmpi	.+1
	.word	tokprint1ret



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; display symbol name string
tokdispsym:
	isz	ai10
	isz	ai10
	tadi	ai10		;T_VALU[2] : get name node pointer
	dca	tspair
	cdf_0			; get display function
	tadi	_tokdispac
	dca	tsdisp
	tad	tspair		; access name node memory
	jmsi	_access
	dca	ai10
	tadi	ai10		;N_LEN=1 num chars in symbol
	cma iac
	dca	tslen
tsloop:
	tadi	ai10		; get char pair
	dca	tspair
	tad	tspair		; print top char
	bsw
	jms	tschar		; .. return if no more
	tad	tspair		; print bottom char
	jms	tschar		; .. return if no more
	jmp	tsloop		; loop if more

tschar:	.word	.-.
	and	_0077
	tad	_0040
	jmsi	tsdisp
	isz	tslen
	jmpi	tschar
	jmpi	_tokdispret

_tokdispac:  .word tokdispac
_tokdispret: .word tokdispret
tslen:	.word	.-.
tspair:	.word	.-.
tsdisp:	.word	.-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (quo/rem intdividend intdivisor)
;  sign of quotient is xor of two signs
;  sign of remainder is always same as sign of dividend
;  magnitude of quotient and remainder is the same regardless of two signs
;     11 / -4  =  -2 r  3
;     11 /  4  =   2 r  3
;    -11 /  4  =  -2 r -3
;    -11 / -4  =   2 r -3
f_rem:
	iac
f_quo:
	jmsi	_pushda		; 0=quotient; 1=remainder
	jms	getabsint	; get dividend
		.word	udiv_dvquo-1
	dca	dvdendneg
	iac			; get negative divisor
	jms	getabsint
		.word	udiv_ndvsr-1
	dca	divsorneg
	jmsi	_checklast	; check for end of arguments

	jmsi	_udiv2424	; do computation

	tad	_0005		;T_VALU=3
	jmsi	_allrestok	; allocate result token
		.word	TT_INT

	jmsi	_popda		; see if doing f_quo or f_rem
	sza cla
	jmp	doremainder

	tad	dvdendneg	; quotient sign is xor of operand signs
	tad	divsorneg
	sna cla
	jmp	posquotient

	tad	udiv_dvquo+0	; signs different, negate quotient
	cll cma iac
	dca	udiv_dvquo+0
	tad	udiv_dvquo+1
	cma
	szl
	iac
	dca	udiv_dvquo+1
posquotient:
	tad	udiv_dvquo+0	; return quotient
	dcai	ai10
	tad	udiv_dvquo+1
	dcai	ai10
	jmpi	_evalret

doremainder:
	tad	dvdendneg	; remainder sign is same as dividend sign
	sna cla
	jmp	posremainder
	tad	udiv_remdr+0	; dividend negative, negate remainder
	cll cma iac
	dca	udiv_remdr+0
	tad	udiv_remdr+1
	cma
	szl
	iac
	dca	udiv_remdr+1
posremainder:
	tad	udiv_remdr+0	; return remainder
	dcai	ai10
	tad	udiv_remdr+1
	dcai	ai10
	jmpi	_evalret

_udiv2424: .word udiv2424

divsorneg: .word .-.		; negative if divisor is negative
dvdendneg: .word .-.		; negative if dividend is negative

; get integer argument absolute value
;  input:
;   ac = 0: return positive; 1: return negative
;   retadr+0 : pointer to two-word output buffer - 1
;  output:
;   ac = high word original value, just the sign bit
;  scratch:
;   df,ai10
getabsint: .word .-.
	dca	divnegval	; save that caller wants negative value
	jmsi	_checkmiss
	jmsi	_nextint	; get next integer argument
	tadi	ai10		; get two words to temp
	dca	divloword
	tadi	ai10
	dca	divhiword
	tad	divhiword
	dca	divhiorig
	tad	divhiword	; see if value is negative
	rtl
	tad	divnegval	; and opposite of what caller wants
	rar
	snl cla
	jmp	getabsint_noflip
	tad	divloword	; opposite, negate value
	cll cma iac
	dca	divloword
	tad	divhiword
	cma
	szl
	iac
	dca	divhiword
getabsint_noflip:
	cdf_0
	tadi	getabsint	; return absolute value
	dca	ai10
	tad	divloword
	dcai	ai10
	tad	divhiword
	dcai	ai10
	tad	divhiorig	; return original hi-order word
	and	_4000		; - just the sign bit
	isz	getabsint
	jmpi	getabsint

divnegval: .word .-.
divloword: .word .-.
divhiword: .word .-.
divhiorig: .word .-.



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (string values...) => string
; build string by appending all the given values
f_string:
	tad	fstrchptr	; save from recursion because we call evaluate
	jmsi	_pushda
	tad	fstrlnptr
	jmsi	_pushda
	tad	fstrouttok
	jmsi	_pushcp
	tad	fstrroom
	jmsi	_pushda
	tad	fstrtoklen
	jmsi	_pushda
	tad	_0017		; start out with 15-word token
	dca	fstrtoklen
	tad	fstrtoklen
	jmsi	_allrestok
		.word	TT_STR
	tad	evaltok
	dca	fstrouttok	; save token composite pointer
	dcai	ai10		; set initial string length to zero
	tad	ai10		; save pointer to length word
	dca	fstrlnptr
	tad	ai10		; save pointer to just before where 1st char goes
	dca	fstrchptr
	tad	_6201		; save data frame for string
	rdf
	dca	fstrcdf
	tad	_7764		;T_VALU=3 : it can hold 11 character words
	dca	fstrroom	; - fstrroom = 1s comp of number of char words
fstrloop:
	jmsi	_nexteval	; get evaluated argument
	sna
	jmp	fstrdone
	jmsi	_tokdisplay	; append to output string
		.word	fstrprintac
	jmp	fstrloop
fstrdone:
	tad	fstrouttok	; return output string
	dca	evaltok
	jmsi	_popda		; restore recursion
	dca	fstrtoklen
	jmsi	_popda
	dca	fstrroom
	jmsi	_popcp
	dca	fstrouttok
	jmsi	_popda
	dca	fstrlnptr
	jmsi	_popda
	dca	fstrchptr
	jmpi	_evalret	; all done

_tokdisplay: .word tokdisplay

fstrchptr:  .word .-.		; pointer just before where next char goes
fstrlnptr:  .word .-.		; pointer to length word in output token
fstrouttok: .word .-.		; composite pointer to output token
fstrroom:   .word .-.		; how much room left in token (1s comp number of words, -1 = zero words left)
fstrtoklen: .word .-.		; total length of token (not including malloc flag word)

; append char in ac to output token
;  input:
;   ac = character to append
;  output:
;   character appended
;   ac = zero
;   df = preserved
;   updated:
;     fstrroom
;     fstrchptr
;   may have changed:
;     fstrtoklen
;     fstrouttok
;     fstrlnptr
;  scratch:
;   ai17
fstrprintac: .word .-.
	dca	fstrchar	; save character being appended
	tad	_6201		; preserve current df register
	rdf
	dca	fstrrescdf
	isz	fstrroom	; see if room left in current token (-1 -> 0 when no room left)
	jmp	fstrappend	; if so, append char

	tad	ai10
	dca	fstroldai10

	tad	fstrtoklen	; malloc double-sized token
	cll cml ral		; eg, old 7 -> new 15
	dca	fstrnewtoklen
	tad	fstrnewtoklen
	jmsi	_allrestok
		.word	TT_STR
	; evaltok = newly allocated token
	; ac = zero
	; df,ai10 = points just before T_VALU
	tad	_6201
	rdf
	dca	fstrnewtokcdf

	tad	fstrnewtoklen	; compute number of words that will be available when done copying
	cma iac			; ... - 1 for the new char and 1s comp for isz
	tad	fstrtoklen	; ... eg, -8 if we just went from 7 to 15 words
	dca	fstrroom

	tad	fstrcdf		; set up pointer to old string
	dca	fstroldtokcdf
	cma			; - point just before old string length word
	tad	fstrlnptr
	dca	ai17

	tad	ai10		; save pointer to where new string length word goes
	iac
	dca	fstrlnptr

	tad	fstrroom	; compute negative number of words to copy
	tad	_0004		;T_VALU=3
	dca	fstrralcount	; - if we just added 8 words,
				;   old token was 8 words cuz we always double size
				;   so fstrroom would be -8
				;   and there are 4 words to copy from the old node
				;   so add 4 to the -8 to get -4

fstrralloop:
fstroldtokcdf: .word .-.
	tadi	ai17		; copy old string to new string
fstrnewtokcdf: .word .-.
	dcai	ai10
	isz	fstrralcount
	jmp	fstrralloop

	tad	ai10		; save pointer just before where next char goes in new token
	dca	fstrchptr
	tad	evaltok		; start working on new token
	dca	fstrouttok
	tad	fstrnewtokcdf
	dca	fstrcdf
	tad	fstrnewtoklen
	dca	fstrtoklen

	tad	fstroldai10
	dca	ai10

fstrappend:
fstrcdf: .word	.-.		; address output string token
	iszi	fstrlnptr	; increment output string length
	isz	fstrchptr	; store output character in token
	tad	fstrchar
	dcai	fstrchptr
fstrrescdf: .word .-.		; restore df register
	jmpi	fstrprintac

fstrchar:      .word .-.
fstrnewtoklen: .word .-.
fstroldai10:   .word .-.
fstrralcount:  .word .-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (strlen string) => integer
f_strlen:
	jmsi	_checkmiss
	jmsi	_nextstr
	jmsi	_checklast
	tadi	ai10
	jmpi	_retsint12



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (strpos needle haystack [start]) => integer
; returns #F if not found
f_strpos:
	jmsi	_checkmiss
	jmsi	_nextstr	; get needle string
	tad	evaltok		; save from recursion and garbage collection
	jmsi	_pushcp
	jmsi	_checkmiss
	jmsi	_nextstr	; get haystack string
	tad	evaltok		; save from recursion and garbage collection
	jmsi	_pushcp
	jmsi	_nextint	; get optional haystart index
	sna cla
	tadi	ai10
	dca	haystart
	jmsi	_checklast	; no more arguments allowed

	jmsi	_popcp		; get haystack string token
	jmsi	_access		; access its memory
	tad	_0002		;T_VALU=3
	dca	ai10
	tadi	ai10		; get haystack string length
	dca	haylen
	tad	ai10		; save haystack string pointer
	dca	hayptr
	tad	_6201		; save haystack string data frame
	rdf
	dca	strposhaycdf

	jmsi	_popcp		; get needle string token
	jmsi	_access		; access its memory
	tad	_0002		;T_VALU=3
	dca	ai11
	tadi	ai11		; get needle string length
	sna
	jmpi	_retsint12	; needle string null, always found at index 0
	dca	nedlen		; save needle length
	tadi	ai11		; save negative first char of needle
	cma iac
	dca	nedfirst
	tad	ai11		; save needle pointer
	dca	nedptr
	tad	_6201		; save needle data frame
	rdf
	dca	strposnedcdf

	tad	haystart	; subtract start index and needle length from haystack length
	cma iac
	tad	haylen		; compute number of times through main loop
	cma			; make it 1s complement suitable for isz
	tad	nedlen		; eg, if both strings same length, make this -1
	sma
	jmpi	_retfalse	; not found if needle longer than haystack
	dca	haylen

	; nedfirst = negative first char of needle
	; nedptr   = needle string pointer (points to first char word)
	; nedcdf   = needle string cdf
	; nedlen   = needle string length
	; hayptr   = haystack string pointer (points to length word)
	; haycdf   = df   = haystack string cdf
	; haylen   = neg number of times through strposloop2
	; haystart = number of chars to skip in haystack

	tad	hayptr		; get pointer to first char to check
	tad	haystart
	dca	ai10
	tad	strposhaycdf
	dca	.+1
	.word	.-.

strposloop2:
	tadi	ai10		; get haystack character
	tad	nedfirst	; see if it matches first needle char
	sna cla
	jmp	strposfoundfirst
strposcont2:
	isz	haylen		; see if more characters in haystack
	jmp	strposloop2
	jmpi	_retfalse	; not found, return false

strposfoundfirst:

	; ai10 = points to first matching char

	; nedptr   = needle string pointer (points to first char word)
	; nedcdf   = needle string cdf
	; nedlen   = needle string length
	; hayptr   = haystack string pointer (points to length word)
	; haycdf   = haystack string cdf

	tad	ai10		; copy pointer to haystack char that matches first needle char
	dca	ai12
	tad	nedptr		; set up pointer to first needle string char
	dca	ai11
	tad	nedlen		; get negative length that excludes first char
	cma iac			; eg, if needle strlen is 2, we want -1, ie, 1 more char to check
	iac
	sna
	jmp	strposfound	; - needle is just one char, that char was found
	dca	strposcount	; number of chars remaining to check
strposloop3:
strposnedcdf: .word .-.
	tadi	ai11		; get next needle char
	cma iac
strposhaycdf: .word .-.
	tadi	ai12		; see if matches next haystack char
	sza cla
	jmp	strposcont2	; - mismatch, resume searching haystack for first needle char
	isz	strposcount	; matches, see if more needle chars to match
	jmp	strposloop3
strposfound:
	tad	hayptr		; found, return zero-based index
	cma
	tad	ai10
	jmpi	_retsint12

haylen:   .word	.-.
hayptr:   .word	.-.
nedfirst: .word	.-.
nedlen:   .word	.-.
nedptr:   .word .-.
haystart:
strposcount: .word .-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (int value) => integer
;  value string : convert numeric string to integer (returns #F if invalid conversion)
;          char : convert char to ascii integer
;          bool : #F => 0; #T => 1
f_int:
	jmsi	_checkmiss	; make sure there is an argument
	jmsi	_nexteval	; evaluate the argument
	jmsi	_accai10	; access the argument memory
	jmsi	_checklast	; make sure there are no more args
	tadi	ai10		;T_TYPE=0 : get value type
	isz	ai10		;T_VALU=3 : point just before token value
	isz	ai10
	tad	_7775		;TT_STR=3 : check for string
	sna
	jmp	stringtoint
	tad	_7775		;TT_BOO=6 : check for boolean
	sna
	jmp	booltoint
	tad	_7777		;TT_CHR=7 : check for character
	sna cla
	jmp	chartoint
	tad	_intmsg1	; unsupported type
	jmpi	_evalerr
booltoint:
chartoint:
	tadi	ai10		; bool or char, get 12-bit value
	jmpi	_retsint12
stringtoint:
	tadi	ai10		; T_VALU[0] : get string length
	jmsi	_strtoint	; convert string to integer
	sza cla			; make sure all chars converted
	jmpi	_retfalse	; if not, return #F
	tad	_0005		;T_TYPE=3
	jmsi	_allrestok	; allocate integer token
		.word	TT_INT
	tad	umul_prod+0	; get low word being returned
	dcai	ai10		; store in token
	tad	umul_prod+1	; get high word being returned
	dcai	ai10		; store in token
	jmpi	_evalret

_intmsg1: .word	intmsg1
_strtoint: .word strtoint



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (userdefinedfunction arguments ...)
;  T_VALU[1] = symbol table frame
;  T_VALU[2] = param name first list entry or varargsparamname
;  T_VALU[3] = function body token
userfuncva:
	cma
userfunc:
	dca	ufvarargs	; 0000=fixed args; 7777=variable args
	tadi	ai10		; T_VALU[1] = outer symbol frame
	dca	ufouter
	tadi	ai10		; T_VALU[2] = param name list
	dca	ufplist
	tadi	ai10		; T_VALU[3] = function body token
	jmsi	_pushcp
	iac			;SF_SIZE=2 : allocate new symbol frame
	jmsi	_l2malloc
	dca	addsymfrm
	tad	addsymfrm
	jmsi	_accai10
	tad	ufouter		;SF_OUTER=0 : outer symbol frame
	dcai	ai10		; = frame where function was defined
	dcai	ai10		;SF_ENTRIES=1 : no entries in the frame yet

	; on stack = function body token
	; addsymfrm = symbol frame to put parameter = argument definitions in
	; ufvarargs = 0000: fixed args; 7777: variable args
	; ufplist = param name list entries or varargsparamname token
	; evlnext = argument list entries
	isz	ufvarargs
	jmp	ufprmloop
	tad	ufplist		; varargs, save varargsparamname token
	jmsi	_pushcp
	jmsi	_makelist	; make list of arguments, might be lots of recursion
	tad	evaltok
	dca	addsymval	; save list token to be varargs argument value
	dca	ufplist		; there are no more param name tokens
	jmsi	_popcp		; set the one param symbol = arg list
	jmp	ufpaddsyment
ufprmloop:
	cdf_0
	tad	ufplist		; see there is another parameter to fill in
	sna
	jmp	ufprmdone
	jmsi	_pushcp		; save param list entry pointer
	tad	addsymfrm	; save func param frame from recursion
	jmsi	_pushcp
	jmsi	_checkmiss	; there should be another argument to assign to the parameter
	tad	evlnext
	jmsi	_accai10	; access the argument list entry
	tadi	ai10		;L_NEXT=0 : get pointer to next argument list entry
	jmsi	_pushcp		; -> pop to evlnext
	tadi	ai10		;L_TOKN=1 : get pointer to argument token (might be an insanely complex expression)
	jmsi	_evaluate	; evaluate to get the corresponding value (massive recursion here)
	dca	addsymval

	jmsi	_popcp		; restore recursion values
	dca	evlnext		; - next argument list entry
	jmsi	_popcp
	dca	addsymfrm	; - call instance ufsymframe
	jmsi	_popcp		; - this param list entry

	jmsi	_accai10	; access the parameter list entry
	tadi	ai10		;L_NEXT=0 : get pointer to next parameter list entry
	dca	ufplist
	tadi	ai10		;L_TOKN=1 : get pointer to parameter name TT_SYM token
	cdf_0
ufpaddsyment:
	jms	symtok2nament	; get name entry from symbol token
	dca	addsymnam

	jmsi	_addsyment	; add parameter symbol entry to function's symbol frame
	jmp	ufprmloop	; process next parameter and argument
ufprmdone:
	jmsi	_checklast	; no more params, see if too many arguments given

	; parameters defined to arguments, evaluate function body and return result

	jmsi	_popcp		; get function body token
	dca	ufbody

	;*TCO* check for tail call optimization

	;                       [lower addresses]
	;*TCO*  stkcpptr[-3] = outer stack frame
	;*TCO*  stkcpptr[-2] = outer evlnext
	;*TCO*  stkcpptr[-1] = outer evllist
	;                       <-- stkcpptr
	;                           .
	;                           .
	;                           .
	;                       <-- stkdaptr
	;*TCO*  stkdaptr[+1] = evaluate return address
	;                       [higher addresses]

	;*TCO* stack built this way by earlier code marked *TCO*

	cdf_0
	tadi	_stkdaptrb	; get stkdaptr
	dca	ai10
	stackcdf
	tadi	ai10		; peek at stacked evaluate return address in stkdaptr[+1]
	tad	_nufevalret	; - must have been called at ufevalret below
	sza cla
	jmp	ufnotco		; can't tco if this function call is something else like an intermediate argument
	cdf_0
	iszi	_stkdaptrb	; pop the redundant evaluate to ufevalret
	jmsi	_popcp		; restore outer evllist
	dcai	_evllist
	jmsi	_popcp		; restore outer evlnext
	dca	evlnext
	jmp	uftco		; leave the old 'outer stack frame' push in place
ufnotco:
	tad	symframe	;*TCO* save outer symbol frame from recursion
	jmsi	_pushcp		;*TCO* (this is the stkptr[-4] location for TCO checking)
uftco:
	tad	addsymfrm	; make arg list current symbol frame
	dca	symframe
	tad	ufbody		; evaluate function body
	jmsi	_evaluate	; (massive recursion here)
ufevalret:
	jmsi	_popcp		; restore original symbol frame
	dca	symframe
	jmpi	_evalret	; return value in evaltok

_nufevalret: .word -ufevalret
_stkdaptrb:  .word stkdaptr
_evllist:    .word evllist
ufouter:     .word .-.
ufplist:     .word .-.
ufbody:      .word .-.
ufvarargs:   .word .-.

_makelist:   .word makelist

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; extract name entry from symbol token
;  input:
;   ac = composite pointer to symbol token
;  output:
;   ac = composite pointer to name entry
;  scratch:
;   df
symtok2nament: .word .-.
	jmsi	_access		; access symbol token
	dca	st2neptr	;T_TYPE=0
	tadi	st2neptr	; make sure it is a symbol
	tad	_7774		;TT_SYM=4
	sza cla
	jmp	st2neerr1
	tad	st2neptr	;T_VALU=3
	tad	_0005
	dca	st2neptr
	tadi	st2neptr	; T_VALU[2] : get name entry composite pointer
	jmpi	symtok2nament
st2neerr1:
	tad	_st2nemsg1
	jmpi	_evalerr

st2neptr: .word	.-.
_st2nemsg1: .word st2nemsg1



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (substr string start [stop]) => string
; start,stop are zero-based integers
; negative means chars from end
; stop defaults to end of string
; stop before start returns #F
; start or stop beyond end returns #F
; start eq stop returns null string
f_substr:
	tad	substrinlen	; save from recursion
	jmsi	_pushda
	tad	substrstart
	jmsi	_pushda
	tad	substrstop
	jmsi	_pushda
	tad	substrintok
	jmsi	_pushcp
	jmsi	_checkmiss	; make sure there is a string argument
	jmsi	_nextstr	; evaluate the input string argument
	tadi	ai10		; save input string length
	dca	substrinlen
	cdf_0
	tadi	_nextstrtok	; save input string token
	dca	substrintok
	jmsi	_checkmiss	; make sure there is a start argument
	jmsi	_nextint	; get the start integer
	tadi	ai10
	spa
	tad	substrinlen	; - if negative, make it allegedly positive
	spa
	jmp	substrfalse	; if still negative, return false
	dca	substrstart	; save substring start index
	tad	substrinlen	; default stop is end-of-string
	dca	substrstop
	jmsi	_nextint	; maybe there is a stop argument
	sza cla
	jmp	substrnostop
	tadi	ai10		; get stop value
	spa
	tad	substrinlen	; - if negative, make it allegedly positive
	dca	substrstop	; save substring stop index
substrnostop:
	tad	substrstop	; return #F if stop gt length
	cma iac
	tad	substrinlen
	spa cla
	jmp	substrfalse
	tad	substrstart	; return #F if start gt stop
	cma iac
	tad	substrstop
	spa
	jmp	substrfalse
	dca	substroutlen	; output length = stop - start
	tad	substroutlen	; alloc output token
	tad	_0004		;T_VALU=3
	jmsi	_allrestok
		.word	TT_STR
	tad	substroutlen	; T_VALU[0] = output string length
	dcai	ai10
	tad	substroutlen	; negate substroutlen for isz
	sna
	jmp	substrret	; - zero, we're done
	cma iac
	dca	substroutlen
	tad	_6201		; save output frame
	rdf
	dca	substroutcdf
	tad	substrintok	; access input token
	jmsi	_access
	tad	_0003		;T_VALU=3 : point to length word, ie, just before string word 0
	tad	substrstart	; point just before first word to copy
	dca	ai11
	tad	_6201		; save input frame
	rdf
	dca	substrincdf
substrloop:
substrincdf: .word .-.		; copy input to output
	tadi	ai11
substroutcdf: .word .-.
	dcai	ai10
	isz	substroutlen
	jmp	substrloop
substrret:
	jmsi	_popcp		; restore recursion
	dca	substrintok
	jmsi	_popda
	dca	substrstop
	jmsi	_popda
	dca	substrstart
	jmsi	_popda
	dca	substrinlen
	jmpi	_evalret	; return token in evaltok
substrfalse:
	cla
	tad	_falsetok
	dca	evaltok
	jmp	substrret

substrinlen:  .word .-.		; input string length
substrintok:  .word .-.		; input string token
substroutlen: .word .-.		; output string length (neg for isz)
substrstart:  .word .-.		; index to start at
substrstop:   .word .-.		; index to stop at

_nextstrtok:  .word nextstrtok

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (charat index string) => char
; zero-based index
; -1 = last char; -2 = next to last; ...
; returns #F if out of range
f_charat:
	jmsi	_checkmiss
	jmsi	_nextint	; get index
	tadi	ai10		; save from recursion
	jmsi	_pushda
	jmsi	_checkmiss
	jmsi	_nextstr	; get string
	jmsi	_checklast
	jmsi	_popda		; restore index
	sma
	jmp	charatpos
	tadi	ai10		; negative index, add string length
	spa
	jmpi	_retfalse	; - still before beginning of the string
	dca	chrindex	; resulting positive index
	jmp	charatstart
charatpos:
	dca	chrindex	; save positive index
	tadi	ai10		; compare with string length
	cma iac
	tad	chrindex
	sma cla
	jmpi	_retfalse	; - not found if index >= length
charatstart:
	tad	ai10		; increment pointer to indexed char
	tad	chrindex
	dca	ai10
	tadi	ai10		; get character
retchar:
	dca	chrindex
	tad	_0004		;T_VALU=3
	jmsi	_allrestok
		.word	TT_CHR
	tad	chrindex
	dcai	ai10
	jmpi	_evalret

chrindex: .word	.-.



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; add symbol to symbol frame
;  input:
;   ac = zero
;   addsymfrm = frame to add it to composite pointer
;   addsymnam = name entry composite pointer
;   addsymval = value token composite pointer
;  output:
;   ac = zero
;   symbol added to the frame
;   addsymfrm,addsymnam,addsymval = preserved
;  scratch:
;   ai10, df
addsyment: .word .-.
	tad	addsymfrm	; get symbol frame (SF_...) to add symbol to
	jmsi	_access		; access the symbol table frame
	dca	ai10		;SF_ENTRIES=1 : get pointer to first entry for the frame
aseentloop:
	; df,ai10 = just before pointer to symbol entry node (SE_...) to check
	tadi	ai10		; get pointer to symbol entry node
	sna
	jmp	asemalloc	; if no more entries, go malloc a new one
	jmsi	_access		; access the symbol table entry
	dca	ai10		;SE_NAME1=1
	tad	_7775		; three symbols per entry
	dca	asecmpcount
asecmploop:
	; ac   = zero
	; df   = accesses symbol entry node (SE_...)
	; ai10 = just before SE_NAMEn of the symbol entry node
	tadi	ai10		;SE_NAME1=1,SE_NAME2=3,SE_NAME3=5 : get symbol name entry
	sna
	jmp	asereplace	; - entry not used
	cma iac
	tad	addsymnam	; see if it matches name we are trying to add
	sna cla			; check for names match
	jmp	asereplace	; if so, replace the old value with new one
	; ac   = zero
	; df   = accesses symbol entry node (SE_...)
	; ai10 = just before SE_VALUn of the symbol entry node
	isz	ai10		;SE_VALU1=2,SE_VALU2=4,SE_NAME3=6 no match, skip over value
	isz	asecmpcount	; see if more names to search in this entry (1, 2 or 3)
	jmp	asecmploop
	tad	ai10		; back up pointer to SE_NEXT in the entry
	tad	_7771		;SE_NEXT=0;SE_NAME3=6
	dca	ai10		; no more names in entry, check next entry
	jmp	aseentloop

	; need to malloc a new SE_... node to put the entry in
	; df,ai10 = points to where to put pointer to newly allocated SE_... node
	; ac = zero
asemalloc:
	tad	_6201		; save df where the new SE_... node pointer goes
	rdf
	dca	asemalcdf
	tad	ai10		; save address where the new SE_... node pointer goes
	dca	asemalptr
	iac			;SE_NAME3=6
	jmsi	_l2malloc	; allocate a new SE_... node
asemalcdf: .word .-.		; save pointer to the node
	dcai	asemalptr
	tadi	asemalptr	; access the new SE_... node
	jmsi	_accai10	; zero it out except fill in first entry
	dcai	ai10		;SE_NEXT=0
	tad	addsymnam
	dcai	ai10		;SE_NAME1=1
	tad	addsymval
	dcai	ai10		;SE_VALU1=2
	dcai	ai10		;SE_NAME2=3
	dcai	ai10		;SE_VALU2=4
	dcai	ai10		;SE_NAME3=5
	dcai	ai10		;SE_VALU3=6
	jmpi	addsyment

	; add entry to end of current SE_... node
	; df,ai10 = SE_NAMEn that is zero or has same name
	; ac = zero
asereplace:
	tad	ai10		; back up ai10 so we can overwrite entry
	tad	_7777
	dca	ai10
	tad	addsymnam	; write SE_NAME<1,2,3>
	dcai	ai10
	tad	addsymval	; write SE_VALU<1,2,3>
	dcai	ai10
	jmpi	addsyment

asemalptr:   .word .-.
asecmpcount: .word .-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; allocate result token
;  input:
;   ac = number of words needed
;   retadr+0 = token type TT_...
;  output:
;   ac = zero
;   evaltok = composite pointer to token
;   df,ai10 = points just before T_VALU field of token
allrestok: .word .-.
	jmsi	_malloc		; allocate memory
	dca	evaltok
	cdf_0
	tadi	allrestok
	dca	arttype
	isz	allrestok
	tad	evaltok
	jmsi	_accai10	; access memory
	tad	arttype		;T_TYPE=0
	dcai	ai10
	dcai	ai10		;T_SPOS=1
	dcai	ai10		;;TODO;; fill in with list token T_SPOS
	jmpi	allrestok	;T_TYPE=3

arttype: .word	.-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; get and evaluate next token in list
;  input:
;   evlnext = list element to get token from or zero if end of list
;  output:
;   ac = 0: was at end of list
;     else: evaluated token composite pointer
;   evlnext = next element
;   evaltok = original unevaluated token (in case of error message)
;  scratch:
;   ai10, df, and anything munged by evaluate not protected from recursion
nexteval: .word .-.
	jms	nextelem	; get next token in list
	sna
	jmp	nexevdone	; - am at end of list
	dca	nxetoken
	tad	nexteval	; about to call evaluate, save from recursion
	jmsi	_pushcp
	tad	nxetoken
	jmsi	_pushcp
	tad	nxetoken	; evaluate the token
	jmsi	_evaluate
	dca	nxetoken	; save result
	jmsi	_popcp		; restore original token we got from the list
	dca	evaltok		; ... put it here in case caller prints error message
	jmsi	_popcp		; restore return address
	dca	nexteval
	tad	nxetoken	; restore result
nexevdone:
	jmpi	nexteval

nxetoken: .word .-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; get next token in list (don't evaluate)
;  input:
;   evlnext = list element to get token from or zero if end of list
;  output:
;   ac = 0: was at end of list
;       evaltok = unaltered
;     else: token composite pointer
;       evlnext = next element
;       evaltok = same as ac (in case of error message)
;  scratch:
;   ai10, df
nextelem: .word .-.
	tad	evlnext
	sna
	jmp	nexeldone
	jmsi	_accai10
	tadi	ai10		;L_NEXT=0
	dca	nxetoken
	tadi	ai10		;L_TOKN=1
	dca	evaltok
	tad	nxetoken
	dca	evlnext
	tad	evaltok
nexeldone:
	jmpi	nextelem



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (cond (bool begin...) ...)
; returns #F if no condition is true
f_cond:
condloop:
	jmsi	_nextelem	; get next (bool begin...) argument without eval
	sna
	jmpi	_retfalse	; return #F if no more args
	jmsi	_accai10	; make sure arg is a list
	tadi	ai10		;T_TYPE=0
	tad	_7776		;TT_LIS=2
	sza cla
	jmp	conderr1
	tad	evlnext		; save pointer to remaining args
	jmsi	_pushcp
	isz	ai10		; set arg pointer to supposed bool
	isz	ai10
	tadi	ai10		;T_VALU=3
	dca	evlnext
	jmsi	_checkmiss
	jmsi	_nextboolb	; get the bool value
	sza cla
	jmp	conddone
	jmsi	_popcp		; false, check next (bool begin...) argument
	dca	evlnext
	jmp	condloop
conddone:
	jmsi	_popcp		; true, forget about remaining (bool begin...) args
	cla
	jmpi	_f_begin	; evaluate the begin stuff as a begin block
conderr1:
	tad	_condmsg1	; each arg must be a list
	jmp	_evalerr

_condmsg1:  .word condmsg1
_f_begin:   .word f_begin
_nextboolb: .word nextbool

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; print token
;  input:
;   ac = composite pointer to token
;  output:
;   ac = 0
;  scratch:
;   ai10, df
tokprint: .word	.-.
	dca	tptype		; print token composite pointer
	tad	tptype
	jmsi	_ttprintoct4
	tad	_0040
	jmsi	_ttprintac
	tad	tptype
	jmsi	_accai10	; access token memory
	tad	_6201		; save token dataframe
	rdf
	dca	tpcdf1
	tadi	ai10		;T_TYPE=0 : print token type
	dca	tptype
	tad	tptype
	jmsi	_ttprintoct4
	tad	_0040		; print a space
	jmsi	_ttprintac
	tadi	ai10		;T_SPOS=1 : print source position
	dca	tpfirst
	tadi	ai10
	jmsi	_ttprintoct4
	tad	tpfirst
	jmsi	_ttprintoct4
	tad	_0040		; print a space
	jmsi	_ttprintac
	tad	tptype		;T_VALU=3 : passed to tpint,tplist,tpstring,tpsymbol,tpbool
	tad	_7777		;TT_INT=1 : integer token
	sna
	jmpi	_tpint
	tad	_7777		;TT_LIS=2 : list token
	sna
	jmpi	_tplist
	tad	_7777		;TT_STR=3 : string token
	sna
	jmp	tpstring
	tad	_7777		;TT_SYM=4 : symbol token
	sna
	jmp	tpsymbol
	tad	_7776		;TT_BOO=6 : boolean token
	sna
	jmpi	_tpbool
	tad	_7777		;TT_CHR=7 : character token
	sna
	jmpi	_tpchar
tpdone:
	jmsi	_ttprintcrlf
	jmpi	tokprint

tpstring:
	tad	tpcdf1
	dca	tpcdf2
	tad	_0042		; print a quote (")
	jmsi	_ttprintac
	tadi	ai10		;T_VALU=3 : get number of chars
	sna
	jmp	tpstrdone
	cma iac
	dca	tpneglen	; save negative number of chars
tpcdf2:	.word	.-.		; address token memory
tpstrloop:
	tadi	ai10		; read character from token
	jmsi	_tpstrchar	; print it, maybe with escape
	isz	tpneglen	; see if there are more chars in string
	jmp	tpstrloop	; repeat if so
tpstrdone:
	tad	_0042		; print a quote (")
	jmsi	_ttprintac
	jmp	tpdone

tpsymbol:
	isz	ai10		;T_VALU=3 : T_VALU[0,1] : skip frameindex,entryindex
	isz	ai10
	tadi	ai10		; T_VALU[2] : get name entry node
	jms	printname
	jmp	tpdone

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; print name node name string
;  input:
;   ac = composite pointer to name node
;  output:
;   ac = zero
;  scratch:
;   df, ai10
printname: .word .-.
	jmsi	_access
	dca	ai10		;N_LEN=1 : get number of chars in name
	tadi	ai10
	cma iac
	dca	tpneglen	; save negative number of chars
tpsymloop:
	tadi	ai10		;N_STR=2 read word from token
	dca	tpfirst
	tad	tpfirst		; print first char
	bsw
	jmsi	_tpsymchar
	isz	tpneglen	; see if there is a second char
	skp
	jmpi	printname
	tad	tpfirst
	jmsi	_tpsymchar
	isz	tpneglen	; see if more chars
	jmp	tpsymloop
	jmpi	printname

tpcdf1:	  .word	.-.
tptype:   .word .-.
tpneglen: .word .-.
tpfirst:  .word .-.

_tpbool: .word	tpbool
_tpchar: .word	tpchar
_tpint:	 .word	tpint
_tplist: .word	tplist
_tpstrchar: .word tpstrchar
_tpsymchar: .word tpsymchar



	.align	00200

	; print integer token value

tpint:
	cdf_0
	tadi	_tpcdf1		; access token memory frame
	dca	.+1
	.word	.-.
	tadi	ai10		;T_VALU=3 : get 24-bit value (little endian)
	dca	udiv_dvquo+0
	tadi	ai10
	dca	udiv_dvquo+1
	tad	udiv_dvquo+1
	jmsi	_ttprintoct4
	tad	udiv_dvquo+0
	jmsi	_ttprintoct4
	tad	_0040
	jmsi	_ttprintac
	tad	udiv_dvquo+1	; see if negative
	sma cla
	jmp	tpintpos
	tad	_0055		; negative, print a '-'
	jmsi	_ttprintac
	tad	udiv_dvquo+0	; negate the value
	cll cma iac
	dca	udiv_dvquo+0
	tad	udiv_dvquo+1
	cma
	szl
	iac
	dca	udiv_dvquo+1
tpintpos:
	jmsi	_ttprintint	; print it out
	jmpi	_tpdone		; all done

_tpcdf1: .word	tpcdf1
_tpdone: .word	tpdone

	; print boolean token value

tpbool:
	tad	_0043		; print a '#'
	jmsi	_ttprintac
	cdf_0
	tadi	_tpcdf1		; access token memory frame
	dca	.+1
	.word	.-.
	tadi	ai10		;T_VALU=3 : 0:false; 1:true
	sza cla
	tad	_0016		; set to print 'T' instead of 'F'
	tad	_0106		; print a 'F'
	jmsi	_ttprintac
	jmpi	_tpdone		; all done

	; print character token

tpchar:
	tadi	ai10		;T_VALU=3 : 12-bit character
	dca	tplelem
	tad	tplelem
	jmsi	_ttprintoct4
	tad	_0043		; print a '#'
	jmsi	_ttprintac
	tad	_0134		; print a '\'
	jmsi	_ttprintac
	tad	tplelem
	jms	tpstrchar	; print it, maybe with escape
	jmpi	_tpdone		; all done

	; print list token
	;  ac = zero
	;  df/ai10 = points to just before T_VALU of list token

tplist:
	tadi	ai10		;T_VALU=3 : get pointer to first list element
	dca	tplelem
	cdf_0
	tadi	_tokprint	; save this for recursion
	jmsi	_pushda
	tad	_0050		; print '('
	jmsi	_ttprintac
	jmsi	_ttprintcrlf
tplistloop:
	tad	_0040		; print ' '
	jmsi	_ttprintac
	tad	tplelem		; see if any more in list
	sna
	jmp	tplistdone
	jmsi	_accai10	; access the list element
	tadi	ai10		; save pointer to next element in list
	jmsi	_pushcp
	tadi	ai10		;L_TOKN=1 : read composite pointer to token
	jmsi	_tokprint	; print the element token
	jmsi	_popcp		; pop pointer to next element in list
	dca	tplelem
	jmp	tplistloop	; maybe there are more tokens in list to print
tplistdone:
	tad	_0051		; print ')'
	jmsi	_ttprintac
	jmsi	_popda		; restore recursion
	cdf_0
	dcai	_tokprint
	jmpi	_tpdone

_0134:	.word	00134
tplelem: .word	.-.		; composite pointer to list element of token being printed

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; print string character, possibly with escape prefix
;  input:
;   ac = 8-bit string character
;  output:
;   ac = zero
;   df = scratch
tpstrchar: .word .-.
	and	_0377		; make sure no garbage bits in top
	dca	tpstchar
	tad	tpstchar
	tad	_7740		; check for control character (000..037)
	spa
	jmp	tpstrctl
	tad	_7776		; check for quote (" = 042)
	sna
	jmp	tpstresc
	tad	_7777		; check for pound (# = 043)
	sna cla
	jmp	tpstresc
tpstrchok:
	tad	tpstchar	; if not, print given char as is
	jmsi	_ttprintac
	jmpi	tpstrchar
tpstrctl:
	tad	_0140		; fold column 1 onto column 3
	dca	tpstchar	; eg, 015 CR => #M ctrl-M
tpstresc:
	tad	_0043		; escape it, print a '#'
	jmsi	_ttprintac
	jmp	tpstrchok

tpstchar: .word	.-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; print symbol character
;  input:
;   ac = 6-bit symbol character
;  output:
;   ac = zero
;   df = scratch
tpsymchar: .word .-.
	and	_0077		; make sure no garbage bits in top
	tad	_0040		; shift column
	jmsi	_ttprintac	; print
	jmpi	tpsymchar



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; interrupt service
intserv:
	dca	in_accum	; save accumulator
	gtf			; get flags
	dca	in_flags	; save flags
	clsa			; clear realtime clock int req
	cla
	ksf			; check keyboard
	skp
	jmpi	_kbintserv
kbintret:
	tsf			; check printer
	skp
	jmpi	_ttintserv
ttintret:
	tad	in_flags
	rtf			; restore flags
	cla
	tad	in_accum	; restore accumulator
	jmpi	0

in_accum: .word .-.
in_flags: .word .-.
_kbintserv: .word kbintserv
_ttintserv: .word ttintserv

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; read line from keyboard
; returns with kbbuff null terminated
;  and kbnext pointing to that null
kbread:	.word	.-.
	kbbuffcdf
kbread_reset:
	cla			; point to beginning of buffer
	tad	_kbbuff
	dca	kbnext
kbread_loopcla:
	cla
kbread_loop:
	jms	ckctrlc		; check for ctrl-C
	iof
	tad	kbchar		; check for char ready to read
	sna
	jmp	kbread_idle
	dcai	kbnext		; save char in buffer
	dca	kbchar		; clear char buffer
	ion
	tadi	kbnext		; -015 check for <CR>
	tad	_7763
	sna
	jmp	kbread_done
	tad	_7770		; 015-025 check for ctrl-U
	sna
	jmp	kbread_ctlu
	tad	_7765		; 025-040 ignore other control chars
	spa
	jmp	kbread_loopcla
	tad	_7641		; 040-177 check for rubout
	sna cla
	jmp	kbread_rubit
	tadi	kbnext		; printable, print it out
	jmsi	_ttprintac
	isz	kbnext		; increment pointer
	tad	kbnext		; see if buffer full
	tad	kbnend
	sza cla
	jmp	kbread_loop	; repeat if more room
kbread_done:
	dcai	kbnext		; null terminate
	jmsi	_ttprintcrlf
	jmpi	kbread
kbread_idle:
	jmsi	_wfint		; nothing ready to read, wait
	jmp	kbread_loop	; then check again
kbread_ctlu:
	tad	_cumsg
	jmsi	_ttprint12z
	jmp	kbread_reset
kbread_rubit:
	tad	kbnext		; see if at beginning of buffer
	tad	kbnbeg
	sna
	jmp	kbread_loop	; if so, nothing to rubout
	tad	kbbegm1		; if not, decrement pointer
	dca	kbnext
	osr
	and	_srvtrubs
	sza cla
	jmp	kbread_vtrubs
	tadi	kbnext		; print deleted char
	jmsi	_ttprintac
	jmp	kbread_loop	; go back to read more
kbread_vtrubs:
	tad	_bsspbs		; backspace-space-backspace
	jmsi	_ttprint12z
	jmp	kbread_loop	; go back to read more

kbintserv:
	krb
	and	_0177
	tad	_7775
	sna
	jmp	kbintreq_cc
	tad	_0003
	dca	kbchar
	jmpi	_kbintret
kbintreq_cc:
	isz	ccflag
	jmpi	_kbintret
	jmp	kbintreq_cc

_srvtrubs: .word SR_VTRUBS
_bsspbs: .word	bsspbs
_cumsg:	.word	cumsg
_kbintret: .word kbintret
ccflag:	.word	.-.
kbchar:	.word	.-.
kbbegm1: .word	kbbuff-1
kbnbeg:	.word	-kbbuff
kbnend:	.word	-(kbbuff+kbbufflen-1)
kbnext:	.word	.-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; check for control-C
;  input:
;   ac = zero
;   ccflag = nonz iff control-C pressed
;   initdone = 7777 iff initialization complete
;  output:
;   ac = zero
;   ccflag = zero
ckctrlc: .word	.-.
	tad	ccflag		; see if control-C pressed
	and	initdone	; and see if initialization complete
	sna cla
	jmpi	ckctrlc		; done if not
	dca	ccflag		; clear control-C flag
	cdf_1
	tad	_ccmsg		; init complete, print control-C message
	jmsi	_ttprint12z
	jmpi	_closeloop	; read next command from keyboard

_ccmsg:     .word ccmsg
_closeloop: .word closeloop
initdone:   .word 0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; allocate memory block
malloc1:
	jmsi	_malloc
	cif_1
	jmpi	.+1
	.word	malloc1ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; add static symbol
addsyment1:
	jmsi	_addsyment
	cif_1
	jmpi	.+1
	.word	addsyment1ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; print 4-digit octal number
ttprintwait1:
	jmsi	_ttprintwait
	cif_1
	jmpi	.+1
	.word	ttprintwait1ret



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; print crlf
; returns with ac = zero
ttprintcrlf: .word .-.
	cla
	tad	_0012		; 012 = LF
	jms	ttprintac
	jmpi	ttprintcrlf

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; print null-terminated 12-bit character string
;  input:
;   ac = string address
;   df = string frame
;  output:
;   ac = zero
ttprint12z: .word .-.
	dca	ttp12zptr
ttp12z_loop:
	tadi	ttp12zptr
	sna
	jmpi	ttprint12z
	jms	ttprintac
	isz	ttp12zptr
	jmp	ttp12z_loop

ttp12zptr: .word .-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; print udiv_dvquo as a decimal integer
; returns with ac = zero
ttprintint: .word .-.
	cla
	jmsi	_pushda
tpintdiv:
	tad	_0012
	jmsi	_udiv24
	tad	udiv_remdr
	tad	_0060
	jmsi	_pushda
	tad	udiv_dvquo+0
	sza cla
	jmp	tpintdiv
	tad	udiv_dvquo+1
	sza cla
	jmp	tpintdiv
tpintpr:
	jmsi	_popda
	sna
	jmpi	ttprintint
	jms	ttprintac_raw
	jmp	tpintpr

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; print ac in 4 digits octal
; returns with ac = zero
ttprintoct4: .word .-.
	cll cml
	ral
ttprintoct4_loop:
	rtl
	dca	ttpoct_val4
	tad	ttpoct_val4
	ral
	and	_0007
	tad	_0060
	jms	ttprintac_raw
	tad	ttpoct_val4
	and	_7774
	cll ral
	sza
	jmp	ttprintoct4_loop
	jmpi	ttprintoct4

ttpoct_val4: .word .-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; print character in ac, insert CR before LF
; returns with ac = zero
ttprintac: .word .-.
	tad	_7766		; check for LF
	sza
	jmp	ttprintac_end
	tad	_0015		; insert CR
	jms	ttprintac_raw
ttprintac_end:
	tad	_0012
	jms	ttprintac_raw
	jmpi	ttprintac

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; print character in ac without inserting CR before LF
; returns with ac = zero
ttprintac_raw: .word .-.
	dca	ttprintac_tmp	; save char to print
	jmsi	_ckctrlcb
ttprintac_loop:
	iof
	tad	ttbusy		; see if busy
	sna cla
	jmp	ttprintac_idle
	tad	ttinsert	; busy, see if ring buffer full
	cma
	tad	ttremove
	and	_0037		;ttbuflen=32
	sza cla
	jmp	ttprintac_room
	jmsi	_wfint		; full, wait then check again
	jmp	ttprintac_loop
ttprintac_idle:
	tad	ttprintac_tmp	; not doing anything, start printing char
	tls
	isz	ttbusy		; remember we're busy
.if OP_TTWAIT
	jms	ttprintwait	; wait for char to print
.else
	cla
.endif
	ion
	jmpi	ttprintac_raw
ttprintac_room:
	tad	_6201		; save data frame
	rdf
	dca	ttprintac_rdf
	ttbuffcdf		; make sure we are accessing ring buffer
	tad	ttprintac_tmp	; busy but room in ring, get char
	dcai	ttinsert	; insert char in fing
ttprintac_rdf: .word .-.	; restore data frame
	tad	ttinsert	; inc pointer with wrap
	iac
	and	_0037		;ttbuflen=32
	tad	ttbuffbeg
	dca	ttinsert
	ion
	jmpi	ttprintac_raw

ttprintac_tmp: .word .-.
_ckctrlcb: .word ckctrlc

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; wait for printer idle
; returns with ac = zero, interrupts off
ttprintwait: .word .-.
	cla
ttpw_loop:
	iof
	tad	ttbusy
	sna cla
	jmpi	ttprintwait
	jmsi	_wfint
	jmp	ttpw_loop

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; interrupt service called whenever tt ready flag is set
; called with ac = zero, returns with ac = zero
ttintserv:
	tad	ttinsert	; see if anything in ring
	cma iac
	tad	ttremove
	sna cla
	jmp	ttintserv_idle
	ttbuffcdf
	tadi	ttremove	; remove char from ring
	tls			; start printing
	cla iac
	tad	ttremove	; inc pointer with wrap
	and	_0037		;ttbuflen=32
	tad	ttbuffbeg
	dca	ttremove
	jmpi	_ttintret
ttintserv_idle:
	tcf			; ring empty, don't interrupt
	dca	ttbusy		; remember we're idle
	jmpi	_ttintret

ttbusy: .word	0		; set when starting to print first char
				; cleared when finished printing last char
ttbuffbeg: .word ttbuff
ttinsert: .word ttbuff		; where to insert next char into ring
ttremove: .word ttbuff		; where to remove next char from ring
_ttintret: .word ttintret



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; unsigned multiply
;  input:
;   ac = 12-bit multiplier
;   umul_mcand = 24-bit multiplicand (little endian)
;  output:
;   ac = 0
;   umul_prod += 24-bit product (little endian)
;  scratch:
;   umul_multr, umul_mcand
umul24:	.word	.-.

.if OP_XARITH

	dca	umul_multr0	; save multiplier
	tad	umul_mcand+0	; load low-order multiplier into MQ
	mql			; AC => MQ; 0 => AC
	tad	umul_prod+0	; load low-order result into AC so it gets added to result
	muy			; MQ * multr0 + AC => AC,MQ
umul_multr0: .word .-.
	tad	umul_prod+1	; add high-order product to result
	dca	umul_prod+1
	mqa			; MQ | AC => AC
	dca	umul_prod+0	; low-order product already added to result

	tad	umul_multr0	; set up multiplier
	dca	umul_multr1
	tad	umul_mcand+1	; load high-order multiplier into MQ
	mql
	tad	umul_prod+1	; load high-order result into AC so it gets added to result
	muy			; MQ * multr0 + AC => AC,MQ
umul_multr1: .word .-.
	cla mqa			; save summed low-order product to high-order result
	dca	umul_prod+1
	jmpi	umul24

.else

umul24_loop:
	cll rar			; check low multiplier bit
	dca	umul_multr	; and shift multiplier right
	snl
	jmp	umul24_skip
	cll			; set, add multiplicand to product
	tad	umul_prod+0
	tad	umul_mcand+0
	dca	umul_prod+0
	ral
	tad	umul_prod+1
	tad	umul_mcand+1
	dca	umul_prod+1
umul24_skip:
	cla cll			; shift multiplicand left
	tad	umul_mcand+0
	ral
	dca	umul_mcand+0
	tad	umul_mcand+1
	ral
	dca	umul_mcand+1
	tad	umul_multr
	sza
	jmp	umul24_loop
	jmpi	umul24

umul_multr: .blkw 1

.endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; unsigned divide 24 by 24
;  input:
;   ac = 0
;   udiv_dvquo = 24-bit dividend (little endian)
;   udiv_ndvsr = 24-bit negative divisor (little endian)
;  output:
;   ac = 0
;   udiv_remdr = 24-bit remainder (little endian)
;   udiv_dvquo = 24-bit quotient (little endian)
;  scratch:
;   udiv_count
udiv2424: .word	.-.
	cla iac
	tad	udiv_ndvsr+1
	sna cla
	jmp	udiv2424_small

	tad	_7750
	dca	udiv_count
	dca	udiv_remdr+0
	dca	udiv_remdr+1
udiv2424_loop:
	; [link] [remdr+1] [remdr+0] [dvquo+1] [dvquo+0] < quotient bits
	;        [ndvsr+1] [ndvsr+0]
	tad	udiv_dvquo+0	; shift link-remdr-dvquo left one bit
	cll ral			; zero goes in bottom for quotient bit
	dca	udiv_dvquo+0
	tad	udiv_dvquo+1
	ral
	dca	udiv_dvquo+1
	tad	udiv_remdr+0
	ral
	dca	udiv_remdr+0
	tad	udiv_remdr+1
	ral
	dca	udiv_remdr+1

	cll cla
	tad	udiv_remdr+0	; add negative divisor to remainder
	tad	udiv_ndvsr+0
	dca	udiv_updatlo
	ral
	tad	udiv_remdr+1
	tad	udiv_ndvsr+1

	snl
	jmp	udiv2424_skip
	dca	udiv_remdr+1	; boosted negated divisor from neg to pos, it fits
	tad	udiv_updatlo
	dca	udiv_remdr+0
	isz	udiv_dvquo+0	; change bottom quotient bit to a one
udiv2424_skip:
	cla
	isz	udiv_count
	jmp	udiv2424_loop
	jmpi	udiv2424

udiv_updatlo: .word .-.

	; 12-bit divisor, use quicker function
udiv2424_small:
	dca	udiv_remdr+1
	tad	udiv_ndvsr+0
	cma iac
	jms	udiv24
	jmpi	udiv2424

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; unsigned divide
;  input:
;   ac = divisor
;   udiv_dvquo = 24-bit dividend (little endian)
;  output:
;   ac = 0
;   udiv_remdr = remainder
;   udiv_dvquo = 24-bit quotient (little endian)
;  scratch:
;   udiv_count, udiv_ndvsr
udiv24:	.word	.-.

.if OP_XARITH

	dca	udiv_divsr1

	tad	udiv_dvquo+1	; load high-order dividend into MQ
	mql			; AC => MQ; 0 => AC
	dvi			; AC,MQ / divsr => MQ r AC
udiv_divsr1: .word .-.
	dca	udiv_remdr
	mqa			; MQ | AC => AC
	dca	udiv_dvquo+1	; save high-order quotient

	tad	udiv_divsr1
	dca	udiv_divsr2
	tad	udiv_dvquo+0	; load low-order dividend into MQ
	mql			; AC => MQ; 0 => AC
	tad	udiv_remdr	; load remainder of high-order dividend into AC
	dvi			; AC,MQ / divsr => MQ r AC
udiv_divsr2: .word .-.
	dca	udiv_remdr
	mqa			; MQ | AC => AC
	dca	udiv_dvquo+0
	jmpi	udiv24

.else

	cma iac
	dca	udiv_ndvsr	; negative of divisor
	tad	_7750
	dca	udiv_count
	dca	udiv_remdr
udiv24_loop:
	; [link] [remdr] [dvquo+1] [dvquo+0] < quotient bits
	;        [ndvsr]
	tad	udiv_dvquo+0	; shift link-remdr-dvquo left one bit
	cll ral			; zero goes in bottom for quotient bit
	dca	udiv_dvquo+0
	tad	udiv_dvquo+1
	ral
	dca	udiv_dvquo+1
	tad	udiv_remdr
	ral
	dca	udiv_remdr
	tad	udiv_remdr	; get top 12 bits of 36-bit dividend (link = 13th bit)
	tad	udiv_ndvsr	; add negative divisor
	snl
	jmp	udiv24_skip
	dca	udiv_remdr	; boosted negated divisor from neg to pos, it fits
	isz	udiv_dvquo+0	; change bottom quotient bit to a one
udiv24_skip:
	cla
	isz	udiv_count
	jmp	udiv24_loop
	jmpi	udiv24

.endif

udiv_ndvsr: .blkw 2		; negative of divisor
udiv_count: .blkw 1		; iteration counter

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; unsigned divide
;  input:
;   ac = divisor
;   udiv_dvquo = 12-bit dividend
;  output:
;   ac = 0
;   udiv_remdr = remainder
;   udiv_dvquo = 12-bit quotient
;  scratch:
;   udiv_count, udiv_ndvsr
udiv12:	.word	.-.
	cma iac
	dca	udiv_ndvsr	; negative of divisor
	tad	_7764
	dca	udiv_count
	dca	udiv_remdr
udiv12_loop:
	; [link] [remdr] [dvquo+0] < quotient bits
	;        [ndvsr]
	tad	udiv_dvquo	; shift link-remdr-dvquo left one bit
	cll ral			; zero goes in bottom for quotient bit
	dca	udiv_dvquo
	tad	udiv_remdr
	ral
	dca	udiv_remdr
	tad	udiv_remdr	; get top 12 bits of 24-bit dividend (link = 13th bit)
	tad	udiv_ndvsr	; add negative divisor
	snl
	jmp	udiv12_skip
	dca	udiv_remdr	; boosted negated divisor from neg to pos, it fits
	isz	udiv_dvquo	; change bottom quotient bit to a one
udiv12_skip:
	cla
	isz	udiv_count
	jmp	udiv12_loop
	jmpi	udiv12



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; allocate some memory
;  input:
;   ac = number of words not including flag word
;  output:
;   ac<11:03> = address (8-word aligned)
;   ac<02:00> = data frame
malloc:	.word	.-.
	dca	l2mtemp1
	dca	mall2szm2	; compute min(1,ceil(log2(size+1))-2)
				;   malsize  mall2szm2  totalblocksizeweget
				;    0.. 7       1          8
				;    8..15       2         16
				;   16..31       3         32
				;               ...
	tad	l2mtemp1	; (size+1 leaves room for flag word)
	cll rar
	cll rar
mal_l2loop:
	isz	mall2szm2
	iac
	cll rar
	snl
	tad	_7777
	sza
	jmp	mal_l2loop

	tad	mall2szm2	; allocate a block of that size
	jms	l2malloc
	jmpi	malloc

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; allocate power-of-two-sized memory block
;  input:
;   ac = log2(size)-2 (must be in range 1=8wds; 2=16wds; ... 10=4K)
;        size includes flag word
;  output:
;   ac = composite pointer to allocated block
;        <11:03> = address <11:03>
;        <02:00> = data frame <02:00>
l2malloc: .word .-.
	dca	l2ml2szm2orig	; save log2(size)-2
.if 1-OP_GCTEST
	jms	l2mallocnogc	; try malloc, ret via l2malloc if success
.endif
	jmsi	_garbcoll	; failed, try garbage collect
	jms	l2mallocnogc	; try malloc again, ret via l2malloc if success
	tad	_mfulmsg	; out of memory
	jmpi	_evalerr

_mfulmsg: .word	mfulmsg

l2mallocnogc: .word .-.
	tad	l2ml2szm2orig
	sna
	jmsi	_fatal
	dca	mall2szm2
	tad	malwds8m1	; point to corresponding malwds{*} entry
	tad	mall2szm2
	dca	l2mheadptr
	malwdscdf
	cll
l2m_find:
	tad	mall2szm2	; see if > 10, ie, trying to alloc more than 4K
	tad	_7765
	szl cla
	jmpi	l2mallocnogc	; if so, we're out of memory
	tadi	l2mheadptr	; read the malwds{*} entry
	sza
	jmp	l2m_found
	isz	mall2szm2	; if nothing there, maybe there is bigger block to split
	isz	l2mheadptr
	jmp	l2m_find
l2m_found:
	dca	l2mfound	; something there, say we found one
	tad	l2mfound
	jms	l2mallblk	; unlink from free list
l2m_split:
	tad	l2ml2szm2orig	; see if block is size requested
	cma iac
	tad	mall2szm2
	sna cla
	jmp	l2m_done
	tad	l2mfound	; bigger, split the l2mfound block in half
	jmsi	_access
	tad	_7777		; - point to its flag word
	dca	l2mtemp1
	cla cma			; - decrement it, eg, 4=64wds => 3=32wds
	tadi	l2mtemp1
	dcai	l2mtemp1
	cla cma
	tad	mall2szm2
	dca	mall2szm2	; - log2(size)-2 of each half
	pow2tabcdf
	tadi	mall2szm2	; - size of each half
	cll rtl
	tad	l2mfound	; - composite pointer to second half
	jmsi	_l2freeblk	; free off the second half
	jmp	l2m_split	; maybe it needs more splitting
l2m_done:
.if OP_MALLPR
	cdf_1
	tad	_malmsg1
	jmsi	_ttprint12z
	tad	l2mfound
	jmsi	_ttprintoct4
	jmsi	_ttprintcrlf
.endif
	tad	l2mfound	; return the found block pointer
	jmpi	l2malloc

.if OP_MALLPR
_malmsg1: .word	malmsg1
.endif
_l2freeblk: .word l2freeblk
malwds8m1: .word malwds8-1
l2ml2szm2orig: .word .-.	; original log2(size)-2 requested
l2mheadptr:			; malwds{*} entry being checked
l2mtemp1:      .word .-.
l2mfound:      .word .-.	; block found composite pointer
l2mnextblk:    .word .-.	; block following block being allocated in the free list
l2mprevblk:    .word .-.	; block preceding block being allocated in the free list

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; allocate the given block (assumes that it is on the free list)
;  input:
;   ac = composite pointer to block to allocate
;   mall2szm2 = log2(size)-2
;  output:
;   ac = zero
l2mallblk: .word .-.

	; modify block being allocated
	;  set flag word = log2(size)-2 with <11> set saying it is allocated
	;  get pointers to next and prev blocks on the free list

	jmsi	_access		; access block being allocated
	tad	_7777		; point to flag word
	dca	l2mtemp1
	tad	mall2szm2	; get log2(size)-2
	tad	_4000		; ... with <11> set means it is allocated
	dcai	l2mtemp1
	isz	l2mtemp1	; point to next block pointer
	nop
	tadi	l2mtemp1
	dca	l2mnextblk
	isz	l2mtemp1	; point to prev block pointer
	tadi	l2mtemp1
	dca	l2mprevblk

	; modify next block in list
	;  set its prev pointer to prev block in list

	tad	l2mnextblk	; get composite pointer to next block in list
	sna
	jmp	l2m_nonextblk
	jmsi	_access		; access next block in list
	iac			; point to previous block link
	dca	l2mtemp1
	tad	l2mprevblk	; modify its previous block link
	dcai	l2mtemp1
l2m_nonextblk:

	; modify prev block in list
	;  set its next pointer to next block in list

	tad	l2mprevblk	; get composite pointer to prev block in list
	sna
	jmp	l2m_noprevblk
	jmsi	_access		; access previous block in list
	jmp	l2m_finish	; ac=address of previous block's next block link
l2m_noprevblk:

	; no previous block, update listhead

	tad	malwds8m1	; set up ac to point to malwds{*} entry
	tad	mall2szm2
	malwdscdf
l2m_finish:
	dca	l2mtemp1	; save address of either previous block's next block link or malwds{*} entry
	tad	l2mnextblk	; get composite pointer to next block in free list
	dcai	l2mtemp1	; that next block is now the next free block
	jmpi	l2mallblk

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; print 4-digit octal number
ttprintoct41:
	jmsi	_ttprintoct4
	cif_1
	jmpi	.+1
	.word	ttprintoct41ret



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; free block allocated by malloc
;  input:
;   ac = composite block pointer
;  output:
;   ac = zero
;  scratch:
;   ai17, df, mall2szm2
free:	.word	.-.
	dca	freeblock
.if OP_MALLPR
	cdf_1
	tad	_freemsg1
	jmsi	_ttprint12z
	tad	freeblock
	jmsi	_ttprintoct4
	jmsi	_ttprintcrlf
.endif
	tad	freeblock
	jmsi	_access		; access block being freed
	tad	_7777		; point to flag word
	dca	freeblkptr
	tadi	freeblkptr	; read flag word
	sma			; <11> set = block is allocated
	jmsi	_fatal		; block not allocated
	and	_3777		; <10:00> = log2(size)-2
	dca	mall2szm2
	tad	mall2szm2	; make sure in range 1..10
	tad	_7777
	cll
	tad	_7766
	szl cla
	jmsi	_fatal		; log2(size)-2 not in range 1..10
	tad	freeblock	; get composite pointer
	jms	l2freeblk	; free the block
	jmpi	free

freeblock:  .word .-.
freeblkptr: .word .-.

.if OP_MALLPR
_freemsg1:  .word freemsg1
.endif

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; free a log2 sized block
;  input:
;   ac = composite pointer to block to free
;   mall2szm2 = log2(size)-2
;  output:
;   ac = zero
;   mall2szm2 = unchanged
;  scratch:
;   ai17, df
l2freeblk: .word .-.
	dca	l2fblock	; save composite pointer to block being freed
	tad	mall2szm2	; save log2(size)-2 of block being freed
	dca	l2fl2szm2
.if OP_POISON
	cdf_1
	tadi	mall2szm2	; number of 4-word blocks in request
	cll rtl			; number of single words in request
	tad	_7777		; minus 1 for the flag word at end of block
	cma iac
	dca	l2ftemp1
	tad	l2fblock	; access the block's memory
	jmsi	_access
	tad	_7777
	dca	ai17
l2f_poison:
	tad	_3210		; fill the block with poison except flag word at the end
	dcai	ai17		; ... because flag word at the end is for next block
	isz	l2ftemp1
	jmp	l2f_poison
.endif
l2f_loop:

	; l2fblock = composite pointer to block being freed
	; mall2szm2 = log2(size)-2 of block being freed

	tad	mall2szm2	; see if freeing a 4K block
	tad	_7766		; ie, log2(size)-2 = 10
	sna cla
	jmp	l2f_single	; if so, 4K blocks don't have buddies
	pow2tabcdf
	tadi	mall2szm2	; not 4K, get block wordcount
	cll rtl
	dca	l2ftemp1

	; l2ftemp1 = number of single words in block being freed

	; see if buddy block is free

	tad	l2fblock	; get complement of buddy bit
	cma
	and	l2ftemp1
	dca	l2fbuddy
	tad	l2ftemp1	; get zero where the buddy bit is
	cma
	and	l2fblock
	tad	l2fbuddy	; add back in complemented buddy bit
	dca	l2fbuddy

	; l2fbuddy = composite pointer to buddy block

	tad	l2fbuddy	; map in the buddy block
	jmsi	_access
	tad	_7777		; point to buddy block's status word
	dca	l2ftemp1
	tadi	l2ftemp1	; get buddy block's status word
	cma iac			; must match exactly l2szm2 of block being free
	tad	mall2szm2	; ie, <11> clear meaning it's free, and <10:00> meaning it is the same size
	sza cla
	jmp	l2f_single	; <11> set or size different, buddy or part of buddy is in use, just free the single block

	; whole buddy is free, allocate the buddy to remove from free list

	tad	l2fbuddy	; get composite pointer to buddy block
	jmsi	_l2mallblk	; allocate that specific block

	; merge the two blocks, ie, take the lower of the two addresses and free the larger block

	cla
	tad	l2fblock	; they only differ by the buddy bit
	and	l2fbuddy	; ... so anding gets the lower of the two
	dca	l2fblock
	isz	mall2szm2	; block is now twice as big
	jmp	l2f_loop	; go back to free the larger block (maybe it merges again)

	; block won't merge, link it to free list
	; l2fblock = composite pointer to block being freed
	; mall2szm2 = log2(size)-2 of block being freed
l2f_single:
	tad	malwds8m1b	; point to malwds{*} entry for this size block
	tad	mall2szm2
	dca	l2ftemp1
	malwdscdf
	tadi	l2ftemp1	; see what top block on the free list is
	dca	l2foldtop
	tad	l2fblock	; block being freed will be new top free block
	dcai	l2ftemp1
	tad	l2foldtop	; get old top block pointer again
	sna
	jmp	l2f_singleonly
	jmsi	_access		; access the old top block on free list
	iac			; point to the prev block link in old top block
	dca	l2ftemp1
	tad	l2fblock	; point old top block's prev link to block being freed
	dcai	l2ftemp1
l2f_singleonly:
	tad	l2fblock	; access the block being freed
	jmsi	_access
	tad	_7777		; point to flag word
	dca	l2ftemp1
	tad	mall2szm2	; fill it with log2(size)-2, <11> = clear meaning it is free
	dcai	l2ftemp1
	isz	l2ftemp1	; fill in composite pointer to next free block of this size
	nop
	tad	l2foldtop	; = the old top free block
	dcai	l2ftemp1
	isz	l2ftemp1	; fill in composite pointer to prev free block = 0 meaning listhead
	dcai	l2ftemp1	; ... because this block is now the first free block of its size
	tad	l2fl2szm2	; restore original block size being freed
	dca	mall2szm2
	jmpi	l2freeblk

_l2mallblk:    .word l2mallblk
malwds8m1b:    .word malwds8-1
l2ftemp1:      .word .-.
l2fblock:      .word .-.	; composite pointer to block to free
l2fbuddy:      .word .-.	; composite pointer to buddy block
l2fl2szm2:     .word .-.	; log2(size)-2 of block being freed, increments as block merges with buddy
l2foldtop:     .word .-.	; block that was on top of the malwds{*} list that we are freeing to

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; wait for interrupt, roll lights
;  input: interrupts disabled
;  output: ac = zero, interrupts enabled
wfint:	.word	.-.
	cla
	tad	wfiperiod	; set number of ticks
	clab
	tad	wfintenab	; set interrupt enable
	clde
	cla
wfiloop:
	tad	wfilight	; get previous light
wfidir:	cll ral			; step light position
	szl
	jmp	wfiflip		; overflow, flip direction
	dca	wfilight	; save new light position
	tad	wfilight
	ion			; wait for interrupt, rtc or otherwise
	hlt
	cla cma			; clear all rtc enables
	clze
	cla
	jmpi	wfint
wfiflip:
	cla			; cll ral <-> cll rar
	tad	wfidir
	cma iac
	tad	wfiralrar
	dca	wfidir
	jmp	wfiloop		; step again

wfiralrar: .word 07104+07110	; cll ral + cll rar
wfilight:  .word 00003
wfiperiod: .word -3333		; 333300uS
wfintenab: .word 04400+3333	;    <11> = int enab
				; <10:09> = 00  : single cycle
				; <08:06> = 011 : 100uS ticks



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (quote value)
; return value unevaluated
f_quote:
	jmsi	_checkmiss	; make sure there is an argument
	jmsi	_nextelem	; get argument without evaluating it
	cla
	jmsi	_checklast	; make sure no more arguments
	jmpi	_evalret	; value is unevaluated token in evaltok

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; (eval value)
; evaluate the unevaluated value
f_eval:
	jmsi	_checkmiss	; make sure there is an argument
	jmsi	_nexteval	; get argument value, presumably an unevaluated expression created by quote
	dca	evaltok
	jmsi	_checklast	; make sure no more arguments
	tad	evaltok		; clear all symbol lookup links in case being re-evaled from different context
	jmsi	_clearsyms	; ... so symbols will be looked up by name
	cma			; evaltok points to unevaluated expression
	jmpi	_evalret	; ... so evaluate it tco-style

_clearsyms: .word .-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; push composite pointer on stack
; this must be used to push pointers returned by malloc
; - gets scanned by garbage collector
; returns with ac = zero
pushcp:	.word	.-.
	dca	pushval
	tad	_6201
	rdf
	dca	pushcpcdf
	stackcdf
	tad	pushval
	dcai	stkcpptr
pushcpcdf: .word .-.
	isz	stkcpptr
	jms	chkstkovf
	jmpi	pushcp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; push data value on stack
; use this to push everything except pointers returned by malloc
; - not scanned by garbage collector
; returns with ac = zero
pushda:	.word	.-.
	dca	pushval
	tad	_6201
	rdf
	dca	pushdacdf
	stackcdf
	tad	pushval
	dcai	stkdaptr
pushdacdf: .word .-.
	cla cma
	tad	stkdaptr
	dca	stkdaptr
	jms	chkstkovf
	jmpi	pushda

; check for stack overflow
;  barf if stkdaptr has decremented down to stkcpptr
;    or if stkcpptr has incremented up to stkdaptr
chkstkovf: .word .-.
	tad	stkcpptr	; stkcpptr should always be lower than stkdaptr
	cll cma
	tad	stkdaptr	; stkdaptr should always be higher than stkcpptr
	szl cla
	jmpi	chkstkovf
	jms	resetstack
	tad	_stkomsg
	jmpi	_evalerr

; reset stack pointers
resetstack: .word .-.
	tad	_stackbeg
	dca	stkcpptr
	tad	_stackend
	dca	stkdaptr
	jmpi	resetstack

pushval:   .word .-.
_stackbeg: .word stackbeg
_stackend: .word stackend
_stkomsg:  .word stkomsg

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; pop composite pointer from stack
popcp:	.word	.-.
	cla
	tad	_6201
	rdf
	dca	popcpcdf
	stackcdf
	cla cma
	tad	stkcpptr
	dca	stkcpptr
	tadi	stkcpptr
popcpcdf: .word	.-.
	jmpi	popcp

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; pop data value from stack
popda:	.word	.-.
	cla
	tad	_6201
	rdf
	dca	popdacdf
	stackcdf
	isz	stkdaptr
	tadi	stkdaptr
popdacdf: .word	.-.
	jmpi	popda

stkdaptr: .word	.-.		; where next data value gets written
stkcpptr: .word	.-.		; where next composite pointer gets written

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; print fatal error message then abort
fatal:	.word	.-.
	pioff
	dca	fatalac		; save ac to print it
	rdf
	dca	fataldf		; save df to print it
	cdf_1
	tad	_fatalmsg1	; print message and values
	jmsi	_ttprint12z
	cla cma
	tad	fatal
	jmsi	_ttprintoct4
	tad	_fatalmsg2
	jmsi	_ttprint12z
	tad	fatalac
	jmsi	_ttprintoct4
	tad	_fatalmsg3
	jmsi	_ttprint12z
	tad	fataldf
	jmsi	_ttprintoct4
	jmsi	_ttprintcrlf
	jmsi	_ttprintwait
	pion
	hcf			; halt and catch fyre
	jmp	.-1

_fatalmsg1: .word fatalmsg10
_fatalmsg2: .word fatalmsg2
_fatalmsg3: .word fatalmsg3
fatalac:    .word .-.
fataldf:    .word .-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; garbage collect
;  output:
;   ac = zero
garbcoll: .word	.-.
.if OP_MEMCHK
	jmsi	_memck		; make sure memory is ok before collection
.endif
	cdif1			; change both data and instruction to frame 1
	jmpi	.+1
	.word	garbcoll1
garcolret0:
.if OP_MEMCHK
	jmsi	_memck		; make sure memory is ok after collection
.endif
	cla
	jmpi	garbcoll

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; print character in accumulator
ttprintac1:
	jmsi	_ttprintac
	cif_1
	jmpi	.+1
	.word	ttprintac1ret

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; print 12-bit null terminated string
ttprint12z1:
	jmsi	_ttprint12z
	cif_1
	jmpi	.+1
	.word	ttprint12z1ret


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; FRAME 1
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

	.align	010000

	pow2tabcdf = cdf_1
pow2tab:
	.word	00001		; code assumes this is at offset 0 in the frame
_0002_1:.word	00002
	.word	00004
_0010_1:.word	00010
	.word	00020
	.word	00040
	.word	00100
	.word	00200
	.word	00400
	.word	01000
_2000_1:.word	02000
	.word	04000
	.word	00000

ai15_1:	.word	.-.
ai16_1:	.word	.-.
ai17_1:	.word	.-.

_0005_1: .word	00005
_0006_1: .word	00006
_0007_1: .word	00007
_0012_1: .word	00012
_0017_1: .word	00017
_0070_1: .word	00070
_0377_1: .word	00377
_1557_1: .word	01557
_3777_1: .word	03777
_4001_1: .word	04001
_4012_1: .word	04012
_6201_1: .word	06201
_6221_1: .word	06221
_7400_1: .word	07400
_7600_1: .word	07600
_7766_1: .word	07766
_7770_1: .word	07770
_7771_1: .word	07771
_7772_1: .word	07772
_7774_1: .word	07774
_7775_1: .word	07775
_7777_1: .word	07777

_access_1:	.word	access_1
_fatal_1:	.word	fatal_1
_gcbits:	.word	gcbits
_gcmarkrefd:	.word	gcmarkrefd
_initdone:	.word	initdone
_tokprint_1:	.word	tokprint_1
_ttprint12z_1:	.word	ttprint12z_1
_ttprintac_1:	.word	ttprintac_1
_ttprintcrlf_1:	.word	ttprintcrlf_1
_ttprintoct4_1:	.word	ttprintoct4_1
_ttprintwait_1:	.word	ttprintwait_1

gcrefdstk: .word .-.
gcrootptr: .word .-.



	.align	00200

garbcoll1:

	; garbage collection disabled during init
	; because gc uses the ai registers also used by init code

	cla
	cdf_0
	tadi	_initdone
	sna cla
	jmpi	_gcalldone

	; clear all the bit flags
	; 256 words = 3072 bits, ie,
	;  one bit per 8-word block in 24K words

	tad	_gcbits
	tad	_7777_1
	dca	ai17_1
	tad	_7600_1
	dca	gccount
	cdf_1
gcclrloop:
	dcai	ai17_1
	dcai	ai17_1
	isz	gccount
	jmp	gcclrloop

	; for every allocated block, set the corresponding bit,
	; ... making it a candidate for collection

	tad	_7772_1		; we have 6 frames to process, 2..7
	dca	gccount
	tad	_6221_1		; set up a cdf_2 instruction
	dca	gcsetcdf
gcsetloop:
gcsetcdf: .word	.-.		; cdf for the frame to be processed
	tad	_7777_1		; point to flag word for the block
	dca	gcaddr
	tadi	gcaddr		; get flag word from the block
	cdf_1
	dca	gcflag
	tad	gcflag
	sma cla
	jmp	gcsetfree
	tad	gcaddr		; bit <11> set, the block is allocated
	iac			; get index in the 256-word array where the bit is
	cll rtr			; ...from address of the block in the frame <11:04>
	rtr			; leave addr<03> in the link
	tad	_gcbits
	dca	gcindex
	rtl			; put addr<03> in ac<01>
	ral			; ... and now in ac<02>
	tad	gcsetcdf	; get frame 2..7 in ac<05:03> + 06201
	tad	_1557_1		; subtract 06221 (cdf_2)
	cll rtr			; now we have bit number 00..11
	dca	gcbitmsk
	tadi	gcbitmsk	; get corresponding bit from pow2tab
	tadi	gcindex		; add in the previous bitmask word
	dcai	gcindex		; ... now that bit is set (assume it wasn't before)
gcsetfree:
	tad	gcflag		; get flag word
	and	_3777_1		; clear allocated bit <11>
	dca	gcbitmsk	; get block log2(size)-2
	tadi	gcbitmsk	; ... from pow2tab
	cll rtl			; get number of words
	tad	gcaddr		; increment to next block in the frame
	iac
	sza
	jmp	gcsetloop
	tad	gcsetcdf	; increment to next frame
	tad	_0010_1
	dca	gcsetcdf
	isz	gccount		; maybe there aren't any more frames
	jmp	gcsetloop

	; go through all the static roots
	; for those that point to an allocated block, clear the block's bit and push it on the root stack

	tad	_gcroots0m1	; point to beginning of gcroots0 list
	jmsi	_gcrootscan	; process the entries
	tad	_gcroots1m1	; point to beginning of gcroots1 list
	jmsi	_gcrootscan	; process the entries

	; also might have pointers pushed on stack

	tad	_stackbegm1	; point just before where stack begins
	dca	ai17_1
	cdf_0
	cla cma			; point to last word pushed on stack
	tadi	_stkcpptr
	cma iac			; - negative
	dca	gcstknend
gcstackloop:
	tad	gcstknend	; see if last loop checked last word of stack
	tad	ai17_1
	sna cla
	jmp	gcstackdone
	stackcdf		; not yet, read word from stack
	tadi	ai17_1
	sza
	jmsi	_gcmarkrefd	; not null, assume it is block pointer
	jmp	gcstackloop
gcstackdone:

	; referenced blocks may point to other referenced blocks

gcrefdloop1:
	cdf_1
	tad	gcrefdstk	; get composite pointer of referenced block
	sna
	jmpi	_gcrefddone1
	jmsi	_access_1	; access the block
	tad	_7777_1		; point back to the flag word
	dca	gcfwptr
	tadi	gcfwptr		; read the flag word, it's really a link to next refd block
	dca	gcrefdstk	; unlink the block from gcrefdstk
	tad	_4001_1		; restore flag word, we only have 8-word blocks on this list
	dcai	gcfwptr
	tad	_7771_1		; check the first 7 words of the block for pointers
	dca	gccount
	tad	gcfwptr
	dca	ai17_1
	tad	_6201_1
	rdf
	dca	gcrefdcdf2
gcrefdloop2:
gcrefdcdf2: .word .-.		; cdf to access the block
	tadi	ai17_1		; get word from block
	sza
	jmsi	_gcmarkrefd	; non-zero value, assume it is composite pointer to a block
	isz	gccount		; maybe more words in block to check
	jmp	gcrefdloop2
	jmp	gcrefdloop1	; check for more blocks on the gcrefd list

_gcalldone:   .word gcalldone
_gcrefddone1: .word gcrefddone1
_gcroots0m1:  .word gcroots0-1
_gcroots1m1:  .word gcroots1-1
_gcrootscan:  .word gcrootscan
_stackbegm1:  .word stackbeg-1
_stkcpptr:    .word stkcpptr
gccount:      .word .-.
gcaddr:       .word .-.
gcflag:       .word .-.
gcbitmsk:     .word .-.
gcfwptr:      .word .-.
gcindex:      .word .-.
gcstknend:    .word .-.



	.align	00200

gcrefddone1:

	; unlink any name table entries that are not referenced by anything
	; their bit will be set in the bitmap

	dca	gclastname	; no referenced names found
	cdf_0
	tadi	_nametable	; point to first name in table
gcnameloop:
	sna
	jmp	gcnamedone
	dca	gcthisname	; save composite pointer to name node being checked
	tad	gcthisname
	jmsi	_gctestbit	; see if name node is referenced by anything
	sza cla
	jmp	gcnamenext	; bit set, name is unreferenced, don't link to nametable, check next entry
				; name is referenced, link it to nametable
				;  gclastname = points to previous node (or zero if first)
				;  gcthisname = points to this node
	jms	gclinkname	; link last name to this name
	tad	gcthisname	; next one found will be linked to this one
	dca	gclastname
gcnamenext:
	tad	gcthisname	; access the name node memory
	jmsi	_access_1
	dca	gcthisname
	tadi	gcthisname	; see if there is another name node to check after this
	jmp	gcnameloop
gcnamedone:
	dca	gcthisname	; no more names
	jms	gclinkname	; link last name to null

	; for every bit still set in the bitmap, free the corresponding block

	tad	_7400_1		; 256 words in the bitmap
	dca	gccount2
	tad	_gcbits
	tad	_7777_1
	dca	ai17_1
gcfreeloop1:
	dca	gcbitno		; reset bit number in case we need to scan bit by bit
	cdf_1
	tadi	ai17_1		; see if any bits set in the word
	sna
	jmp	gcfreenext1
				; something set, scan word bit by bit
gcfreeloop2:
	cll rar			; check each bit
	snl
	jmp	gcfreenext2
	dca	gcfreebits	; low bit was set, save the rest in word

		; free the corresponding block
		;  gccount<07:00> => address<11:04>
		;     gcbitno<00> => address<03>
		;  gcbitno<03:01> => address<15:12> - 2
	tad	gcbitno
	cll rar			; gcbitno<00> => link
	dca	gcfreecomp	; gcbitno<03:01> => gcfreecomp<02:00>
	tad	gccount2	; gccount<07:00> => ac<07:00>
	and	_0377_1
	rtl			; ac<09:00> = gccount<07:00>,gcbitno<00>,0
	cml rtl			; ac<11:00> = gccount<07:00>,gcbitno<00>,010
	tad	gcfreecomp	; ac<11:00> = gccount<07:00>,gcbitno<00>,(gcbitno<03:01>+2)
	jms	free_1
	tad	gcfreebits
gcfreenext2:
	isz	gcbitno		; count bit number in word
	sza			; done if no more set bits
	jmp	gcfreeloop2

gcfreenext1:
	isz	gccount2	; see if gone through all 256 words
	jmp	gcfreeloop1	; repeat if not

	; all done

gcalldone:
	cdif0
	jmpi	.+1
	.word	garcolret0

_gctestbit: .word gctestbit
_nametable: .word nametable

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; scan list of static roots
;  input:
;   ac = points just before list to be scanned
;  output:
;   ac = zero
;   referenced blocks marked as referenced and pushed on referenced list
;  scratch:
;   ai17_1, df, gcrootptr
gcrootscan: .word .-.
	dca	ai17_1		; back up autoinc pointer just before the list
	tadi	ai17_1		; get the cdf to access the pointers
	dca	gcrootcdf
gcrootloop:
	cdf_1
	tadi	ai17_1		; get pointer to root
	sna
	jmpi	gcrootscan	; gcroots0,1 are null terminated
	dca	gcrootptr
gcrootcdf: .word .-.
	tadi	gcrootptr	; see if the entry points to something
	sza
	jmsi	_gcmarkrefd	; if so, mark that block as allocated
	jmp	gcrootloop

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; link a previous node name to the next name node
;  input:
;   gcthisname = next name node
;   gclastname = previous node name
;  output:
;   ac = zero
;  scratch:
;   df,ai17_1
gclinkname: .word .-.
	tad	gclastname
	sna
	jmp	gclinknamefirst
	jmsi	_access_1	; not the first in nametable,
	dca	gclinkedptr
	tad	gcthisname	;  access previous node
	dcai	gclinkedptr	;N_NXT=0  store link from previous to next
	jmpi	gclinkname
gclinknamefirst:
	tad	gcthisname	; first in nametable
	cdf_0
	dcai	_nametable	; store link to first name in table
	jmpi	gclinkname

gcbitno:     .word .-.
gccount2:    .word .-.
gcframem2:   .word .-.
gcfreebits:  .word .-.
gcfreecomp:  .word .-.
gclastname:  .word .-.
gclinkedptr: .word .-.
gcthisname:  .word .-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; free block of memory
;  input:
;   ac = composite pointer to block
;  output:
;   ac = zero
;  scratch:
;   df, mall2szm2
free_1:	.word	.-.
	cif_0
	jmpi	.+1
	.word	free1
free1ret:
	jmpi	free_1

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; access memory
;  input:
;   ac<11:03> = address (8-word aligned)
;   ac<02:00> = data frame
;  output:
;   data frame set
;   ac<11:03> = address (8-word aligned)
;   ac<02:00> = cleared
access_1: .word .-.
	dca	access_temp_1
	tad	access_temp_1
	ral
	rtl
	and	_0070_1
	tad	_6201_1	; cdf_0
	dca	.+1
	.word .-.
	tad	access_temp_1
	and	_7770_1
	jmpi	access_1

access_temp_1:	.word .-.



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; mark a block as referenced
;  input:
;   ac = composite pointer to block
;  output:
;   ac = 0
;   if bitmap bit was set,
;     bitmap bit is now cleared
;     and if 8-word block, block pushed on gcrefd list
;  scratch:
;   df
gcmarkrefd: .word .-.
	jms	gctestbit
	sna
	jmpi	gcmarkrefd	; if not, it has already been processed
	cma			; if so, clear the bit so we don't free it and we don't process it again
	andi	gcrefdidx
	dcai	gcrefdidx
	tad	gcrefdcptr	; get composite pointer to block
	jmsi	_access_1	; access block being marked
	tad	_7777_1		; point to its flag word
	dca	gcfwptr2
	tadi	gcfwptr2	; see if 4001, ie, allocated 8-word block
	sma
	jmsi	_fatal_1	; - it should always be marked allocated
	tad	_3777_1
	sza cla
	jmpi	gcmarkrefd	; if not, it is symbol or string token and does not have any references
	tad	gcrefdstk	; if so, it may contain references, so push on gcrefd list
	dcai	gcfwptr2	; ... so it will get scanned
	tad	gcrefdcptr
	dca	gcrefdstk
	jmpi	gcmarkrefd

gcfwptr2:   .word .-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; test a bitmap bit for a composite pointer
;  input:
;   ac = composite pointer to block
;  output:
;   gcrefdcptr = saved ac on input
;   link = set: composite pointer in frame 0 or 1
;               ac = zero
;        clear: frame 2..7
;               ac = tested bit
;                    zero: block has not been seen as referenced so far
;                    nonz: block has been seen as referenced
gctestbit: .word .-.
	dca	gcrefdcptr	; save composite pointer to block

	; each block has a 15-bit address <14:00> and is 8-word aligned
	; the addresses are in range 20000..77770
	; the composite pointer = address <11:03><14:12>
	; the 256-word array is indexed with address <11:04>
	; the bitnumber is taken from address (<14:12>-2)<03>

	tad	gcrefdcptr	; get composite pointer
	rtr			; shift to get address <11:04> in bottom
	rtr			; leave address <03> in link
	and	_0377_1		; clear garbage from top
	tad	_gcbits		; make pointer into gcbits array
	dca	gcrefdidx
	tad	gcrefdcptr	; get address <14:12> into ac <02:00>
	ral			; rotate in link so we get address <14:12><03> in ac <03:00>
	and	_0017_1		; clear garbage from top
	tad	_7774_1		; make value in ac = address (<14:12>-2)<03>
	spa
	jmp	gcmarknobit
	dca	gcrefdbit	; save as bitnumber in bitmap word
	pow2tabcdf
	tadi	gcrefdbit	; get mask for that bit
	andi	gcrefdidx	; see if bitmap has that bit set
	cll
	jmpi	gctestbit
gcmarknobit:
	cll cla cml
	jmpi	gctestbit

gcrefdbit:  .word .-.
gcrefdcptr: .word .-.
gcrefdidx:  .word .-.

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; dump symbol table
;  input:
;   symframe = innermost frame to dump
dumpsyms_1: .word .-.
	cla
	cdf_0
	tadi	_symframe	; start with innermost frame
	dca	dsframe
dsloop1:
	cdf_1			; print frame composite pointer
	tad	_dsmsg1
	jmsi	_ttprint12z_1
	tad	dsframe
	jmsi	_ttprintoct4_1
	jmsi	_ttprintcrlf_1
	tad	dsframe		; point to frame
	sna
	jmpi	dumpsyms_1	; - done if zero
	jmsi	_access_1	; access the frame memory
	tad	_7777_1
	dca	ai17_1
	tadi	ai17_1		;SF_OUTER=0 : save link to next outer frame
	dca	dsframe
	tadi	ai17_1		;SF_ENTRIES=1 : save link to entries in the frame
	dca	dsentries
dsloop2:
	tad	dsentries	; get pointer to symbol entries block
	sna
	jmp	dsloop1		; - if zero, do next outer frame
	jmsi	_access_1	; access symbol block
	tad	_7777_1
	dca	ai17_1
	tadi	ai17_1		;SE_NEXT=0 : save link to next symbol entries block in the frame
	dca	dsentries
	tad	_7775_1		; contains 3 name/value pairs
	dca	dscount
	tad	_6201_1
	rdf
	dca	dsloop3cdf
dsloop3:
dsloop3cdf: .word .-.
	tadi	ai17_1		;SE_NAME1=1,SE_VALU1=2,SE_NAME2=3,SE_NAME2=4,SE_NAME3=5,SE_VALU3=6
	sza
	jmsi	_tokprint_1	; print name
	isz	ai17_1		; skip value (can't print value - it uses stack if list)
	isz	dscount
	jmp	dsloop3
	jmp	dsloop2

dscount:   .word .-.
dsentries: .word .-.
dsframe:   .word .-.

_dsmsg1:   .word dsmsg1
_symframe: .word symframe



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; check memory managed by malloc/free
;  output:
;   ac,df = preserved
memck0:
	jms	memck_1		; jumped to from frame 0
	cif_0			; jump back to frame 0
	jmpi	.+1
	.word	memck0ret

memck_1: .word	.-.
	dca	mc_savac	; save AC on entry
	tad	_6201_1		; save DF on entry
	rdf
	dca	mc_rdf

	; make sure we can find all 24K words of frames 2..7 by reading the flag words

	tad	_6221_1		; start scanning in frame 2
	dca	mc_df
	dca	mc_total	; clear total memory found
	dca	mc_totfr	; clear total free mem found
	dca	mc_addr		; start with block at address 0000 within the frame
mcloop:
mc_df:	.word	.-.		; access frame being checked
	cla cma			; point to flag word corresponding to block being checked
	tad	mc_addr
	dca	mc_temp1
	tadi	mc_temp1	; read flag word
	dca	mc_flagwd
	tad	mc_flagwd	; <10:00> must be in range 1..10 (1=8; 2=16; ... 9=2K; 10=4K)
	tad	_7777_1		; 0=8wds; 1=16wds; ... 8=2K; 9=4K
	and	_3777_1		; ignore allocated bit <11>
	tad	_7766_1		; -10=8wds; -9=16wds; ... -1=4K
	sma
	jmsi	_fatal_1
	tad	_0012_1		;   0=8wds; 1=16wds; ... 9=4K
	dca	mc_temp1
	pow2tabcdf
	tadi	mc_temp1	; get number of 8-word blocks in the block
	tad	mc_total	; add that to total number of 8-word blocks found
	dca	mc_total
	tad	mc_flagwd	; see if block is marked free
	spa cla
	jmp	mcallocd
	tadi	mc_temp1	; if so, add its 8-word size to total free found
	tad	mc_totfr
	dca	mc_totfr
mcallocd:
	tadi	mc_temp1	; get number of words in the block
	cll ral
	rtl
	tad	mc_addr		; that tells us where next flag word is
	dca	mc_addr
	snl
	jmp	mcloop
	tad	mc_addr		; we should end up at exact end of the frame
	sza
	jmsi	_fatal_1
	tad	mc_df		; increment to next frame
	tad	_0010_1
	dca	mc_df
	tad	mc_df		; see if checked all frames
	and	_0070_1
	sza cla
	jmp	mcloop
	tad	mc_total	; better have found 24K words total
	tad	_2000_1
	sza
	jmsi	_fatal_1

	; check all free lists to make sure pointers are correct

	dca	mc_l2szm2	; 1=8wds per block
	cma
	dca	mc_negsz	; every time we find an 8-word block, subtract 1 from mc_totfr
mcfrloop:
	isz	mc_l2szm2	; increment to next of malwds{*}
	dca	mc_prevfree	; the first block should have a prev link of zero
	malwdscdf		; get access to malwds{*} elements
	tad	malwds8m1c	; point to corresponding malwds{*} entry
	tad	mc_l2szm2
	dca	mc_addr		; save address of pointer to first block in list
	tadi	mc_addr		; get composite pointer to first block in list
mcfrloop2:
	sna
	jmp	mcfrdone2
	dca	mc_addr		; save composite pointer to block we are checking
	tad	mc_addr		; get access to the block
	jmsi	_access_1
	tad	_7777_1		; point to block's flag word
	dca	mc_blkptr
	tadi	mc_blkptr	; read block's flag word (1=8wds; 2=16wds; ... 10=4K)
	cma iac
	tad	mc_l2szm2	; should be exactly the l2szm2 value with <11> clear
	sza
	jmsi	_fatal_1
	isz	mc_blkptr	; point to block's next link
	nop
	tadi	mc_blkptr	; read block's next link
	dca	mc_nextfree
	isz	mc_blkptr	; point to block's prev link
	tadi	mc_blkptr	; read block's prev link
	cma iac
	tad	mc_prevfree	; should be exactly the previous block's composite pointer
	sza
	jmsi	_fatal_1
	cll
	tad	mc_totfr	; subtract its size from total free 8-word blocks
	tad	mc_negsz
	dca	mc_totfr
	snl			; fail if overflow in case of infinite loop of free blocks
	jmsi	_fatal_1
	tad	mc_addr		; this block is next's previous block
	dca	mc_prevfree
	tad	mc_nextfree	; get composite pointer to next free block for this size
	jmp	mcfrloop2
mcfrdone2:
	tad	mc_negsz	; blocks in next in malwds{*} list are twice the size
	cll ral
	dca	mc_negsz
	tad	mc_l2szm2	; see if just processed malwds4k free list
	tad	_7766_1
	sza cla
	jmp	mcfrloop	; repeat if haven't done the malwds4k free list yet
	tad	mc_totfr	; we should have found every free word
	sza
	jmsi	_fatal_1

	; memory intact

mc_rdf:	.word	.-.		; restore DF register
	tad	mc_savac	; restore AC register
	jmpi	memck_1

malwds8m1c: .word malwds8-1
mc_temp1: .word .-.
mc_savac: .word	.-.		; saved accumulator on input
mc_addr:  .word .-.		; address of block being checked
mc_flagwd: .word .-.		; flag word from block being checked
mc_total: .word .-.		; total number of 8-word blocks found so far
mc_totfr: .word .-.		; how much of mc_total is free
mc_blkptr: .word .-.		; pointer to block being checked
mc_l2szm2: .word .-.		; log2(size)-2 of block being checked
mc_negsz:  .word .-.		; negative size of 8-word blocks being checked
mc_nextfree: .word .-.		; next free block composite pointer (zero if end of list)
mc_prevfree: .word .-.		; prev free block composite pointer (zero if beg of list)



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; call the ttprint functions in cif_0
ttprintac_1: .word .-.
	cif_0
	jmpi	.+1
	.word	ttprintac1
ttprintac1ret:
	jmpi	ttprintac_1

ttprintcrlf_1: .word .-.
	cif_0
	jmpi	.+1
	.word	ttprintcrlf1
ttprintcrlf1ret:
	jmpi	ttprintcrlf_1

ttprintoct4_1: .word .-.
	cif_0
	jmpi	.+1
	.word	ttprintoct41
ttprintoct41ret:
	jmpi	ttprintoct4_1

ttprintwait_1: .word .-.
	cif_0
	jmpi	.+1
	.word	ttprintwait1
ttprintwait1ret:
	jmpi	ttprintwait_1

ttprint12z_1: .word .-.
	cif_0
	jmpi	.+1
	.word	ttprint12z1
ttprint12z1ret:
	jmpi	ttprint12z_1

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; print fatal error message then abort
fatal_1: .word	.-.
	pioff
	dca	fatalac_1	; save ac to print it
	rdf
	dca	fataldf_1	; save df to print it
	cdf_1
	tad	_fatalmsg1_1	; print message and values
	jmsi	_ttprint12z_1
	cla cma
	tad	fatal_1
	jmsi	_ttprintoct4_1
	tad	_fatalmsg2_1
	jmsi	_ttprint12z_1
	tad	fatalac_1
	jmsi	_ttprintoct4_1
	tad	_fatalmsg3_1
	jmsi	_ttprint12z_1
	tad	fataldf_1
	jmsi	_ttprintoct4_1
	jmsi	_ttprintcrlf_1
	jmsi	_ttprintwait_1
	pion
	hcf			; halt and catch fyre
	jmp	.-1

_fatalmsg1_1: .word fatalmsg11
_fatalmsg2_1: .word fatalmsg2
_fatalmsg3_1: .word fatalmsg3
fatalac_1:    .word .-.
fataldf_1:    .word .-.



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; perform unsigned multiply 24 x 24 => 48 for f_umullo,f_umullhi
;  input:
;   umullohival = contains a operand
;   df,ac = points just before b operand
;   link = 0: return low order product
;          1: return high order product
;  output:
;   umullohival = resultant low or high order product
umullohi_1:
	dca	ai17_1
	tadi	ai17_1		; save b integer
	dca	umulb+0
	tadi	ai17_1
	dca	umulb+1

	rar			; save hi/lo flag
	dca	umulhfl

	cdf_0
	tadi	umullohival0	; save a integer
	dca	umula+0
	tadi	umullohival1
	dca	umula+1

.if OP_XARITH

	tad	umula+0		; set up mcand[0] => MQ
	mql			; AC => MQ; 0 => AC
	tad	umulb+0		; set up mplyr[0]
	dca	.+2
	muy			; MQ * .+1 + AC => AC,MQ
	.word	.-.
	dca	umulp+1		; save prod[1]
	mqa			; MQ | AC => AC
	dca	umulp+0		; save prod[0]

	tad	umula+1		; set up mcand[1] => MQ
	mql			; AC => MQ; 0 => AC
	tad	umulb+0		; set up mplyr[0]
	dca	.+3
	tad	umulp+1		; set up to add in prod[1]
	muy			; MQ * .+1 + AC => AC,MQ
	.word	.-.
	dca	umulp+2		; save prod[2]
	mqa			; MQ | AC => AC
	dca	umulp+1		; save prod[1]

	tad	umula+0		; set up mcand[0] => MQ
	mql			; AC => MQ; 0 => AC
	tad	umulb+1		; set up mplyr[1]
	dca	.+3
	tad	umulp+1		; set up to add in prod[1]
	muy			; MQ * .+1 + AC => AC,MQ
	.word	.-.
	tad	umulp+2		; add to prod[2]
	dca	umulp+2
	mqa			; MQ | AC => AC
	dca	umulp+1		; save prod[1]

	tad	umula+1		; set up mcand[1] => MQ
	mql			; AC => MQ; 0 => AC
	tad	umulb+1		; set up mplyr[1]
	dca	.+3
	tad	umulp+2		; set up to add in prod[2]
	muy			; MQ * .+1 + AC => AC,MQ
	.word	.-.
	dca	umulp+3		; save prod[3]
	mqa			; MQ | AC => AC
	dca	umulp+2		; save prod[2]

.else

	dca	umulp+0
	dca	umulp+1

	tad	_7750_1
	dca	umulctr
umulloop:
	cll
	tad	umulp+0		; shift product left
	ral
	dca	umulp+0
	tad	umulp+1
	ral
	dca	umulp+1
	tad	umulp+2
	ral
	dca	umulp+2
	tad	umulp+3
	ral
	dca	umulp+3

	cll
	tad	umulb+0		; rotate multiplier left
	ral
	dca	umulb+0
	tad	umulb+1
	ral
	dca	umulb+1
	snl
	jmp	umulnext

	cll
	tad	umulp+0		; high bit was set, add multiplicand to product
	tad	umula+0
	dca	umulp+0
	ral
	tad	umulp+1
	tad	umula+1
	dca	umulp+1
	szl
	isz	umulp+2
	skp
	isz	umulp+3
	nop
umulnext:
	isz	umulctr
	jmp	umulloop

.endif

	tad	umulhfl
	sma cla
	jmp	umulret
	tad	umulp+2		; return high order value
	dca	umulp+0
	tad	umulp+3
	dca	umulp+1
umulret:
	cdif0
	tad	umulp+0		; copy value back to where f_umullo/hi can get it
	dcai	umullohival0
	tad	umulp+1
	dcai	umullohival1
	jmpi	.+1
	.word	umullohi1ret

umulhfl: .blkw	1		; 0=return low 24 bits; 1=return high 24 bits
umulctr: .blkw	1		; count 24 iterations in loop
umula:	 .blkw	2		; A operand (24-bit little endian)
umulb:	 .blkw	2		; B operand (24-bit little endian)
umulp:	 .blkw	4		; product (48-bit little endian)

umullohival0: .word umullohival+0
umullohival1: .word umullohival+1



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; static roots in frame 0
; these are all the things that can possibly have composite pointers to mallocd blocks
; ...that must remain valid across malloc calls
gcroots0:
	cdf_0
	.word	addsymfrm
	.word	addsymnam
	.word	addsymval
	.word	cdrlist
	.word	consapple
	.word	conslastok
	.word	consresbeg
	.word	consresend
	.word	constoken
	.word	evaltok
	.word	evframe
	.word	evllist
	.word	evlnext
	.word	fstrouttok
	.word	lamfbody
	.word	lamparams
	.word	lamsymfrm
	.word	listel
	.word	loadstack
	.word	mainresult
	.word	nextstrtok
	.word	nxetoken
	.word	slucmpvalu
	.word	sluframe
	.word	sluentry
	.word	substrintok
	.word	symframe
	.word	tksymnam
	.word	token
	.word	tokfirst
	.word	toklast
	.word	toklists
	.word	tplelem
	.word	ufbody
	.word	ufouter
	.word	ufplist
	.word	0

; static roots in frame 1
gcroots1:
	cdf_1
	.word	0

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; one bit per 8-word block that malloc can possibly allocate
; each 8-word block has a 15-bit address <14:00> and is 8-word aligned
; the addresses are in range 20000..77770
; the 256-word array is indexed with address <11:04>
; the bitnumber is taken from address (<14:12>-2)<03>
gcbits:	.blkw	256



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; misc data at end of frame 1

	kbbuffcdf = cdf_1
	kbbufflen = 00400
kbbuff:	.blkw	kbbufflen	; keyboard input buffer

getcmdarg:
	.word	SCN_GETCMDARG12	; read command line argument, 12-bit string
	.word	1		; read from argv[1]
	.word	kbbufflen	; length of buffer
	.word	kbbuff		; buffer address

openfile:
	.word	SCN_OPEN12	; open file, 12-bit null terminated string
	.word	kbbuff		; pointer to name string
	.word	O_RDONLY	; mode
	.word	0		; flags

readlnbuff:
	.word	SCN_READLN12	; read to newline, 12-bit string
readlnbuffd: .word -1		; fd (or -1 if not open)
	.word	kbbuff		; buffer address
	.word	kbbufflen-1	; buffer length (leave room for null terminator)

closefile:
	.word	SCN_CLOSE	; close file
closefilefd: .word -1		; fd (or -1 to nop it out)

	ttbuffcdf = cdf_1
	.align	040		; must be naturally aligned
ttbuff:	.blkw	040		; printer output buffer

	.align	010
emptytok:
	.word	TT_LIS		;T_TYPE=0 : a list
	.word	0,0		;T_SPOS=1 : zeroes for builtins
	.word	0		;T_VALU=3 : no elements

	.align	010
nulltok:
	.word	TT_NUL		;T_TYPE=0 : null (invisible)
	.word	0,0		;T_SPOS=1 : zeroes for builtins

	; code assumes truetok-falsetok = 0010

	.align	010
falsetok:
	.word	TT_BOO		;T_TYPE=0 : a boolean
	.word	0,0		;T_SPOS=1 : zeroes for builtins
	.word	0		;T_VALU=3 : zero is false

	.align	010
truetok:
	.word	TT_BOO		;T_TYPE=0 : a boolean
	.word	0,0		;T_SPOS=1 : zeroes for builtins
	.word	1		;T_VALU=3 : one is true

bsspbs:     .asciz "\b \b"
ccmsg:      .asciz "#C\n"
cumsg:      .asciz "#U\n"
dsmsg1:     .asciz "dumpsyms: frame "
fatalmsg10: .asciz "\n*FATAL* PC=0"
fatalmsg11: .asciz "\n*FATAL* PC=1"
fatalmsg2:  .asciz " AC="
fatalmsg3:  .asciz " DF="

.if OP_MALLPR
freemsg1:   .asciz "free: block "
malmsg1:    .asciz "malloc: block "
.endif
mfulmsg:    .asciz "malloc: memory full"

evalmsg1:   .asciz "evaluate: undefined symbol"
evlmsg1:    .asciz "evaluate: empty list does not have function"
evlmsg2:    .asciz "evaluate: first list element is not a function"

carmsg2:    .asciz "car/cdr/null?: argument not a list"
carmsg3:    .asciz "car/cdr: list is empty"
carmsg4:    .asciz "checklast: extra argument"
carmsg5:    .asciz "checkmiss: missing argument"
condmsg1:   .asciz "cond: expecting (bool begin...)"
intmsg1:    .asciz "int: must be a bool, char or string"
loadmsg1:   .asciz "load: null filename string"
loadmsg3:   .asciz "load: filename string too long"
loadmsg4:   .asciz "load: error opening file"
lammsg2:    .asciz "lambda: expecting (paramnames ...) or varargsparamname"
.if OP_MAKNAM
mnemsg1:    .asciz "makenament: block "
.endif
nimsg1:     .asciz "nextint: argument not an integer"
nsmsg1:     .asciz "nextstr: argument not a string"
ormsg1:     .asciz "nextbool: argument not a boolean"
st2nemsg1:  .asciz "st2ne: must be a symbol name"
stkomsg:    .asciz "chkstkovf: stack overflow"
tcxmsg:     .asciz "tokenize: excess close parenthesis"

	malwdscdf = cdf_1

malwds8:    .word 0		; log2(size)-2 =  1  composite pointer to free   8-word blocks
malwds16:   .word 0		; log2(size)-2 =  2  composite pointer to free  16-word blocks
malwds32:   .word 0		; log2(size)-2 =  3  composite pointer to free  32-word blocks
malwds64:   .word 0		; log2(size)-2 =  4  composite pointer to free  64-word blocks
malwds128:  .word 0		; log2(size)-2 =  5  composite pointer to free 128-word blocks
malwds256:  .word 0		; log2(size)-2 =  6  composite pointer to free 256-word blocks
malwds512:  .word 0		; log2(size)-2 =  7  composite pointer to free 512-word blocks
malwds1k:   .word 0		; log2(size)-2 =  8  composite pointer to free  1k-word blocks
malwds2k:   .word 0		; log2(size)-2 =  9  composite pointer to free  2k-word blocks
malwds4k:   .word 0		; log2(size)-2 = 10  composite pointer to free  4k-word blocks

	stackcdf = 06211
	stackbeg = .
	stackend = stackbeg | 07777

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;               overwritten              by              stack               ;;
;;                 ||   ||                ||              || ||               ;;
;;                 vv   vv                vv              vv vv               ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; initialize everything
	initcif = 06212
initialize:
	cla iac			; enable keyboard and printer interrupts
	kie
	ion
	cla
	cdf_1
	tad	_initmsg1	; print init message
	jmsi	_ttprint12z_1
	jms	meminit		; init malloc's free memory list
	jmsi	_topinit	; initialize top-level symbol frame
	jmsi	_memck_1	; validate memory inited properly
	cdf_0
	cla cma
	dcai	_initdone	; initialization complete, enable garbage collection and control-C
	dcai	_ccflag		; forget about control-C pressed during init
	jmsi	_ttprintcrlf_1	; print CR/LF to start us off
	cif_0
	jmpi	.+1
	.word	initret

_ccflag:   .word ccflag
_initmsg1: .word initmsg1
_memck_1:  .word memck_1
_topinit:  .word topinit

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; initialize memory
; frees frames 2..7 as 4K-word blocks
; returns with ac = 0
meminit: .word	.-.
	cdf_0
	cla			; log2(4096)-2 = 10
	tad	_0007_1		; free off starting at frame 7
	dca	miframe
mi_loop:
	tad	miframe		; access frame memory
	jmsi	_access_1
	tad	_7777_1		; point to flag word
	dca	mipointer
	tad	_4012_1		; write it to be 4K allocated block
	dcai	mipointer
	tad	miframe		; free it as a 4K block
	jmsi	_free_1
	cla cma			; decrement frame number
	tad	miframe
	dca	miframe
	cla cma			; stop if we freed off frame 2
	tad	miframe
	sza cla
	jmp	mi_loop
	jmpi	meminit

miframe:   .word .-.
mipointer: .word .-.
_free_1:   .word free_1

; allocate memory block
malloc_1: .word	.-.
	cif_0
	jmpi	.+1
	.word	malloc1
malloc1ret:
	jmpi	malloc_1

; print token
tokprint_1: .word	.-.
	cif_0
	jmpi	.+1
	.word	tokprint1
tokprint1ret:
	jmpi	tokprint_1



	.align	00200

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; initialize top-level symbol table with built-in constants and functions
;  output:
;   ac = zero
;   topsymframe = filled in
;  scratch:
;   ai17, ai16, ai15, df

;   allocate top-level symbol frame
;   use ai15 to scan through initialize list
;   while more to scan
;     save ai15 in ai17
;     count string length using ai15
;     if seen two empty name strings, all done
;     allocate symbol node memory

topinit: .word .-.
	tad	_0002_1		;SF_SIZE=2 allocate symbol frame
	jmsi	_malloc_1
	dca	tinstaticframe	; it is going to hold the static constants and functions
	tad	tinstaticframe
	cdf_0
	dcai	_addsymfrmc	; - save it as symbol frame we are adding to
	tad	tinstaticframe
	jmsi	_access_1	; access its memory
	tad	_7777_1
	dca	ai17_1
	dcai	ai17_1		;SF_OUTER=0 : it is outermost frame
	dcai	ai17_1		;SF_ENTRIES=1 : doesn't have any entries yet
	tad	_staticfuncsm1	; point to beginning of static funciton list
	dca	ai15_1
tinloop:
	; ai15 = points just before symbol name string (or .word 0) in staticfuncs table
	tad	ai15_1		; save pointer to .asciz name string
	dca	ai17_1
	dca	tinamelen	; count number of chars in name string
	staticfuncscdf
tincountloop:
	tadi	ai15_1
	sna cla
	jmp	tincountdone
	isz	tinamelen
	jmp	tincountloop
tincountdone:
	; ai17 = points just before name string
	; ai15 = points to terminating 0 of name string
	; tinamelen = number of characters in name string
	tad	tinamelen	; check for '.word 0', ie, null string
	sza cla
	jmp	tinmakesym
	isz	tinzeroes	; if so, increment zero counter
	jmp	tinloop		; haven't seen two '.word 0' yet, keep going
	jmp	tindone		; if we've seen two '.word 0', we're all done
tinmakesym:
	tad	ai17_1		; make name table entry node
	cdf_0
	dcai	_ai11
	staticfuncscdf
	tad	tinamelen
	jms	makenament_1
	cdf_0
	dcai	_addsymnamc	; save composite pointer for addsyment subroutine
	; ai15 = points to terminating null in name string
	staticfuncscdf
	cla
	tadi	ai15_1		; get builtin value
	cdf_0
	dcai	_addsymvalc	; assume it is a constant token value composite pointer
	tad	tinzeroes	; see if it is constant token composite pointer
	iac
	sza cla
	jmp	tindefine	; if so, go make symbol table entry
	tadi	_addsymvalc	; it is a function machine code address
	dca	tinfunc
	tad	_0006_1		;T_VALU=3 : allocate function token node
	jmsi	_malloc_1
	cdf_0
	dcai	_addsymvalc	; it is the function symbol value
	tadi	_addsymvalc
	jmsi	_access_1	; access its memory
	tad	_7777_1
	dca	ai16_1
	tad	_0005_1		;TT_FUN=5
	dcai	ai16_1		;T_TYPE=0 : token type
	dcai	ai16_1		;T_SPOS=1 : zero for builtins
	dcai	ai16_1
	tad	tinfunc		;T_VALU=3 : entrypoint
	dcai	ai16_1
tindefine:
	; addsymfrm = symbol frame
	; addsymnam = name token
	; addsymval = value token
	jms	addsyment_1	; add symbol entry to symbol frame for constant or function
	; ai15 = still points to token or function address in staticfuncs table
	jmp	tinloop

	; all done building static frame
	; make an empty frame for top-level defines
	; and that will be the top-level frame from here on
tindone:
	tad	_0002_1		;SF_SIZE=2 allocate symbol frame
	jmsi	_malloc_1
	cdf_0
	dcai	_topsymframe	; it is the top-level frame
	tadi	_topsymframe
	jmsi	_access_1	; access its memory
	tad	_7777_1
	dca	ai17_1
	tad	tinstaticframe
	dcai	ai17_1		;SF_OUTER=0 : outer frame is the static frame
	dcai	ai17_1		;SF_ENTRIES=1 : doesn't have any entries yet
	jmpi	topinit

makenament_1: .word .-.
	cif_0
	jmpi	.+1
	.word	makenament1
makenament1ret:
	jmpi	makenament_1

addsyment_1: .word .-.
	cif_0
	jmpi	.+1
	.word	addsyment1
addsyment1ret:
	jmpi	addsyment_1

_ai11:        .word ai11
_malloc_1:    .word malloc_1
_tisymframe:  .word symframe
_topsymframe: .word topsymframe
_addsymfrmc:  .word addsymfrm
_addsymnamc:  .word addsymnam
_addsymvalc:  .word addsymval
tinstaticframe: .word .-.
tinzeroes: .word -2
tinfunc:   .word .-.		; function entrypoint
tinamelen: .word .-.		; function name length

_staticfuncsm1: .word staticfuncs-1

initmsg1: .asciz "\nSCHEME/8"

	staticfuncscdf = 06211
staticfuncs:
	.asciz	"empty"
			.word	emptytok|(emptytok/4096)
	.asciz	"null"
			.word	nulltok|(nulltok/4096)
	.asciz	"#f"
			.word	falsetok|(falsetok/4096)
	.asciz	"#t"
			.word	truetok|(truetok/4096)
	.word	0

	.asciz	"and"
			.word	f_and

	.asciz	"begin"
			.word	f_begin
	.asciz	"bool?"
			.word	f_bool_q
	.asciz	"car"
			.word	f_car
	.asciz	"cdr"
			.word	f_cdr
	.asciz	"char"
			.word	f_char
	.asciz	"char?"
			.word	f_char_q
	.asciz	"charat"
			.word	f_charat
	.asciz	"cond"
			.word	f_cond
	.asciz	"cons"
			.word	f_cons
	.asciz	"define"
			.word	f_define
	.asciz	"display"
			.word	f_display
	.asciz	"eval"
			.word	f_eval
	.asciz	"fatal"
			.word	f_fatal
	.asciz	"halt"
			.word	f_halt
	.asciz	"heaproom"
			.word	f_heaproom
	.asciz	"heaproom-gc"
			.word	f_heaproom_gc
	.asciz	"if"
			.word	f_if
	.asciz	"int"
			.word	f_int
	.asciz	"int?"
			.word	f_int_q
	.asciz	"lambda"
			.word	f_lambda
	.asciz	"lambda?"
			.word	f_lambda_q
	.asciz	"length"
			.word	f_length
	.asciz	"list"
			.word	f_list
	.asciz	"list?"
			.word	f_list_q
	.asciz	"load"
			.word	f_load
	.asciz	"not"
			.word	f_not
	.asciz	"null?"
			.word	f_null_q
	.asciz	"or"
			.word	f_or
	.asciz	"prompt"
			.word	f_prompt
	.asciz	"quo"
			.word	f_quo
	.asciz	"quote"
			.word	f_quote
	.asciz	"rem"
			.word	f_rem
	.asciz	"stackroom"
			.word	f_stackroom
	.asciz	"strcmp"
			.word	f_strcmp
	.asciz	"string"
			.word	f_string
	.asciz	"string?"
			.word	f_string_q
	.asciz	"strlen"
			.word	f_strlen
	.asciz	"strpos"
			.word	f_strpos
	.asciz	"substr"
			.word	f_substr
	.asciz	"umulhi"
			.word	f_umulhi
	.asciz	"umullo"
			.word	f_umullo
	.asciz	"+"
			.word	f_plus
	.asciz	"-"
			.word	f_minus
	.asciz	"&"
			.word	f_amper
	.asciz	"|"
			.word	f_orbar
	.asciz	"_"
			.word	f_under
	.asciz	"~"
			.word	f_tilde
	.asciz	"*"
			.word	f_times
	.asciz	">="
			.word	f__ge
	.asciz	"<="
			.word	f__le
	.asciz	"="
			.word	f__eq
	.asciz	"<>"
			.word	f__ne
	.asciz	">"
			.word	f__gt
	.asciz	"<"
			.word	f__lt
	.asciz	"<<"
			.word	f__shl
	.asciz	">>"
			.word	f__shr
	.word	0

