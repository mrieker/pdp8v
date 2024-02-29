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

	.psect	.50.text
	.align	2

.if 1 - RASPIFLOATS

;
; floatingpoint arithmetic
;  input:
;   R0 = result address
;   R1 = operand address
;   R2 = operand address
;   R3 = return address
;  output:
;   R0,R4,R5 = same as on input
;   R1,R2,R3 = trashed
;



;;;;;;;;;;;;;;;;;;;;;;;;
;;  SINGLE PRECISION  ;;
;;;;;;;;;;;;;;;;;;;;;;;;

	; <s><exp><man>
	;  s = 0: positive
	;      1: negative
	;  exp =  8 bits
	;  man = 23 bits

	sf_radr = -64
	sf_bval = -62
	sf_size =   6

sf_8000: .word 0x8000
sf_return:
	ldw	%r3,sf_radr(%r6)
	lda	%r6,sf_size(%r6)
	lda	%pc,0(%r3)

	.global	__sub_fff
__sub_fff:
	lda	%r6,-sf_size(%r6)
	stw	%r3,sf_radr(%r6)
	ldw	%r3,0(%r2)
	stw	%r3,sf_bval+0(%r6)
	ldw	%r3,2(%r2)
	ldw	%r2,sf_8000
	xor	%r3,%r3,%r2
	stw	%r3,sf_bval+2(%r6)
	lda	%r2,sf_bval(%r6)
	lda	%r3,sf_return
	;br	__add_fff

	af_sptr = -64
	af_aptr = -62
	af_bptr = -60
	af_radr = -58
	af_a    = -56
	af_b    = -48
	af_sav4 = -40
	af_sav5 = -38
	af_size = -36+64

	.global	__add_fff
__add_fff:
	lda	%r6,-af_size(%r6)
	stw	%r0,af_sptr(%r6)
	stw	%r1,af_aptr(%r6)
	stw	%r2,af_bptr(%r6)
	stw	%r3,af_radr(%r6)
	stw	%r4,af_sav4(%r6)
	stw	%r5,af_sav5(%r6)

	; chop up A operand
	lda	%r2,af_a(%r6)
	lda	%r3,af_achopped
	br	parsefloat
af_achopped:
	bcs	af_retnan

	; chop up B operand
	ldw	%r1,af_bptr(%r6)
	lda	%r2,af_b(%r6)
	lda	%r3,af_bchopped
	br	parsefloat
af_bchopped:
	bcc	af_bothnums

	; at least one operand isn't a number, so return not-a-number
af_retnan:
	ldw	%r4,af_FFFF
	mov	%r5,%r4
	br	af_retcompos
af_FFFF: .word	0xFFFF
af_23_b: .word	23
af_FF:	.word	0xFF

	; both are infinity
	; if opposite signs, return nan, else return infinity
af_bothinf:
	ldw	%r0,af_a+pf_neg(%r6)
	ldw	%r1,af_b+pf_neg(%r6)
	cmp	%r0,%r1
	bne	af_retnan
	br	af_returna

	; both are numbers, check inf-inf case
af_bothnums:
	ldw	%r2,af_a+pf_exp(%r6)	; abig = aexp - bexp
	ldw	%r1,af_b+pf_exp(%r6)
	ldw	%r0,af_FF
	and	%r3,%r2,%r1
	cmp	%r3,%r0
	beq	af_bothinf

	; shift aman or bman right to align exponents
	sub	%r0,%r2,%r1		; abig = aexp - bexp
	ble	af_anotbig		; if (abig <= 0) goto anotbig
	ldw	%r1,af_23_b		; if (abig > 23) goto returna
	cmp	%r0,%r1
	bgt	af_returna
	stw	%r2,af_b+pf_exp(%r6)	; bexp = aexp
	ldw	%r4,af_b+pf_man+0(%r6)	; R5..R4 = bman
	ldw	%r5,af_b+pf_man+2(%r6)
	lda	%r1,16-23(%r1)		; R5..R4 >>= abig
	and	%pc,%r1,%r0
	beq	af_bshift0
	mov	%r4,%r5
	clr	%r5
af_bshift0:
	lda	%r1,15-16(%r1)
	and	%r0,%r0,%r1
	beq	af_bshiftdone
	neg	%r0,%r0
af_bshiftbit:
	lsr	%r5,%r5
	ror	%r4,%r4
	inc	%r0,%r0
	bne	af_bshiftbit
af_bshiftdone:
	stw	%r4,af_b+pf_man+0(%r6)
	stw	%r5,af_b+pf_man+2(%r6)
	br	af_aligned
af_15_b: .word	15
af_23:	.word	23
af_returna:
	ldw	%r0,af_aptr(%r6)
	br	af_returnab

af_anotbig:
	ldw	%r2,af_b+pf_exp(%r6)	; bbig = bexp - aexp
	ldw	%r1,af_a+pf_exp(%r6)
	sub	%r0,%r2,%r1
	ble	af_aligned		; if (bbig <= 0) goto aligned
	ldw	%r1,af_23		; if (bbig > 23) goto returnb
	cmp	%r0,%r1
	bgt	af_returnb
	stw	%r2,af_a+pf_exp(%r6)	; aexp = bexp
	ldw	%r4,af_a+pf_man+0(%r6)	; R5..R4 = aman
	ldw	%r5,af_a+pf_man+2(%r6)
	lda	%r1,16-23(%r1)		; R5..R2 >>= bbig
	and	%pc,%r1,%r0
	beq	af_ashift0
	mov	%r4,%r5
	clr	%r5
af_ashift0:
	lda	%r1,15-16(%r1)
	and	%r0,%r0,%r1
	beq	af_ashiftdone
	neg	%r0,%r0
af_ashiftbit:
	lsr	%r5,%r5
	ror	%r4,%r4
	inc	%r0,%r0
	bne	af_ashiftbit
af_ashiftdone:
	stw	%r4,af_a+pf_man+0(%r6)
	stw	%r5,af_a+pf_man+2(%r6)
	br	af_aligned
af_15_c: .word	15
af_returnb:
	ldw	%r0,af_bptr(%r6)
af_returnab:
	ldw	%r4,0(%r0)
	ldw	%r5,2(%r0)
	br	af_retcompos
af_aligned:

	; convert bman to 2s complement
	ldw	%r2,af_b+pf_man+0(%r6)	; R3..R2 = bman
	ldw	%r3,af_b+pf_man+2(%r6)
	clr	%r0
	ldbu	%r1,af_b+pf_neg(%r6)
	tst	%r1
	beq	af_bispos
	sub	%r2,%r0,%r2		; R3..R2 = 0 - R3..R2
	sbb	%r3,%r0,%r3
af_bispos:

	; convert aman to 2s complement
	ldw	%r4,af_a+pf_man+0(%r6)	; R5..R4 = aman
	ldw	%r5,af_a+pf_man+2(%r6)
	ldbu	%r1,af_a+pf_neg(%r6)
	tst	%r1
	beq	af_aispos
	sub	%r4,%r0,%r4		; R5..R4 = 0 - R5..R4
	sbb	%r5,%r0,%r5
af_aispos:

	; perform addition
	add	%r4,%r4,%r2
	adc	%r5,%r5,%r3

	; convert sum to sign/magnitude
	clr	%r0
	shl	%r1,%r5			; aneg = aman < 0
	sbb	%r1,%r1,%r1
	stb	%r1,af_a+pf_neg(%r6)
	beq	af_sumispos		; if (aneg) aman = 0 - aman
	sub	%r4,%r0,%r4		; R5..R4 = 0 - R5..R4
	sbb	%r5,%r0,%r5
af_sumispos:

	; normalize result
	ldw	%r1,af_a+pf_exp(%r6)
	ldw	%r0,af_x100		; if (aman < 0x1000000) goto norm1;
	cmp	%r5,%r0
	ldw	%r0,af_FF_b
	blt	af_norm1
	cmp	%r1,%r0			; if (aexp >= 0xFF) goto norm2;
	bge	af_norm2
	inc	%r1,%r1			; aexp ++;
	lsr	%r5,%r5			; aman >>= 1;
	ror	%r4,%r4
af_norm1:
	cmp	%r1,%r0
af_norm2:
	blt	af_norm3		; if (aexp < 0xFF) goto norm3;
	mov	%r1,%r0			; aexp = 0xFF
	clr	%r4			; aman = 0
	clr	%r5
	br	af_normd
af_FF_b: .word 0xFF
af_x100: .word	0x100
af_x80:	.word	0x80
af_norm3:
	ldw	%r0,af_x80		; if (aman >= 0x800000) goto normd
af_norm4:
	cmp	%r5,%r0
	bge	af_normd
	tst	%r1			; if (aexp <= 0) goto subnorm
	beq	af_subnorm
	shl	%r4,%r4			; aman <<= 1
	rol	%r5,%r5
	lda	%r1,-1(%r1)		; -- aexp
	br	af_norm4
af_normd:

	; combine result components
	tst	%r1
	beq	af_subnorm
	ldw	%r0,af_x80
	sub	%r5,%r5,%r0		; clear hidden 1 bit
	shl	%r1,%r1			; aman |= aexp << 23
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	or	%r5,%r5,%r1
	br	af_getsign
af_subnorm:
	lsr	%r5,%r5			; aman >>= 1;
	ror	%r4,%r4
af_getsign:
	ldbs	%r0,af_a+pf_neg(%r6)	; shift sign bit from aneg into aman
	shl	%r5,%r5
	shl	%r0,%r0
	ror	%r5,%r5

	; store result
af_retcompos:
	ldw	%r0,af_sptr(%r6)
	stw	%r4,0(%r0)
	stw	%r5,2(%r0)

	ldw	%r3,af_radr(%r6)
	ldw	%r4,af_sav4(%r6)
	ldw	%r5,af_sav5(%r6)
	lda	%r6,af_size(%r6)
	lda	%pc,0(%r3)


