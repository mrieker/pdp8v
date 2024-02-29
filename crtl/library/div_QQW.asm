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
;   R0 = unsigned quad address
;   R1 = unsigned quad address
;   R2 = unsigned word
;  output:
;   R0 = same as input
;
	.align	2
	.global	__div_QQW
__div_QQW:
	lda	%r6,-10(%r6)
	stw	%r0,-64(%r6)
	stw	%r1,-62(%r6)
	stw	%r3,-60(%r6)
	stw	%r4,-58(%r6)
	stw	%r5,-56(%r6)

	ldw	%r0,6(%r1)
	clr	%r1
	lda	%r5,div_QQW_6348
	ldw	%pc,#__udiv
div_QQW_6348:
	ldw	%r3,-64(%r6)
	stw	%r0,6(%r3)

	ldw	%r3,-62(%r6)
	ldw	%r0,4(%r3)
	lda	%r5,div_QQW_4732
	ldw	%pc,#__udiv
div_QQW_4732:
	ldw	%r3,-64(%r6)
	stw	%r0,4(%r3)

	ldw	%r3,-62(%r6)
	ldw	%r0,2(%r3)
	lda	%r5,div_QQW_3116
	ldw	%pc,#__udiv
div_QQW_3116:
	ldw	%r3,-64(%r6)
	stw	%r0,2(%r3)

	ldw	%r3,-62(%r6)
	ldw	%r0,0(%r3)
	lda	%r5,div_QQW_1500
	ldw	%pc,#__udiv
div_QQW_1500:
	ldw	%r3,-64(%r6)
	stw	%r0,0(%r3)

	ldw	%r0,-64(%r6)
	ldw	%r3,-60(%r6)
	ldw	%r4,-58(%r6)
	ldw	%r5,-56(%r6)
	lda	%r6,10(%r6)
	lda	%pc,0(%r3)
