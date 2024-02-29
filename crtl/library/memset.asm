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

; void memset (char *, int, int)
	.align	2
	.global	memset
memset:
	ldw	%r0,-64(%r6)	; R0 = dest addr
	ldw	%r2,-60(%r6)	; R2 = byte count
	or	%r1,%r0,%r2	; R1 = check for alignment
	lsr	%r1,%r1
	ldbu	%r1,-62(%r6)	; R1 = data byte
	bcc	memset_w
	neg	%r2,%r2		; fill bytes
	beq	memset_done
memset_loop:
	stb	%r1,0(%r0)	; write byte
	lda	%r0,1(%r0)	; increment pointer
	inc	%r2,%r2
	bne	memset_loop
memset_done:
	lda	%r6,6(%r6)
	lda	%r7,0(%r3)
memset_w:
	lsr	%r2,%r2		; R2 = word count
	neg	%r2,%r2
	beq	memset_w_done
	stb	%r1,-61(%r6)	; R1 = double byte
	ldw	%r1,-62(%r6)
memset_w_loop:
	stw	%r1,0(%r0)	; write word
	lda	%r0,2(%r0)	; increment pointer
	inc	%r2,%r2
	bne	memset_w_loop
memset_w_done:
	lda	%r6,6(%r6)
	lda	%r7,0(%r3)

