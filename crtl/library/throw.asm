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

	.psect	.70.data
	.align	2
	.global	__trystack
__trystack: .word 0

	.psect	.50.text
	.align	2

;
; throw an exception
;  input:
;   R0 = thrown object pointer
;   R1 = thrown object type id
;        throwntypeid:
;            .word   addr-of-asciz-type-name-string
;            .word   subtypeid_1
;            .word   subtypeid_2
;            .word   subtypeid_3
;                 ...
;            .word   0
;
	.global	__throw
__throw:
	ldw	%r4,#__trystack
	ldw	%r4,0(%r4)		; point to active trymark
	tst	%r4
	beq	emptystack
	stw	%r0,4(%r4)		; save thrown object pointer in case of rethrow
	stw	%r1,6(%r4)		; save thrown object type id in case of rethrow
	ldw	%r5,2(%r4)		; point to try descriptor block
	lsr	%pc,%r5			; if catch in progress, ie, a catch handler is throwing a new exception ...
	bcs	catthrowing		; ... call its finally handler then rethrow the new exception
	ldw	%r2,0(%r5)		; get stack pointer offset
	sub	%r6,%r4,%r2		; reset stack pointer for try/catch/finally block
;
; search descriptor block for catch handler that matches thrown object
;
;  R4 =	points to active trymark
;	.word	next_outer_trymark
;	.word	descriptor_block
;	.word	thrown_object_pointer
;	.word	thrown_object_type_id
;
;  R5 = points to descriptor block
;	.word	stack pointer offset
;	.word	finally entrypoint
;	.word	catch handler[0] type id
;	.word	catch handler[0] entrypoint
;	.word	catch handler[1] type id
;	.word	catch handler[1] entrypoint
;			...
;	.word	0
;
;  R6 = stack pointer for the try/catch/finally block
;
	lda	%r3,4-4(%r5)		; point to first catch element in descriptor block - 4
getthrowntypeid:
	ldw	%r1,6(%r4)		; get thrown object type id
checkcatchtypeid:
	lda	%r3,4(%r3)		; increment to next descriptor block entry
	ldw	%r2,0(%r3)		; get type id from descriptor block entry
	tst	%r2
	beq	callfinally		; end of catches, call finally and rethrow
	cmp	%r2,%r1			; see if it matches thrown object type id
	beq	foundmatchingcatch
checksubtypes:
	lda	%r1,2(%r1)		; first time, skip over typename string pointer, else, skip to next subtypeid
	ldw	%r0,0(%r1)		; get a subtype id of thrown object
	tst	%r0
	beq	getthrowntypeid		; no more, look for another catch handler
	cmp	%r2,%r0			; see if it matches
	bne	checksubtypes		; if not, check for more subtyoes
	ldw	%r0,4(%r4)		; matches, get original thrown object pointer
	ldw	%r2,-2(%r1)		; cast it to the matching subtype
	add	%r0,%r0,%r2		; ... by pointing to subobject within
	ldw	%r1,6(%r4)		; get original thrown object type id
	br	foundmatchingsubtype	; call the catch handler
;
; type id matches indicating appropriate catch handler found in descriptor block
; jump to the corresponding catch handler, passing the thrown exception object
;
;  R1 = original throw object type id
;  R3 = points to catch handler entry in descriptor block
;  R4 =	points to active trymark
;  R5 = points to descriptor block
;  R6 = stack pointer for the catch/finally handlers
;
foundmatchingcatch:
	ldw	%r0,4(%r4)		; get thrown object pointer
foundmatchingsubtype:
	inc	%r2,%r5			; set low bit of desciptor pointer which indicates catch handler in progress
	stw	%r2,2(%r4)		; ... in case the catch handler throws an exception
	ldw	%pc,2(%r3)		; jump to catch entrypoint
;
; exception thrown with no active try block
; should never happen cuz top-level start contains a catch-all handler
;
emptystack:
	lda	%r6,-4(%r6)
	lda	%r0,ehmsg
	stw	%r0,-64(%r6)
	clr	%r0
	stw	%r0,-62(%r6)
	lda	%r3,emptystack
	ldw	%pc,#assertfail
ehmsg:	.asciz	"__throw: no active catch handler"
	.align	2
;
; catch handler is throwing another exception
;  call the old finally handler
;  rethrow the new exception
;
catthrowing:
	lda	%r5,-1(%r5)
	ldw	%r2,0(%r5)		; get stack pointer offset
	sub	%r6,%r4,%r2		; reset stack pointer for try block
;
; no suitable catch handler found, call the finally handler then rethrow the exception
;  R4 =	points to active trymark
;       0(%r4) = next outer trymark or 0 if none
;       2(%r4) = points to descriptor block (possibly + 1)
;       4(%r4) = thrown object pointer
;       6(%r4) = thrown object type id
;  R5 = points to descriptor block
;  R6 = stack pointer for the try/catch/finally blocks
;
; call finally handler which unlinks the trymark from the trystack, then rethrow the exception
;  R5 = points to descriptor block
; it returns with
;  R0 = trymark that it just unlinked from trystack
; 
callfinally:
	lda	%r3,.+4
	ldw	%pc,2(%r5)		; call finally handler
	ldw	%r1,6(%r0)		; ... then rethrow exception
	ldw	%r0,4(%r0)
	br	__throw

;
; currently active call handler is re-throwing the exception
; call the corresponding finally handler then rethrow exception
;
;  __trystack = trymark for catch that is doing rethrow
;    0(__trystack) = next outer trymark or 0 if none
;    2(__trystack) = descriptor block pointer with low bit set indicating catch is active
;    4(__trystack) = original thrown object pointer
;    6(__trystack) = original thrown object type id
;
	.global	__rethrow
__rethrow:
	ldw	%r4,#__trystack
	ldw	%r4,0(%r4)		; point to active trymark
	ldw	%r5,2(%r4)		; point to try descriptor block, should have low bit set
	lda	%r5,-1(%r5)		; ... indicating thrown typeid,objectpointer are valid
	lsr	%pc,%r5
	bcc	callfinally
afloop:
	lda	%r6,-4(%r6)
	lda	%r0,afmsg
	stw	%r0,-64(%r6)
	clr	%r0
	stw	%r0,-62(%r6)
	lda	%r3,afloop
	ldw	%pc,#assertfail

afmsg:	.asciz	"__rethrow: no active catch handler"

