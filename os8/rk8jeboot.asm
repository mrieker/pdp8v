
	caf  = 06007
	dlag = 06743
	dlca = 06744
	dldc = 06746

	. = 0023

	.global	__boot
__boot:
	caf
	dlca
	tad	unit
	dldc
	dlag
	tad	unit
	jmp	.

unit:	.word	0