;
; parse float into parts
;  input:
;   R1 = pointer to composite float
;   R2 = output pointer
;   R3 = return address
;  output:
;   c-bit set: input is not a number
;   else: pf_man(R2) = 32-bit mantissa (with hidden 1 bit filled in, unless subnormal)
;         pf_exp(R2) = 16-bit exponent
;         pf_neg(R2) = 0000: value is positive; FFFF: value is negative
;  scratch:
;   R0,R1,R4,R5
;
	pf_man = 0
	pf_exp = 4
	pf_neg = 6

pf_7F:	.word	0x7F
pf_FF:	.word	0xFF
pf_x80:	.word	0x80

parsefloat:

	; __int32_t aman = *aptr & 0x7FFFFF;
	ldw	%r4,0(%r1)
	stw	%r4,pf_man+0(%r2)
	ldw	%r4,2(%r1)
	ldw	%r0,pf_7F
	and	%r0,%r0,%r4
	stw	%r0,pf_man+2(%r2)

	; __int16_t aneg = aman < 0;
	shl	%r5,%r4
	sbb	%r5,%r5,%r5
	stw	%r5,pf_neg(%r2)

	; __int16_t aexp = (aman >> 23) & 0xFF;
	asr	%r5,%r4
	asr	%r5,%r5
	asr	%r5,%r5
	asr	%r5,%r5
	asr	%r5,%r5
	asr	%r5,%r5
	asr	%r5,%r5
	ldw	%r0,pf_FF
	and	%r5,%r5,%r0
	stw	%r5,pf_exp(%r2)

	; if isnan (a) goto retnan
	cmp	%r5,%r0
	bne	pf_isanum
	ldw	%r0,pf_man+0(%r2)
	ldw	%r1,pf_man+2(%r2)
	or	%r1,%r1,%r0
	bne	pf_retnan
pf_isanum:

	; insert hidden 1 bit or fix exponent
	; if (exp > 0) man |= 0x800000; else exp = 1
	tst	%r5
	beq	pf_subnor
	ldw	%r0,pf_man+2(%r2)	; set hidden 1 bit
	ldw	%r1,pf_x80
	add	%r0,%r0,%r1
	stw	%r0,pf_man+2(%r2)
	lda	%pc,0(%r3)		; return with carry clear
pf_subnor:
	inc	%r5,%r5			; subnormal, set exponent = 1
	stw	%r5,pf_exp(%r2)
	cmp	%r0,%r0			; clear carry
	lda	%pc,0(%r3)

	; input is not-a-number
pf_retnan:
	lsr	%r5,%r5			; R5 contained 0xFF, so this sets carry
	lda	%pc,0(%r3)



	mf_sptr = -64	;02
	mf_aptr = -62	;02
	mf_bptr = -60	;02
	mf_radr = -58	;02
	mf_a    = -56	;08
	mf_b    = -48	;08
	mf_sav4 = -40	;02
	mf_sav5 = -38	;02
	mf_pman = -36	;06
	mf_size = -30+64

	.global	__mul_fff
__mul_fff:
	lda	%r6,-mf_size(%r6)
	stw	%r0,mf_sptr(%r6)
	stw	%r1,mf_aptr(%r6)
	stw	%r2,mf_bptr(%r6)
	stw	%r3,mf_radr(%r6)
	stw	%r4,mf_sav4(%r6)
	stw	%r5,mf_sav5(%r6)

	; chop up A operand
	lda	%r2,mf_a(%r6)
	lda	%r3,mf_achopped
	br	parsefloat
mf_achopped:
	bcs	mf_retnan

	; chop up B operand
	ldw	%r1,mf_bptr(%r6)
	lda	%r2,mf_b(%r6)
	lda	%r3,mf_bchopped
	br	parsefloat
mf_bchopped:
	bcc	mf_bothnums

	; at least one operand isn't a number, return not-a-number
mf_retnan:
	ldw	%r4,mf_FFFF
	mov	%r5,%r4
	br	mf_retcompos
mf_FFFF: .word	0xFFFF
mf_FF:	.word	0xFF

	; one operand is infinity, if other is zero, return nan, else return inf
mf_haveinf:
	lda	%r3,.+6
	ldw	%pc,#findtopmanbitf
	tst	%r0
	bmi	mf_retnan
	br	mf_retinf

	; both are numbers, check infinity cases
mf_bothnums:
	ldw	%r0,mf_FF
	lda	%r1,mf_b+pf_man(%r6)
	ldw	%r2,mf_a+pf_exp(%r6)
	cmp	%r2,%r0
	beq	mf_haveinf
	lda	%r1,mf_a+pf_man(%r6)
	ldw	%r2,mf_b+pf_exp(%r6)
	cmp	%r2,%r0
	beq	mf_haveinf

	; prescale subnormals to put top bit in hidden position man<23>
	lda	%r1,mf_b+pf_man(%r6)
	lda	%r3,mf_bcounted
	ldw	%pc,#findtopmanbitf
mf_bcounted:
	tst	%r0
	bmi	mf_retzero
	ldw	%r1,mf_23_b
	sub	%r1,%r1,%r0
	ble	mf_binplace
	ldw	%r0,mf_b+pf_exp(%r6)
	sub	%r0,%r0,%r1
	stw	%r0,mf_b+pf_exp(%r6)
	lda	%r0,mf_b+pf_man(%r6)
	lda	%r3,mf_binplace
	ldw	%pc,#shiftlongleft
mf_23_b: .word	23
mf_binplace:
	lda	%r1,mf_a+pf_man(%r6)
	lda	%r3,mf_acounted
	ldw	%pc,#findtopmanbitf
mf_acounted:
	tst	%r0
	bmi	mf_retzero
	ldw	%r1,mf_23_b
	sub	%r1,%r1,%r0
	ble	mf_ainplace
	ldw	%r0,mf_a+pf_exp(%r6)
	sub	%r0,%r0,%r1
	stw	%r0,mf_a+pf_exp(%r6)
	lda	%r0,mf_a+pf_man(%r6)
	lda	%r3,mf_ainplace
	ldw	%pc,#shiftlongleft
mf_ainplace:

	; multiply
	;        00fa aaaa  mf_a+pf_man
	;     x  00fb bbbb  mf_b+pf_man
	;   pppp pppp xxxx  mf_pman
	;     +4   +2   +0
	clr	%r3
	ldw	%r2,mf_b+pf_man+0(%r6)
	ldw	%r1,mf_a+pf_man+0(%r6)
	lda	%r5,.+6
	ldw	%pc,#__umul		; R1R0 = R3R2 * R1; preserves R3R2
	stw	%r1,mf_pman+2(%r6)
	ldw	%r1,mf_a+pf_man+2(%r6)
	lda	%r5,.+6
	ldw	%pc,#__umul		; R1R0 = R3R2 * R1; preserves R3R2
	ldw	%r2,mf_pman+2(%r6)
	add	%r0,%r0,%r2
	adc	%r1,%r1,%r3
	stw	%r0,mf_pman+2(%r6)
	stw	%r1,mf_pman+4(%r6)

	ldw	%r2,mf_b+pf_man+2(%r6)
	ldw	%r1,mf_a+pf_man+0(%r6)
	lda	%r5,.+6
	ldw	%pc,#__umul		; R1R0 = R3R2 * R1; preserves R3R2
	ldw	%r4,mf_pman+2(%r6)
	ldw	%r5,mf_pman+4(%r6)
	add	%r0,%r0,%r4
	adc	%r1,%r1,%r5
	stw	%r0,mf_pman+2(%r6)
	stw	%r1,mf_pman+4(%r6)
	ldw	%r1,mf_a+pf_man+2(%r6)
	lda	%r5,.+6
	ldw	%pc,#__umul		; R1R0 = R3R2 * R1; preserves R3R2
	ldw	%r2,mf_pman+4(%r6)
	add	%r0,%r0,%r2
	stw	%r0,mf_pman+4(%r6)

	; compute exponent as sum with 127 offset
	;  sum = a + b - 127
	ldw	%r0,mf_a+pf_exp(%r6)
	ldw	%r1,mf_b+pf_exp(%r6)
	add	%r0,%r0,%r1
	ldw	%r1,mf_expoffs
	add	%r0,%r0,%r1

	; the product mantissa begins with mf_pman+4 >= 0x4000
	; shift so 0x0080 <= mf_pman+4 <= 0xFF
	ldw	%r4,mf_pman+2(%r6)
	ldw	%r5,mf_pman+4(%r6)
	ldw	%r1,mf_FF_b
mf_pmanshift:
	lsr	%r5,%r5
	ror	%r4,%r4
	inc	%r0,%r0
	cmp	%r5,%r1
	bhi	mf_pmanshift
	cmp	%r0,%r1			; check for overflow
	bge	mf_retinf

	; R5..R4 = value 00pp pppp
	; R0 = exponent
	tst	%r0
	bgt	mf_expgood

	; R0 = 0: shift R5..R4 right one
	;     -1: shift R5..R4 right two
	;     -2: shift R5..R4 right three
	;         ...
	neg	%r0,%r0
	inc	%r0,%r0
	ldw	%r1,mf_23
	cmp	%r0,%r1
	bgt	mf_retzero
	lda	%r1,16-23(%r1)
	cmp	%r0,%r1
	blo	mf_shiftbits
	mov	%r4,%r5
	clr	%r5
	sub	%r0,%r0,%r1
mf_shiftbits:
	neg	%r0,%r0
	beq	mf_combine
mf_shiftbit:
	lsr	%r5,%r5
	ror	%r4,%r4
	inc	%r0,%r0
	bne	mf_shiftbit
	br	mf_combine

mf_expoffs:.word -127-7
mf_FF_b: .word	0xFF
mf_23:	.word	23
mf_15:	.word	15
mf_7F80: .word	0x7F80

mf_retinf:
	ldw	%r5,mf_7F80
	br	mf_lowzs
mf_retzero:
	clr	%r5
mf_lowzs:
	clr	%r4
	br	mf_insign

mf_x80:	.word	0x80

