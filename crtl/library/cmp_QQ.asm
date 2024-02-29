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
; compare two quadwords
;  input:
;   R0 = branch opcode (0=blt, 4=blo, 8=bge, 12=bhis, 16=ble, 20=blos, 24=bgt, 28=bhi, 32/36=beq, 40/44=bne)
;   R1 = left operand address
;   R2 = right operand address
;  output:
;   R0 = 0 : branch opcode taken
;        1 : branch opcode not taken
;   R1,R2,R3 = trashed
;   R4,R5 = as on input
;
	.align	8
	.word	0		; to align cmp_qq_base on 8-byte boundary
cmp_qq_four:
	.word	4

	.global	__cmp_QQ
	.global	__cmp_qq
__cmp_QQ:
__cmp_qq:
	lda	%r6,-6(%r6)
	stw	%r3,-64(%r6)
	stw	%r4,-62(%r6)
	stw	%r5,-60(%r6)
	ldw	%r5,cmp_qq_four
	add	%r0,%pc,%r0	; test table has 4 bytes per entry
cmp_qq_base:
	ldw	%r3,6(%r1)	; compare bits <63:48>
	ldw	%r4,6(%r2)
	cmp	%r3,%r4
	bne	cmp_qq_test
	or	%r0,%r0,%r5	; switch to unsigned compares if not already
	ldw	%r3,4(%r1)	; compare bits <47:32>
	ldw	%r4,4(%r2)
	cmp	%r3,%r4
	bne	cmp_qq_test
	ldw	%r3,2(%r1)	; compare bits <31:16>
	ldw	%r4,2(%r2)
	cmp	%r3,%r4
	bne	cmp_qq_test
	ldw	%r3,0(%r1)	; compare bits <15:00>
	ldw	%r4,0(%r2)
	cmp	%r3,%r4
cmp_qq_test:
	lda	%pc,cmp_qq_here-cmp_qq_base(%r0)
cmp_qq_here:
	blt	cmp_qq_zero	; given 0=BLT, return 0 if branch taken
	br	cmp_qq_one	; given 0=BLT, return 1 if not taken
	blo	cmp_qq_zero
	br	cmp_qq_one
	bge	cmp_qq_zero
	br	cmp_qq_one
	bhis	cmp_qq_zero
	br	cmp_qq_one
	ble	cmp_qq_zero
	br	cmp_qq_one
	blos	cmp_qq_zero
	br	cmp_qq_one
	bge	cmp_qq_zero
	br	cmp_qq_one
	bhi	cmp_qq_zero
	br	cmp_qq_one
	beq	cmp_qq_zero
	br	cmp_qq_one
	beq	cmp_qq_zero
	br	cmp_qq_one
	bne	cmp_qq_zero
	br	cmp_qq_one
	bne	cmp_qq_zero
cmp_qq_one:
	inc	%r0,%r0		; R0 was even, make it odd to return 1
cmp_qq_zero:
	ldw	%r1,#1		; mask off upper bits of R0
	and	%r0,%r0,%r1
	ldw	%r3,-64(%r6)
	ldw	%r4,-62(%r6)
	ldw	%r5,-60(%r6)
	lda	%r6,6(%r6)
	lda	%pc,0(%r3)
