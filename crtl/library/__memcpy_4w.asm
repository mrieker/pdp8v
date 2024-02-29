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

; R0 = points to 4-word word-aligned output
; R1 = points to 4-word word-aligned input
; R3 = return address
	.global	__memcpy_4w
__memcpy_4w:
	ldw	%r2,0(%r1)
	stw	%r2,0(%r0)
	ldw	%r2,2(%r1)
	stw	%r2,2(%r0)
	ldw	%r2,4(%r1)
	stw	%r2,4(%r0)
	ldw	%r2,6(%r1)
	stw	%r2,6(%r0)
	lda	%r7,0(%r3)

