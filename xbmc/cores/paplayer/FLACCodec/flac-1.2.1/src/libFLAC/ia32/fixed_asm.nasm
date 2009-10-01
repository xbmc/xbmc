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

cglobal FLAC__fixed_compute_best_predictor_asm_ia32_mmx_cmov

	code_section

; **********************************************************************
;
; unsigned FLAC__fixed_compute_best_predictor(const FLAC__int32 *data, unsigned data_len, FLAC__float residual_bits_per_sample[FLAC__MAX_FIXED_ORDER+1])
; {
; 	FLAC__int32 last_error_0 = data[-1];
; 	FLAC__int32 last_error_1 = data[-1] - data[-2];
; 	FLAC__int32 last_error_2 = last_error_1 - (data[-2] - data[-3]);
; 	FLAC__int32 last_error_3 = last_error_2 - (data[-2] - 2*data[-3] + data[-4]);
; 	FLAC__int32 error, save;
; 	FLAC__uint32 total_error_0 = 0, total_error_1 = 0, total_error_2 = 0, total_error_3 = 0, total_error_4 = 0;
; 	unsigned i, order;
;
; 	for(i = 0; i < data_len; i++) {
; 		error  = data[i]     ; total_error_0 += local_abs(error);                      save = error;
; 		error -= last_error_0; total_error_1 += local_abs(error); last_error_0 = save; save = error;
; 		error -= last_error_1; total_error_2 += local_abs(error); last_error_1 = save; save = error;
; 		error -= last_error_2; total_error_3 += local_abs(error); last_error_2 = save; save = error;
; 		error -= last_error_3; total_error_4 += local_abs(error); last_error_3 = save;
; 	}
;
; 	if(total_error_0 < min(min(min(total_error_1, total_error_2), total_error_3), total_error_4))
; 		order = 0;
; 	else if(total_error_1 < min(min(total_error_2, total_error_3), total_error_4))
; 		order = 1;
; 	else if(total_error_2 < min(total_error_3, total_error_4))
; 		order = 2;
; 	else if(total_error_3 < total_error_4)
; 		order = 3;
; 	else
; 		order = 4;
;
; 	residual_bits_per_sample[0] = (FLAC__float)((data_len > 0 && total_error_0 > 0) ? log(M_LN2 * (FLAC__double)total_error_0 / (FLAC__double)data_len) / M_LN2 : 0.0);
; 	residual_bits_per_sample[1] = (FLAC__float)((data_len > 0 && total_error_1 > 0) ? log(M_LN2 * (FLAC__double)total_error_1 / (FLAC__double)data_len) / M_LN2 : 0.0);
; 	residual_bits_per_sample[2] = (FLAC__float)((data_len > 0 && total_error_2 > 0) ? log(M_LN2 * (FLAC__double)total_error_2 / (FLAC__double)data_len) / M_LN2 : 0.0);
; 	residual_bits_per_sample[3] = (FLAC__float)((data_len > 0 && total_error_3 > 0) ? log(M_LN2 * (FLAC__double)total_error_3 / (FLAC__double)data_len) / M_LN2 : 0.0);
; 	residual_bits_per_sample[4] = (FLAC__float)((data_len > 0 && total_error_4 > 0) ? log(M_LN2 * (FLAC__double)total_error_4 / (FLAC__double)data_len) / M_LN2 : 0.0);
;
; 	return order;
; }
	ALIGN 16
