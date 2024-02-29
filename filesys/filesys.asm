

;
; allocate a block
;
;	ac = volume-pointer
;	jms	vol_allocblk
;
;  output:
;	link clear: success, ac = lbn
;	      else: ac = error code
;
vol_allocblk:
	dca	vol_allocblk_vol		; save volume struct pointer
	tad	vol_allocblk_vol		; calc &(vol->lofreeblk)
	tad	vol_allocblk_vollofreeblkofs
	dca	vol_allocblk_vollofreeblkptr
	tadi	vol_allocblk_vollofreeblkptr	; calculate lbn to start searching at
	jms	mulby12				;  = vol->lofreeblk * 12
	dca	vol_allocblk_lbn
	tadi	vol_allocblk_vollofreeblkptr	; calculate word index starting at beg of home block
	tad	vol_allocblk_hbw		;  = vol->lofreeblk + HBW
	dca	vol_allocblk_indx
	dca	vol_allocblk_havebmlbn		; we don't have a bitmap block read in
	tad	vol_allocblk_vol		; get negative vol->nblocks (number of blocks on volume)
	tad	vol_allocblk_volnblocksofs
	dca	vol_allocblk_negvolnblocks
	tadi	vol_allocblk_negvolnblocks
	cia
	dca	vol_allocblk_negvolnblocks
vol_allocblk_loop:
	cla cll					; see if enough bits left in bitmap to cover a word of blocks
	tad	vol_allocblk_lbn
	tad	vol_allocblk_p013
	tad	vol_allocblk_negvolnblocks
	szl cla
	jmp	vol_allocblk_done
	jms	vol_allocblk_readbmword		; get index word from bitmap
	iac					; see if that word has any zero bits
	sza cla
	jmp	vol_allocblk_gotone
	isz	vol_allocblk_indx		; it's all ones, on to next word
	tad	vol_allocblk_lbn
	tad	vol_allocblk_p014
	dca	vol_allocblk_lbn
	jmp	vol_allocblk_loop
vol_allocblk_done:
	tad	vol_allocblk_lbn		; see if there is a partial word on the end
	tad	vol_allocblk_negvolnblocks
	szl cla
	jmp	vol_allocblk_full
	jms	vol_allocblk_readbmblock	; get that last word
	dca	vol_allocblk_bmword
vol_allocblk_lastloop:
	tad	vol_allocblk_bmword		; look for zero bit in last word
	and	vol_allocblk_mask
	sna cla
	jmp	vol_allocblk_gotbit
	tad	vol_allocblk_mask		; shift mask over
	tad	vol_allocblk_mask
	dca	vol_allocblk_mask
	isz	vol_allocblk_lbn		; increment block number
	tad	vol_allocblk_lbn		; repeat if block number still in range
	tad	vol_allocblk_negvolnblocks
	snl cla
	jmp	vol_allocblk_lastloop
vol_allocblk_full:
	tad	vol_allocblk_erful		; no zero bit found, return ER_FUL
	jmpi	vol_allocblk
vol_allocblk_gotone:
	tad	vol_allocblk_bmword		; word has at least one zero bit, find lowest one
	and	vol_allocblk_mask
	sna cla
	jmp	vol_allocblk_gotbit
	tad	vol_allocblk_mask
	tad	vol_allocblk_mask
	dca	vol_allocblk_mask
	isz	vol_allocblk_lbn
	jmp	vol_allocblk_gotone
vol_allocblk_gotbit:
	tad	vol_allocblk_indx		; save index of where to start searching for next block
	tad	vol_allocblk_neghbw
	dcai	vol_allocblk_vollofreeblkptr
	tad	vol_allocblk_bmword		; set bit for the block
	cma
	dca	vol_allocblk_bmword
	tad	vol_allocblk_mask
	cma
	and	vol_allocblk_bmword
	dcai	vol_allocblk_bmwordptr
	tad	vol_allocblk_voldisk		; write updated block out
	jmsi	vol_allocblk_diskwlbptr
vol_allocblk_havebmlbn: .word .-.
		.word blockc
		.word	WPB
	szl
	jmpi	vol_allocblk
	cla
	tad	vol_allocblk_vol		; decrement number of free blocks
	tad	vol_allocblk_volnfreeblksofs
	dca	vol_allocblk_vol
	cla cll cma cml
	tadi	vol_allocblk_vol
	dcai	vol_allocblk_vol
	tad	vol_allocblk_lbn		; return allocated block number
	jmpi	vol_allocblk			; return with link clear

