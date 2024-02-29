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

;
; divide
;  input:
;   R0 = divide: unsigned long address for quotient; modulus: undefined
;   R1 = unsigned long address of dividend
;   R2 = unsigned word divisor
;  output:
;   R0 = divide: same as input; modulus: remainder
;
	.align	2
	.global	__mod_WLW
	.global	__div_LLW
__mod_WLW:
	clr	%r0
__div_LLW:
	lda	%r6,-10(%r6)
	stw	%r1,-62(%r6)
	stw	%r3,-60(%r6)
	stw	%r4,-58(%r6)
	stw	%r5,-56(%r6)

	mov	%r3,%r0			; quotient pointer (zero for modulus)

	ldw	%r0,2(%r1)		; get high dividend
	clr	%r1
	lda	%r5,div_LLW_3116
	ldw	%pc,#__udiv		; R1R0 / R2 => R0 r R1
div_LLW_3116:
	tst	%r3			; save high quotient
	beq	div_LLW_modhi
	stw	%r0,2(%r3)
div_LLW_modhi:

	ldw	%r0,-62(%r6)		; get low dividend
	ldw	%r0,0(%r0)
	lda	%r5,div_LLW_1500
	ldw	%pc,#__udiv		; R1R0 / R2 => R0 r R1
div_LLW_1500:
	tst	%r3			; save high quotient
	beq	div_LLW_modlo
	stw	%r0,0(%r3)
	mov	%r1,%r3			; return quotient pointer
div_LLW_modlo:

	mov	%r0,%r1			; return remainder
	ldw	%r3,-60(%r6)
	ldw	%r4,-58(%r6)
	ldw	%r5,-56(%r6)
	lda	%r6,10(%r6)
	lda	%pc,0(%r3)
