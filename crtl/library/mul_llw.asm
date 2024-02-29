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
; multiply
;  input:
;   R0 = signed long address
;   R1 = signed long address
;   R2 = signed word value
;   R3 = return address
;  output:
;   R0,R4,R5 = same as on input
;   R1,R2,R3 = trashed
;
;  stack:
;   -64 : input R0
;   -62 : unused
;   -60 : output sign
;   -58 : input R3
;   -56 : input R4
;   -54 : input R5
;
	.align	2
	.global	__mul_llw
__mul_llw:
	lda	%r6,-12(%r6)
	stw	%r0,-64(%r6)
	stw	%r3,-58(%r6)
	stw	%r4,-56(%r6)
	stw	%r5,-54(%r6)

	ldw	%r3,2(%r1)
	xor	%r4,%r3,%r2
	stw	%r4,-60(%r6)

	mov	%r4,%r2
	bpl	mul_llw_posr4
	neg	%r4,%r2
mul_llw_posr4:

	ldw	%r2,0(%r1)
	tst	%r3
	bpl	mul_llw_posr3
	clr	%r1
	sub	%r2,%r1,%r2
	sbb	%r3,%r1,%r3
mul_llw_posr3:

	mov	%r1,%r4
	lda	%r5,.+6
	ldw	%pc,#__umul	; R1R0 = R3R2 * R1

	ldw	%r2,-60(%r6)
	tst	%r2
	bpl	mul_llw_posr2
	clr	%r2
	sub	%r0,%r2,%r0
	sbb	%r1,%r2,%r1
mul_llw_posr2:

	mov	%r2,%r0

	ldw	%r0,-64(%r6)
	stw	%r2,0(%r0)
	stw	%r1,2(%r0)

	ldw	%r3,-58(%r6)
	ldw	%r4,-56(%r6)
	ldw	%r5,-54(%r6)
	lda	%r6,12(%r6)
	lda	%pc,0(%r3)
