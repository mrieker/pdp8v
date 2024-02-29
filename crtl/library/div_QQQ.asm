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
;   R0 = address of output quotient quad
;   R1 = address of input dividend quad
;   R2 = address of input divisor quad
;   R3 = return address
;  output:
;   *R0 = filled in
;   R1,R2,R3 = trashed
;   R0,R4,R5 = as on input
;
	sav0 = -64
	sav3 = -62
	sav4 = -60
	sav5 = -58
	dvnd = -56
	size = -44+64

	.align	2
	.global	__mod_QQQ
	.global	__div_QQQ
__mod_QQQ:
	lda	%r0,1(%r0)
__div_QQQ:
	lda	%r6,-size(%r6)
	stw	%r0,sav0(%r6)
	stw	%r3,sav3(%r6)
	stw	%r4,sav4(%r6)
	stw	%r5,sav5(%r6)

	; copy dividend where it's easy to access
	ldw	%r0,0(%r1)
	stw	%r0,dvnd+0(%r6)
	ldw	%r0,2(%r1)
	stw	%r0,dvnd+2(%r6)
	ldw	%r0,4(%r1)
	stw	%r0,dvnd+4(%r6)
	ldw	%r0,6(%r1)
	stw	%r0,dvnd+6(%r6)

	; zero extend dividend to 128 bits
	clr	%r4
	clr	%r5
	stw	%r4,dvnd+ 8(%r6)
	stw	%r5,dvnd+10(%r6)

	; divide out:
	;   r5.r4.00.00.DI.VI.DE.ND   dvnd(%r6)
	; / DI.VI.SO.RR               0(%r2)
	;   -- -- -- -- -- -- -- --
	;   rr.rr.rr.rr qq.qq.qq.qq

	lda	%r3,-64(%r4)
div_QQQ_loop:

	; R0 =
	; R1 =
	; R2 = divisor pointer
	; R3 = loop counter
	; R4 = dividend<111:096>
	; R5 = dividend<127:112>

	; shift 128-bit dividend left one bit
	ldw	%r0,dvnd+ 0(%r6)
	shl	%r0,%r0
	stw	%r0,dvnd+ 0(%r6)
	ldw	%r0,dvnd+ 2(%r6)
	rol	%r0,%r0
	stw	%r0,dvnd+ 2(%r6)
	ldw	%r0,dvnd+ 4(%r6)
	rol	%r0,%r0
	stw	%r0,dvnd+ 4(%r6)
	ldw	%r0,dvnd+ 6(%r6)
	rol	%r0,%r0
	stw	%r0,dvnd+ 6(%r6)
	ldw	%r0,dvnd+ 8(%r6)
	rol	%r0,%r0
	stw	%r0,dvnd+ 8(%r6)
	ldw	%r0,dvnd+10(%r6)
	rol	%r0,%r0
	stw	%r0,dvnd+10(%r6)
	rol	%r4,%r4
	rol	%r5,%r5

	; compare dividend<127:064> to divisor<63:00>
	ldw	%r0,dvnd+ 8(%r6)
	ldw	%r1,0(%r2)
	sub	%pc,%r0,%r1
	ldw	%r0,dvnd+10(%r6)
	ldw	%r1,2(%r2)
	sbb	%pc,%r0,%r1
	ldw	%r1,4(%r2)
	sbb	%pc,%r4,%r1
	ldw	%r1,6(%r2)
	sbb	%pc,%r5,%r1

	bcs	div_QQQ_next

	; dividend<127:064> .ge. divisor<63:00>
	; subtract divisor<63:00> from dividend<127:064>
	ldw	%r0,dvnd+ 8(%r6)
	ldw	%r1,0(%r2)
	sub	%r0,%r0,%r1
	stw	%r0,dvnd+ 8(%r6)
	ldw	%r0,dvnd+10(%r6)
	ldw	%r1,2(%r2)
	sbb	%r0,%r0,%r1
	stw	%r0,dvnd+10(%r6)
	ldw	%r1,4(%r2)
	sbb	%r4,%r4,%r1
	ldw	%r1,6(%r2)
	sbb	%r5,%r5,%r1

	; and set low quotient bit (dividend<00>)
	ldw	%r0,dvnd+ 0(%r6)
	inc	%r0,%r0
	stw	%r0,dvnd+ 0(%r6)
div_QQQ_next:

	; repeat for 32 bits
	inc	%r3,%r3
	bne	div_QQQ_loop

	; store quotient (dividend<063:000>) or remainder (dividend<127:000>) as result
	lda	%r1,dvnd+0(%r6)	; point to quotient
	ldw	%r0,sav0(%r6)		; point to output
	lsr	%pc,%r0
	bcc	div_QQQ_store
	lda	%r1,dvnd+8(%r6)	; point to remainder
	lda	%r0,-1(%r0)
div_QQQ_store:
	ldw	%r2,0(%r1)		; copy quotient or remainder to output
	stw	%r2,0(%r0)
	ldw	%r2,2(%r1)
	stw	%r2,2(%r0)
	ldw	%r2,4(%r1)
	stw	%r2,4(%r0)
	ldw	%r2,6(%r1)
	stw	%r2,6(%r0)

	; restore and return
	ldw	%r3,sav3(%r6)
	ldw	%r4,sav4(%r6)
	ldw	%r5,sav5(%r6)
	lda	%r6,size(%r6)
	lda	%pc,0(%r3)