vol_allocblk_bmword:		.word	.-.	; blocks[indx%WPB]
vol_allocblk_bmwordptr:		.word	.-.	; &blocks[indx%WPB]
vol_allocblk_erful:		.word	ER_FUL
vol_allocblk_diskrlbptr:	.word	.-.	; vol->disk->rlb
vol_allocblk_diskwlbptr:	.word	.-.	; vol->disk->wlb
vol_allocblk_hbw:		.word	HBW
vol_allocblk_indx:		.word	.-.	; index of word in bitmap starting with home block
vol_allocblk_lbn:		.word	.-.	; logical block number being allocated
vol_allocblk_negvolnblocks:	.word	.-.	; - vol->nblocks
vol_allocblk_p013:		.word	013
vol_allocblk_p014:		.word	014
vol_allocblk_mask:		.word	.-.	; mask of bit being checked in bmword
vol_allocblk_neghbw:		.word	-HBW	; neg number of words in home block
vol_allocblk_vol:		.word	.-.	; vol struct pointer
vol_allocblk_voldisk:		.word	.-.	; vol->disk
vol_allocblk_vollofreeblkofs:	.word	VOL_LOFREEBLK
vol_allocblk_vollofreeblkptr:	.word	.-.	; &(vol->lofreeblk)
vol_allocblk_volnblocksofs:	.word	VOL_NBLOCKS

; read word from bitmap
;  input:
;   indx = word in index from start of home block
;   havebmlbm = block number currently in blockc (or 0 if none)
;  output:
;   ac = indx word in bitmap
;   havebmlbn = needbmlbn = bitmap block number for indx word
;   blockc = contains bitmap block
;   bmwordptr = points to indx word in blockc
vol_allocblk_readbmword: .word .-.
	tad	vol_allocblk_indx		; get needed bitmap block number
	bsw
	rtr
	and	vol_allocblk_p017
	tad	vol_allocblk_hblbn
	dca	vol_allocblk_needbmlbn
	tad	vol_allocblk_needbmlbn		; see if we already have that block
	cia
	tad	vol_allocblk_havebmlbn
	sna cla
	jmp	vol_allocblk_havebmblock
	tad	vol_allocblk_needbmlbn		; if not, read it in
	dca	vol_allocblk_havebmlbn
	tad	vol_allocblk_voldiskptr
	jmsi	vol_allocblk_diskrlbptr
vol_allocblk_needbmlbn: .word .-.
vol_allocblk_blockc:    .word blockc
		.word	WPB
	szl
	jmpi	vol_allocblk
	cla
vol_allocblk_havebmblock:
	tad	vol_allocblk_indx		; point to bitmap word
	and	vol_allocblk_wpbm1		; - &blockc[indx%WPB]
	tad	vol_allocblk_blockc
	dca	vol_allocblk_bmwordptr
	tadi	vol_allocblk_bmwordptr		; read that word in
	jmpi	vol_allocblk_readbmword

;
; free blocks listed in the given array
;
;	ac = volume-pointer
;	jms	vol_freeblks
;		.word	address-of-array
;		.word	number-of-blocks
;
;  output:
;	link clear: success, ac = 0
;	      else: ac = error code
;
;  scratch:
;	ai17, blockc
;
vol_freeblks: .word .-.
	dca	vol_freeblks_vol		; save volume struct pointer
	tad	vol_freeblks_vol		; calc &(vol->lofreeblk)
	tad	vol_freeblks_vollofreeblkofs
	dca	vol_freeblks_vollofreeblkptr
	cla cll cma cml				; get array-1 pointer
	tadi	vol_freeblks			; (side effect of clearing link)
	isz	vol_freeblks
	dca	ai17
	tadi	vol_freeblks			; get number of blocks to free
	isz	vol_freeblks
	sna
	jmpi	vol_freeblks			; all done if zero (with l,ac=00000)
	cia					; save negative count
	dca	vol_freeblks_negcount
	tad	vol_freeblks_vol		; get vol->disk
	tad	vol_freeblks_voldiskofs
	dca	vol_freeblks_voldiskptr
	tadi	vol_freeblks_voldiskptr
	dca	vol_freeblks_voldiskptr
	tad	vol_freeblks_voldiskptr		; get disk->rlb
	tad	vol_freeblks_diskrlbofs
	dca	vol_freeblks_diskrlbptr
	tadi	vol_freeblks_diskrlbptr
	dca	vol_freeblks_diskrlbptr
	tad	vol_freeblks_voldiskptr		; get disk->wlb
	tad	vol_freeblks_diskwlbofs
	dca	vol_freeblks_diskwlbptr
	tadi	vol_freeblks_diskwlbptr
	dca	vol_freeblks_diskwlbptr
	dca	vol_freeblks_havebmlbn		; nothing in blockc
