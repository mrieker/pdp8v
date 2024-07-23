
; from OS8 Handbook p41

;  ../asm/assemble tc08boot.asm tc08boot.obj > tc08boot.lis
;  ../asm/link -o tc08boot.oct tc08boot.obj > tc08boot.map

	. = 07613

	.global	__boot
__boot:

	.word	06774	; 7613 load status register B (with zeroes)
	.word	01222	; 7614 TAD 7622 / 0600 (rewind tape)
	.word	06766	; 7615 clear and load status register A
	.word	06771	; 7616 skip on flag
	.word	05216	; 7617 JMP .-1
	.word	01223	; 7620 TAD 7623 / 0220 (read tape)
	.word	05215	; 7621 JMP 7615
	.word	00600	; 7622
	.word	00220	; 7623

	. = 07754

	.word	07577	; 7754
	.word	07577	; 7755