mf_expgood:
	ldw	%r1,mf_x80
	sub	%r5,%r5,%r1		; clear hidden 1 bit

	; combine result components
mf_combine:
	shl	%r0,%r0			; insert exponent into pman<62:52>
	shl	%r0,%r0
	shl	%r0,%r0
	shl	%r0,%r0
	shl	%r0,%r0
	shl	%r0,%r0
	shl	%r0,%r0
	or	%r5,%r5,%r0
mf_insign:
	ldbs	%r0,mf_a+pf_neg(%r6)	; shift sign bit from aneg^bneg into pman<31>
	ldbs	%r1,mf_b+pf_neg(%r6)
	xor	%r0,%r0,%r1
	shl	%r5,%r5
	shl	%r0,%r0
	ror	%r5,%r5

	; store result
mf_retcompos:
	ldw	%r0,mf_sptr(%r6)
	stw	%r4,0(%r0)
	stw	%r5,2(%r0)

	ldw	%r3,mf_radr(%r6)
	ldw	%r4,mf_sav4(%r6)
	ldw	%r5,mf_sav5(%r6)
	lda	%r6,mf_size(%r6)
	lda	%pc,0(%r3)



	df_sptr = -64	;02
	df_aptr = -62	;02
	df_bptr = -60	;02
	df_radr = -58	;02
	df_a    = -56	;08
	df_b    = -48	;08
	df_sav4 = -40	;02
	df_sav5 = -38	;02
	df_atop = -36	;01
	df_btop = -35	;01
	df_qexp = -34	;02

	df_size = -32+64

	.global	__div_fff
__div_fff:
	lda	%r6,-df_size(%r6)
	stw	%r0,df_sptr(%r6)
	stw	%r1,df_aptr(%r6)
	stw	%r2,df_bptr(%r6)
	stw	%r3,df_radr(%r6)
	stw	%r4,df_sav4(%r6)
	stw	%r5,df_sav5(%r6)

	; chop up A operand
	lda	%r2,df_a(%r6)
	lda	%r3,df_achopped
	ldw	%pc,#parsefloat
df_achopped:
	bcs	df_retnan

	; chop up B operand
	ldw	%r1,df_bptr(%r6)
	lda	%r2,df_b(%r6)
	lda	%r3,df_bchopped
	ldw	%pc,#parsefloat
df_bchopped:
	bcc	df_bothnums

	; at least one operand isn't a number, return not-a-number
df_retnan:
	ldw	%r4,df_FFFF
	mov	%r5,%r4
	br	df_retcompos
df_FFFF: .word	0xFFFF
df_FF_b: .word	0xFF

df_aisinf:
	cmp	%r2,%r0
	beq	df_retnan
	br	df_retinf

	; both are numbers, check infinity cases
df_bothnums:
	ldw	%r0,df_FF_b
	ldw	%r1,df_a+pf_exp(%r6)
	ldw	%r2,df_b+pf_exp(%r6)
	cmp	%r1,%r0
	beq	df_aisinf
	cmp	%r2,%r0
	beq	df_retzero

	; make sure bman >= aman by shifting bman left and adjusting bexp
	lda	%r1,df_b(%r6)
	lda	%r3,df_bcounted
	br	findtopmanbitf
df_bcounted:
	stb	%r0,df_btop(%r6)
	lda	%r1,df_a(%r6)
	lda	%r3,df_acounted
	br	findtopmanbitf
df_acounted:
	stb	%r0,df_atop(%r6)
	mov	%r1,%r0
	bmi	df_aiszero
	ldbs	%r0,df_btop(%r6)	; A is nonzero, see if B is zero
	tst	%r0
	bmi	df_retinf		; A is nonzero, B is zero, so return infinity
	sub	%r1,%r1,%r0		; R0 = number of bits to shift bman left to match
	ble	df_bmangtaman		; if negative, bman is already >= aman
	ldw	%r0,df_b+pf_exp(%r6)	; update bexp
	sub	%r0,%r0,%r1
	stw	%r0,df_b+pf_exp(%r6)
	lda	%r0,df_b+pf_man(%r6)	; shift bman left
	lda	%r3,df_bmangtaman
	br	shiftlongleft
df_1:	.word	1
df_expoffs: .word 127+24
df_bmangtaman:

	; compute exponent
	;  sum = a - b + 127 would be the point between bits <23> and <22>
	;  but we have to go through the divide loop 24 times to get top bit there
	ldw	%r0,df_a+pf_exp(%r6)
	ldw	%r1,df_b+pf_exp(%r6)
	sub	%r0,%r0,%r1
	ldw	%r1,df_expoffs
	add	%r0,%r0,%r1
	ble	df_retzero
	stw	%r0,df_qexp(%r6)

	; divide (knowing bman >= aman)
	;     001a aaaa  df_a+pf_man
	;  /  001b bbbb  df_b+pf_man
	;     qqqq qqqq  df_qman (R5..R4)
	;       +2   +0
	; keep looping until qman<23> sets (normal number)
	; ...or until exponent decrements to zero (subnormal)
	ldw	%r2,df_a+pf_man+0(%r6)		; R3..R2 = aman
	ldw	%r3,df_a+pf_man+2(%r6)
	clr	%r4				; R5..R4 = qman = 0
	clr	%r5
df_divloop:
	shl	%r4,%r4				; qman = qman << 1
	rol	%r5,%r5
	ldw	%r0,df_b+pf_man+0(%r6)		; R1..R0 = aman - bman
	ldw	%r1,df_b+pf_man+2(%r6)
	sub	%r0,%r2,%r0
	sbb	%r1,%r3,%r1
	blo	df_divskip
	mov	%r2,%r0				; if (R1..R0 >= 0) aman = aman - bman
	mov	%r3,%r1
	inc	%r4,%r4				; ...and set qman<00>
df_divskip:
	shl	%r2,%r2				; aman = aman << 1
	rol	%r3,%r3
	ldw	%r0,df_qexp(%r6)		; qman just shifted left so decrement exponent
	lda	%r0,-1(%r0)
	tst	%r0				; stop if subnormal result
	beq	df_subnormal
	stw	%r0,df_qexp(%r6)
	ldw	%r1,df_x80			; repeat until qman<23> set
	cmp	%r5,%r1
	blt	df_divloop
	ldw	%r2,df_FF			; done, see if exponent too big
	cmp	%r0,%r2
	bge	df_retinf			; return infinity if so
	sub	%r5,%r5,%r1			; in range, clear hidden 1 qman<23>
	br	df_qshifted
df_16:	.word	16
df_FF:	.word	0xFF
df_x80:	.word	0x80

	; quotient exponent gt FE, return INF
df_retinf:
	ldw	%r5,df_7F80
	br	df_lowzs
df_7F80: .word	0x7F80

	; A is zero
	; if B is also zero, return NAN
	;         otherwise, return zero
df_aiszero:
	ldbs	%r0,df_btop(%r6)
	tst	%r0
	bmi	df_retnan

	; quotient exponent lt 000, return 0
df_retzero:
	clr	%r5
df_lowzs:
	clr	%r4
	br	df_insign

	; subnormal (exponent zero), mantissa needs to be shifted right one bit
df_subnormal:
	lsr	%r5,%r5
	ror	%r4,%r4

	; mantissa shifted in place, combine result components
	;  R5..R4 = mantissa (hidden bit stripped)
	;  R0 = exponent
	;  df_aneg^df_bneg = sign
df_qshifted:
	shl	%r0,%r0			; insert exponent into qman<30:23>
	shl	%r0,%r0
	shl	%r0,%r0
	shl	%r0,%r0
	shl	%r0,%r0
	shl	%r0,%r0
	shl	%r0,%r0
	or	%r5,%r5,%r0
df_insign:
	ldbs	%r0,df_a+pf_neg(%r6)	; shift sign bit from aneg^bneg into qman<31>
	ldbs	%r1,df_b+pf_neg(%r6)
	xor	%r0,%r0,%r1
	shl	%r5,%r5
	shl	%r0,%r0
	ror	%r5,%r5

	; store result
df_retcompos:
	ldw	%r0,df_sptr(%r6)
	stw	%r4,0(%r0)
	stw	%r5,2(%r0)

	ldw	%r3,df_radr(%r6)
	ldw	%r4,df_sav4(%r6)
	ldw	%r5,df_sav5(%r6)
	lda	%r6,df_size(%r6)
	lda	%pc,0(%r3)



;
; find top bit set in mantissa
;  input:
;   R1 = points to 32-bit mantissa
;   R3 = return address
;  output:
;   returns R0 = -1: mantissa is zero
;              else: bit number 0..31
;
findtopmanbitf:
	ldw	%r0,ftbf_31
	ldw	%r2,2(%r1)
	tst	%r2
	bne	ftbf_loop
	lda	%r0,-16(%r0)
	ldw	%r2,0(%r1)
	tst	%r2
	bne	ftbf_loop
	lda	%r0,-16(%r0)
ftbf_done:
	lda	%pc,0(%r3)
ftbf_loop:
	shl	%r2,%r2
	bcs	ftbf_done
	lda	%r0,-1(%r0)
	br	ftbf_loop
ftbf_31: .word	31

;
; shift longword left
;  input:
;   R0 = points to long
;   R1 = shift count 0..31
;   R3 = return address
;  output:
;   long shifted and stored
;   R5..R4 = shifted long
;
shiftlongleft:
	ldw	%r4,0(%r0)
	ldw	%r5,2(%r0)
	lda	%r2,-16(%r1)
	tst	%r2
	bmi	sll_shift0
	mov	%r5,%r4
	clr	%r4
	mov	%r1,%r2
sll_shift0:
	neg	%r1,%r1
	beq	sll_done
sll_shift:
	shl	%r4,%r4
	rol	%r5,%r5
	inc	%r1,%r1
	bne	sll_shift
sll_done:
	stw	%r4,0(%r0)
	stw	%r5,2(%r0)
	lda	%pc,0(%r3)


