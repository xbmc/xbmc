;  vim:filetype=nasm ts=8

;  libFLAC - Free Lossless Audio Codec library
;  Copyright (C) 2001,2002,2003,2004,2005,2006,2007  Josh Coalson
;
;  Redistribution and use in source and binary forms, with or without
;  modification, are permitted provided that the following conditions
;  are met:
;
;  - Redistributions of source code must retain the above copyright
;  notice, this list of conditions and the following disclaimer.
;
;  - Redistributions in binary form must reproduce the above copyright
;  notice, this list of conditions and the following disclaimer in the
;  documentation and/or other materials provided with the distribution.
;
;  - Neither the name of the Xiph.org Foundation nor the names of its
;  contributors may be used to endorse or promote products derived from
;  this software without specific prior written permission.
;
;  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
;  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
;  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
;  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
;  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
;  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
;  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
;  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
;  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
;  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
;  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

%include "nasm.h"

	data_section

cextern FLAC__crc16_table		; unsigned FLAC__crc16_table[256];
cextern bitreader_read_from_client_	; FLAC__bool bitreader_read_from_client_(FLAC__BitReader *br);

cglobal FLAC__bitreader_read_rice_signed_block_asm_ia32_bswap

	code_section


; **********************************************************************
;
; void FLAC__bool FLAC__bitreader_read_rice_signed_block(FLAC__BitReader *br, int vals[], unsigned nvals, unsigned parameter)
;
; Some details like assertions and other checking is performed by the caller.
	ALIGN 16
cident FLAC__bitreader_read_rice_signed_block_asm_ia32_bswap

	;ASSERT(0 != br);
	;ASSERT(0 != br->buffer);
	; WATCHOUT: code only works if sizeof(brword)==32; we can make things much faster with this assertion
	;ASSERT(FLAC__BITS_PER_WORD == 32);
	;ASSERT(parameter < 32);
	; the above two asserts also guarantee that the binary part never straddles more than 2 words, so we don't have to loop to read it

	;; peppered throughout the code at major checkpoints are keys like this as to where things are at that point in time
	;; [esp + 16]	unsigned parameter
	;; [esp + 12]	unsigned nvals
	;; [esp + 8]	int vals[]
	;; [esp + 4]	FLAC__BitReader *br
	mov	eax, [esp + 12]		; if(nvals == 0)
	test	eax, eax
	ja	.nvals_gt_0
	mov	eax, 1			;   return true;
	ret

.nvals_gt_0:
	push	ebp
	push	ebx
	push	esi
	push	edi
	sub	esp, 4
	;; [esp + 36]	unsigned parameter
	;; [esp + 32]	unsigned nvals
	;; [esp + 28]	int vals[]
	;; [esp + 24]	FLAC__BitReader *br
	;; [esp]	ucbits
	mov	ebp, [esp + 24]		; ebp <- br == br->buffer
	mov	esi, [ebp + 16]		; esi <- br->consumed_words (aka 'cwords' in the C version)
	mov	ecx, [ebp + 20]		; ecx <- br->consumed_bits  (aka 'cbits'  in the C version)
	xor	edi, edi		; edi <- 0  'uval'
	;; ecx		cbits
	;; esi		cwords
	;; edi		uval
	;; ebp		br
	;; [ebp]	br->buffer
	;; [ebp + 8]	br->words
	;; [ebp + 12]	br->bytes
	;; [ebp + 16]	br->consumed_words
	;; [ebp + 20]	br->consumed_bits
	;; [ebp + 24]	br->read_crc
	;; [ebp + 28]	br->crc16_align

					; ucbits = (br->words-cwords)*FLAC__BITS_PER_WORD + br->bytes*8 - cbits;
	mov	eax, [ebp + 8]		;   eax <- br->words
	sub	eax, esi		;   eax <- br->words-cwords
	shl	eax, 2			;   eax <- (br->words-cwords)*FLAC__BYTES_PER_WORD
	add	eax, [ebp + 12]		;   eax <- (br->words-cwords)*FLAC__BYTES_PER_WORD + br->bytes
	shl	eax, 3			;   eax <- (br->words-cwords)*FLAC__BITS_PER_WORD + br->bytes*8
	sub	eax, ecx		;   eax <- (br->words-cwords)*FLAC__BITS_PER_WORD + br->bytes*8 - cbits
	mov	[esp], eax		;   ucbits <- eax

	ALIGN 16
