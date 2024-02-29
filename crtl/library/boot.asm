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

;;;;;;;;;;;;;;;;;
;;  Bootstrap  ;;
;;;;;;;;;;;;;;;;;

	intsaver0 = 0xFFEA
	intsaver1 = 0xFFEC
	intsaver2 = 0xFFEE
	intsaver3 = 0xFFF4
	intsaver4 = 0xFFF6
	intsaver5 = 0xFFF8
	intserv   = 0xFFFA
;
; starts at location 0
;
	.psect	.00.boot
	.global	__boot
__boot:					; used by linker to force loading this module
zeropc:	br	procreset		; processor resets here
	stw	%r0,zeropc+intsaver0	; processor interrupts here
	stw	%r1,zeropc+intsaver1
	stw	%r2,zeropc+intsaver2
	stw	%r3,zeropc+intsaver3
	stw	%r4,zeropc+intsaver4
	stw	%r5,zeropc+intsaver5
	lda	%r3,intreturn
	ldw	%pc,zeropc+intserv
intreturn:
	ldw	%r0,zeropc+intsaver0
	ldw	%r1,zeropc+intsaver1
	ldw	%r2,zeropc+intsaver2
	ldw	%r3,zeropc+intsaver3
	ldw	%r4,zeropc+intsaver4
	ldw	%r5,zeropc+intsaver5
	iret

procreset:
	lda	%r0,intreturn
	stw	%r0,zeropc+intserv
	clr	%r0
	clr	%r1
	clr	%r2
	clr	%r3
	clr	%r4
	clr	%r5
	clr	%r6
	ldw	%pc,#_start

