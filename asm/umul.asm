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
;;    http:;;www.gnu.org/licenses/gpl-2.0.html
; unsigned multiply and unsigned divide routines
; ...and a little test program

; prints product 093F9AE5
;   in decimal 39653

; ./master.sh -sim master.sim
; takes about 13m to run
; (3021 cycles)

	.include "magicdefs.asm"

test:
	br	skipih				; 0000: reset entrypoint
	br	inthand				; 0002: interrupt entrypoint
skipih:
	ldw	%r5,#MAGIC_SCN			; tell RaspiModule to turn on IRQ line
	lda	%r1,intson
	stw	%r1,0(%r5)
	ldw	%r2,#0x800A			; enable interrupts, set N,V bits
	wrps	%r2
	lda	%r0,1(%r2)			; this should execute
ireqhere:
	halt	%r0				; this should not execute
intson:	 .word	SCN_INTREQ,1
intsoff: .word	SCN_INTREQ,0
inthand:
	clr	%r1				; change cond codes to Z,C set
	lda	%r1,5(%r1)
	wrps	%r1
	lda	%r1,intsoff			; tell RaspiModule to turn off IRQ line
	stw	%r1,0(%r5)
	ldw	%r1,0xFFFC-MAGIC_SCN(%r5)	; modify return address
	lda	%r1,irethere-ireqhere(%r1)
	stw	%r1,0xFFFC-MAGIC_SCN(%r5)
	iret
	halt	%r0				; should not get here
irethere:

; do a bunch of regular arithmetic
; check result value and condition codes
; - shadow.cc will check it as we run
	clr	%r2
	clr	%r3
	ldw	%r4,#5432
	ldw	%r5,#4321
test_loop1:
	add	%r1,%r2,%r3
	rdps	%r0

	sub	%r1,%r2,%r3
	rdps	%r1

	sub	%r1,%r3,%r2
	rdps	%r0

	cmp	%r2,%r3
	rdps	%r1

	cmp	%r3,%r2
	rdps	%r0

	or	%r1,%r2,%r3
	rdps	%r1

	and	%r1,%r2,%r3
	rdps	%r0

	xor	%r1,%r2,%r3
	rdps	%r1

	clr	%r1
	wrps	%r1
	adc	%r1,%r2,%r3
	rdps	%r0

	ldw	%r1,#1
	wrps	%r1
	adc	%r1,%r2,%r3
	rdps	%r1

	clr	%r1
	wrps	%r1
	sbb	%r1,%r2,%r3
	rdps	%r0

	ldw	%r1,#1
	wrps	%r1
	sbb	%r1,%r2,%r3
	rdps	%r1

	clr	%r1
	wrps	%r1
	sbb	%r1,%r3,%r2
	rdps	%r0

	ldw	%r1,#1
	wrps	%r1
	sbb	%r1,%r3,%r2
	rdps	%r1

	bit	%r2,%r3
	rdps	%r0

	add	%r2,%r2,%r4
	add	%r3,%r3,%r5
	bcc	test_loop1

test_loop2:
	lsr	%r1,%r2
	rdps	%r1

	asr	%r1,%r2
	rdps	%r0

	clr	%r1
	wrps	%r1
	ror	%r1,%r2
	rdps	%r1

	mov	%r1,%r2
	rdps	%r0

	neg	%r1,%r2
	rdps	%r1

	inc	%r1,%r2
	rdps	%r0

	com	%r1,%r2
	rdps	%r1

	add	%r2,%r2,%r5
	bcc	test_loop2

; test that the branches do the condition codes
; - Shadow.java monitors the states and %pc
	clr	%r2
	ldw	%r3,#16
test_loop3:
	wrps	%r2
	rdps	%r0
	br	.+4
	lda	%r0,0(%r0)
	ble	.+4
	lda	%r0,0(%r0)
	bgt	.+4
	lda	%r0,0(%r0)
	blt	.+4
	lda	%r0,0(%r0)
	bge	.+4
	lda	%r0,0(%r0)
	blos	.+4
	lda	%r0,0(%r0)
	bhi	.+4
	lda	%r0,0(%r0)
	bmi	.+4
	lda	%r0,0(%r0)
	bpl	.+4
	lda	%r0,0(%r0)
	beq	.+4
	lda	%r0,0(%r0)
	bne	.+4
	lda	%r0,0(%r0)
	bvs	.+4
	lda	%r0,0(%r0)
	bvc	.+4
	lda	%r0,0(%r0)
	bcs	.+4
	lda	%r0,0(%r0)
	bcc	.+4
	lda	%r0,0(%r0)

	inc	%r2,%r2
	sub	%r0,%r2,%r3
	blt	test_loop3