.val_loop:				; while(1) {

	;
	; read unary part
	;
.unary_loop:				;   while(1) {
	;; ecx		cbits
	;; esi		cwords
	;; edi		uval
	;; ebp		br
	cmp	esi, [ebp + 8]		;     while(cwords < br->words)   /* if we've not consumed up to a partial tail word... */
	jae	near .c1_next1
.c1_loop:				;     {
	mov	ebx, [ebp]
	mov	eax, [ebx + 4*esi]	;       b = br->buffer[cwords]
	mov	edx, eax		;       edx = br->buffer[cwords] (saved for later use)
	shl	eax, cl 		;       b = br->buffer[cwords] << cbits
	test	eax, eax		;         (still have to test since cbits may be 0, thus ZF not updated for shl eax,0)
	jz	near .c1_next2		;       if(b) {
	bsr	ebx, eax
	not	ebx
	and	ebx, 31			;         ebx = 'i' = # of leading 0 bits in 'b' (eax)
	add	ecx, ebx		;         cbits += i;
	add	edi, ebx		;         uval += i;
	add	ecx, byte 1		;         cbits++; /* skip over stop bit */
	test	ecx, ~31
	jz	near .break1 		;         if(cbits >= FLAC__BITS_PER_WORD) { /* faster way of testing if(cbits == FLAC__BITS_PER_WORD) */
					;           crc16_update_word_(br, br->buffer[cwords]);
	push	edi			;		[need more registers]
	bswap	edx			;		edx = br->buffer[cwords] swapped; now we can CRC the bytes from LSByte to MSByte which makes things much easier
	mov	ecx, [ebp + 28]		;		ecx <- br->crc16_align
	mov	eax, [ebp + 24]		;		ax <- br->read_crc (a.k.a. crc)
%ifdef FLAC__PUBLIC_NEEDS_UNDERSCORE
	mov	edi, _FLAC__crc16_table
%else
	mov	edi, FLAC__crc16_table
%endif
	;; eax (ax)	crc a.k.a. br->read_crc
	;; ebx (bl)	intermediate result index into FLAC__crc16_table[]
	;; ecx		br->crc16_align
	;; edx		byteswapped brword to CRC
	;; esi		cwords
	;; edi		unsigned FLAC__crc16_table[]
	;; ebp		br
	test	ecx, ecx		;		switch(br->crc16_align) ...
	jnz	.c0b4			;		[br->crc16_align is 0 the vast majority of the time so we optimize the common case]
.c0b0:	xor	dl, ah			;		dl <- (crc>>8)^(word>>24)
	movzx	ebx, dl
	mov	ecx, [ebx*4 + edi]	;		cx <- FLAC__crc16_table[(crc>>8)^(word>>24)]
	shl	eax, 8			;		ax <- (crc<<8)
	xor	eax, ecx		;		crc <- ax <- (crc<<8) ^ FLAC__crc16_table[(crc>>8)^(word>>24)]
.c0b1:	xor	dh, ah			;		dh <- (crc>>8)^((word>>16)&0xff))
	movzx	ebx, dh
	mov	ecx, [ebx*4 + edi]	;		cx <- FLAC__crc16_table[(crc>>8)^((word>>16)&0xff))]
	shl	eax, 8			;		ax <- (crc<<8)
	xor	eax, ecx		;		crc <- ax <- (crc<<8) ^ FLAC__crc16_table[(crc>>8)^((word>>16)&0xff))]
	shr	edx, 16
.c0b2:	xor	dl, ah			;		dl <- (crc>>8)^((word>>8)&0xff))
	movzx	ebx, dl
	mov	ecx, [ebx*4 + edi]	;		cx <- FLAC__crc16_table[(crc>>8)^((word>>8)&0xff))]
	shl	eax, 8			;		ax <- (crc<<8)
	xor	eax, ecx		;		crc <- ax <- (crc<<8) ^ FLAC__crc16_table[(crc>>8)^((word>>8)&0xff))]
