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
; modulus
;  input:
;   R1 = signed word
;   R2 = unsigned word
;   R3 = return address
;  output:
;   R0 = signed word
;   R1,R2,R3 = trashed
;   R4,R5 = as on input
;
	.align	2
	.global	__mod_wwW
__mod_wwW:
	lda	%r6,-6(%r6)
	stw	%r3,-64(%r6)
	stw	%r4,-62(%r6)
	stw	%r5,-60(%r6)
	mov	%r3,%r1
	mov	%r0,%r1
	bpl	mod_wwW_posr1
	neg	%r0,%r1
mod_wwW_posr1:
	clr	%r1
	lda	%r5,mod_wwW_subret
	ldw	%pc,#__udiv
mod_wwW_subret:
	mov	%r0,%r1
	tst	%r3
	bpl	mod_wwW_posr0
	neg	%r0,%r1
mod_wwW_posr0:
	ldw	%r3,-64(%r6)
	ldw	%r4,-62(%r6)
	stw	%r5,-60(%r6)
	lda	%r6,6(%r6)
	lda	%pc,0(%r3)