;
; floatingpoint comparison
;  input:
;   R0 = condition code
;     <5:4> = 0: left >= rite
;             1: left >  rite
;     	      2: left != rite
;       <3> = 0: as above
;             1: opposite
;   R1 = operand address
;   R2 = operand address
;   R3 = return address
;  output:
;   R0 = result code (0 or 1)
;   R4,R5 = same as on input
;   R1,R2,R3 = trashed
;
	cf_sav0 = -64
	cf_sav1 = -62
	cf_sav2 = -60
	cf_radr = -58
	cf_sav4 = -56
	cf_sav5 = -54
	cf_a    = -52
	cf_b    = -44
	cf_size = -36+64

	.align	2
	.global	__cmp_ff
__cmp_ff:
	lda	%r6,-cf_size(%r6)
	stw	%r0,cf_sav0(%r6)
	stw	%r1,cf_sav1(%r6)
	stw	%r2,cf_sav2(%r6)
	stw	%r3,cf_radr(%r6)
	stw	%r4,cf_sav4(%r6)
	stw	%r5,cf_sav5(%r6)

	; chop up A operand
	lda	%r2,cf_a(%r6)
	lda	%r3,cf_achopped
	ldw	%pc,#parsefloat
cf_achopped:
	bcs	cf_gotanan

	; chop up B operand
	ldw	%r1,cf_sav2(%r6)
	lda	%r2,cf_b(%r6)
	lda	%r3,cf_bchopped
	ldw	%pc,#parsefloat
cf_bchopped:
	bcs	cf_gotanan

	; 2s complement with sign bit flipped
	lda	%r1,cf_a(%r6)
	lda	%r3,cf_aflipped
	br	cmpfflip
cf_aflipped:
	lda	%r1,cf_b(%r6)
	lda	%r3,cf_bflipped
	br	cmpfflip
cf_bflipped:

	; compare
	clr	%r0			; assume false result
	ldw	%r1,cf_sav0(%r6)	; get compare code
	lsr	%r1,%r1
	lsr	%r1,%r1
	lsr	%r2,%r1			; save flip bit in R2
	lsr	%r1,%r2			; R1=0:GE; 1:GT; 2:NE

	ldw	%r4,cf_a+pf_exp(%r6)	; compare
	ldw	%r5,cf_b+pf_exp(%r6)
	cmp	%r4,%r5
	bne	cf_noteql
	ldw	%r4,cf_a+pf_man+2(%r6)
	ldw	%r5,cf_b+pf_man+2(%r6)
	cmp	%r4,%r5
	bne	cf_noteql
	ldw	%r4,cf_a+pf_man+0(%r6)
	ldw	%r5,cf_b+pf_man+0(%r6)
	cmp	%r4,%r5
	beq	cf_equal		; eq, only code 0 (GE) is true
cf_noteql:
	bhi	cf_greater		; gt, all codes (GE,GT,NE) are true
	lda	%r1,-2(%r1)		; lt, only code 2 (NE) is true
cf_equal:
	tst	%r1
	bne	cf_false
cf_greater:
	inc	%r0,%r0
cf_false:
	ldw	%r1,cf_1		; maybe flip true/false bit
	and	%r2,%r2,%r1
	xor	%r0,%r0,%r2
cf_return:
	ldw	%r3,cf_radr(%r6)
	ldw	%r4,cf_sav4(%r6)
	ldw	%r5,cf_sav5(%r6)
	lda	%r6,cf_size(%r6)
	lda	%pc,0(%r3)

cf_1:	.word	1

cf_gotanan:
	clr	%r0			; nans are always false
	br	cf_return

;
; if operand negative, take 2s complement of exponent:mantissa (so neg zero == pos zero)
; always flip exponent sign bit an extra time so unsigned compare works
;  input:
;   R1 = points to parsed value
;   R3 = return address
;  scratch:
;   R0,R2,R4
;
cmpfflip:
	ldw	%r2,pf_exp(%r1)
	ldw	%r0,cff_x8000
	xor	%r2,%r2,%r0
	ldbs	%r0,pf_neg(%r1)
	tst	%r0
	beq	cff_ispos
	clr	%r4
	ldw	%r0,pf_man+0(%r1)
	sub	%r0,%r4,%r0
	stw	%r0,pf_man+0(%r1)
	ldw	%r0,pf_man+2(%r1)
	sbb	%r0,%r4,%r0
	stw	%r0,pf_man+2(%r1)
	sbb	%r2,%r4,%r2
cff_ispos:
	stw	%r2,pf_exp(%r1)
	lda	%pc,0(%r3)

cff_x8000: .word	0x8000



;
; negation
;  input:
;   R0 = output pointer
;   R1 = input pointer
;
	.global	__neg_ff
__neg_ff:
	ldw	%r2,0(%r1)
	stw	%r2,0(%r0)
	ldw	%r2,2(%r1)
	ldw	%r1,negf_x8000
	xor	%r2,%r2,%r1
	stw	%r2,2(%r0)
	lda	%pc,0(%r3)
negf_x8000: .word 0x8000



;;;;;;;;;;;;;;;;;;;;;;;;
;;  DOUBLE PRECISION  ;;
;;;;;;;;;;;;;;;;;;;;;;;;

	; <s><exp><man>
	;  s = 0: positive
	;      1: negative
	;  exp = 11 bits
	;  man = 52 bits

	sd_radr = -64
	sd_bval = -62
	sd_size =  10

sd_8000: .word 0x8000
sd_return:
	ldw	%r3,sd_radr(%r6)
	lda	%r6,sd_size(%r6)
	lda	%pc,0(%r3)

	.global	__sub_ddd
__sub_ddd:
	lda	%r6,-sd_size(%r6)
	stw	%r3,sd_radr(%r6)
	ldw	%r3,0(%r2)
	stw	%r3,sd_bval+0(%r6)
	ldw	%r3,2(%r2)
	stw	%r3,sd_bval+2(%r6)
	ldw	%r3,4(%r2)
	stw	%r3,sd_bval+4(%r6)
	ldw	%r3,6(%r2)
	ldw	%r2,sd_8000
	xor	%r3,%r3,%r2
	stw	%r3,sd_bval+6(%r6)
	lda	%r2,sd_bval(%r6)
	lda	%r3,sd_return
	;br	__add_ddd

	ad_sptr = -64
	ad_aptr = -62
	ad_bptr = -60
	ad_radr = -58
	ad_a    = -56
	ad_b    = -44
	ad_sav4 = -32
	ad_sav5 = -30
	ad_size = -28+64

	.global	__add_ddd
__add_ddd:
	lda	%r6,-ad_size(%r6)
	stw	%r0,ad_sptr(%r6)
	stw	%r1,ad_aptr(%r6)
	stw	%r2,ad_bptr(%r6)
	stw	%r3,ad_radr(%r6)
	stw	%r4,ad_sav4(%r6)
	stw	%r5,ad_sav5(%r6)

	; chop up A operand
	lda	%r2,ad_a(%r6)
	lda	%r3,ad_achopped
	br	parsedouble
ad_achopped:
	bcs	ad_retnan

	; chop up B operand
	ldw	%r1,ad_bptr(%r6)
	lda	%r2,ad_b(%r6)
	lda	%r3,ad_bchopped
	br	parsedouble
ad_bchopped:
	bcc	ad_bothnums

	; at least one operand isn't a number, so return not-a-number
ad_retnan:
	ldw	%r2,ad_FFFF
	mov	%r3,%r2
	mov	%r4,%r2
	mov	%r5,%r2
	br	ad_retcompos
ad_FFFF: .word	0xFFFF
ad_52_b: .word	52
ad_7FF: .word	0x7FF

	; both are infinity
	; if opposite signs, return nan, else return infinity
ad_bothinf:
	ldw	%r0,ad_a+pd_neg(%r6)
	ldw	%r1,ad_b+pd_neg(%r6)
	cmp	%r0,%r1
	bne	ad_retnan
	br	ad_returna

	; both are numbers, check inf-inf case
ad_bothnums:
	ldw	%r2,ad_a+pd_exp(%r6)	; abig = aexp - bexp
	ldw	%r1,ad_b+pd_exp(%r6)
	ldw	%r0,ad_7FF
	and	%r3,%r2,%r1
	cmp	%r3,%r0
	beq	ad_bothinf

	; shift aman or bman right to align exponents
	sub	%r0,%r2,%r1		; abig = aexp - bexp
	ble	ad_anotbig		; if (abig <= 0) goto anotbig
	ldw	%r1,ad_52_b		; if (abig > 52) goto returna
	cmp	%r0,%r1
	bgt	ad_returna
	stw	%r2,ad_b+pd_exp(%r6)	; bexp = aexp
	ldw	%r2,ad_b+pd_man+0(%r6)	; R5..R2 = bman
	ldw	%r3,ad_b+pd_man+2(%r6)
	ldw	%r4,ad_b+pd_man+4(%r6)
	ldw	%r5,ad_b+pd_man+6(%r6)
	lda	%r1,48-52(%r1)		; R5..R2 >>= abig
	and	%r1,%r1,%r0
	lsr	%r1,%r1
	sub	%r1,%pc,%r1
	lda	%pc,ad_bshift0-.(%r1)
ad_bshift3:
	mov	%r2,%r5
	clr	%r3
	clr	%r4
	br	ad_bshclrr5
ad_bshift2:
	mov	%r2,%r4
	mov	%r3,%r5
	clr	%r4
	br	ad_bshclrr5
ad_bshift1:
	mov	%r2,%r3
	mov	%r3,%r4
	mov	%r4,%r5
ad_bshclrr5:
	clr	%r5
ad_bshift0:
	ldw	%r1,ad_15_b
	and	%r0,%r0,%r1
	beq	ad_bshiftdone
	neg	%r0,%r0
ad_bshiftbit:
	lsr	%r5,%r5
	ror	%r4,%r4
	ror	%r3,%r3
	ror	%r2,%r2
	inc	%r0,%r0
	bne	ad_bshiftbit