vol_freeblks_loop:
	tadi	ai17				; ac = lbn from array
	jms	divby12				; get word index within whole bitmap
		.word	vol_freeblks_bitno	; bit number within bitmap word
	dca	vol_freeblks_index
	tad	vol_freeblks_index		; compute vol->lofreeblk - index
	cll cia
	tadi	vol_freeblks_vollofreeblkptr
	snl cla
	jmp	vol_freeblks_noupdlofreeblk
	tad	vol_freeblks_index		; if (vol->lofreeblk > index) vol->lofreeblk = index
	dcai	vol_freeblks_vollofreeblkptr	; (index of lowest word in bitmap where we might find a free block)
vol_freeblks_noupdlofreeblk:
	tad	vol_freeblks_index		; index += HBW to get word starting at beginning of home block
	tad	vol_freeblks_hbw
	dca	vol_freeblks_index
	tad	vol_freeblks_index		; get needed bitmap lbn
	bsw					;  = index / WPB + HBLBN
	rtr
	and	vol_freeblks_p15
	tad	vol_freeblks_hblbn
	dca	vol_freeblks_needbmlbn
	tad	vol_freeblks_needbmlbn		; compare needed bitmap lbn to have bitmap lbn
	cia
	tad	vol_freeblks_havebmlbn
	sna cla
	jmp	vol_freeblks_havebmblock	; - already have needed bitmap block
	jms	vol_freeblks_flusholdblock	; need different block, flush old bitmap block
	tad	vol_freeblks_needbmlbn		; read in needed bitmap block
	dca	vol_freeblks_havebmlbn
	tad	vol_freeblks_voldiskptr
	jmsi	vol_freeblks_diskrlbptr
vol_freeblks_needbmlbn: .word .-.
		.word	blockc
		.word	WPB
	szl
	jmpi	vol_freeblks
	cla
vol_freeblks_havebmblock:
	tad	vol_freeblks_index		; point to bitmap word in blockc = &blockc[index%WPB]
	and	vol_freeblks_wpbm1
	tad	vol_freeblks_blockcptr
	dca	vol_freeblks_wordptr
	tad	vol_freeblks_bitno		; 11 -> -1; 10 -> -2; ...; 0 -> -12
	tad	vol_freeblls_m14
	dca	vol_freeblks_bitno
	cla cll cma				; set up l,ac = 07777
vol_freeblks_bitloop:
	rar					; shift right to get 11 -> 13777; ...; 0 -> 17776
	isz	vol_freeblks_bitno
	jmp	vol_freeblks_bitloop
	andi	vol_freeblks_wordptr		; clear that bit in blockc[index%WPB]
	dcai	vol_freeblks_wordptr
	isz	vol_freeblks_negcount		; repeat if more blocks to free
	jmp	vol_freeblks_loop
	jms	vol_freeblks_flusholdblock	; all free, flush bitmap block
	jmpi	vol_freeblks			; done (ac = 0, l = 0)

; flush bitmap block
vol_freeblks_flusholdblock: .word .-.
	tad	vol_freeblks_havebmlbn		; see if we have a block to flush
	sna cla
	jmpi	vol_freeblks_flusholdblock
	tad	vol_freeblks_voldiskptr		; have an old one, flush old bitmap block out
	jmsi	vol_freeblks_diskwlbptr
vol_freeblks_havebmlbn: .word .-.
		.word	blockc
		.word	WPB
	szl
	jmpi	vol_freeblks			; error, return error status
	cla					; ok, return with clear accumulator
	jmpi	vol_freeblks_flusholdblock