.c0b3:	xor	dh, ah			;		dh <- (crc>>8)^(word&0xff)
	movzx	ebx, dh
	mov	ecx, [ebx*4 + edi]	;		cx <- FLAC__crc16_table[(crc>>8)^(word&0xff)]
	shl	eax, 8			;		ax <- (crc<<8)
	xor	eax, ecx		;		crc <- ax <- (crc<<8) ^ FLAC__crc16_table[(crc>>8)^(word&0xff)]
	movzx	eax, ax
	mov	[ebp + 24], eax		;		br->read_crc <- crc
	pop	edi

	add	esi, byte 1		;           cwords++;
	xor	ecx, ecx		;           cbits = 0;
					;         }
	jmp	near .break1		;         goto break1;
	;; this section relocated out of the way for performance
.c0b4:
	mov	[ebp + 28], dword 0	;		br->crc16_align <- 0
	cmp	ecx, 8
	je	.c0b1
	shr	edx, 16
	cmp	ecx, 16
	je	.c0b2
	jmp	.c0b3

	;; this section relocated out of the way for performance
.c1b4:
	mov	[ebp + 28], dword 0	;		br->crc16_align <- 0
	cmp	ecx, 8
	je	.c1b1
	shr	edx, 16
	cmp	ecx, 16
	je	.c1b2
	jmp	.c1b3

.c1_next2:				;       } else {
	;; ecx		cbits
	;; edx		current brword 'b'
	;; esi		cwords
	;; edi		uval
	;; ebp		br
	add	edi, 32
	sub	edi, ecx		;         uval += FLAC__BITS_PER_WORD - cbits;
					;         crc16_update_word_(br, br->buffer[cwords]);
	push	edi			;		[need more registers]
	bswap	edx			;		edx = br->buffer[cwords] swapped; now we can CRC the bytes from LSByte to MSByte which makes things much easier
	mov	ecx, [ebp + 28]		;		ecx <- br->crc16_align
	mov	eax, [ebp + 24]		;		ax <- br->read_crc (a.k.a. crc)
%ifdef FLAC__PUBLIC_NEEDS_UNDERSCORE
	mov	edi, _FLAC__crc16_table
%else
	mov	edi, FLAC__crc16_table
%endif
	;; eax (ax)	crc a.k.a. br->read_crc
	;; ebx (bl)	intermediate result index into FLAC__crc16_table[]
	;; ecx		br->crc16_align
	;; edx		byteswapped brword to CRC
	;; esi		cwords
	;; edi		unsigned FLAC__crc16_table[]
	;; ebp		br
	test	ecx, ecx		;		switch(br->crc16_align) ...
	jnz	.c1b4			;		[br->crc16_align is 0 the vast majority of the time so we optimize the common case]
.c1b0:	xor	dl, ah			;		dl <- (crc>>8)^(word>>24)
	movzx	ebx, dl
	mov	ecx, [ebx*4 + edi]	;		cx <- FLAC__crc16_table[(crc>>8)^(word>>24)]
	shl	eax, 8			;		ax <- (crc<<8)
	xor	eax, ecx		;		crc <- ax <- (crc<<8) ^ FLAC__crc16_table[(crc>>8)^(word>>24)]
.c1b1:	xor	dh, ah			;		dh <- (crc>>8)^((word>>16)&0xff))
	movzx	ebx, dh
	mov	ecx, [ebx*4 + edi]	;		cx <- FLAC__crc16_table[(crc>>8)^((word>>16)&0xff))]
	shl	eax, 8			;		ax <- (crc<<8)
	xor	eax, ecx		;		crc <- ax <- (crc<<8) ^ FLAC__crc16_table[(crc>>8)^((word>>16)&0xff))]
	shr	edx, 16
.c1b2:	xor	dl, ah			;		dl <- (crc>>8)^((word>>8)&0xff))
	movzx	ebx, dl
	mov	ecx, [ebx*4 + edi]	;		cx <- FLAC__crc16_table[(crc>>8)^((word>>8)&0xff))]
	shl	eax, 8			;		ax <- (crc<<8)
	xor	eax, ecx		;		crc <- ax <- (crc<<8) ^ FLAC__crc16_table[(crc>>8)^((word>>8)&0xff))]
