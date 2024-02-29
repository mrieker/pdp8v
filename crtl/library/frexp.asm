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

	.align	2

; double frexp (double x, int *exp)
	frexp_sav3 = -64
	frexp_sav4 = -62
	frexp_sav5 = -60
	frexp_out  = -58
	frexp_x    = -56
	frexp_exp  = -48

	.global	frexp
frexp:
	lda	%r6,-6(%r6)
	stw	%r3,frexp_sav3(%r6)
	stw	%r4,frexp_sav4(%r6)
	stw	%r5,frexp_sav5(%r6)

	ldw	%r2,frexp_x+0(%r6)	; get double value
	ldw	%r3,frexp_x+2(%r6)
	ldw	%r4,frexp_x+4(%r6)
	ldw	%r5,frexp_x+6(%r6)

	ldw	%r0,#0x7FF0		; extract exponent
	and	%r1,%r0,%r5
	beq	frexp_subnor		; - subnormal
	cmp	%r1,%r0			; check non-finite exponent
	beq	frexp_retzex		; - if so, return value as is

	ldw	%r0,#0x3FE0		; normal exponent, remove bias
	sub	%r1,%r1,%r0

	sub	%r5,%r5,%r1		; remove exponent from double but leave bias in double

	asr	%r1,%r1			; return unbiased/unshifted exponent to caller
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1

frexp_retexp:
	ldw	%r0,frexp_exp(%r6)
	stw	%r1,0(%r0)
	ldw	%r0,frexp_out(%r6)
	stw	%r2,0(%r0)
	stw	%r3,2(%r0)
	stw	%r4,4(%r0)
	stw	%r5,6(%r0)
	ldw	%r3,frexp_sav3(%r6)
	ldw	%r4,frexp_sav4(%r6)
	ldw	%r5,frexp_sav5(%r6)
	lda	%r6,18(%r6)
	lda	%pc,0(%r3)

frexp_retzex:
	clr	%r1
	br	frexp_retexp

frexp_subnor:
	shl	%r1,%r5			; subnormal, check for zero value
	or	%r1,%r1,%r4
	or	%r1,%r1,%r3
	or	%r1,%r1,%r2
	beq	frexp_retexp		; - if so, return value as is
	ldw	%r1,#-1021		; non-zero subnormal, set up exponent
	ldw	%r0,#0x10		; shift left until hidden bit gets set
frexp_subloop:
	lda	%r1,-1(%r1)
	shl	%r2,%r2
	rol	%r3,%r3
	rol	%r4,%r4
	rol	%r5,%r5
	cmp	%r5,%r0
	blo	frexp_subloop
	sub	%r5,%r5,%r0		; strip out hidden bit
	ldw	%r0,frexp_x+6(%r6)	; re-insert sign bit
	shl	%r5,%r5
	shl	%r0,%r0
	ror	%r5,%r5
	ldw	%r0,#0x3FE0		; insert biased exponent in double
	or	%r5,%r5,%r0
	br	frexp_retexp
