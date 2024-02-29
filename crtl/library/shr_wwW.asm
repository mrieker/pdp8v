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
;   R1 = signed word
;   R2 = unsigned word
;   R3 = return address
;  output:
;   R0 = signed word
;
	.align	2
	.global	__shr_wwW
__shr_wwW:
	ldw	%r0,#15
	and	%r2,%r2,%r0
	shl	%r2,%r2
	sub	%r2,%pc,%r2
shr_wwW_base:
	lda	%pc,shr_wwW_zero-shr_wwW_base(%r2)
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
	asr	%r1,%r1
shr_wwW_zero:
	mov	%r0,%r1
	lda	%pc,0(%r3)

