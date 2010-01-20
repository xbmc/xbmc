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

cglobal precompute_partition_info_sums_32bit_asm_ia32_

	code_section


; **********************************************************************
;
; void FLAC__bool FLAC__bitreader_read_rice_signed_block(FLAC__BitReader *br, int vals[], unsigned nvals, unsigned parameter)
; void precompute_partition_info_sums_32bit_(
; 	const FLAC__int32 residual[],
; 	FLAC__uint64 abs_residual_partition_sums[],
; 	unsigned blocksize,
; 	unsigned predictor_order,
; 	unsigned min_partition_order,
; 	unsigned max_partition_order
; )
;
	ALIGN 16
cident precompute_partition_info_sums_32bit_asm_ia32_

	;; peppered throughout the code at major checkpoints are keys like this as to where things are at that point in time
	;; [esp + 4]	const FLAC__int32 residual[]
	;; [esp + 8]	FLAC__uint64 abs_residual_partition_sums[]
	;; [esp + 12]	unsigned blocksize
	;; [esp + 16]	unsigned predictor_order
	;; [esp + 20]	unsigned min_partition_order
	;; [esp + 24]	unsigned max_partition_order
	push	ebp
	push	ebx
	push	esi
	push	edi
	sub	esp, 8
	;; [esp + 28]	const FLAC__int32 residual[]
	;; [esp + 32]	FLAC__uint64 abs_residual_partition_sums[]
	;; [esp + 36]	unsigned blocksize
	;; [esp + 40]	unsigned predictor_order
	;; [esp + 44]	unsigned min_partition_order
	;; [esp + 48]	unsigned max_partition_order
	;; [esp]	partitions
	;; [esp + 4]	default_partition_samples

	mov	ecx, [esp + 48]
	mov	eax, 1
	shl	eax, cl
	mov	[esp], eax		; [esp] <- partitions = 1u << max_partition_order;
	mov	eax, [esp + 36]
	shr	eax, cl
	mov	[esp + 4], eax		; [esp + 4] <- default_partition_samples = blocksize >> max_partition_order;

	;
	; first do max_partition_order
	;
	mov	edi, [esp + 4]
	sub	edi, [esp + 40]		; edi <- end = (unsigned)(-(int)predictor_order) + default_partition_samples
	xor	esi, esi		; esi <- residual_sample = 0
	xor	ecx, ecx		; ecx <- partition = 0
	mov	ebp, [esp + 28]		; ebp <- residual[]
	xor	ebx, ebx		; ebx <- abs_residual_partition_sum = 0;
	; note we put the updates to 'end' and 'abs_residual_partition_sum' at the end of loop0 and in the initialization above so we could align loop0 and loop1
	ALIGN	16
.loop0:					; for(partition = residual_sample = 0; partition < partitions; partition++) {
.loop1:					;   for( ; residual_sample < end; residual_sample++)
	mov	eax, [ebp + esi * 4]
	cdq
	xor	eax, edx
	sub	eax, edx
	add	ebx, eax		;     abs_residual_partition_sum += abs(residual[residual_sample]);
	;@@@@@@ check overflow flag and abort here?
	add	esi, byte 1
	cmp	esi, edi		;   /* since the loop will always run at least once, we can put the loop check down here */
	jb	.loop1
.next1:
	add	edi, [esp + 4]		;   end += default_partition_samples;
	mov	eax, [esp + 32]
	mov	[eax + ecx * 8], ebx	;   abs_residual_partition_sums[partition] = abs_residual_partition_sum;
	mov	[eax + ecx * 8 + 4], dword 0
	xor	ebx, ebx		;   abs_residual_partition_sum = 0;
	add	ecx, byte 1
	cmp	ecx, [esp]		; /* since the loop will always run at least once, we can put the loop check down here */
	jb	.loop0
.next0:					; }
	;
	; now merge partitions for lower orders
	;
	mov	esi, [esp + 32]		; esi <- abs_residual_partition_sums[from_partition==0];
	mov	eax, [esp]
	lea	edi, [esi + eax * 8]	; edi <- abs_residual_partition_sums[to_partition==partitions];
	mov	ecx, [esp + 48]
	sub	ecx, byte 1		; ecx <- partition_order = (int)max_partition_order - 1;
	ALIGN 16
.loop2:					; for(; partition_order >= (int)min_partition_order; partition_order--) {
	cmp	ecx, [esp + 44]
	jl	.next2
	mov	edx, 1
	shl	edx, cl			;   const unsigned partitions = 1u << partition_order;
	ALIGN 16
.loop3:					;   for(i = 0; i < partitions; i++) {
	mov	eax, [esi]
	mov	ebx, [esi + 4]
	add	eax, [esi + 8]
	adc	ebx, [esi + 12]
	mov	[edi], eax
	mov	[edi + 4], ebx		;     a_r_p_s[to_partition] = a_r_p_s[from_partition] + a_r_p_s[from_partition+1];
	add	esi, byte 16
	add	edi, byte 8
	sub	edx, byte 1
	jnz	.loop3			;   }
	sub	ecx, byte 1
	jmp	.loop2			; }
.next2:

	add	esp, 8
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret

end

%ifdef OBJ_FORMAT_elf
	section .note.GNU-stack noalloc
%endif
