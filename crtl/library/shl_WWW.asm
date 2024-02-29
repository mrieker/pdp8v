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
; shift left
;  input:
;   R1 = unsigned word
;   R2 = unsigned word
;   R3 = return address
;  output:
;   R0 = unsigned word
;
	.align	2
	.global	__shl_WWW
	.global	__shl_WwW
	.global	__shl_wWW
	.global	__shl_wwW
__shl_WWW:
__shl_WwW:
__shl_wWW:
__shl_wwW:
	ldw	%r0,#15
	and	%r2,%r2,%r0
	shl	%r2,%r2
	sub	%r2,%pc,%r2
shl_WWW_base:
	lda	%pc,shl_WWW_zero-shl_WWW_base(%r2)
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
shl_WWW_zero:
	mov	%r0,%r1
	lda	%pc,0(%r3)