ad_bshiftdone:
	stw	%r2,ad_b+pd_man+0(%r6)
	stw	%r3,ad_b+pd_man+2(%r6)
	stw	%r4,ad_b+pd_man+4(%r6)
	stw	%r5,ad_b+pd_man+6(%r6)
	br	ad_aligned
ad_15_b: .word	15
ad_52:	.word	52
ad_returna:
	ldw	%r0,ad_aptr(%r6)
	br	ad_returnab

ad_anotbig:
	ldw	%r2,ad_b+pd_exp(%r6)	; bbig = bexp - aexp
	ldw	%r1,ad_a+pd_exp(%r6)
	sub	%r0,%r2,%r1
	ble	ad_aligned		; if (bbig <= 0) goto aligned
	ldw	%r1,ad_52		; if (bbig > 52) goto returnb
	cmp	%r0,%r1
	bgt	ad_returnb
	stw	%r2,ad_a+pd_exp(%r6)	; aexp = bexp
	ldw	%r2,ad_a+pd_man+0(%r6)	; R5..R2 = aman
	ldw	%r3,ad_a+pd_man+2(%r6)
	ldw	%r4,ad_a+pd_man+4(%r6)
	ldw	%r5,ad_a+pd_man+6(%r6)
	lda	%r1,48-52(%r1)		; R5..R2 >>= bbig
	and	%r1,%r1,%r0
	lsr	%r1,%r1
	sub	%r1,%pc,%r1
	lda	%pc,ad_ashift0-.(%r1)
ad_ashift3:
	mov	%r2,%r5
	clr	%r3
	clr	%r4
	br	ad_ashclrr5
ad_ashift2:
	mov	%r2,%r4
	mov	%r3,%r5
	clr	%r4
	br	ad_ashclrr5
ad_ashift1:
	mov	%r2,%r3
	mov	%r3,%r4
	mov	%r4,%r5
ad_ashclrr5:
	clr	%r5
ad_ashift0:
	ldw	%r1,ad_15_c
	and	%r0,%r0,%r1
	beq	ad_ashiftdone
	neg	%r0,%r0
ad_ashiftbit:
	lsr	%r5,%r5
	ror	%r4,%r4
	ror	%r3,%r3
	ror	%r2,%r2
	inc	%r0,%r0
	bne	ad_ashiftbit
ad_ashiftdone:
	stw	%r2,ad_a+pd_man+0(%r6)
	stw	%r3,ad_a+pd_man+2(%r6)
	stw	%r4,ad_a+pd_man+4(%r6)
	stw	%r5,ad_a+pd_man+6(%r6)
	br	ad_aligned
ad_15_c: .word	15
ad_returnb:
	ldw	%r0,ad_bptr(%r6)
ad_returnab:
	ldw	%r2,0(%r0)
	ldw	%r3,2(%r0)
	ldw	%r4,4(%r0)
	ldw	%r5,6(%r0)
	br	ad_retcompos
ad_aligned:

	; convert bman to 2s complement
	clr	%r0
	ldbu	%r1,ad_b+pd_neg(%r6)
	tst	%r1
	beq	ad_bispos
	ldw	%r2,ad_b+pd_man+0(%r6)	; R5..R2 = bman
	ldw	%r3,ad_b+pd_man+2(%r6)
	ldw	%r4,ad_b+pd_man+4(%r6)
	ldw	%r5,ad_b+pd_man+6(%r6)
	sub	%r2,%r0,%r2		; R5..R2 = 0 - R5..R2
	sbb	%r3,%r0,%r3
	sbb	%r4,%r0,%r4
	sbb	%r5,%r0,%r5
	stw	%r2,ad_b+pd_man+0(%r6)	; bman = R5..R2
	stw	%r3,ad_b+pd_man+2(%r6)
	stw	%r4,ad_b+pd_man+4(%r6)
	stw	%r5,ad_b+pd_man+6(%r6)
ad_bispos:

	; convert aman to 2s complement
	ldw	%r2,ad_a+pd_man+0(%r6)	; R5..R2 = aman
	ldw	%r3,ad_a+pd_man+2(%r6)
	ldw	%r4,ad_a+pd_man+4(%r6)
	ldw	%r5,ad_a+pd_man+6(%r6)
	ldbu	%r1,ad_a+pd_neg(%r6)
	tst	%r1
	beq	ad_aispos
	sub	%r2,%r0,%r2		; R5..R2 = 0 - R5..R2
	sbb	%r3,%r0,%r3
	sbb	%r4,%r0,%r4
	sbb	%r5,%r0,%r5
ad_aispos:

	; perform addition
	ldw	%r0,ad_b+pd_man+0(%r6)	; R5..R2 += bman
	ldw	%r1,ad_b+pd_man+2(%r6)
	add	%r2,%r2,%r0
	adc	%r3,%r3,%r1
	ldw	%r0,ad_b+pd_man+4(%r6)
	ldw	%r1,ad_b+pd_man+6(%r6)
	adc	%r4,%r4,%r0
	adc	%r5,%r5,%r1

	; convert sum to sign/magnitude
	clr	%r0
	shl	%r1,%r5			; aneg = aman < 0
	sbb	%r1,%r1,%r1
	stb	%r1,ad_a+pd_neg(%r6)
	beq	ad_sumispos		; if (aneg) aman = 0 - aman
	sub	%r2,%r0,%r2		; R5..R2 = 0 - R5..R2
	sbb	%r3,%r0,%r3
	sbb	%r4,%r0,%r4
	sbb	%r5,%r0,%r5
ad_sumispos:

	; normalize result
	ldw	%r1,ad_a+pd_exp(%r6)
	ldw	%r0,ad_20		; if (aman < 0x20000000000000) goto norm1;
	cmp	%r5,%r0
	ldw	%r0,ad_7FF_b
	blt	ad_norm1
	cmp	%r1,%r0			; if (aexp >= 0x7FF) goto norm2;
	bge	ad_norm2
	inc	%r1,%r1			; aexp ++;
	lsr	%r5,%r5			; aman >>= 1;
	ror	%r4,%r4
	ror	%r3,%r3
	ror	%r2,%r2
ad_norm1:
	cmp	%r1,%r0
ad_norm2:
	blt	ad_norm3		; if (aexp < 0x7FF) goto norm3;
	mov	%r1,%r0			; aexp = 0x7FF
	clr	%r2			; aman = 0
	clr	%r3
	clr	%r4
	clr	%r5
	br	ad_normd
ad_7FF_b: .word 0x7FF
ad_20:	.word	0x20
ad_10:	.word	0x10
ad_norm3:
	ldw	%r0,ad_10		; if (aman >= 0x10000000000000) goto normd
ad_norm4:
	cmp	%r5,%r0
	bge	ad_normd
	tst	%r1			; if (aexp <= 0) goto subnorm
	beq	ad_subnorm
	shl	%r2,%r2			; aman <<= 1
	rol	%r3,%r3
	rol	%r4,%r4
	rol	%r5,%r5
	lda	%r1,-1(%r1)		; -- aexp
	br	ad_norm4
ad_normd:

	; combine result components
	tst	%r1
	beq	ad_subnorm
	lda	%r5,-16(%r5)		; clear hidden 1 bit
	shl	%r1,%r1			; aman |= aexp << 52
	shl	%r1,%r1
	shl	%r1,%r1
	shl	%r1,%r1
	or	%r5,%r5,%r1
	br	ad_getsign
ad_subnorm:
	lsr	%r5,%r5			; aman >>= 1;
	ror	%r4,%r4
	ror	%r3,%r3
	ror	%r2,%r2
ad_getsign:
	ldbs	%r0,ad_a+pd_neg(%r6)	; shift sign bit from aneg into aman
	shl	%r5,%r5
	shl	%r0,%r0
	ror	%r5,%r5

	; store result
ad_retcompos:
	ldw	%r0,ad_sptr(%r6)
	stw	%r2,0(%r0)
	stw	%r3,2(%r0)
	stw	%r4,4(%r0)
	stw	%r5,6(%r0)

	ldw	%r3,ad_radr(%r6)
	ldw	%r4,ad_sav4(%r6)
	ldw	%r5,ad_sav5(%r6)
	lda	%r6,ad_size(%r6)
	lda	%pc,0(%r3)


;
; parse double into parts
;  input:
;   R1 = pointer to composite double
;   R2 = output pointer
;   R3 = return address
;  output:
;   c-bit set: input is not a number
;   else: pd_man(R2) = 64-bit mantissa (with hidden 1 bit filled in, unless subnormal)
;         pd_exp(R2) = 16-bit exponent
;         pd_neg(R2) = 0000: value is positive; FFFF: value is negative
;         z-bit set: +- infinity
;  scratch:
;   R0,R1,R4,R5
;
	pd_man =  0
	pd_exp =  8
	pd_neg = 10

pd_15:	.word	15
pd_7FF: .word	0x7FF

parsedouble:

	; __int64_t aman = *aptr & 0xFFFFFFFFFFFFF;
	ldw	%r4,0(%r1)
	stw	%r4,pd_man+0(%r2)
	ldw	%r4,2(%r1)
	stw	%r4,pd_man+2(%r2)
	ldw	%r4,4(%r1)
	stw	%r4,pd_man+4(%r2)
	ldw	%r4,6(%r1)
	ldw	%r0,pd_15
	and	%r0,%r0,%r4
	stw	%r0,pd_man+6(%r2)

	; __int16_t aneg = aman < 0;
	shl	%r5,%r4
	sbb	%r5,%r5,%r5
	stw	%r5,pd_neg(%r2)

	; __int16_t aexp = (aman >> 52) & 0x7FF;
	asr	%r5,%r4
	asr	%r5,%r5
	asr	%r5,%r5
	asr	%r5,%r5
	ldw	%r0,pd_7FF
	and	%r5,%r5,%r0
	stw	%r5,pd_exp(%r2)

	; if isnan (a) goto retnan
	cmp	%r5,%r0
	bne	pd_isanum
	ldw	%r0,pd_man+0(%r2)
	ldw	%r1,pd_man+2(%r2)
	or	%r1,%r1,%r0
	ldw	%r1,pd_man+4(%r2)
	or	%r1,%r1,%r0
	ldw	%r1,pd_man+6(%r2)
	or	%r1,%r1,%r0
	bne	pd_retnan
