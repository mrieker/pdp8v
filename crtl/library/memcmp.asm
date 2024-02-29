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

; int memcmp (char const *, char const *, size_t)
	.align	2
	.global	memcmp
memcmp:
	lda	%r6,-4(%r6)
	stw	%r3,-64(%r6)
	stw	%r4,-62(%r6)
	ldw	%r2,-60(%r6)
	ldw	%r3,-58(%r6)
	ldw	%r0,-56(%r6)
	neg	%r4,%r0
	beq	memcmp_done
memcmp_loop:
	ldbs	%r0,0(%r2)
	ldbs	%r1,0(%r3)
	sub	%r0,%r0,%r1
	bne	memcmp_done
	lda	%r2,1(%r2)
	lda	%r3,1(%r3)
	inc	%r4,%r4
	bne	memcmp_loop
memcmp_done:
	ldw	%r3,-64(%r6)
	ldw	%r4,-62(%r6)
	lda	%r6,10(%r6)
	lda	%pc,0(%r3)

