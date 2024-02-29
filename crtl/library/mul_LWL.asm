
;
; multiply
;  input:
;   R0 = unsigned long address
;   R1 = unsigned word value
;   R2 = unsigned long address
;   R3 = return address
;  output:
;   R0,R4,R5 = same as on input
;   R1,R2,R3 = trashed
;
;  stack:
;   -64 : input R0
;   -62 : input R1
;   -60 : top word partial product
;   -58 : input R3
;   -56 : input R4
;   -54 : input R5
;
	.align	2
	.global	__mul_LWL
__mul_LWL:
	lda	%r6,-8(%r6)
	stw	%r0,-64(%r6)
	stw	%r3,-62(%r6)
	stw	%r4,-60(%r6)
	stw	%r5,-58(%r6)

	ldw	%r3,2(%r2)
	ldw	%r2,0(%r2)

	lda	%r5,mul_LWL_ret
	ldw	%pc,#__umul		; R1R0 = R3R2 * R1
mul_LWL_ret:

	mov	%r2,%r0

	ldw	%r0,-64(%r6)
	stw	%r2,0(%r0)
	stw	%r1,2(%r0)

	ldw	%r3,-62(%r6)
	ldw	%r4,-60(%r6)
	ldw	%r5,-58(%r6)
	lda	%r6,8(%r6)
	lda	%pc,0(%r3)