pd_isanum:

	; insert hidden 1 bit or fix exponent
	; if (exp > 0) man |= 0x10000000000000; else exp = 1
	tst	%r5
	beq	pd_subnor
	ldw	%r0,pd_man+6(%r2)	; set hidden 1 bit
	lda	%r0,16(%r0)
	stw	%r0,pd_man+6(%r2)
	cmp	%r0,%r0			; clear carry
	lda	%pc,0(%r3)
pd_subnor:
	inc	%r5,%r5			; subnormal, set exponent = 1
	stw	%r5,pd_exp(%r2)
	cmp	%r0,%r0			; clear carry
	lda	%pc,0(%r3)

	; input is not-a-number
pd_retnan:
	lsr	%r5,%r5			; R5 contained 0x7FF, so this sets carry
	lda	%pc,0(%r3)



	md_sptr = -64	;02
	md_aptr = -62	;02
	md_bptr = -60	;02
	md_radr = -58	;02
	md_a    = -56	;12
	md_b    = -44	;12
	md_sav4 = -32	;02
	md_sav5 = -30	;02
	md_pman = -28	;16
	md_aofs = -12	;01
	md_bofs = -11	;01

	md_size = -10+64

	.global	__mul_ddd
__mul_ddd:
	lda	%r6,-md_size(%r6)
	stw	%r0,md_sptr(%r6)
	stw	%r1,md_aptr(%r6)
	stw	%r2,md_bptr(%r6)
	stw	%r3,md_radr(%r6)
	stw	%r4,md_sav4(%r6)
	stw	%r5,md_sav5(%r6)

	; chop up A operand
	lda	%r2,md_a(%r6)
	lda	%r3,md_achopped
	br	parsedouble
md_achopped:
	bcs	md_retnan

	; chop up B operand
	ldw	%r1,md_bptr(%r6)
	lda	%r2,md_b(%r6)
	lda	%r3,md_bchopped
	br	parsedouble
md_bchopped:
	bcc	md_bothnums

	; at least one operand isn't a number, return not-a-number
md_retnan:
	ldw	%r2,md_FFFF
	mov	%r3,%r2
	mov	%r4,%r2
	mov	%r5,%r2
	br	md_retcompos
md_FFFF: .word	0xFFFF
md_7FF: .word	0x7FF

	; one operand is infinity, if other is zero, return nan, else return inf
md_haveinf:
	lda	%r3,.+6
	ldw	%pc,#findtopmanbitd
	tst	%r0
	bmi	md_retnan
	br	md_retinf

	; both are numbers, check infinity cases
md_bothnums:
	ldw	%r0,md_7FF
	lda	%r1,md_b+pd_man(%r6)
	ldw	%r2,md_a+pd_exp(%r6)
	cmp	%r2,%r0
	beq	md_haveinf
	lda	%r1,md_a+pd_man(%r6)
	ldw	%r2,md_b+pd_exp(%r6)
	cmp	%r2,%r0
	beq	md_haveinf

	; prescale subnormals to put top bit in hidden position man<52>
	lda	%r1,md_b+pd_man(%r6)
	lda	%r3,md_bcounted
	ldw	%pc,#findtopmanbitd
md_bcounted:
	tst	%r0
	bmi	md_retzero
	ldw	%r1,md_52_b
	sub	%r1,%r1,%r0
	ble	md_binplace
	ldw	%r0,md_b+pd_exp(%r6)
	sub	%r0,%r0,%r1
	stw	%r0,md_b+pd_exp(%r6)
	lda	%r0,md_b+pd_man(%r6)
	lda	%r3,md_binplace
	ldw	%pc,#shiftquadleft
md_52_b: .word	52
md_binplace:
	lda	%r1,md_a+pd_man(%r6)
	lda	%r3,md_acounted
	ldw	%pc,#findtopmanbitd
md_acounted:
	tst	%r0
	bmi	md_retzero
	ldw	%r1,md_52_b
	sub	%r1,%r1,%r0
	ble	md_ainplace
	ldw	%r0,md_a+pd_exp(%r6)
	sub	%r0,%r0,%r1
	stw	%r0,md_a+pd_exp(%r6)
	lda	%r0,md_a+pd_man(%r6)
	lda	%r3,md_ainplace
	ldw	%pc,#shiftquadleft