; --------------------------------------------

	ldw	%r6,#MAGIC_SCN			; set stack pointer to end of memory

	ldw	%r1,#0x4321			; multiply some values
	ldw	%r2,#0x2345
	clr	%r3
	lda	%r5,2(%pc)
	br	umul

	lda	%r6,0-2(%r6)			; save low-order on stack
	stw	%r0,0(%r6)

	mov	%r0,%r1				; convert high-order to string
	lda	%r1,hexbuf
	lda	%r5,2(%pc)
	br	hextostr

	ldw	%r0,0(%r6)			; convert low-order to string
	lda	%r5,2(%pc)
	br	hextostr

	ldw	%r0,0(%r6)			; convert low-order to decimal string
	ldw	%r1,4(%pc)
	lda	%r5,4(%pc)
	br	dectostr
	.word	decbuf

	clr	%r0				; null terminate string
	stb	%r0,0(%r1)

	ldw	%r1,#MAGIC_SCN			; print message
	lda	%r0,printctl
	stw	%r0,0(%r1)

	lda	%r0,exitctl
	stw	%r0,0(%r1)			; exit(0)

	halt	%r0

printctl:
	.word	SCN_PRINTLN
	.word	message

message: .ascii	"multiply result "
hexbuf:	.ascii	"xxxxxxxx\n  in decimal "
decbuf:	.asciz	"xxxxxx"

	.p2align 1

exitctl: .word	SCN_EXIT,0

;	%r3%r2
;     x	  %r1
;	%r1%r0
umul:
	lda	%r6,0-2(%r6)
	stw	%r4,0(%r6)
	clr	%r0
	lda	%r4,0-16(%r0)
umul_loop:
	lsl	%r0,%r0
	rol	%r1,%r1
	bcc	umul_skip
	add	%r0,%r0,%r2
	adc	%r1,%r1,%r3
umul_skip:
	inc	%r4,%r4
	bne	umul_loop
	ldw	%r4,0(%r6)
	lda	%r6,2(%r6)
	lda	%pc,0(%r5)

;	%r1%r0
;     /	  %r2
;	  %r0 r %r1
udiv:
	ldw	%r3,#0-16
udiv_loop:
	lsl	%r0,%r0
	rol	%r1,%r1
	bcs	udiv_fits
	cmp	%r1,%r2
	blo	udiv_next
udiv_fits:
	lda	%r0,1(%r0)
	sub	%r1,%r1,%r2
udiv_next:
	inc	%r3,%r3
	bne	udiv_loop
	lda	%pc,0(%r5)

; input:
;  %r0 = number
;  %r1 = buffer
; output:
;  %r0,%r2,%r3,%r4 = scratch
;  %r1 = points just past string
hextostr:
	ldw	%r4,#0-4			; number of digits to produce
	sub	%r1,%r1,%r4			; point to end of buffer
hextostr_loop:
	ldw	%r3,#15				; mask low digit
	and	%r2,%r0,%r3			; ...into %r2
	ldw	%r3,#48				; convert to ascii 0..?
	add	%r2,%r2,%r3
	ldw	%r3,#48+10			; see if it should be A..F
	cmp	%r2,%r3
	blo	hextostr_0_9
	lda	%r2,7(%r2)			; if so, convert to A..F
hextostr_0_9:
	lda	%r1,0-1(%r1)			; decrement pointer
	stb	%r2,0(%r1)			; store character
	lsr	%r0,%r0				; shift out bottom digit
	lsr	%r0,%r0
	lsr	%r0,%r0
	lsr	%r0,%r0
	inc	%r4,%r4				; decrement counter
	bne	hextostr_loop
	lda	%r1,4(%r1)			; point past output string
	lda	%pc,0(%r5)


;  input:
;   %r0 = number
;   %r1 = buffer
;   %r5 = return address
;   %r6 = stack pointer
;  output:
;   %r0,%r2,%r3 = scratch
;   %r1 = points just past string
dectostr:
	lda	%r6,0-6(%r6)
	stw	%r5,  4(%r6)
	stw	%r4,  2(%r6)
	stw	%r1,  0(%r6)
	lda	%r4,  0(%r6)
dectostr_loop:
	clr	%r1
	lda	%r2,10(%r1)
	lda	%r5,2(%pc)
	br	udiv
	lda	%r1,48(%r1)
	lda	%r6,0-2(%r6)
	stw	%r1,0(%r6)
	tst	%r0
	bne	dectostr_loop
	ldw	%r1, 0(%r4)
dectostr_fini:
	ldw	%r0, 0(%r6)
	lda	%r6, 2(%r6)
	stb	%r0, 0(%r1)
	lda	%r1, 1(%r1)
	cmp	%r4,%r6
	bne	dectostr_fini
	ldw	%r4, 2(%r6)
	ldw	%r5, 4(%r6)
	lda	%r6, 6(%r6)
	lda	%pc, 0(%r5)

