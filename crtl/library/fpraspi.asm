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

;
; floatingpoint arithmetic via raspi
; use fpmp.asm for software floatingpoint
;
;  input:
;   R0 = result address
;   R1 = operand address
;   R2 = operand address
;   R3 = return address
;  output:
;   R0,R4,R5 = same as on input
;   R1,R2,R3 = trashed
;
	.align	2

.if RASPIFLOATS
	.global	__add_ddd
__add_ddd:
	lda	%r6,-8(%r6)
	stw	%r1,-60(%r6)
	ldw	%r1,ss_add_ddd
	br	fparith
ss_add_ddd: .word SCN_ADD_DDD

	.global	__add_fff
__add_fff:
	lda	%r6,-8(%r6)
	stw	%r1,-60(%r6)
	ldw	%r1,ss_add_fff
	br	fparith
ss_add_fff: .word SCN_ADD_FFF

	.global	__sub_ddd
__sub_ddd:
	lda	%r6,-8(%r6)
	stw	%r1,-60(%r6)
	ldw	%r1,ss_sub_ddd
	br	fparith
ss_sub_ddd: .word SCN_SUB_DDD

	.global	__sub_fff
__sub_fff:
	lda	%r6,-8(%r6)
	stw	%r1,-60(%r6)
	ldw	%r1,ss_sub_fff
	br	fparith
ss_sub_fff: .word SCN_SUB_FFF

	.global	__mul_ddd
__mul_ddd:
	lda	%r6,-8(%r6)
	stw	%r1,-60(%r6)
	ldw	%r1,ss_mul_ddd
	br	fparith
ss_mul_ddd: .word SCN_MUL_DDD

	.global	__mul_fff
__mul_fff:
	lda	%r6,-8(%r6)
	stw	%r1,-60(%r6)
	ldw	%r1,ss_mul_fff
	br	fparith
ss_mul_fff: .word SCN_MUL_FFF

	.global	__div_ddd
__div_ddd:
	lda	%r6,-8(%r6)
	stw	%r1,-60(%r6)
	ldw	%r1,ss_div_ddd
	br	fparith
ss_div_ddd: .word SCN_DIV_DDD

	.global	__div_fff
__div_fff:
	lda	%r6,-8(%r6)
	stw	%r1,-60(%r6)
	ldw	%r1,ss_div_fff
	br	fparith
ss_div_fff: .word SCN_DIV_FFF

fparith:
	stw	%r0,-62(%r6)
	stw	%r2,-58(%r6)
	stw	%r1,-64(%r6)
	lda	%r2,-64(%r6)
	clr	%r1
	stw	%r2,MAGIC_SCN(%r1)
	lda	%r6,8(%r6)
	lda	%pc,0(%r3)

;
; floatingpoint comparison
;  input:
;   R0 = condition code
;   R1 = operand address
;   R2 = operand address
;   R3 = return address
;  output:
;   R0 = result code
;   R4,R5 = same as on input
;   R1,R2,R3 = trashed
;
	.align	2
	.global	__cmp_dd
__cmp_dd:
	lda	%r6,-8(%r6)
	stw	%r1,-60(%r6)
	ldw	%r1,ss_cmp_dd
	br	fpcomp
ss_cmp_dd: .word SCN_CMP_DD

	.global	__cmp_ff
__cmp_ff:
	lda	%r6,-8(%r6)
	stw	%r1,-60(%r6)
	ldw	%r1,ss_cmp_ff
	br	fpcomp
ss_cmp_ff: .word SCN_CMP_FF

fpcomp:
	stw	%r0,-62(%r6)
	stw	%r2,-58(%r6)
	stw	%r1,-64(%r6)
	lda	%r2,-64(%r6)
	clr	%r1
	stw	%r2,MAGIC_SCN(%r1)
	lda	%r6,8(%r6)
	ldw	%r0,MAGIC_SCN(%r1)
	lda	%pc,0(%r3)
.endif

