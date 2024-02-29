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

	.align	2
	.global	__mul_www
__mul_www:
	lda	%r6,-8(%r6)
	stw	%r3,-62(%r6)
	stw	%r4,-60(%r6)
	stw	%r5,-58(%r6)
	mov	%r0,%r1
	bpl	mul_www_posi
	neg	%r1,%r1
mul_www_posi:
	tst	%r2
	bpl	mul_www_posj
	com	%r0,%r0
	neg	%r2,%r2
mul_www_posj:
	stw	%r0,-64(%r6)
	clr	%r3
	lda	%r5,mul_www_done
	ldw	%pc,#__umul
mul_www_done:
	ldw	%r1,-64(%r6)
	tst	%r1
	bpl	mul_www_poso
	neg	%r0,%r0
mul_www_poso:
	ldw	%r3,-62(%r6)
	ldw	%r4,-60(%r6)
	ldw	%r5,-58(%r6)
	lda	%r6,8(%r6)
	lda	%pc,0(%r3)
