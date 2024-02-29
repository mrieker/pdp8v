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

; char *getenv (char const *)
	.psect	.70.data
	.align	2
getenv_scnblk: .word SCN_GETENV
getenv_bufptr: .word 0
getenv_buflen: .word 0
getenv_name:   .word 0
	.psect	.50.text
	.global	getenv
getenv:
	lda	%r6,-12(%r6)
	stw	%r3,-64(%r6)
	stw	%r4,-62(%r6)
	stw	%r5,-60(%r6)
	ldw	%r2,#getenv_scnblk
	ldw	%r0,-52(%r6)
	stw	%r0,getenv_name-getenv_scnblk(%r2)
getenv_loop:
	clr	%r0
	stw	%r2,MAGIC_SCN(%r0)
	ldw	%r0,MAGIC_SCN(%r0)
	tst	%r0
	beq	getenv_ret
	ldw	%r1,getenv_buflen-getenv_scnblk(%r2)
	cmp	%r1,%r0
	bhis	getenv_good
	stw	%r0,getenv_buflen-getenv_scnblk(%r2)
	lda	%r6,-4(%r6)
	ldw	%r1,getenv_bufptr-getenv_scnblk(%r2)
	stw	%r1,-64(%r6)
	stw	%r0,-62(%r6)
	lda	%r3,.+6
	ldw	%pc,#realloc
	ldw	%r2,#getenv_scnblk
	stw	%r0,getenv_bufptr-getenv_scnblk(%r2)
	br	getenv_loop
getenv_good:
	ldw	%r0,getenv_bufptr-getenv_scnblk(%r2)
getenv_ret:
	ldw	%r3,-64(%r6)
	ldw	%r4,-62(%r6)
	ldw	%r5,-60(%r6)
	lda	%r6,14(%r6)
	lda	%pc,0(%r3)

