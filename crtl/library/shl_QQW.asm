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

	.include "magicdefs.asm"

;
; shift left
;  input:
;   R0 = quad address result
;   R1 = quad address operand
;   R2 = shift count
;   R3 = return address
;  output:
;   R0,R4,R5 = same as on input
;   R1,R2,R3 = trashed
;
;  stack:
;   -64(R6) = saved R0
;   -62(R6) = operand pointer
;   -60(R6) = result pointer
;   -58(R6) = saved R3
;   -56(R6) = saved R4
;   -54(R6) = saved R5
;   -52(R6) = copy of operand if overlaps result
;
	.align	2
	.global	__shl_qqB
	.global	__shl_qqW
	.global	__shl_QQB
	.global	__shl_QQW
__shl_qqB:
__shl_qqW:
__shl_QQB:
__shl_QQW:
	lda	%r6,-20(%r6)
	stw	%r0,-64(%r6)
	stw	%r3,-58(%r6)
	stw	%r4,-56(%r6)
	stw	%r5,-54(%r6)

	; R0 = result pointer
	; R1 = operand pointer
	; R2 = raw shift count

	; R2 = masked number of bits to shift by
	ldw	%r3,#63
	and	%r2,%r2,%r3

	; R3 = number of whole words to shift by
	lsr	%r3,%r2
	lsr	%r3,%r3
	lsr	%r3,%r3
	lsr	%r3,%r3

	; if R0==R1, result overlaps operand
	; copy operand to temp buffer so it doesn't get stomped writing result
	; but not needed if shifting less than one word of bits
	beq	shl_QQW_copyskip
	cmp	%r0,%r1
	bne	shl_QQW_copyskip
	ldw	%r4,  0(%r1)
	stw	%r4,-52(%r6)
	ldw	%r4,  2(%r1)
	stw	%r4,-50(%r6)
	ldw	%r4,  4(%r1)
	stw	%r4,-48(%r6)
	lda	%r1,-52(%r6)
shl_QQW_copyskip:

	; R0 = result pointer
	; R1 = operand pointer
	; R2 = 6-bit shift count
	; R3 = whole word shift count

	; save operand pointer
	stw	%r1,-62(%r6)

	; zero that many bottom result words
	clr	%r1
	shl	%r3,%r3
	sub	%r4,%pc,%r3
shl_QQW_padbase:
	lda	%pc,shl_QQW_padskip-shl_QQW_padbase(%r4)
	stw	%r1,4(%r0)
	stw	%r1,2(%r0)
	stw	%r1,0(%r0)
shl_QQW_padskip:

	; offset result pointer by that much
	add	%r0,%r0,%r3
	stw	%r0,-60(%r6)

	; get number of bits in word to shift by
	ldw	%r0,#15
	and	%r2,%r2,%r0

	; get negative word count to shift
	;  skip=0: negcount=-4
	;  skip=1: negcount=-3
	;  skip=2: negcount=-2
	;  skip=3: negcount=-1
	lda	%r3,-4(%r3)

	; shift zeroes into bottom of low word
	clr	%r5

shl_QQW_wordloop:
	; R5 = bits shifted out top of next lower word
	; R3 = negative number of words to shift
	; R2 = number of bits to shift word by

	; R1 = bits shifted out top of next lower word
	mov	%r1,%r5

	; R0 = word to shift
	ldw	%r5,-62(%r6)
	ldw	%r0,0(%r5)
	lda	%r5,2(%r5)
	stw	%r5,-62(%r6)

	; save copy in R5
	mov	%r5,%r0

	; shift R0R1 left by R2
	neg	%r4,%r2
	beq	shl_QQW_bitsnull
shl_QQW_bitsloop:
	shl	%r1,%r1
	rol	%r0,%r0
	inc	%r4,%r4
	bne	shl_QQW_bitsloop
shl_QQW_bitsnull:

	; store shifted value
	ldw	%r4,-60(%r6)
	stw	%r0,0(%r4)
	lda	%r4,2(%r4)
	stw	%r4,-60(%r6)

	; maybe have another word to shift
	inc	%r3,%r3
	bne	shl_QQW_wordloop

	; all done
	ldw	%r0,-64(%r6)
	ldw	%r3,-58(%r6)
	ldw	%r4,-56(%r6)
	ldw	%r5,-54(%r6)
	lda	%r6,20(%r6)
	lda	%pc,0(%r3)