cident FLAC__fixed_compute_best_predictor_asm_ia32_mmx_cmov

	; esp + 36 == data[]
	; esp + 40 == data_len
	; esp + 44 == residual_bits_per_sample[]

	push	ebp
	push	ebx
	push	esi
	push	edi
	sub	esp, byte 16
	; qword [esp] == temp space for loading FLAC__uint64s to FPU regs

	; ebx == &data[i]
	; ecx == loop counter (i)
	; ebp == order
	; mm0 == total_error_1:total_error_0
	; mm1 == total_error_2:total_error_3
	; mm2 == :total_error_4
	; mm3 == last_error_1:last_error_0
	; mm4 == last_error_2:last_error_3

	mov	ecx, [esp + 40]			; ecx = data_len
	test	ecx, ecx
	jz	near .data_len_is_0

	mov	ebx, [esp + 36]			; ebx = data[]
	movd	mm3, [ebx - 4]			; mm3 = 0:last_error_0
	movd	mm2, [ebx - 8]			; mm2 = 0:data[-2]
	movd	mm1, [ebx - 12]			; mm1 = 0:data[-3]
	movd	mm0, [ebx - 16]			; mm0 = 0:data[-4]
	movq	mm5, mm3			; mm5 = 0:last_error_0
	psubd	mm5, mm2			; mm5 = 0:last_error_1
	punpckldq	mm3, mm5		; mm3 = last_error_1:last_error_0
	psubd	mm2, mm1			; mm2 = 0:data[-2] - data[-3]
	psubd	mm5, mm2			; mm5 = 0:last_error_2
	movq	mm4, mm5			; mm4 = 0:last_error_2
	psubd	mm4, mm2			; mm4 = 0:last_error_2 - (data[-2] - data[-3])
	paddd	mm4, mm1			; mm4 = 0:last_error_2 - (data[-2] - 2 * data[-3])
	psubd	mm4, mm0			; mm4 = 0:last_error_3
	punpckldq	mm4, mm5		; mm4 = last_error_2:last_error_3
	pxor	mm0, mm0			; mm0 = total_error_1:total_error_0
	pxor	mm1, mm1			; mm1 = total_error_2:total_error_3
	pxor	mm2, mm2			; mm2 = 0:total_error_4

	ALIGN 16
.loop:
	movd	mm7, [ebx]			; mm7 = 0:error_0
	add	ebx, byte 4
	movq	mm6, mm7			; mm6 = 0:error_0
	psubd	mm7, mm3			; mm7 = :error_1
	punpckldq	mm6, mm7		; mm6 = error_1:error_0
	movq	mm5, mm6			; mm5 = error_1:error_0
	movq	mm7, mm6			; mm7 = error_1:error_0
	psubd	mm5, mm3			; mm5 = error_2:
	movq	mm3, mm6			; mm3 = error_1:error_0	
	psrad	mm6, 31
	pxor	mm7, mm6
	psubd	mm7, mm6			; mm7 = abs(error_1):abs(error_0)
	paddd	mm0, mm7			; mm0 = total_error_1:total_error_0
	movq	mm6, mm5			; mm6 = error_2:
	psubd	mm5, mm4			; mm5 = error_3:
	punpckhdq	mm5, mm6		; mm5 = error_2:error_3
	movq	mm7, mm5			; mm7 = error_2:error_3
	movq	mm6, mm5			; mm6 = error_2:error_3
	psubd	mm5, mm4			; mm5 = :error_4
	movq	mm4, mm6			; mm4 = error_2:error_3
	psrad	mm6, 31
	pxor	mm7, mm6
	psubd	mm7, mm6			; mm7 = abs(error_2):abs(error_3)
	paddd	mm1, mm7			; mm1 = total_error_2:total_error_3
	movq	mm6, mm5			; mm6 = :error_4
	psrad	mm5, 31
	pxor	mm6, mm5
	psubd	mm6, mm5			; mm6 = :abs(error_4)
	paddd	mm2, mm6			; mm2 = :total_error_4
	
	dec	ecx
	jnz	short .loop