md_ainplace:

	; multiply
	;                      001a aaaa aaaa aaaa  md_a+pd_man
	;                   x  001b bbbb bbbb bbbb  md_b+pd_man
	;  0000 03pp pppp pppp pppp pppp xxxx xxxx  md_pman (xxxx = don't care/not used)
	;   +14  +12  +10   +8   +6   +4   +2   +0
	clr	%r3
	stw	%r3,md_pman+ 4(%r6)
	stw	%r3,md_pman+ 6(%r6)
	stw	%r3,md_pman+ 8(%r6)
	stw	%r3,md_pman+10(%r6)
	stw	%r3,md_pman+12(%r6)
	stw	%r3,md_pman+14(%r6)
	clr	%r2			; bofs = 0
md_mulbloop:
	stb	%r2,md_bofs(%r6)
	mov	%r5,%r2
	add	%r2,%r2,%r6
	ldw	%r2,md_b+pd_man(%r2)	; R2 = bman[bofs]
	tst	%r2			; if zero, go on to next bofs
	beq	md_mulbnext
	ldw	%r1,md_4		; don't compute pman words 0,2
	sub	%r1,%r1,%r5		; ...cuz we never use them
	bhis	md_mulaloop		; ...pman word 4 needed for precision
	clr	%r1			; aofs = 0
md_mulaloop:
	stb	%r1,md_aofs(%r6)
	add	%r1,%r1,%r6
	ldw	%r1,md_a+pd_man(%r1)	; R1 = aman[aofs]
	tst	%r1			; if zero, go on to next aofs
	beq	md_mulanext
	lda	%r5,md_umulret
	ldw	%pc,#__umul		; R1R0 = R3R2 * R1; preserves R3R2
md_4:	.word	4
md_8:	.word	8
md_expoffs: .word -1023
md_umulret:
	ldbu	%r5,md_aofs(%r6)	; R5 = pofs = aofs+bofs
	ldbu	%r4,md_bofs(%r6)	; ...should always be at least 4
	add	%r5,%r5,%r4
	add	%r5,%r5,%r6
	ldw	%r4,md_pman+0(%r5)	; pman[pofs+2:pofs+0] += R1R0
	add	%r4,%r0,%r4
	stw	%r4,md_pman+0(%r5)
	ldw	%r4,md_pman+2(%r5)
	adc	%r4,%r1,%r4
	stw	%r4,md_pman+2(%r5)
	bcc	md_mulanext
md_carries:
	ldw	%r0,md_pman+4(%r5)	; ripple carry
	inc	%r0,%r0
	stw	%r0,md_pman+4(%r5)
	lda	%r5,2(%r5)
	beq	md_carries		; top pman[] always zero so cannot go beyond that
md_mulanext:
	ldw	%r0,md_8		; increment aofs, loop if more to do
	ldbu	%r1,md_aofs(%r6)
	lda	%r1,2(%r1)
	cmp	%r1,%r0
	bne	md_mulaloop
md_mulbnext:
	ldw	%r0,md_8		; increment bofs, loop if more to do
	ldbu	%r2,md_bofs(%r6)
	lda	%r2,2(%r2)
	cmp	%r2,%r0
	bne	md_mulbloop

	; compute exponent as sum with 1023 offset
	;  sum = a + b - 1023
	ldw	%r0,md_a+pd_exp(%r6)
	ldw	%r1,md_b+pd_exp(%r6)
	add	%r0,%r0,%r1
	ldw	%r1,md_expoffs
	add	%r0,%r0,%r1

	; the product mantissa begins with md_pman+12=0nFF, 1 <= n <= 3
	; maybe shift right 1 bit so n = 1
	ldw	%r2,md_pman+ 6(%r6)
	ldw	%r3,md_pman+ 8(%r6)
	ldw	%r4,md_pman+10(%r6)
	ldw	%r5,md_pman+12(%r6)
	ldw	%r1,#0x200
	cmp	%r5,%r1
	blo	md_pmansmall
	lsr	%r5,%r5			; n was 2 or 3, shift one more bit right
	ror	%r4,%r4			;  so that R5 contains 001p
	ror	%r3,%r3
	ror	%r2,%r2
	lda	%r0,1(%r0)		; update exponent
md_pmansmall:
	ldw	%r1,#0x7FF		; check for overflow
	cmp	%r0,%r1
	bge	md_retinf
	lsr	%r5,%r5			; shift 01pp pppp pppp pppp
	ror	%r4,%r4			;    to 001p pppp pppp pppp
	ror	%r3,%r3
	ror	%r2,%r2
	lsr	%r5,%r5
	ror	%r4,%r4
	ror	%r3,%r3
	ror	%r2,%r2
	lsr	%r5,%r5
	ror	%r4,%r4
	ror	%r3,%r3
	ror	%r2,%r2
	lsr	%r5,%r5
	ror	%r4,%r4
	ror	%r3,%r3
	ror	%r2,%r2

	; R5..R2 = value 001p pppp pppp pppp
	; R0 = exponent
	tst	%r0
	bgt	md_expgood

	; R0 = 0: shift R5..R2 right one
	;     -1: shift R5..R2 right two
	;     -2: shift R5..R2 right three
	;         ...
	neg	%r0,%r0
	inc	%r0,%r0
	ldw	%r1,md_52
	cmp	%r0,%r1
	bgt	md_retzero
	lda	%r1,16-52(%r1)
	cmp	%r0,%r1
	blo	md_shiftbits
md_shiftword:
	mov	%r2,%r3
	mov	%r3,%r4
	mov	%r4,%r5
	clr	%r5
	sub	%r0,%r0,%r1
	cmp	%r0,%r1
	bhis	md_shiftword
md_shiftbits:
	neg	%r0,%r0
	beq	md_combine
md_shiftbit:
	lsr	%r5,%r5
	ror	%r4,%r4
	ror	%r3,%r3
	ror	%r2,%r2
	inc	%r0,%r0
	bne	md_shiftbit
	br	md_combine

md_52:	.word	52
md_15:	.word	15
md_7FF0: .word	0x7FF0

md_retinf:
	ldw	%r5,md_7FF0
	br	md_lowzs
md_retzero:
	clr	%r5
md_lowzs:
	clr	%r4
	clr	%r3
	clr	%r2
	br	md_insign

md_expgood:
	lda	%r5,-16(%r5)		; clear hidden 1 bit

	; combine result components
md_combine:
	shl	%r0,%r0			; insert exponent into pman<62:52>
	shl	%r0,%r0
	shl	%r0,%r0
	shl	%r0,%r0
	or	%r5,%r5,%r0
md_insign:
	ldbs	%r0,md_a+pd_neg(%r6)	; shift sign bit from aneg^bneg into pman<63>
	ldbs	%r1,md_b+pd_neg(%r6)
	xor	%r0,%r0,%r1
	shl	%r5,%r5
	shl	%r0,%r0
	ror	%r5,%r5

	; store result
md_retcompos:
	ldw	%r0,md_sptr(%r6)
	stw	%r2,0(%r0)
	stw	%r3,2(%r0)
	stw	%r4,4(%r0)
	stw	%r5,6(%r0)

	ldw	%r3,md_radr(%r6)
	ldw	%r4,md_sav4(%r6)
	ldw	%r5,md_sav5(%r6)
	lda	%r6,md_size(%r6)
	lda	%pc,0(%r3)



	dd_sptr = -64	;02
	dd_aptr = -62	;02
	dd_bptr = -60	;02
	dd_radr = -58	;02
	dd_a    = -56	;12
	dd_b    = -44	;12
	dd_sav4 = -32	;02
	dd_sav5 = -30	;02
	dd_qman = -28	;16
	dd_atop = -12	;01
	dd_btop = -11	;01
	dd_qexp = -10	;02

	dd_size =  -8+64

	.global	__div_ddd
__div_ddd:
	lda	%r6,-dd_size(%r6)
	stw	%r0,dd_sptr(%r6)
	stw	%r1,dd_aptr(%r6)
	stw	%r2,dd_bptr(%r6)
	stw	%r3,dd_radr(%r6)
	stw	%r4,dd_sav4(%r6)
	stw	%r5,dd_sav5(%r6)

	; chop up A operand
	lda	%r2,dd_a(%r6)
	lda	%r3,dd_achopped
	ldw	%pc,#parsedouble
dd_achopped:
	bcs	dd_retnan

	; chop up B operand
	ldw	%r1,dd_bptr(%r6)
	lda	%r2,dd_b(%r6)
	lda	%r3,dd_bchopped
	ldw	%pc,#parsedouble
dd_bchopped:
	bcc	dd_bothnums

	; at least one operand isn't a number, return not-a-number
dd_retnan:
	ldw	%r2,dd_FFFF
	mov	%r3,%r2
	mov	%r4,%r2
	mov	%r5,%r2
	br	dd_retcompos
dd_FFFF: .word	0xFFFF
dd_7FF_b: .word 0x7FF

dd_aisinf:
	cmp	%r2,%r0
	beq	dd_retnan
	br	dd_retinf

	; both are numbers, check infinity cases
dd_bothnums:
	ldw	%r0,dd_7FF_b
	ldw	%r1,dd_a+pd_exp(%r6)
	ldw	%r2,dd_b+pd_exp(%r6)
	cmp	%r1,%r0
	beq	dd_aisinf
	cmp	%r2,%r0
	beq	dd_retzero

	; make sure bman >= aman by shifting bman left and adjusting bexp
	lda	%r1,dd_b(%r6)
	lda	%r3,dd_bcounted
	br	findtopmanbitd
dd_bcounted:
	stb	%r0,dd_btop(%r6)
	lda	%r1,dd_a(%r6)
	lda	%r3,dd_acounted
	br	findtopmanbitd
dd_acounted:
	stb	%r0,dd_atop(%r6)
	mov	%r1,%r0
	bmi	dd_aiszero
	ldbs	%r0,dd_btop(%r6)	; A is nonzero, see if B is zero
	tst	%r0
	bmi	dd_retinf		; A is nonzero, B is zero, so return infinity
	sub	%r1,%r1,%r0		; R0 = number of bits to shift bman left to match
	ble	dd_bmangtaman		; if negative, bman is already >= aman
	ldw	%r0,dd_b+pd_exp(%r6)	; update bexp
	sub	%r0,%r0,%r1
	stw	%r0,dd_b+pd_exp(%r6)
	lda	%r0,dd_b+pd_man(%r6)	; shift bman left
	lda	%r3,dd_bmangtaman
	br	shiftquadleft
dd_1:	.word	1
dd_expoffs: .word 1023+53
dd_bmangtaman:

	; compute exponent
	;  sum = a - b + 1023 would be the point between bits <52> and <51>
	;  but we have to go through the divide loop 53 times to get top bit there
	ldw	%r0,dd_a+pd_exp(%r6)
	ldw	%r1,dd_b+pd_exp(%r6)
	sub	%r0,%r0,%r1
	ldw	%r1,dd_expoffs
	add	%r0,%r0,%r1
	ble	dd_retzero
	stw	%r0,dd_qexp(%r6)

	; divide (knowing bman >= aman)
	;     001a aaaa aaaa aaaa  dd_a+pd_man
	;  /  001b bbbb bbbb bbbb  dd_b+pd_man
	;     qqqq qqqq qqqq qqqq  dd_qman
	;       +6   +4   +2   +0
	; keep looping until qman<52> sets (normal number)
	; ...or until exponent decrements to zero (subnormal)
	clr	%r2
	clr	%r3
	clr	%r4
	clr	%r5
dd_divloop:
	stw	%r2,dd_qman+0(%r6)
	stw	%r3,dd_qman+2(%r6)
	stw	%r4,dd_qman+4(%r6)
	stw	%r5,dd_qman+6(%r6)
	ldw	%r2,dd_a+pd_man+0(%r6)		; R5..R2 = aman - bman
	ldw	%r3,dd_a+pd_man+2(%r6)
	ldw	%r4,dd_a+pd_man+4(%r6)
	ldw	%r5,dd_a+pd_man+6(%r6)
	ldw	%r0,dd_b+pd_man+0(%r6)
	sub	%r2,%r2,%r0
	ldw	%r0,dd_b+pd_man+2(%r6)
	sbb	%r3,%r3,%r0
	ldw	%r0,dd_b+pd_man+4(%r6)
	sbb	%r4,%r4,%r0
	ldw	%r0,dd_b+pd_man+6(%r6)
	sbb	%r5,%r5,%r0
	ldw	%r0,dd_1			; R0 = aman >= bman
	bhis	dd_divfits
	clr	%r0
	ldw	%r2,dd_a+pd_man+0(%r6)		; if (! R0) R5..R2 = aman
	ldw	%r3,dd_a+pd_man+2(%r6)
	ldw	%r4,dd_a+pd_man+4(%r6)
	ldw	%r5,dd_a+pd_man+6(%r6)
dd_divfits:
	shl	%r2,%r2				; aman = R2 << 1
	rol	%r3,%r3
	rol	%r4,%r4
	rol	%r5,%r5
	stw	%r2,dd_a+pd_man+0(%r6)
	stw	%r3,dd_a+pd_man+2(%r6)
	stw	%r4,dd_a+pd_man+4(%r6)
	stw	%r5,dd_a+pd_man+6(%r6)
	ldw	%r2,dd_qman+0(%r6)		; qman = (qman << 1) + R0
	ldw	%r3,dd_qman+2(%r6)
	ldw	%r4,dd_qman+4(%r6)
	ldw	%r5,dd_qman+6(%r6)
	lsr	%r0,%r0
	rol	%r2,%r2
	rol	%r3,%r3
	rol	%r4,%r4
	rol	%r5,%r5
	ldw	%r0,dd_qexp(%r6)		; qman just shifted left so decrement exponent
	lda	%r0,-1(%r0)
	stw	%r0,dd_qexp(%r6)
	tst	%r0				; stop if subnormal result
	beq	dd_subnormal
	ldw	%r1,dd_16			; repeat until qman<52> set
	cmp	%r5,%r1
	blt	dd_divloop
	ldw	%r1,dd_7FF			; done, see if exponent too big
	cmp	%r0,%r1
	bge	dd_retinf			; return infinity if so
	lda	%r5,-16(%r5)			; in range, clear hidden 1 qman<52>
	br	dd_qshifted
dd_16:	.word	16
dd_7FF: .word	0x7FF

	; quotient exponent gt 7FE, return INF
dd_retinf:
	ldw	%r5,dd_7FF0
	br	dd_lowzs
dd_7FF0: .word	0x7FF0

	; A is zero
	; if B is also zero, return NAN
	;         otherwise, return zero
dd_aiszero:
	ldbs	%r0,dd_btop(%r6)
	tst	%r0
	bmi	dd_retnan

	; quotient exponent lt 000, return 0
dd_retzero:
	clr	%r5
dd_lowzs:
	clr	%r4
	clr	%r3
	clr	%r2
	br	dd_insign

	; subnormal (exponent zero), mantissa needs to be shifted right one bit
dd_subnormal:
	lsr	%r5,%r5
	ror	%r4,%r4
	ror	%r3,%r3
	ror	%r2,%r2

	; mantissa shifted in place, combine result components
	;  R5..R2 = mantissa (hidden bit stripped)
	;  dd_qexp = exponent
	;  dd_aneg^dd_bneg = sign
dd_qshifted:
	ldw	%r0,dd_qexp(%r6)
	shl	%r0,%r0			; insert exponent into qman<62:52>
	shl	%r0,%r0
	shl	%r0,%r0
	shl	%r0,%r0
	or	%r5,%r5,%r0
dd_insign:
	ldbs	%r0,dd_a+pd_neg(%r6)	; shift sign bit from aneg^bneg into qman<63>
	ldbs	%r1,dd_b+pd_neg(%r6)
	xor	%r0,%r0,%r1
	shl	%r5,%r5
	shl	%r0,%r0
	ror	%r5,%r5

	; store result
dd_retcompos:
	ldw	%r0,dd_sptr(%r6)
	stw	%r2,0(%r0)
	stw	%r3,2(%r0)
	stw	%r4,4(%r0)
	stw	%r5,6(%r0)

	ldw	%r3,dd_radr(%r6)
	ldw	%r4,dd_sav4(%r6)
	ldw	%r5,dd_sav5(%r6)
	lda	%r6,dd_size(%r6)
	lda	%pc,0(%r3)



;
; find top bit set in mantissa
;  input:
;   R1 = points to 64-bit mantissa
;   R3 = return address
;  output:
;   returns R0 = -1: mantissa is zero
;              else: bit number 0..63
;
findtopmanbitd:
	ldw	%r0,ftbd_63
	ldw	%r2,6(%r1)
	tst	%r2
	bne	ftbd_loop
	lda	%r0,-16(%r0)
	ldw	%r2,4(%r1)
	tst	%r2
	bne	ftbd_loop
	lda	%r0,-16(%r0)
	ldw	%r2,2(%r1)
	tst	%r2
	bne	ftbd_loop
	lda	%r0,-16(%r0)
	ldw	%r2,0(%r1)
	tst	%r2
	bne	ftbd_loop
	lda	%r0,-16(%r0)
ftbd_done:
	lda	%pc,0(%r3)
ftbd_loop:
	shl	%r2,%r2
	bcs	ftbd_done
	lda	%r0,-1(%r0)
	br	ftbd_loop
ftbd_63:.word	63

;
; shift quadword left
;  input:
;   R0 = points to quad
;   R1 = shift count 0..63
;   R3 = return address
;  output:
;   quad shifted and stored
;   R1 = as on input
;   R5..R2 = shifted quad
;
sql_48:	.word	48
shiftquadleft:
	lda	%r6,-4(%r6)
	stw	%r0,-64(%r6)
	stw	%r3,-62(%r6)
	ldw	%r2,0(%r0)
	ldw	%r3,2(%r0)
	ldw	%r4,4(%r0)
	ldw	%r5,6(%r0)
	ldw	%r0,sql_48
	and	%r0,%r1,%r0
	lsr	%r0,%r0
	sub	%r0,%pc,%r0
	lda	%pc,sql_shift0-.(%r0)
sql_shift3:
	mov	%r5,%r2
	clr	%r4
	clr	%r3
	br	sql_clrr5
sql_shift2:
	mov	%r5,%r3
	mov	%r4,%r2
	clr	%r3
	br	sql_clrr5
sql_shift1:
	mov	%r5,%r4
	mov	%r4,%r3
	mov	%r3,%r2
sql_clrr5:
	clr	%r2
sql_shift0:
	ldw	%r0,sql_15
	and	%r0,%r0,%r1
	beq	sql_done
	neg	%r0,%r0
sql_shift:
	shl	%r2,%r2
	rol	%r3,%r3
	rol	%r4,%r4
	rol	%r5,%r5
	inc	%r0,%r0
	bne	sql_shift
sql_done:
	ldw	%r0,-64(%sp)
	stw	%r2,0(%r0)
	stw	%r3,2(%r0)
	stw	%r4,4(%r0)
	stw	%r5,6(%r0)
	ldw	%r0,-62(%r6)
	lda	%r6,4(%r6)
	lda	%pc,0(%r0)
sql_15:	.word	15


;
; floatingpoint comparison
;  input:
;   R0 = condition code
;     <5:4> = 0: left >= rite
;             1: left >  rite
;     	      2: left != rite
;       <3> = 0: as above
;             1: opposite
;   R1 = operand address
;   R2 = operand address
;   R3 = return address
;  output:
;   R0 = result code (0 or 1)
;   R4,R5 = same as on input
;   R1,R2,R3 = trashed
;
	cd_sav0 = -64
	cd_sav1 = -62
	cd_sav2 = -60
	cd_radr = -58
	cd_sav4 = -56
	cd_sav5 = -54
	cd_a    = -52
	cd_b    = -40
	cd_size = -28+64

	.align	2
	.global	__cmp_dd
__cmp_dd:
	lda	%r6,-cd_size(%r6)
	stw	%r0,cd_sav0(%r6)
	stw	%r1,cd_sav1(%r6)
	stw	%r2,cd_sav2(%r6)
	stw	%r3,cd_radr(%r6)
	stw	%r4,cd_sav4(%r6)
	stw	%r5,cd_sav5(%r6)

	; chop up A operand
	lda	%r2,cd_a(%r6)
	lda	%r3,cd_achopped
	ldw	%pc,#parsedouble
cd_achopped:
	bcs	cd_gotanan

	; chop up B operand
	ldw	%r1,cd_sav2(%r6)
	lda	%r2,cd_b(%r6)
	lda	%r3,cd_bchopped
	ldw	%pc,#parsedouble
cd_bchopped:
	bcs	cd_gotanan

	; 2s complement with sign bit flipped
	lda	%r1,cd_a(%r6)
	lda	%r3,cd_aflipped
	br	cmpdflip
cd_aflipped:
	lda	%r1,cd_b(%r6)
	lda	%r3,cd_bflipped
	br	cmpdflip
cd_bflipped:

	; compare
	clr	%r0			; assume false result
	ldw	%r1,cd_sav0(%r6)	; get compare code
	lsr	%r1,%r1
	lsr	%r1,%r1
	lsr	%r2,%r1			; save flip bit in R2
	lsr	%r1,%r2			; R1=0:GE; 1:GT; 2:NE

	ldw	%r4,cd_a+pd_exp(%r6)	; compare
	ldw	%r5,cd_b+pd_exp(%r6)
	cmp	%r4,%r5
	bne	cd_noteql
	ldw	%r4,cd_a+pd_man+6(%r6)
	ldw	%r5,cd_b+pd_man+6(%r6)
	cmp	%r4,%r5
	bne	cd_noteql
	ldw	%r4,cd_a+pd_man+4(%r6)
	ldw	%r5,cd_b+pd_man+4(%r6)
	cmp	%r4,%r5
	bne	cd_noteql
	ldw	%r4,cd_a+pd_man+2(%r6)
	ldw	%r5,cd_b+pd_man+2(%r6)
	cmp	%r4,%r5
	bne	cd_noteql
	ldw	%r4,cd_a+pd_man+0(%r6)
	ldw	%r5,cd_b+pd_man+0(%r6)
	cmp	%r4,%r5
	beq	cd_equal		; eq, only code 0 (GE) is true
cd_noteql:
	bhi	cd_greater		; gt, all codes (GE,GT,NE) are true
	lda	%r1,-2(%r1)		; lt, only code 2 (NE) is true
cd_equal:
	tst	%r1
	bne	cd_false
cd_greater:
	inc	%r0,%r0
cd_false:
	ldw	%r1,cd_1		; maybe flip true/false bit
	and	%r2,%r2,%r1
	xor	%r0,%r0,%r2
cd_return:
	ldw	%r3,cd_radr(%r6)
	ldw	%r4,cd_sav4(%r6)
	ldw	%r5,cd_sav5(%r6)
	lda	%r6,cd_size(%r6)
	lda	%pc,0(%r3)

cd_1:	.word	1

cd_gotanan:
	clr	%r0			; nans are always false
	br	cd_return

;
; if operand negative, take 2s complement of exponent:mantissa (so neg zero == pos zero)
; always flip exponent sign bit an extra time so unsigned compare works
;  input:
;   R1 = points to parsed value
;   R3 = return address
;  scratch:
;   R0,R2,R4
;
cmpdflip:
	ldw	%r2,pd_exp(%r1)
	ldw	%r0,cfd_x8000
	xor	%r2,%r2,%r0
	ldbs	%r0,pd_neg(%r1)
	tst	%r0
	beq	cfd_ispos
	clr	%r4
	ldw	%r0,pd_man+0(%r1)
	sub	%r0,%r4,%r0
	stw	%r0,pd_man+0(%r1)
	ldw	%r0,pd_man+2(%r1)
	sbb	%r0,%r4,%r0
	stw	%r0,pd_man+2(%r1)
	ldw	%r0,pd_man+4(%r1)
	sbb	%r0,%r4,%r0
	stw	%r0,pd_man+4(%r1)
	ldw	%r0,pd_man+6(%r1)
	sbb	%r0,%r4,%r0
	stw	%r0,pd_man+6(%r1)
	sbb	%r2,%r4,%r2
cfd_ispos:
	stw	%r2,pd_exp(%r1)
	lda	%pc,0(%r3)

cfd_x8000: .word	0x8000



;
; negation
;  input:
;   R0 = output pointer
;   R1 = input pointer
;
	.global	__neg_dd
__neg_dd:
	ldw	%r2,0(%r1)
	stw	%r2,0(%r0)
	ldw	%r2,2(%r1)
	stw	%r2,2(%r0)
	ldw	%r2,4(%r1)
	stw	%r2,4(%r0)
	ldw	%r2,6(%r1)
	ldw	%r1,negd_x8000
	xor	%r2,%r2,%r1
	stw	%r2,6(%r0)
	lda	%pc,0(%r3)
negd_x8000: .word 0x8000



;;printinstron: .word SCN_PRINTINSTR,1
;;	lda	%r3,printinstron
;;	clr	%r4
;;	stw	%r3,MAGIC_SCN(%r4)

;;	lda	%r3,printinstroff
;;	clr	%r4
;;	stw	%r3,MAGIC_SCN(%r4)
;;printinstroff: .word SCN_PRINTINSTR,0

.endif