.c1b3:	xor	dh, ah			;		dh <- (crc>>8)^(word&0xff)
	movzx	ebx, dh
	mov	ecx, [ebx*4 + edi]	;		cx <- FLAC__crc16_table[(crc>>8)^(word&0xff)]
	shl	eax, 8			;		ax <- (crc<<8)
	xor	eax, ecx		;		crc <- ax <- (crc<<8) ^ FLAC__crc16_table[(crc>>8)^(word&0xff)]
	movzx	eax, ax
	mov	[ebp + 24], eax		;		br->read_crc <- crc
	pop	edi

	add	esi, byte 1		;         cwords++;
	xor	ecx, ecx		;         cbits = 0;
					;         /* didn't find stop bit yet, have to keep going... */
					;       }

	cmp	esi, [ebp + 8]		;     } while(cwords < br->words)   /* if we've not consumed up to a partial tail word... */
	jb	near .c1_loop

.c1_next1:
	; at this point we've eaten up all the whole words; have to try
	; reading through any tail bytes before calling the read callback.
	; this is a repeat of the above logic adjusted for the fact we
	; don't have a whole word.  note though if the client is feeding
	; us data a byte at a time (unlikely), br->consumed_bits may not
	; be zero.
	;; ecx		cbits
	;; esi		cwords
	;; edi		uval
	;; ebp		br
	mov	edx, [ebp + 12]		;     edx <- br->bytes
	test	edx, edx
	jz	.read1			;     if(br->bytes) {  [NOTE: this case is rare so it doesn't have to be all that fast ]
	mov	ebx, [ebp]
	shl	edx, 3			;       edx <- const unsigned end = br->bytes * 8;
	mov	eax, [ebx + 4*esi]	;       b = br->buffer[cwords]
	xchg	edx, ecx		;       [edx <- cbits , ecx <- end]
	mov	ebx, 0xffffffff		;       ebx <- FLAC__WORD_ALL_ONES
	shr	ebx, cl			;       ebx <- FLAC__WORD_ALL_ONES >> end
	not	ebx			;       ebx <- ~(FLAC__WORD_ALL_ONES >> end)
	xchg	edx, ecx		;       [edx <- end , ecx <- cbits]
	and	eax, ebx		;       b = (br->buffer[cwords] & ~(FLAC__WORD_ALL_ONES >> end));
	shl	eax, cl 		;       b = (br->buffer[cwords] & ~(FLAC__WORD_ALL_ONES >> end)) << cbits;
	test	eax, eax		;         (still have to test since cbits may be 0, thus ZF not updated for shl eax,0)
	jz	.c1_next3		;       if(b) {
	bsr	ebx, eax
	not	ebx
	and	ebx, 31			;         ebx = 'i' = # of leading 0 bits in 'b' (eax)
	add	ecx, ebx		;         cbits += i;
	add	edi, ebx		;         uval += i;
	add	ecx, byte 1		;         cbits++; /* skip over stop bit */
	jmp	short .break1 		;         goto break1;
.c1_next3:				;       } else {
	sub	edi, ecx
	add	edi, edx		;         uval += end - cbits;
	add	ecx, edx		;         cbits += end
					;         /* didn't find stop bit yet, have to keep going... */
					;       }
					;     }
.read1:
	; flush registers and read; bitreader_read_from_client_() does
	; not touch br->consumed_bits at all but we still need to set
	; it in case it fails and we have to return false.
	;; ecx		cbits
	;; esi		cwords
	;; edi		uval
	;; ebp		br
	mov	[ebp + 16], esi		;     br->consumed_words = cwords;
	mov	[ebp + 20], ecx		;     br->consumed_bits = cbits;
	push	ecx			;     /* save */
	push	ebp			;     /* push br argument */
%ifdef FLAC__PUBLIC_NEEDS_UNDERSCORE
	call	_bitreader_read_from_client_
%else
	call	bitreader_read_from_client_
%endif
	pop	edx			;     /* discard, unused */
	pop	ecx			;     /* restore */
	mov	esi, [ebp + 16]		;     cwords = br->consumed_words;
					;     ucbits = (br->words-cwords)*FLAC__BITS_PER_WORD + br->bytes*8 - cbits;
	mov	ebx, [ebp + 8]		;       ebx <- br->words
	sub	ebx, esi		;       ebx <- br->words-cwords
	shl	ebx, 2			;       ebx <- (br->words-cwords)*FLAC__BYTES_PER_WORD
	add	ebx, [ebp + 12]		;       ebx <- (br->words-cwords)*FLAC__BYTES_PER_WORD + br->bytes
	shl	ebx, 3			;       ebx <- (br->words-cwords)*FLAC__BITS_PER_WORD + br->bytes*8
	sub	ebx, ecx		;       ebx <- (br->words-cwords)*FLAC__BITS_PER_WORD + br->bytes*8 - cbits
	add	ebx, edi		;       ebx <- (br->words-cwords)*FLAC__BITS_PER_WORD + br->bytes*8 - cbits + uval
					;           + uval to offset our count by the # of unary bits already
					;           consumed before the read, because we will add these back
					;           in all at once at break1
	mov	[esp], ebx		;       ucbits <- ebx
	test	eax, eax		;     if(!bitreader_read_from_client_(br))
	jnz	near .unary_loop
	jmp	.end			;       return false; /* eax (the return value) is already 0 */
					;   } /* end while(1) unary part */

	ALIGN 16