; 	if(total_error_0 < min(min(min(total_error_1, total_error_2), total_error_3), total_error_4))
; 		order = 0;
; 	else if(total_error_1 < min(min(total_error_2, total_error_3), total_error_4))
; 		order = 1;
; 	else if(total_error_2 < min(total_error_3, total_error_4))
; 		order = 2;
; 	else if(total_error_3 < total_error_4)
; 		order = 3;
; 	else
; 		order = 4;
	movq	mm3, mm0			; mm3 = total_error_1:total_error_0
	movd	edi, mm2			; edi = total_error_4
	movd	esi, mm1			; esi = total_error_3
	movd	eax, mm0			; eax = total_error_0
	punpckhdq	mm1, mm1		; mm1 = total_error_2:total_error_2
	punpckhdq	mm3, mm3		; mm3 = total_error_1:total_error_1
	movd	edx, mm1			; edx = total_error_2
	movd	ecx, mm3			; ecx = total_error_1

	xor	ebx, ebx
	xor	ebp, ebp
	inc	ebx
	cmp	ecx, eax
	cmovb	eax, ecx			; eax = min(total_error_0, total_error_1)
	cmovbe	ebp, ebx
	inc	ebx
	cmp	edx, eax
	cmovb	eax, edx			; eax = min(total_error_0, total_error_1, total_error_2)
	cmovbe	ebp, ebx
	inc	ebx
	cmp	esi, eax
	cmovb	eax, esi			; eax = min(total_error_0, total_error_1, total_error_2, total_error_3)
	cmovbe	ebp, ebx
	inc	ebx
	cmp	edi, eax
	cmovb	eax, edi			; eax = min(total_error_0, total_error_1, total_error_2, total_error_3, total_error_4)
	cmovbe	ebp, ebx
	movd	ebx, mm0			; ebx = total_error_0
	emms

	; 	residual_bits_per_sample[0] = (FLAC__float)((data_len > 0 && total_error_0 > 0) ? log(M_LN2 * (FLAC__double)total_error_0 / (FLAC__double)data_len) / M_LN2 : 0.0);
	; 	residual_bits_per_sample[1] = (FLAC__float)((data_len > 0 && total_error_1 > 0) ? log(M_LN2 * (FLAC__double)total_error_1 / (FLAC__double)data_len) / M_LN2 : 0.0);
	; 	residual_bits_per_sample[2] = (FLAC__float)((data_len > 0 && total_error_2 > 0) ? log(M_LN2 * (FLAC__double)total_error_2 / (FLAC__double)data_len) / M_LN2 : 0.0);
	; 	residual_bits_per_sample[3] = (FLAC__float)((data_len > 0 && total_error_3 > 0) ? log(M_LN2 * (FLAC__double)total_error_3 / (FLAC__double)data_len) / M_LN2 : 0.0);
	; 	residual_bits_per_sample[4] = (FLAC__float)((data_len > 0 && total_error_4 > 0) ? log(M_LN2 * (FLAC__double)total_error_4 / (FLAC__double)data_len) / M_LN2 : 0.0);
	xor	eax, eax
	fild	dword [esp + 40]		; ST = data_len (NOTE: assumes data_len is <2gigs)
.rbps_0:
	test	ebx, ebx
	jz	.total_error_0_is_0
	fld1					; ST = 1.0 data_len
	mov	[esp], ebx
	mov	[esp + 4], eax			; [esp] = (FLAC__uint64)total_error_0
	mov	ebx, [esp + 44]
	fild	qword [esp]			; ST = total_error_0 1.0 data_len
	fdiv	st2				; ST = total_error_0/data_len 1.0 data_len
	fldln2					; ST = ln2 total_error_0/data_len 1.0 data_len
	fmulp	st1				; ST = ln2*total_error_0/data_len 1.0 data_len
	fyl2x					; ST = log2(ln2*total_error_0/data_len) data_len
	fstp	dword [ebx]			; residual_bits_per_sample[0] = log2(ln2*total_error_0/data_len)   ST = data_len
	jmp	short .rbps_1
.total_error_0_is_0:
	mov	ebx, [esp + 44]
	mov	[ebx], eax			; residual_bits_per_sample[0] = 0.0
