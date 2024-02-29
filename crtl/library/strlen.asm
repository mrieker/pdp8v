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

; int strlen (char const *)
	.align	2
	.global	strlen
strlen:
	ldw	%r0,-64(%r6)
	lda	%r1,1(%r0)
strlen_loop:
	ldbu	%r2,0(%r0)
	lda	%r0,1(%r0)
	tst	%r2
	bne	strlen_loop
	sub	%r0,%r0,%r1
	lda	%r6,2(%r6)
	lda	%r7,0(%r3)