.break1:
	;; ecx		cbits
	;; esi		cwords
	;; edi		uval
	;; ebp		br
	;; [esp]	ucbits
	sub	[esp], edi		;   ucbits -= uval;
	sub	dword [esp], byte 1	;   ucbits--; /* account for stop bit */

	;
	; read binary part
	;
	mov	ebx, [esp + 36]		;   ebx <- parameter
	test	ebx, ebx		;   if(parameter) {
	jz	near .break2
.read2:
	cmp	[esp], ebx		;     while(ucbits < parameter) {
	jae	.c2_next1
	; flush registers and read; bitreader_read_from_client_() does
	; not touch br->consumed_bits at all but we still need to set
	; it in case it fails and we have to return false.
	mov	[ebp + 16], esi		;       br->consumed_words = cwords;
	mov	[ebp + 20], ecx		;       br->consumed_bits = cbits;
	push	ecx			;       /* save */
	push	ebp			;       /* push br argument */
%ifdef FLAC__PUBLIC_NEEDS_UNDERSCORE
	call	_bitreader_read_from_client_
%else
	call	bitreader_read_from_client_
%endif
	pop	edx			;       /* discard, unused */
	pop	ecx			;       /* restore */
	mov	esi, [ebp + 16]		;       cwords = br->consumed_words;
					;       ucbits = (br->words-cwords)*FLAC__BITS_PER_WORD + br->bytes*8 - cbits;
	mov	edx, [ebp + 8]		;         edx <- br->words
	sub	edx, esi		;         edx <- br->words-cwords
	shl	edx, 2			;         edx <- (br->words-cwords)*FLAC__BYTES_PER_WORD
	add	edx, [ebp + 12]		;         edx <- (br->words-cwords)*FLAC__BYTES_PER_WORD + br->bytes
	shl	edx, 3			;         edx <- (br->words-cwords)*FLAC__BITS_PER_WORD + br->bytes*8
	sub	edx, ecx		;         edx <- (br->words-cwords)*FLAC__BITS_PER_WORD + br->bytes*8 - cbits
	mov	[esp], edx		;         ucbits <- edx
	test	eax, eax		;       if(!bitreader_read_from_client_(br))
	jnz	.read2
	jmp	.end			;         return false; /* eax (the return value) is already 0 */
					;     }
.c2_next1:
	;; ebx		parameter
	;; ecx		cbits
	;; esi		cwords
	;; edi		uval
	;; ebp		br
	;; [esp]	ucbits
	cmp	esi, [ebp + 8]		;     if(cwords < br->words) { /* if we've not consumed up to a partial tail word... */
	jae	near .c2_next2
	test	ecx, ecx		;       if(cbits) {
	jz	near .c2_next3		;         /* this also works when consumed_bits==0, it's just a little slower than necessary for that case */
	mov	eax, 32
	mov	edx, [ebp]
	sub	eax, ecx		;         const unsigned n = FLAC__BITS_PER_WORD - cbits;
	mov	edx, [edx + 4*esi]	;         const brword word = br->buffer[cwords];
	cmp	ebx, eax		;         if(parameter < n) {
	jae	.c2_next4
					;           uval <<= parameter;
					;           uval |= (word & (FLAC__WORD_ALL_ONES >> cbits)) >> (n-parameter);
	shl	edx, cl
	xchg	ebx, ecx
	shld	edi, edx, cl
	add	ebx, ecx		;           cbits += parameter;
	xchg	ebx, ecx		;           ebx <- parameter, ecx <- cbits
	jmp	.break2			;           goto break2;
					;         }
.c2_next4:
					;         uval <<= n;
					;         uval |= word & (FLAC__WORD_ALL_ONES >> cbits);
%if 1
	rol	edx, cl			;            @@@@@@OPT: may be faster to use rol to save edx so we can restore it for CRC'ing
					;            @@@@@@OPT: or put parameter in ch instead and free up ebx completely again
%else
	shl	edx, cl
%endif
	xchg	eax, ecx
	shld	edi, edx, cl
	xchg	eax, ecx
%if 1
	ror	edx, cl			;            restored.
%else
	mov	edx, [ebp]
	mov	edx, [edx + 4*esi]
%endif
					;         crc16_update_word_(br, br->buffer[cwords]);
	push	edi			;		[need more registers]
	push	ebx			;		[need more registers]
	push	eax			;		[need more registers]
	bswap	edx			;		edx = br->buffer[cwords] swapped; now we can CRC the bytes from LSByte to MSByte which makes things much easier
	mov	ecx, [ebp + 28]		;		ecx <- br->crc16_align
	mov	eax, [ebp + 24]		;		ax <- br->read_crc (a.k.a. crc)
%ifdef FLAC__PUBLIC_NEEDS_UNDERSCORE
	mov	edi, _FLAC__crc16_table
%else
	mov	edi, FLAC__crc16_table
%endif
	;; eax (ax)	crc a.k.a. br->read_crc
	;; ebx (bl)	intermediate result index into FLAC__crc16_table[]
	;; ecx		br->crc16_align
	;; edx		byteswapped brword to CRC
	;; esi		cwords
	;; edi		unsigned FLAC__crc16_table[]
	;; ebp		br
	test	ecx, ecx		;		switch(br->crc16_align) ...
	jnz	.c2b4			;		[br->crc16_align is 0 the vast majority of the time so we optimize the common case]
.c2b0:	xor	dl, ah			;		dl <- (crc>>8)^(word>>24)
	movzx	ebx, dl
	mov	ecx, [ebx*4 + edi]	;		cx <- FLAC__crc16_table[(crc>>8)^(word>>24)]
	shl	eax, 8			;		ax <- (crc<<8)
	xor	eax, ecx		;		crc <- ax <- (crc<<8) ^ FLAC__crc16_table[(crc>>8)^(word>>24)]
.c2b1:	xor	dh, ah			;		dh <- (crc>>8)^((word>>16)&0xff))
	movzx	ebx, dh
	mov	ecx, [ebx*4 + edi]	;		cx <- FLAC__crc16_table[(crc>>8)^((word>>16)&0xff))]
	shl	eax, 8			;		ax <- (crc<<8)
	xor	eax, ecx		;		crc <- ax <- (crc<<8) ^ FLAC__crc16_table[(crc>>8)^((word>>16)&0xff))]
	shr	edx, 16
