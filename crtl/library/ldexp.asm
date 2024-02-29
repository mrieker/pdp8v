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

; double ldexp (double x, int exp)
	ldexp_sav3 = -64
	ldexp_sav4 = -62
	ldexp_sav5 = -60
	ldexp_out  = -58
	ldexp_x    = -56
	ldexp_exp  = -48

	.global	ldexp
ldexp:
	lda	%r6,-6(%r6)
	stw	%r3,ldexp_sav3(%r6)
	stw	%r4,ldexp_sav4(%r6)
	stw	%r5,ldexp_sav5(%r6)

	ldw	%r2,ldexp_x+0(%r6)	; get double value
	ldw	%r3,ldexp_x+2(%r6)
	ldw	%r4,ldexp_x+4(%r6)
	ldw	%r5,ldexp_x+6(%r6)

	ldw	%r0,#0x7FF0		; extract exponent
	and	%r1,%r0,%r5
	bne	ldexp_normal
	shl	%r2,%r2
	rol	%r3,%r3
	rol	%r4,%r4
	rol	%r5,%r5
	or	%r0,%r2,%r3
	or	%r0,%r0,%r4
	or	%r0,%r0,%r5
	bne	ldexp_addexp
	ror	%r5,%r5
	br	ldexp_return
ldexp_normal:
	cmp	%r1,%r0			; check non-finite exponent
	beq	ldexp_return		; - if so, return value as is
	sub	%r5,%r5,%r1		; normal, remove exponent
	lda	%r5,0x10(%r5)		; insert hidden bit

ldexp_addexp:
	asr	%r1,%r1			; shift for adding
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	ldw	%r0,ldexp_exp(%r6)	; add given exponent
	add	%r1,%r1,%r0
	ble	ldexp_subnor		; - so small it is subnormal or zero
	ldw	%r0,#0x10		; shift to get hidden bit
ldexp_normlize:
	and	%pc,%r5,%r0
	bne	ldexp_normlizd
	shl	%r2,%r2
	rol	%r3,%r3
	rol	%r4,%r4
	rol	%r5,%r5
	lda	%r1,-1(%r1)
	tst	%r1
	bgt	ldexp_normlize
	lsr	%r5,%r5			; exponent went zero looking for hidden bit,
	ror	%r4,%r4			; ... result is subnormal
	ror	%r3,%r3
	ror	%r2,%r2
	br	ldexp_retsign
ldexp_normlizd:
	sub	%r5,%r5,%r0		; found hidden bit, remove it
	ldw	%r0,#0x07FF		; check for exponent too big
	cmp	%r1,%r0
	bhis	ldexp_retinf		; - if so, return infinity
	shl	%r1,%r1			; insert non-zero (normal) exponent
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	or	%r5,%r5,%r1
	br	ldexp_retsign

ldexp_subnor:
	beq	ldexp_subnormz
	ldw	%r0,#-52		; subnormal, see if too small
	cmp	%r1,%r0
	blo	ldexp_retzero		; - yes unsigned, pos %r1 means add above underflowed
ldexp_subnormz:
	shl	%r5,%r5			; clear sign bit
	lsr	%r5,%r5
ldexp_subnorml:
	lsr	%r5,%r5			; shift subnormals in place
	ror	%r4,%r4
	ror	%r3,%r3
	ror	%r2,%r2
	inc	%r1,%r1
	ble	ldexp_subnorml
	br	ldexp_retsign		; put sign bit back
ldexp_retinf:
	ldw	%r5,#0x7FF0
	br	ldexp_retloz
ldexp_retzero:
	clr	%r5
ldexp_retloz:
	clr	%r4
	clr	%r3
	clr	%r2
ldexp_retsign:
	ldw	%r0,ldexp_x+6(%r6)
	shl	%r5,%r5
	shl	%r0,%r0
	ror	%r5,%r5
ldexp_return:
	ldw	%r0,ldexp_out(%r6)
	stw	%r2,0(%r0)
	stw	%r3,2(%r0)
	stw	%r4,4(%r0)
	stw	%r5,6(%r0)
	ldw	%r3,ldexp_sav3(%r6)
	ldw	%r4,ldexp_sav4(%r6)
	ldw	%r5,ldexp_sav5(%r6)
	lda	%r6,18(%r6)
	lda	%pc,0(%r3)