vol_freeblks_bitno: .word .-.			; bit number in blockc[index%WPB] to clear
vol_freeblks_blockcptr: .word blockc
vol_freeblks_diskrlbofs: .word DISK_RLB
vol_freeblks_diskwlbptr: .word .-.		; vol->disk->wlb
vol_freeblks_diskwlbofs: .word DISK_WLB
vol_freeblks_diskwlbptr: .word .-.		; vol->disk->wlb
vol_freeblks_hblbn: .word HBLBN
vol_freeblks_hbw: .word HBW
vol_freeblks_index: .word .-.			; word index within bitmap to operate on
vol_freeblocks_m14: .word -014
vol_freeblks_negcount: .word .-.		; neg count of array elements to process
vol_freeblks_vol: .word .-.			; vol struct pointer
vol_freeblks_voldiskofs: .word VOL_DISK
vol_freeblks_voldiskptr: .word .-.		; vol->disk
vol_freeblks_vollofreeblkofs: .word VOL_LOFREEBLK
vol_freeblks_vollofreeblkptr: .word .-.		; &(vol->lofreeblk)
vol_freeblks_wordptr: .word .-.			; points to blocks[index%WPB]
vol_freeblkc_wpbm1: .word WPB-1


;
; calc file header checksum
;
;	jms	file_calchdrchksum
;	.word	hdrptrptr
;
; hdrptr: .word	hdr
;
file_calchdrchksum: .word .-.
	cla
	tadi	file_calchdrchksum		; ac = hdrptrptr
	dca	file_calchdrchksum_hdrptr
	cla cma
	tadi	file_calchdrchksum_hdrptr	; ac = hdrptr-1
	dca	ai17
	tadi	ai17
	tad	file_calchdrchksum_hdrchksump1	; ac = &(hdr->chksum)
	dca	file_calchdrchksum_csptr
	dcai	file_calchdrchksum_csptr	; hdr->chksum = NULL
	isz	file_calchdrchksum
	tad	file_calchdrchksum_negwpb	; initialize wordcount
	dca	file_calchdrchksum_count
file_calchdrchksum_loop:
	tadi	ai17				; sum up all words with checksum = 0
	isz	file_calchdrchksum_count
	jmp	file_calchdrchksum_loop
	cia					; save negated sum
	dcai	file_calchdrchksum_csptr
	jmpi	file_calchdrchksum_csptr

file_calchdrchksum_hdrchksump1: .word HDR_CHKSUM+1 ; offset of hdr->chksum
file_calchdrchksum_negwpb: .word -WPB	; negative words per block
file_calchdrchksum_hdrptr: .word .-.	; pointer to beginning of header
file_calchdrchksum_csptr: .word .-.	; pointer to checksum word in header
file_calchdrchksum_count: .word .-.	; count words when checksumming

;
; destruct volume object
; has to be unmounted first
;
;   input:
;     vol = volume pointer
;
vol__vol: .word	.-.
	cla
	tad	vol			; get vol pointer
	tad	vol__vol_voldiskoff	; &(vol->disk)
	dca	vol__vol_voldiskptr
	tadi	vol__vol_voldiskptr	; vol->disk
	sna
	jmpi	vol__vol		; if (vol->disk == NULL) return
	tad	vol__vol_diskvoloff	; &(vol->disk->vol)
	dca	vol__vol_voldiskvolptr
	dcai	vol__vol_voldiskptr	; vol->disk = NULL
	dcai	vol__vol_voldiskvolptr	; vol->disk->vol = NULL
	jmpi	vol__vol		; return

vol__vol_voldiskoff: .word VOL_DISK
vol__vol_diskvoloff: .word DISK_VOL
vol__vol_voldiskptr: .word .-.		; address of vol->disk
vol__vol_voldiskvolptr: .word .-.	; address of vol->disk->vol

;
; get current 48-bit time
;
;   input:
;     ac = where to put result
;
;   output:
;     time filled in:
;       +0..2: seconds since 1970-01-01 00:00:00 UTC' (big endian)
;           3: 4096ths of a second
;
getnow48: .word	.-.
	dca	now48p+2
	tad	now48p
	iot	scn_io+4
	jmpi	getnow48

now48p:	.word	.+1
	.word	SCN_GETNOW48
	.word	.-.