.c2b2:	xor	dl, ah			;		dl <- (crc>>8)^((word>>8)&0xff))
	movzx	ebx, dl
	mov	ecx, [ebx*4 + edi]	;		cx <- FLAC__crc16_table[(crc>>8)^((word>>8)&0xff))]
	shl	eax, 8			;		ax <- (crc<<8)
	xor	eax, ecx		;		crc <- ax <- (crc<<8) ^ FLAC__crc16_table[(crc>>8)^((word>>8)&0xff))]
.c2b3:	xor	dh, ah			;		dh <- (crc>>8)^(word&0xff)
	movzx	ebx, dh
	mov	ecx, [ebx*4 + edi]	;		cx <- FLAC__crc16_table[(crc>>8)^(word&0xff)]
	shl	eax, 8			;		ax <- (crc<<8)
	xor	eax, ecx		;		crc <- ax <- (crc<<8) ^ FLAC__crc16_table[(crc>>8)^(word&0xff)]
	movzx	eax, ax
	mov	[ebp + 24], eax		;		br->read_crc <- crc
	pop	eax
	pop	ebx
	pop	edi
	add	esi, byte 1		;         cwords++;
	mov	ecx, ebx
	sub	ecx, eax		;         cbits = parameter - n;
	jz	.break2			;         if(cbits) { /* parameter > n, i.e. if there are still bits left to read, there have to be less than 32 so they will all be in the next word */
					;           uval <<= cbits;
					;           uval |= (br->buffer[cwords] >> (FLAC__BITS_PER_WORD-cbits));
	mov	eax, [ebp]
	mov	eax, [eax + 4*esi]
	shld	edi, eax, cl
					;         }
	jmp	.break2			;         goto break2;

	;; this section relocated out of the way for performance
