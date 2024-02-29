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
;;    http:;;www.gnu.org/licenses/gpl-2.0.html

	MAGIC_SCN = 0xFFF0
	MAGIC_ERR = 0xFFF2

	SCN_EXIT	=  0
	SCN_PRINTLN	=  1
	SCN_INTREQ	=  2
	SCN_CLOSE	=  3
	SCN_GETNOWNS	=  4
	SCN_OPEN	=  5
	SCN_READ	=  6
	SCN_WRITE	=  7
	SCN_IRQATNS	=  8
	SCN_INTACK	=  9
	SCN_UNLINK	= 10
	SCN_GETCMDARG	= 11
	SCN_ADD_DDD	= 12
	SCN_ADD_FFF	= 13
	SCN_SUB_DDD	= 14
	SCN_SUB_FFF	= 15
	SCN_MUL_DDD	= 16
	SCN_MUL_FFF	= 17
	SCN_DIV_DDD	= 18
	SCN_DIV_FFF	= 19
	SCN_CMP_DD	= 20
	SCN_CMP_FF	= 21
	SCN_CVT_FP	= 22
	SCN_UDIV	= 23
	SCN_UMUL	= 24
	SCN_SETROSIZE	= 25
	SCN_SETSTKLIM	= 26
	SCN_PRINTINSTR  = 27
	SCN_NEG_DD	= 28
	SCN_NEG_FF	= 29
	SCN_WATCHWRITE	= 30
	SCN_GETENV	= 31
	SCN_SETTTYECHO	= 32

	IRQ_SCNINTREQ	= 0x1
	IRQ_LINECLOCK	= 0x2
	IRQ_RANDMEM	= 0x4

	; /usr/include/asm-generic/fcntl.h
	O_ACCMODE	= 00000003
	O_RDONLY	= 00000000
	O_WRONLY	= 00000001
	O_RDWR		= 00000002
	O_CREAT		= 00000100
	O_EXCL		= 00000200
	O_NOCTTY	= 00000400
	O_TRUNC		= 00001000
	O_APPEND	= 00002000
	O_NONBLOCK	= 00004000
	O_DSYNC		= 00010000
	FASYNC		= 00020000
	O_DIRECT	= 00040000
	O_LARGEFILE	= 00100000
	O_DIRECTORY	= 00200000
	O_NOFOLLOW	= 00400000
	O_NOATIME	= 01000000
	O_CLOEXEC	= 02000000
	__O_SYNC	= 04000000
	O_SYNC		= __O_SYNC | O_DSYNC
	O_PATH		= 010000000
	__O_TMPFILE	= 020000000

	RASPIMULDIV = 1	; 0=software integer multiply/divide
			; 1=raspi integer multiply/divide

	RASPIFLOATS = 1	; 0=software floatingpoint
			; 1=raspi floatingpoint

