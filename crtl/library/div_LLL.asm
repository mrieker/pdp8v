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
;   R0 = address of output quotient long
;   R1 = address of input dividend long
;   R2 = address of input divisor long
;   R3 = return address
;  output:
;   *R0 = filled in
;   R1,R2,R3 = trashed
;   R0,R4,R5 = as on input
;
;  stack:
;   DI.VI.DE.ND : -52
;         DV.SR : -56
;            R5 : -58
;            R4 : -60
;            R3 : -62
;            R0 : -64
;
	dvden = -52
	dvsor = -56
	.align	2
	.global	__mod_LLL
	.global	__div_LLL
__mod_LLL:
	lda	%r0,1(%r0)
__div_LLL:
	lda	%r6,-20(%r6)
	stw	%r0,-64(%r6)
	stw	%r3,-62(%r6)
	stw	%r4,-60(%r6)
	stw	%r5,-58(%r6)

	; copy divisor where it's easy to access
	ldw	%r0,0(%r2)
	stw	%r0,dvsor+0(%r6)
	ldw	%r0,2(%r2)
	stw	%r0,dvsor+2(%r6)

	; copy dividend where it's easy to access
	ldw	%r3,0(%r1)
	ldw	%r0,2(%r1)
	stw	%r0,dvden+2(%r6)
	clr	%r2
	clr	%r4

	; divide out:
	;  R4.R2.DE.R3
	;    /   DV.SR
	;  -- -- -- --
	;  rr rr qq qq

	lda	%r5,-32(%r4)
div_LLL_loop:

	; shift dividend left one bit
	shl	%r3,%r3
	ldw	%r0,dvden+2(%r6)
	rol	%r0,%r0
	stw	%r0,dvden+2(%r6)
	rol	%r2,%r2
	rol	%r4,%r4

	; subtract R1R0 = top dividend - divisor
	ldw	%r1,dvsor+0(%r6)
	sub	%r0,%r2,%r1
	ldw	%r1,dvsor+2(%r6)
	sbb	%r1,%r4,%r1

	; if R1R0 >= 0 {
	;   top dividend = R1R0
	;   set low quotient bit
	; }
	bcs	div_LLL_next
	mov	%r2,%r0
	mov	%r4,%r1
	inc	%r3,%r3
div_LLL_next:

	; repeat for 32 bits
	inc	%r5,%r5
	bne	div_LLL_loop

	; store quotient as result
	ldw	%r0,-64(%r6)
	lsr	%pc,%r0
	bcs	div_LLL_mod
	stw	%r3,0(%r0)
	ldw	%r1,dvden+2(%r6)
	stw	%r1,2(%r0)
	br	div_LLL_ret

	; store remainder as result
div_LLL_mod:
	lda	%r0,-1(%r0)
	stw	%r2,0(%r0)
	stw	%r4,2(%r0)

	; restore and return
div_LLL_ret:
	ldw	%r3,-62(%r6)
	ldw	%r4,-60(%r6)
	ldw	%r5,-58(%r6)
	lda	%r6,20(%r6)
	lda	%pc,0(%r3)