.c2b4:
	mov	[ebp + 28], dword 0	;		br->crc16_align <- 0
	cmp	ecx, 8
	je	.c2b1
	shr	edx, 16
	cmp	ecx, 16
	je	.c2b2
	jmp	.c2b3

.c2_next3:				;       } else {
	mov	ecx, ebx		;         cbits = parameter;
					;         uval <<= cbits;
					;         uval |= (br->buffer[cwords] >> (FLAC__BITS_PER_WORD-cbits));
	mov	eax, [ebp]
	mov	eax, [eax + 4*esi]
	shld	edi, eax, cl
	jmp	.break2			;         goto break2;
					;       }
.c2_next2:				;     } else {
	; in this case we're starting our read at a partial tail word;
	; the reader has guaranteed that we have at least 'parameter'
	; bits available to read, which makes this case simpler.
					;       uval <<= parameter;
					;       if(cbits) {
					;         /* this also works when consumed_bits==0, it's just a little slower than necessary for that case */
					;         uval |= (br->buffer[cwords] & (FLAC__WORD_ALL_ONES >> cbits)) >> (FLAC__BITS_PER_WORD-cbits-parameter);
					;         cbits += parameter;
					;         goto break2;
					;       } else {
					;         cbits = parameter;
					;         uval |= br->buffer[cwords] >> (FLAC__BITS_PER_WORD-cbits);
					;         goto break2;
					;       }
					;       the above is much shorter in assembly:
	mov	eax, [ebp]
	mov	eax, [eax + 4*esi]	;       eax <- br->buffer[cwords]
	shl	eax, cl			;       eax <- br->buffer[cwords] << cbits
	add	ecx, ebx		;       cbits += parameter
	xchg	ebx, ecx		;       ebx <- cbits, ecx <- parameter
	shld	edi, eax, cl		;       uval <<= parameter <<< 'parameter' bits of tail word
	xchg	ebx, ecx		;       ebx <- parameter, ecx <- cbits
					;     }
					;   }
.break2:
	sub	[esp], ebx		;   ucbits -= parameter;

	;
	; compose the value
	;
	mov	ebx, [esp + 28]		;   ebx <- vals
	mov	edx, edi		;   edx <- uval
	and	edi, 1			;   edi <- uval & 1
	shr	edx, 1			;   edx <- uval >> 1
	neg	edi			;   edi <- -(int)(uval & 1)
	xor	edx, edi		;   edx <- (uval >> 1 ^ -(int)(uval & 1))
	mov	[ebx], edx		;   *vals <- edx
	sub	dword [esp + 32], byte 1	;   --nvals;
	jz	.finished		;   if(nvals == 0) /* jump to finish */
	xor	edi, edi		;   uval = 0;
	add	dword [esp + 28], 4	;   ++vals
	jmp	.val_loop		; }

.finished:
	mov	[ebp + 16], esi		; br->consumed_words = cwords;
	mov	[ebp + 20], ecx		; br->consumed_bits = cbits;
	mov	eax, 1
.end:
	add	esp, 4
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret

end

%ifdef OBJ_FORMAT_elf
	section .note.GNU-stack noalloc
%endif