.rbps_1:
	test	ecx, ecx
	jz	.total_error_1_is_0
	fld1					; ST = 1.0 data_len
	mov	[esp], ecx
	mov	[esp + 4], eax			; [esp] = (FLAC__uint64)total_error_1
	fild	qword [esp]			; ST = total_error_1 1.0 data_len
	fdiv	st2				; ST = total_error_1/data_len 1.0 data_len
	fldln2					; ST = ln2 total_error_1/data_len 1.0 data_len
	fmulp	st1				; ST = ln2*total_error_1/data_len 1.0 data_len
	fyl2x					; ST = log2(ln2*total_error_1/data_len) data_len
	fstp	dword [ebx + 4]			; residual_bits_per_sample[1] = log2(ln2*total_error_1/data_len)   ST = data_len
	jmp	short .rbps_2
.total_error_1_is_0:
	mov	[ebx + 4], eax			; residual_bits_per_sample[1] = 0.0
.rbps_2:
	test	edx, edx
	jz	.total_error_2_is_0
	fld1					; ST = 1.0 data_len
	mov	[esp], edx
	mov	[esp + 4], eax			; [esp] = (FLAC__uint64)total_error_2
	fild	qword [esp]			; ST = total_error_2 1.0 data_len
	fdiv	st2				; ST = total_error_2/data_len 1.0 data_len
	fldln2					; ST = ln2 total_error_2/data_len 1.0 data_len
	fmulp	st1				; ST = ln2*total_error_2/data_len 1.0 data_len
	fyl2x					; ST = log2(ln2*total_error_2/data_len) data_len
	fstp	dword [ebx + 8]			; residual_bits_per_sample[2] = log2(ln2*total_error_2/data_len)   ST = data_len
	jmp	short .rbps_3
.total_error_2_is_0:
	mov	[ebx + 8], eax			; residual_bits_per_sample[2] = 0.0
.rbps_3:
	test	esi, esi
	jz	.total_error_3_is_0
	fld1					; ST = 1.0 data_len
	mov	[esp], esi
	mov	[esp + 4], eax			; [esp] = (FLAC__uint64)total_error_3
	fild	qword [esp]			; ST = total_error_3 1.0 data_len
	fdiv	st2				; ST = total_error_3/data_len 1.0 data_len
	fldln2					; ST = ln2 total_error_3/data_len 1.0 data_len
	fmulp	st1				; ST = ln2*total_error_3/data_len 1.0 data_len
	fyl2x					; ST = log2(ln2*total_error_3/data_len) data_len
	fstp	dword [ebx + 12]		; residual_bits_per_sample[3] = log2(ln2*total_error_3/data_len)   ST = data_len
	jmp	short .rbps_4
.total_error_3_is_0:
	mov	[ebx + 12], eax			; residual_bits_per_sample[3] = 0.0
.rbps_4:
	test	edi, edi
	jz	.total_error_4_is_0
	fld1					; ST = 1.0 data_len
	mov	[esp], edi
	mov	[esp + 4], eax			; [esp] = (FLAC__uint64)total_error_4
	fild	qword [esp]			; ST = total_error_4 1.0 data_len
	fdiv	st2				; ST = total_error_4/data_len 1.0 data_len
	fldln2					; ST = ln2 total_error_4/data_len 1.0 data_len
	fmulp	st1				; ST = ln2*total_error_4/data_len 1.0 data_len
	fyl2x					; ST = log2(ln2*total_error_4/data_len) data_len
	fstp	dword [ebx + 16]		; residual_bits_per_sample[4] = log2(ln2*total_error_4/data_len)   ST = data_len
	jmp	short .rbps_end
.total_error_4_is_0:
	mov	[ebx + 16], eax			; residual_bits_per_sample[4] = 0.0
.rbps_end:
	fstp	st0				; ST = [empty]
	jmp	short .end
.data_len_is_0:
	; data_len == 0, so residual_bits_per_sample[*] = 0.0
	xor	ebp, ebp
	mov	edi, [esp + 44]
	mov	[edi], ebp
	mov	[edi + 4], ebp
	mov	[edi + 8], ebp
	mov	[edi + 12], ebp
	mov	[edi + 16], ebp
	add	ebp, byte 4			; order = 4

.end:
	mov	eax, ebp			; return order
	add	esp, byte 16
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret

end

%ifdef OBJ_FORMAT_elf
       section .note.GNU-stack noalloc
%endif
