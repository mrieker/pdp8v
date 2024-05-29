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

;; echo keyboard to printer

;;  ../asm/assemble kbecho.asm kbecho.obj > kbecho.lis
;;  ../asm/link -o kbecho.oct kbecho.obj > kbecho.map

	ksf = 06031	; (TTY) skip if there is a kb character to be read
    	krb = 06036	; (TTY) read kb character and clear flag  

	tfl = 06040	; (TTY) pretend the printer is ready to accept a character
	tsf = 06041	; (TTY) skip if printer is ready to accept a character
	tls = 06046	; (TTY) turn off interrupt request for previous char and start printing new char

	. = 0020

_0012:	.word	00012
_0015:	.word	00015
_0177:	.word	00177
_7766:	.word	07766

	. = 0200
	.global	__boot
__boot:
	krb
	tfl

loop:
	ksf
	jmp	.-1
	cla
	krb
	and	_0177

	tad	_7766
	sza
	jmp	print

	tad	_0015
	jms	printac
	cla
print:
	tad	_0012
	jms	printac
	jmp	loop

printac: .word	.-.
	tsf
	jmp	.-1
	tls
	jmpi	printac

