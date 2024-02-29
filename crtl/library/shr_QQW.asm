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
; shift right
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
	.global	__shr_qqW
__shr_qqW:
	lda	%r6,-20(%r6)
	stw	%r5,-54(%r6)
	ldw	%r5,6(%r1)	; sign-extend result
	shl	%r5,%r5
	sbb	%r5,%r5,%r5
	br	shr_QQW_common

shr_QQW_63: .word 63

	.global	__shr_QQW
__shr_QQW:
	lda	%r6,-20(%r6)
	stw	%r5,-54(%r6)
	clr	%r5		; zero-extend result

shr_QQW_common:
	stw	%r0,-64(%r6)
	stw	%r3,-58(%r6)
	stw	%r4,-56(%r6)

	; R0 = result pointer
	; R1 = operand pointer
	; R2 = raw shift count
	; R5 = sign/zero-extend word

	; R2 = masked number of bits to shift by
	ldw	%r3,shr_QQW_63
	and	%r2,%r2,%r3

	; R3 = number of whole words to shift by
	lsr	%r3,%r2
	lsr	%r3,%r3
	lsr	%r3,%r3
	lsr	%r3,%r3

	; if R0==R1, result overlaps operand
	; copy operand to temp buffer so it doesn't get stomped writing result
	; but not needed if shifting less than one word of bits
	beq	shr_QQW_copyskip
	cmp	%r0,%r1
	bne	shr_QQW_copyskip
	ldw	%r4,  2(%r1)
	stw	%r4,-50(%r6)
	ldw	%r4,  4(%r1)
	stw	%r4,-48(%r6)
	ldw	%r4,  6(%r1)
	stw	%r4,-46(%r6)
	lda	%r1,-52(%r6)
shr_QQW_copyskip:

	; R0 = result pointer
	; R1 = operand pointer
	; R2 = 6-bit shift count
	; R3 = whole word shift count

	; save operand pointer
	stw	%r1,-62(%r6)

	; sign/zero-extend that many top result words
	shl	%r4,%r3
	sub	%r1,%pc,%r4
shr_QQW_padbase:
	lda	%pc,shr_QQW_padskip-shr_QQW_padbase(%r1)
	stw	%r5,2(%r0)
	stw	%r5,4(%r0)
	stw	%r5,6(%r0)
shr_QQW_padskip:

	; offset result pointer by that much
	sub	%r0,%r0,%r4
	stw	%r0,-60(%r6)

	; get number of bits in word to shift by
	ldw	%r0,#15
	and	%r2,%r2,%r0

	; get negative word count to shift
	;  skip=0: negcount=-4; 4 words to copy and shift
	;  skip=1: negcount=-3; 3 words to copy and shift
	;  skip=2: negcount=-2; 2 words to copy and shift
	;  skip=3: negcount=-1; 1 word  to copy and shift
	lda	%r3,-4(%r3)

shr_QQW_wordloop:
	; R5 = bits shifted out bottom of next higher word
	; R3 = negative number of words to shift
	; R2 = number of bits to shift word by

	; R1 = bits shifted out bottom of next higher word
	mov	%r1,%r5

	; R0 = word to shift
	ldw	%r5,-62(%r6)
	ldw	%r0,6(%r5)
	lda	%r5,-2(%r5)
	stw	%r5,-62(%r6)

	; save copy in R5
	mov	%r5,%r0

	; shift R1R0 right by R2
	neg	%r4,%r2
	beq	shr_QQW_bitsnull
shr_QQW_bitsloop:
	lsr	%r1,%r1
	ror	%r0,%r0
	inc	%r4,%r4
	bne	shr_QQW_bitsloop
shr_QQW_bitsnull:

	; store shifted value
	ldw	%r4,-60(%r6)
	stw	%r0,6(%r4)
	lda	%r4,-2(%r4)
	stw	%r4,-60(%r6)

	; maybe have another word to shift
	inc	%r3,%r3
	bne	shr_QQW_wordloop

	; all done
	ldw	%r0,-64(%r6)
	ldw	%r3,-58(%r6)
	ldw	%r4,-56(%r6)
	ldw	%r5,-54(%r6)
	lda	%r6,20(%r6)
	lda	%pc,0(%r3)

