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
; unsigned quad AND
;  input:
;   R0 = output address
;   R1 = input address
;   R2 = input address
;   R3 = return address
;  output:
;   R0,R4,R5 = same as on input
;   R1,R2,R3 = trashed
;
	.align	2
	.global	__and_QQQ
__and_QQQ:
	lda	%r6,-4(%r6)
	stw	%r3,-64(%r6)
	stw	%r4,-62(%r6)

	ldw	%r3,0(%r1)
	ldw	%r4,0(%r2)
	and	%r3,%r3,%r4
	stw	%r3,0(%r0)

	ldw	%r3,2(%r1)
	ldw	%r4,2(%r2)
	and	%r3,%r3,%r4
	stw	%r3,2(%r0)

	ldw	%r3,4(%r1)
	ldw	%r4,4(%r2)
	and	%r3,%r3,%r4
	stw	%r3,4(%r0)

	ldw	%r3,6(%r1)
	ldw	%r4,6(%r2)
	and	%r3,%r3,%r4
	stw	%r3,6(%r0)

	ldw	%r3,-64(%r6)
	ldw	%r4,-62(%r6)
	lda	%r6,4(%r6)
	lda	%pc,0(%r3)