;
; floatingpoint conversion
;   __cvt_<resulttype><operandtype>
;  input:
;   R0 = l L q Q f d: where to put result
;   R1 = b B w W: operand value
;           else: where to get operand
;   R2<15:08> = result type b B w W l L q Q f d z
;   R2<07:00> = operand type b B w W l L q Q f d
;   R3 = return address
;  output:
;   R0 = b B w W z: result value
;             else: same as input
;   R1,R2,R3 = trashed
;   R4,R5 = same as input
;
	.global	__cvt_Qd
__cvt_Qd:
	ldw	%r2,#'dQ'
	br	cvt_fp

	.global	__cvt_Qf
__cvt_Qf:
	ldw	%r2,#'fQ'
	br	cvt_fp

	.global	__cvt_dQ
__cvt_dQ:
	ldw	%r2,#'Qd'
	br	cvt_fp

	.global	__cvt_dW
__cvt_dW:
	ldw	%r2,#'Wd'
	br	cvt_fp

	.global	__cvt_db
__cvt_db:
	ldw	%r2,#'bd'
	br	cvt_fp

	.global	__cvt_df
__cvt_df:
	ldw	%r2,#'fd'
	br	cvt_fp

	.global	__cvt_dw
__cvt_dw:
	ldw	%r2,#'wd'
	br	cvt_fp

	.global	__cvt_fQ
__cvt_fQ:
	ldw	%r2,#'Qf'
	br	cvt_fp

	.global	__cvt_fb
__cvt_fb:
	ldw	%r2,#'bf'
	br	cvt_fp

	.global	__cvt_fd
__cvt_fd:
	ldw	%r2,#'df'
	br	cvt_fp

	.global	__cvt_fw
__cvt_fw:
	ldw	%r2,#'wf'
	br	cvt_fp

	.global	__cvt_wd
__cvt_wd:
	ldw	%r2,#'dw'
	br	cvt_fp

	.global	__cvt_wf
__cvt_wf:
	ldw	%r2,#'fw'
	;br	cvt_fp

cvt_fp:
	lda	%r6,-8(%r6)
	stw	%r0,-62(%r6)
	stw	%r1,-60(%r6)
	stw	%r2,-58(%r6)
	ldw	%r1,ss_cvt_fp
	stw	%r1,-64(%r6)
	lda	%r2,-64(%r6)
	stw	%r2,MAGIC_SCN-SCN_CVT_FP(%r1)
	ldw	%r0,MAGIC_SCN-SCN_CVT_FP(%r1)
	lda	%r6,8(%r6)
	lda	%pc,0(%r3)
ss_cvt_fp: .word SCN_CVT_FP

;
; floatingpoint negation
;  input:
;   R0 = points to where result goes
;   R1 = points to where operand is
;   R3 = return address
;  output:
;   R0 = same as on input
;   R1,R2,R3 = trashed
;
.if RASPIFLOATS
	.global	__neg_ff
__neg_ff:
	lda	%r6,-6(%r6)
	stw	%r0,-62(%r6)
	stw	%r1,-60(%r6)
	ldw	%r1,ss_neg_ff
	stw	%r1,-64(%r6)
	lda	%r2,-64(%r6)
	stw	%r2,MAGIC_SCN-SCN_NEG_FF(%r1)
	lda	%r6,6(%r6)
	lda	%pc,0(%r3)
ss_neg_ff: .word SCN_NEG_FF

	.global	__neg_dd
__neg_dd:
	lda	%r6,-6(%r6)
	stw	%r0,-62(%r6)
	stw	%r1,-60(%r6)
	ldw	%r1,ss_neg_dd
	stw	%r1,-64(%r6)
	lda	%r2,-64(%r6)
	stw	%r2,MAGIC_SCN-SCN_NEG_DD(%r1)
	lda	%r6,6(%r6)
	lda	%pc,0(%r3)
ss_neg_dd: .word SCN_NEG_DD
.endif

