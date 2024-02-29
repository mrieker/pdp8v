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

; R0 = dest address
; R1 = source address
; R2 = double-word count
	.global	__memcpy_ww
__memcpy_ww:
	lda	%r6,-2(%r6)
	stw	%r3,-64(%r6)
	neg	%r2,%r2
	beq	memcpy_ww_done
memcpy_ww_loop:
	ldw	%r3,0(%r1)
	stw	%r3,0(%r0)
	ldw	%r3,2(%r1)
	stw	%r3,2(%r0)
	lda	%r0,4(%r0)
	lda	%r1,4(%r1)
	inc	%r2,%r2
	bne	memcpy_ww_loop
memcpy_ww_done:
	ldw	%r3,-64(%r6)
	lda	%r6,2(%r6)
	lda	%r7,0(%r3)

