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

; void memmove (char *, char const *, int)
	.align	2
	.global	memmove
memmove:
	lda	%r6,-2(%r6)
	stw	%r3,-64(%r6)
	ldw	%r0,-62(%r6)
	ldw	%r1,-60(%r6)
	ldw	%r2,-58(%r6)
	cmp	%r0,%r1
	beq	memmove_done
	blo	memmove_fwd
	add	%r0,%r0,%r2
	add	%r1,%r1,%r2
	or	%r3,%r0,%r1
	or	%r3,%r3,%r2
	lsr	%r3,%r3
	bcc	memmove_w
	neg	%r2,%r2
	beq	memmove_done
memmove_loop:
	lda	%r1,-1(%r1)
	lda	%r0,-1(%r0)
	ldbu	%r3,0(%r1)
	stb	%r3,0(%r0)
	inc	%r2,%r2
	bne	memmove_loop
memmove_done:
	ldw	%r3,-64(%r6)
	lda	%r6,8(%r6)
	lda	%r7,0(%r3)
memmove_w:
	lsr	%r2,%r2
	neg	%r2,%r2
	beq	memmove_w_done
memmove_w_loop:
	lda	%r1,-2(%r1)
	lda	%r0,-2(%r0)
	ldw	%r3,0(%r1)
	stw	%r3,0(%r0)
	inc	%r2,%r2
	bne	memmove_w_loop
memmove_w_done:
	ldw	%r3,-64(%r6)
	lda	%r6,8(%r6)
	lda	%r7,0(%r3)
memmove_fwd:
	lda	%r6,8(%r6)
	ldw	%pc,#__memcpy

