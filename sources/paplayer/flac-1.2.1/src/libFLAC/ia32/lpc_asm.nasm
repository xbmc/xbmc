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

cglobal FLAC__lpc_compute_autocorrelation_asm_ia32
cglobal FLAC__lpc_compute_autocorrelation_asm_ia32_sse_lag_4
cglobal FLAC__lpc_compute_autocorrelation_asm_ia32_sse_lag_8
cglobal FLAC__lpc_compute_autocorrelation_asm_ia32_sse_lag_12
cglobal FLAC__lpc_compute_autocorrelation_asm_ia32_3dnow
cglobal FLAC__lpc_compute_residual_from_qlp_coefficients_asm_ia32
cglobal FLAC__lpc_compute_residual_from_qlp_coefficients_asm_ia32_mmx
cglobal FLAC__lpc_restore_signal_asm_ia32
cglobal FLAC__lpc_restore_signal_asm_ia32_mmx

	code_section

; **********************************************************************
;
; void FLAC__lpc_compute_autocorrelation_asm(const FLAC__real data[], unsigned data_len, unsigned lag, FLAC__real autoc[])
; {
;	FLAC__real d;
;	unsigned sample, coeff;
;	const unsigned limit = data_len - lag;
;
;	FLAC__ASSERT(lag > 0);
;	FLAC__ASSERT(lag <= data_len);
;
;	for(coeff = 0; coeff < lag; coeff++)
;		autoc[coeff] = 0.0;
;	for(sample = 0; sample <= limit; sample++) {
;		d = data[sample];
;		for(coeff = 0; coeff < lag; coeff++)
;			autoc[coeff] += d * data[sample+coeff];
;	}
;	for(; sample < data_len; sample++) {
;		d = data[sample];
;		for(coeff = 0; coeff < data_len - sample; coeff++)
;			autoc[coeff] += d * data[sample+coeff];
;	}
; }
;
	ALIGN 16
cident FLAC__lpc_compute_autocorrelation_asm_ia32
	;[esp + 28] == autoc[]
	;[esp + 24] == lag
	;[esp + 20] == data_len
	;[esp + 16] == data[]

	;ASSERT(lag > 0)
	;ASSERT(lag <= 33)
	;ASSERT(lag <= data_len)

.begin:
	push	esi
	push	edi
	push	ebx

	;	for(coeff = 0; coeff < lag; coeff++)
	;		autoc[coeff] = 0.0;
	mov	edi, [esp + 28]			; edi == autoc
	mov	ecx, [esp + 24]			; ecx = # of dwords (=lag) of 0 to write
	xor	eax, eax
	rep	stosd

	;	const unsigned limit = data_len - lag;
	mov	eax, [esp + 24]			; eax == lag
	mov	ecx, [esp + 20]
	sub	ecx, eax			; ecx == limit

	mov	edi, [esp + 28]			; edi == autoc
	mov	esi, [esp + 16]			; esi == data
	inc	ecx				; we are looping <= limit so we add one to the counter

	;	for(sample = 0; sample <= limit; sample++) {
	;		d = data[sample];
	;		for(coeff = 0; coeff < lag; coeff++)
	;			autoc[coeff] += d * data[sample+coeff];
	;	}
	fld	dword [esi]			; ST = d <- data[sample]
	; each iteration is 11 bytes so we need (-eax)*11, so we do (-12*eax + eax)
	lea	edx, [eax + eax*2]
	neg	edx
	lea	edx, [eax + edx*4 + .jumper1_0 - .get_eip1]
	call	.get_eip1
.get_eip1:
	pop	ebx
	add	edx, ebx
	inc	edx				; compensate for the shorter opcode on the last iteration
	inc	edx				; compensate for the shorter opcode on the last iteration
	inc	edx				; compensate for the shorter opcode on the last iteration
	cmp	eax, 33
	jne	.loop1_start
	sub	edx, byte 9			; compensate for the longer opcodes on the first iteration
.loop1_start:
	jmp	edx

	fld	st0				; ST = d d
	fmul	dword [esi + (32*4)]		; ST = d*data[sample+32] d		WATCHOUT: not a byte displacement here!
	fadd	dword [edi + (32*4)]		; ST = autoc[32]+d*data[sample+32] d	WATCHOUT: not a byte displacement here!
	fstp	dword [edi + (32*4)]		; autoc[32]+=d*data[sample+32]  ST = d	WATCHOUT: not a byte displacement here!
	fld	st0				; ST = d d
	fmul	dword [esi + (31*4)]		; ST = d*data[sample+31] d
	fadd	dword [edi + (31*4)]		; ST = autoc[31]+d*data[sample+31] d
	fstp	dword [edi + (31*4)]		; autoc[31]+=d*data[sample+31]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (30*4)]		; ST = d*data[sample+30] d
	fadd	dword [edi + (30*4)]		; ST = autoc[30]+d*data[sample+30] d
	fstp	dword [edi + (30*4)]		; autoc[30]+=d*data[sample+30]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (29*4)]		; ST = d*data[sample+29] d
	fadd	dword [edi + (29*4)]		; ST = autoc[29]+d*data[sample+29] d
	fstp	dword [edi + (29*4)]		; autoc[29]+=d*data[sample+29]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (28*4)]		; ST = d*data[sample+28] d
	fadd	dword [edi + (28*4)]		; ST = autoc[28]+d*data[sample+28] d
	fstp	dword [edi + (28*4)]		; autoc[28]+=d*data[sample+28]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (27*4)]		; ST = d*data[sample+27] d
	fadd	dword [edi + (27*4)]		; ST = autoc[27]+d*data[sample+27] d
	fstp	dword [edi + (27*4)]		; autoc[27]+=d*data[sample+27]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (26*4)]		; ST = d*data[sample+26] d
	fadd	dword [edi + (26*4)]		; ST = autoc[26]+d*data[sample+26] d
	fstp	dword [edi + (26*4)]		; autoc[26]+=d*data[sample+26]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (25*4)]		; ST = d*data[sample+25] d
	fadd	dword [edi + (25*4)]		; ST = autoc[25]+d*data[sample+25] d
	fstp	dword [edi + (25*4)]		; autoc[25]+=d*data[sample+25]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (24*4)]		; ST = d*data[sample+24] d
	fadd	dword [edi + (24*4)]		; ST = autoc[24]+d*data[sample+24] d
	fstp	dword [edi + (24*4)]		; autoc[24]+=d*data[sample+24]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (23*4)]		; ST = d*data[sample+23] d
	fadd	dword [edi + (23*4)]		; ST = autoc[23]+d*data[sample+23] d
	fstp	dword [edi + (23*4)]		; autoc[23]+=d*data[sample+23]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (22*4)]		; ST = d*data[sample+22] d
	fadd	dword [edi + (22*4)]		; ST = autoc[22]+d*data[sample+22] d
	fstp	dword [edi + (22*4)]		; autoc[22]+=d*data[sample+22]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (21*4)]		; ST = d*data[sample+21] d
	fadd	dword [edi + (21*4)]		; ST = autoc[21]+d*data[sample+21] d
	fstp	dword [edi + (21*4)]		; autoc[21]+=d*data[sample+21]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (20*4)]		; ST = d*data[sample+20] d
	fadd	dword [edi + (20*4)]		; ST = autoc[20]+d*data[sample+20] d
	fstp	dword [edi + (20*4)]		; autoc[20]+=d*data[sample+20]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (19*4)]		; ST = d*data[sample+19] d
	fadd	dword [edi + (19*4)]		; ST = autoc[19]+d*data[sample+19] d
	fstp	dword [edi + (19*4)]		; autoc[19]+=d*data[sample+19]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (18*4)]		; ST = d*data[sample+18] d
	fadd	dword [edi + (18*4)]		; ST = autoc[18]+d*data[sample+18] d
	fstp	dword [edi + (18*4)]		; autoc[18]+=d*data[sample+18]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (17*4)]		; ST = d*data[sample+17] d
	fadd	dword [edi + (17*4)]		; ST = autoc[17]+d*data[sample+17] d
	fstp	dword [edi + (17*4)]		; autoc[17]+=d*data[sample+17]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (16*4)]		; ST = d*data[sample+16] d
	fadd	dword [edi + (16*4)]		; ST = autoc[16]+d*data[sample+16] d
	fstp	dword [edi + (16*4)]		; autoc[16]+=d*data[sample+16]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (15*4)]		; ST = d*data[sample+15] d
	fadd	dword [edi + (15*4)]		; ST = autoc[15]+d*data[sample+15] d
	fstp	dword [edi + (15*4)]		; autoc[15]+=d*data[sample+15]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (14*4)]		; ST = d*data[sample+14] d
	fadd	dword [edi + (14*4)]		; ST = autoc[14]+d*data[sample+14] d
	fstp	dword [edi + (14*4)]		; autoc[14]+=d*data[sample+14]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (13*4)]		; ST = d*data[sample+13] d
	fadd	dword [edi + (13*4)]		; ST = autoc[13]+d*data[sample+13] d
	fstp	dword [edi + (13*4)]		; autoc[13]+=d*data[sample+13]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (12*4)]		; ST = d*data[sample+12] d
	fadd	dword [edi + (12*4)]		; ST = autoc[12]+d*data[sample+12] d
	fstp	dword [edi + (12*4)]		; autoc[12]+=d*data[sample+12]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (11*4)]		; ST = d*data[sample+11] d
	fadd	dword [edi + (11*4)]		; ST = autoc[11]+d*data[sample+11] d
	fstp	dword [edi + (11*4)]		; autoc[11]+=d*data[sample+11]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (10*4)]		; ST = d*data[sample+10] d
	fadd	dword [edi + (10*4)]		; ST = autoc[10]+d*data[sample+10] d
	fstp	dword [edi + (10*4)]		; autoc[10]+=d*data[sample+10]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + ( 9*4)]		; ST = d*data[sample+9] d
	fadd	dword [edi + ( 9*4)]		; ST = autoc[9]+d*data[sample+9] d
	fstp	dword [edi + ( 9*4)]		; autoc[9]+=d*data[sample+9]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + ( 8*4)]		; ST = d*data[sample+8] d
	fadd	dword [edi + ( 8*4)]		; ST = autoc[8]+d*data[sample+8] d
	fstp	dword [edi + ( 8*4)]		; autoc[8]+=d*data[sample+8]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + ( 7*4)]		; ST = d*data[sample+7] d
	fadd	dword [edi + ( 7*4)]		; ST = autoc[7]+d*data[sample+7] d
	fstp	dword [edi + ( 7*4)]		; autoc[7]+=d*data[sample+7]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + ( 6*4)]		; ST = d*data[sample+6] d
	fadd	dword [edi + ( 6*4)]		; ST = autoc[6]+d*data[sample+6] d
	fstp	dword [edi + ( 6*4)]		; autoc[6]+=d*data[sample+6]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + ( 5*4)]		; ST = d*data[sample+4] d
	fadd	dword [edi + ( 5*4)]		; ST = autoc[4]+d*data[sample+4] d
	fstp	dword [edi + ( 5*4)]		; autoc[4]+=d*data[sample+4]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + ( 4*4)]		; ST = d*data[sample+4] d
	fadd	dword [edi + ( 4*4)]		; ST = autoc[4]+d*data[sample+4] d
	fstp	dword [edi + ( 4*4)]		; autoc[4]+=d*data[sample+4]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + ( 3*4)]		; ST = d*data[sample+3] d
	fadd	dword [edi + ( 3*4)]		; ST = autoc[3]+d*data[sample+3] d
	fstp	dword [edi + ( 3*4)]		; autoc[3]+=d*data[sample+3]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + ( 2*4)]		; ST = d*data[sample+2] d
	fadd	dword [edi + ( 2*4)]		; ST = autoc[2]+d*data[sample+2] d
	fstp	dword [edi + ( 2*4)]		; autoc[2]+=d*data[sample+2]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + ( 1*4)]		; ST = d*data[sample+1] d
	fadd	dword [edi + ( 1*4)]		; ST = autoc[1]+d*data[sample+1] d
	fstp	dword [edi + ( 1*4)]		; autoc[1]+=d*data[sample+1]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi]			; ST = d*data[sample] d			WATCHOUT: no displacement byte here!
	fadd	dword [edi]			; ST = autoc[0]+d*data[sample] d	WATCHOUT: no displacement byte here!
	fstp	dword [edi]			; autoc[0]+=d*data[sample]  ST = d	WATCHOUT: no displacement byte here!
.jumper1_0:

	fstp	st0				; pop d, ST = empty
	add	esi, byte 4			; sample++
	dec	ecx
	jz	.loop1_end
	fld	dword [esi]			; ST = d <- data[sample]
	jmp	edx
.loop1_end:

	;	for(; sample < data_len; sample++) {
	;		d = data[sample];
	;		for(coeff = 0; coeff < data_len - sample; coeff++)
	;			autoc[coeff] += d * data[sample+coeff];
	;	}
	mov	ecx, [esp + 24]			; ecx <- lag
	dec	ecx				; ecx <- lag - 1
	jz	near .end			; skip loop if 0 (i.e. lag == 1)

	fld	dword [esi]			; ST = d <- data[sample]
	mov	eax, ecx			; eax <- lag - 1 == data_len - sample the first time through
	; each iteration is 11 bytes so we need (-eax)*11, so we do (-12*eax + eax)
	lea	edx, [eax + eax*2]
	neg	edx
	lea	edx, [eax + edx*4 + .jumper2_0 - .get_eip2]
	call	.get_eip2
.get_eip2:
	pop	ebx
	add	edx, ebx
	inc	edx				; compensate for the shorter opcode on the last iteration
	inc	edx				; compensate for the shorter opcode on the last iteration
	inc	edx				; compensate for the shorter opcode on the last iteration
	jmp	edx

	fld	st0				; ST = d d
	fmul	dword [esi + (31*4)]		; ST = d*data[sample+31] d
	fadd	dword [edi + (31*4)]		; ST = autoc[31]+d*data[sample+31] d
	fstp	dword [edi + (31*4)]		; autoc[31]+=d*data[sample+31]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (30*4)]		; ST = d*data[sample+30] d
	fadd	dword [edi + (30*4)]		; ST = autoc[30]+d*data[sample+30] d
	fstp	dword [edi + (30*4)]		; autoc[30]+=d*data[sample+30]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (29*4)]		; ST = d*data[sample+29] d
	fadd	dword [edi + (29*4)]		; ST = autoc[29]+d*data[sample+29] d
	fstp	dword [edi + (29*4)]		; autoc[29]+=d*data[sample+29]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (28*4)]		; ST = d*data[sample+28] d
	fadd	dword [edi + (28*4)]		; ST = autoc[28]+d*data[sample+28] d
	fstp	dword [edi + (28*4)]		; autoc[28]+=d*data[sample+28]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (27*4)]		; ST = d*data[sample+27] d
	fadd	dword [edi + (27*4)]		; ST = autoc[27]+d*data[sample+27] d
	fstp	dword [edi + (27*4)]		; autoc[27]+=d*data[sample+27]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (26*4)]		; ST = d*data[sample+26] d
	fadd	dword [edi + (26*4)]		; ST = autoc[26]+d*data[sample+26] d
	fstp	dword [edi + (26*4)]		; autoc[26]+=d*data[sample+26]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (25*4)]		; ST = d*data[sample+25] d
	fadd	dword [edi + (25*4)]		; ST = autoc[25]+d*data[sample+25] d
	fstp	dword [edi + (25*4)]		; autoc[25]+=d*data[sample+25]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (24*4)]		; ST = d*data[sample+24] d
	fadd	dword [edi + (24*4)]		; ST = autoc[24]+d*data[sample+24] d
	fstp	dword [edi + (24*4)]		; autoc[24]+=d*data[sample+24]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (23*4)]		; ST = d*data[sample+23] d
	fadd	dword [edi + (23*4)]		; ST = autoc[23]+d*data[sample+23] d
	fstp	dword [edi + (23*4)]		; autoc[23]+=d*data[sample+23]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (22*4)]		; ST = d*data[sample+22] d
	fadd	dword [edi + (22*4)]		; ST = autoc[22]+d*data[sample+22] d
	fstp	dword [edi + (22*4)]		; autoc[22]+=d*data[sample+22]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (21*4)]		; ST = d*data[sample+21] d
	fadd	dword [edi + (21*4)]		; ST = autoc[21]+d*data[sample+21] d
	fstp	dword [edi + (21*4)]		; autoc[21]+=d*data[sample+21]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (20*4)]		; ST = d*data[sample+20] d
	fadd	dword [edi + (20*4)]		; ST = autoc[20]+d*data[sample+20] d
	fstp	dword [edi + (20*4)]		; autoc[20]+=d*data[sample+20]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (19*4)]		; ST = d*data[sample+19] d
	fadd	dword [edi + (19*4)]		; ST = autoc[19]+d*data[sample+19] d
	fstp	dword [edi + (19*4)]		; autoc[19]+=d*data[sample+19]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (18*4)]		; ST = d*data[sample+18] d
	fadd	dword [edi + (18*4)]		; ST = autoc[18]+d*data[sample+18] d
	fstp	dword [edi + (18*4)]		; autoc[18]+=d*data[sample+18]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (17*4)]		; ST = d*data[sample+17] d
	fadd	dword [edi + (17*4)]		; ST = autoc[17]+d*data[sample+17] d
	fstp	dword [edi + (17*4)]		; autoc[17]+=d*data[sample+17]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (16*4)]		; ST = d*data[sample+16] d
	fadd	dword [edi + (16*4)]		; ST = autoc[16]+d*data[sample+16] d
	fstp	dword [edi + (16*4)]		; autoc[16]+=d*data[sample+16]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (15*4)]		; ST = d*data[sample+15] d
	fadd	dword [edi + (15*4)]		; ST = autoc[15]+d*data[sample+15] d
	fstp	dword [edi + (15*4)]		; autoc[15]+=d*data[sample+15]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (14*4)]		; ST = d*data[sample+14] d
	fadd	dword [edi + (14*4)]		; ST = autoc[14]+d*data[sample+14] d
	fstp	dword [edi + (14*4)]		; autoc[14]+=d*data[sample+14]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (13*4)]		; ST = d*data[sample+13] d
	fadd	dword [edi + (13*4)]		; ST = autoc[13]+d*data[sample+13] d
	fstp	dword [edi + (13*4)]		; autoc[13]+=d*data[sample+13]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (12*4)]		; ST = d*data[sample+12] d
	fadd	dword [edi + (12*4)]		; ST = autoc[12]+d*data[sample+12] d
	fstp	dword [edi + (12*4)]		; autoc[12]+=d*data[sample+12]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (11*4)]		; ST = d*data[sample+11] d
	fadd	dword [edi + (11*4)]		; ST = autoc[11]+d*data[sample+11] d
	fstp	dword [edi + (11*4)]		; autoc[11]+=d*data[sample+11]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + (10*4)]		; ST = d*data[sample+10] d
	fadd	dword [edi + (10*4)]		; ST = autoc[10]+d*data[sample+10] d
	fstp	dword [edi + (10*4)]		; autoc[10]+=d*data[sample+10]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + ( 9*4)]		; ST = d*data[sample+9] d
	fadd	dword [edi + ( 9*4)]		; ST = autoc[9]+d*data[sample+9] d
	fstp	dword [edi + ( 9*4)]		; autoc[9]+=d*data[sample+9]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + ( 8*4)]		; ST = d*data[sample+8] d
	fadd	dword [edi + ( 8*4)]		; ST = autoc[8]+d*data[sample+8] d
	fstp	dword [edi + ( 8*4)]		; autoc[8]+=d*data[sample+8]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + ( 7*4)]		; ST = d*data[sample+7] d
	fadd	dword [edi + ( 7*4)]		; ST = autoc[7]+d*data[sample+7] d
	fstp	dword [edi + ( 7*4)]		; autoc[7]+=d*data[sample+7]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + ( 6*4)]		; ST = d*data[sample+6] d
	fadd	dword [edi + ( 6*4)]		; ST = autoc[6]+d*data[sample+6] d
	fstp	dword [edi + ( 6*4)]		; autoc[6]+=d*data[sample+6]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + ( 5*4)]		; ST = d*data[sample+4] d
	fadd	dword [edi + ( 5*4)]		; ST = autoc[4]+d*data[sample+4] d
	fstp	dword [edi + ( 5*4)]		; autoc[4]+=d*data[sample+4]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + ( 4*4)]		; ST = d*data[sample+4] d
	fadd	dword [edi + ( 4*4)]		; ST = autoc[4]+d*data[sample+4] d
	fstp	dword [edi + ( 4*4)]		; autoc[4]+=d*data[sample+4]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + ( 3*4)]		; ST = d*data[sample+3] d
	fadd	dword [edi + ( 3*4)]		; ST = autoc[3]+d*data[sample+3] d
	fstp	dword [edi + ( 3*4)]		; autoc[3]+=d*data[sample+3]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + ( 2*4)]		; ST = d*data[sample+2] d
	fadd	dword [edi + ( 2*4)]		; ST = autoc[2]+d*data[sample+2] d
	fstp	dword [edi + ( 2*4)]		; autoc[2]+=d*data[sample+2]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi + ( 1*4)]		; ST = d*data[sample+1] d
	fadd	dword [edi + ( 1*4)]		; ST = autoc[1]+d*data[sample+1] d
	fstp	dword [edi + ( 1*4)]		; autoc[1]+=d*data[sample+1]  ST = d
	fld	st0				; ST = d d
	fmul	dword [esi]			; ST = d*data[sample] d			WATCHOUT: no displacement byte here!
	fadd	dword [edi]			; ST = autoc[0]+d*data[sample] d	WATCHOUT: no displacement byte here!
	fstp	dword [edi]			; autoc[0]+=d*data[sample]  ST = d	WATCHOUT: no displacement byte here!
.jumper2_0:

	fstp	st0				; pop d, ST = empty
	add	esi, byte 4			; sample++
	dec	ecx
	jz	.loop2_end
	add	edx, byte 11			; adjust our inner loop counter by adjusting the jump target
	fld	dword [esi]			; ST = d <- data[sample]
	jmp	edx
.loop2_end:

.end:
	pop	ebx
	pop	edi
	pop	esi
	ret

	ALIGN 16
cident FLAC__lpc_compute_autocorrelation_asm_ia32_sse_lag_4
	;[esp + 16] == autoc[]
	;[esp + 12] == lag
	;[esp + 8] == data_len
	;[esp + 4] == data[]

	;ASSERT(lag > 0)
	;ASSERT(lag <= 4)
	;ASSERT(lag <= data_len)

	;	for(coeff = 0; coeff < lag; coeff++)
	;		autoc[coeff] = 0.0;
	xorps	xmm5, xmm5

	mov	edx, [esp + 8]			; edx == data_len
	mov	eax, [esp + 4]			; eax == &data[sample] <- &data[0]

	movss	xmm0, [eax]			; xmm0 = 0,0,0,data[0]
	add	eax, 4
	movaps	xmm2, xmm0			; xmm2 = 0,0,0,data[0]
	shufps	xmm0, xmm0, 0			; xmm0 == data[sample],data[sample],data[sample],data[sample] = data[0],data[0],data[0],data[0]
.warmup:					; xmm2 == data[sample-3],data[sample-2],data[sample-1],data[sample]
	mulps	xmm0, xmm2			; xmm0 = xmm0 * xmm2
	addps	xmm5, xmm0			; xmm5 += xmm0 * xmm2
	dec	edx
	jz	.loop_end
	ALIGN 16
.loop_start:
	; start by reading the next sample
	movss	xmm0, [eax]			; xmm0 = 0,0,0,data[sample]
	add	eax, 4
	shufps	xmm0, xmm0, 0			; xmm0 = data[sample],data[sample],data[sample],data[sample]
	shufps	xmm2, xmm2, 93h			; 93h=2-1-0-3 => xmm2 gets rotated left by one float
	movss	xmm2, xmm0
	mulps	xmm0, xmm2			; xmm0 = xmm0 * xmm2
	addps	xmm5, xmm0			; xmm5 += xmm0 * xmm2
	dec	edx
	jnz	.loop_start
.loop_end:
	; store autoc
	mov	edx, [esp + 16]			; edx == autoc
	movups	[edx], xmm5

.end:
	ret

	ALIGN 16
cident FLAC__lpc_compute_autocorrelation_asm_ia32_sse_lag_8
	;[esp + 16] == autoc[]
	;[esp + 12] == lag
	;[esp + 8] == data_len
	;[esp + 4] == data[]

	;ASSERT(lag > 0)
	;ASSERT(lag <= 8)
	;ASSERT(lag <= data_len)

	;	for(coeff = 0; coeff < lag; coeff++)
	;		autoc[coeff] = 0.0;
	xorps	xmm5, xmm5
	xorps	xmm6, xmm6

	mov	edx, [esp + 8]			; edx == data_len
	mov	eax, [esp + 4]			; eax == &data[sample] <- &data[0]

	movss	xmm0, [eax]			; xmm0 = 0,0,0,data[0]
	add	eax, 4
	movaps	xmm2, xmm0			; xmm2 = 0,0,0,data[0]
	shufps	xmm0, xmm0, 0			; xmm0 == data[sample],data[sample],data[sample],data[sample] = data[0],data[0],data[0],data[0]
	movaps	xmm1, xmm0			; xmm1 == data[sample],data[sample],data[sample],data[sample] = data[0],data[0],data[0],data[0]
	xorps	xmm3, xmm3			; xmm3 = 0,0,0,0
.warmup:					; xmm3:xmm2 == data[sample-7],data[sample-6],...,data[sample]
	mulps	xmm0, xmm2
	mulps	xmm1, xmm3			; xmm1:xmm0 = xmm1:xmm0 * xmm3:xmm2
	addps	xmm5, xmm0
	addps	xmm6, xmm1			; xmm6:xmm5 += xmm1:xmm0 * xmm3:xmm2
	dec	edx
	jz	.loop_end
	ALIGN 16
.loop_start:
	; start by reading the next sample
	movss	xmm0, [eax]			; xmm0 = 0,0,0,data[sample]
	; here we reorder the instructions; see the (#) indexes for a logical order
	shufps	xmm2, xmm2, 93h			; (3) 93h=2-1-0-3 => xmm2 gets rotated left by one float
	add	eax, 4				; (0)
	shufps	xmm3, xmm3, 93h			; (4) 93h=2-1-0-3 => xmm3 gets rotated left by one float
	shufps	xmm0, xmm0, 0			; (1) xmm0 = data[sample],data[sample],data[sample],data[sample]
	movss	xmm3, xmm2			; (5)
	movaps	xmm1, xmm0			; (2) xmm1 = data[sample],data[sample],data[sample],data[sample]
	movss	xmm2, xmm0			; (6)
	mulps	xmm1, xmm3			; (8)
	mulps	xmm0, xmm2			; (7) xmm1:xmm0 = xmm1:xmm0 * xmm3:xmm2
	addps	xmm6, xmm1			; (10)
	addps	xmm5, xmm0			; (9) xmm6:xmm5 += xmm1:xmm0 * xmm3:xmm2
	dec	edx
	jnz	.loop_start
.loop_end:
	; store autoc
	mov	edx, [esp + 16]			; edx == autoc
	movups	[edx], xmm5
	movups	[edx + 16], xmm6

.end:
	ret

	ALIGN 16
cident FLAC__lpc_compute_autocorrelation_asm_ia32_sse_lag_12
	;[esp + 16] == autoc[]
	;[esp + 12] == lag
	;[esp + 8] == data_len
	;[esp + 4] == data[]

	;ASSERT(lag > 0)
	;ASSERT(lag <= 12)
	;ASSERT(lag <= data_len)

	;	for(coeff = 0; coeff < lag; coeff++)
	;		autoc[coeff] = 0.0;
	xorps	xmm5, xmm5
	xorps	xmm6, xmm6
	xorps	xmm7, xmm7

	mov	edx, [esp + 8]			; edx == data_len
	mov	eax, [esp + 4]			; eax == &data[sample] <- &data[0]

	movss	xmm0, [eax]			; xmm0 = 0,0,0,data[0]
	add	eax, 4
	movaps	xmm2, xmm0			; xmm2 = 0,0,0,data[0]
	shufps	xmm0, xmm0, 0			; xmm0 == data[sample],data[sample],data[sample],data[sample] = data[0],data[0],data[0],data[0]
	xorps	xmm3, xmm3			; xmm3 = 0,0,0,0
	xorps	xmm4, xmm4			; xmm4 = 0,0,0,0
.warmup:					; xmm3:xmm2 == data[sample-7],data[sample-6],...,data[sample]
	movaps	xmm1, xmm0
	mulps	xmm1, xmm2
	addps	xmm5, xmm1
	movaps	xmm1, xmm0
	mulps	xmm1, xmm3
	addps	xmm6, xmm1
	mulps	xmm0, xmm4
	addps	xmm7, xmm0			; xmm7:xmm6:xmm5 += xmm0:xmm0:xmm0 * xmm4:xmm3:xmm2
	dec	edx
	jz	.loop_end
	ALIGN 16
.loop_start:
	; start by reading the next sample
	movss	xmm0, [eax]			; xmm0 = 0,0,0,data[sample]
	add	eax, 4
	shufps	xmm0, xmm0, 0			; xmm0 = data[sample],data[sample],data[sample],data[sample]

	; shift xmm4:xmm3:xmm2 left by one float
	shufps	xmm2, xmm2, 93h			; 93h=2-1-0-3 => xmm2 gets rotated left by one float
	shufps	xmm3, xmm3, 93h			; 93h=2-1-0-3 => xmm3 gets rotated left by one float
	shufps	xmm4, xmm4, 93h			; 93h=2-1-0-3 => xmm4 gets rotated left by one float
	movss	xmm4, xmm3
	movss	xmm3, xmm2
	movss	xmm2, xmm0

	; xmm7:xmm6:xmm5 += xmm0:xmm0:xmm0 * xmm3:xmm3:xmm2
	movaps	xmm1, xmm0
	mulps	xmm1, xmm2
	addps	xmm5, xmm1
	movaps	xmm1, xmm0
	mulps	xmm1, xmm3
	addps	xmm6, xmm1
	mulps	xmm0, xmm4
	addps	xmm7, xmm0

	dec	edx
	jnz	.loop_start
.loop_end:
	; store autoc
	mov	edx, [esp + 16]			; edx == autoc
	movups	[edx], xmm5
	movups	[edx + 16], xmm6
	movups	[edx + 32], xmm7

.end:
	ret

	ALIGN 16
cident FLAC__lpc_compute_autocorrelation_asm_ia32_3dnow
	;[ebp + 32] autoc
	;[ebp + 28] lag
	;[ebp + 24] data_len
	;[ebp + 20] data

	push	ebp
	push	ebx
	push	esi
	push	edi
	mov	ebp, esp

	mov	esi, [ebp + 20]
	mov	edi, [ebp + 24]
	mov	edx, [ebp + 28]
	inc	edx
	and	edx, byte -2
	mov	eax, edx
	neg	eax
	and	esp, byte -8
	lea	esp, [esp + 4 * eax]
	mov	ecx, edx
	xor	eax, eax
.loop0:
	dec	ecx
	mov	[esp + 4 * ecx], eax
	jnz	short .loop0

	mov	eax, edi
	sub	eax, edx
	mov	ebx, edx
	and	ebx, byte 1
	sub	eax, ebx
	lea	ecx, [esi + 4 * eax - 12]
	cmp	esi, ecx
	mov	eax, esi
	ja	short .loop2_pre
	ALIGN	16		;4 nops
.loop1_i:
	movd	mm0, [eax]
	movd	mm2, [eax + 4]
	movd	mm4, [eax + 8]
	movd	mm6, [eax + 12]
	mov	ebx, edx
	punpckldq	mm0, mm0
	punpckldq	mm2, mm2
	punpckldq	mm4, mm4
	punpckldq	mm6, mm6
	ALIGN	16		;3 nops
.loop1_j:
	sub	ebx, byte 2
	movd	mm1, [eax + 4 * ebx]
	movd	mm3, [eax + 4 * ebx + 4]
	movd	mm5, [eax + 4 * ebx + 8]
	movd	mm7, [eax + 4 * ebx + 12]
	punpckldq	mm1, mm3
	punpckldq	mm3, mm5
	pfmul	mm1, mm0
	punpckldq	mm5, mm7
	pfmul	mm3, mm2
	punpckldq	mm7, [eax + 4 * ebx + 16]
	pfmul	mm5, mm4
	pfmul	mm7, mm6
	pfadd	mm1, mm3
	movq	mm3, [esp + 4 * ebx]
	pfadd	mm5, mm7
	pfadd	mm1, mm5
	pfadd	mm3, mm1
	movq	[esp + 4 * ebx], mm3
	jg	short .loop1_j

	add	eax, byte 16
	cmp	eax, ecx
	jb	short .loop1_i

.loop2_pre:
	mov	ebx, eax
	sub	eax, esi
	shr	eax, 2
	lea	ecx, [esi + 4 * edi]
	mov	esi, ebx
.loop2_i:
	movd	mm0, [esi]
	mov	ebx, edi
	sub	ebx, eax
	cmp	ebx, edx
	jbe	short .loop2_j
	mov	ebx, edx
.loop2_j:
	dec	ebx
	movd	mm1, [esi + 4 * ebx]
	pfmul	mm1, mm0
	movd	mm2, [esp + 4 * ebx]
	pfadd	mm1, mm2
	movd	[esp + 4 * ebx], mm1

	jnz	short .loop2_j

	add	esi, byte 4
	inc	eax
	cmp	esi, ecx
	jnz	short .loop2_i

	mov	edi, [ebp + 32]
	mov	edx, [ebp + 28]
.loop3:
	dec	edx
	mov	eax, [esp + 4 * edx]
	mov	[edi + 4 * edx], eax
	jnz	short .loop3

	femms

	mov	esp, ebp
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret

;void FLAC__lpc_compute_residual_from_qlp_coefficients(const FLAC__int32 *data, unsigned data_len, const FLAC__int32 qlp_coeff[], unsigned order, int lp_quantization, FLAC__int32 residual[])
;
;	for(i = 0; i < data_len; i++) {
;		sum = 0;
;		for(j = 0; j < order; j++)
;			sum += qlp_coeff[j] * data[i-j-1];
;		residual[i] = data[i] - (sum >> lp_quantization);
;	}
;
	ALIGN	16
cident FLAC__lpc_compute_residual_from_qlp_coefficients_asm_ia32
	;[esp + 40]	residual[]
	;[esp + 36]	lp_quantization
	;[esp + 32]	order
	;[esp + 28]	qlp_coeff[]
	;[esp + 24]	data_len
	;[esp + 20]	data[]

	;ASSERT(order > 0)

	push	ebp
	push	ebx
	push	esi
	push	edi

	mov	esi, [esp + 20]			; esi = data[]
	mov	edi, [esp + 40]			; edi = residual[]
	mov	eax, [esp + 32]			; eax = order
	mov	ebx, [esp + 24]			; ebx = data_len

	test	ebx, ebx
	jz	near .end			; do nothing if data_len == 0
.begin:
	cmp	eax, byte 1
	jg	short .i_1more

	mov	ecx, [esp + 28]
	mov	edx, [ecx]			; edx = qlp_coeff[0]
	mov	eax, [esi - 4]			; eax = data[-1]
	mov	cl, [esp + 36]			; cl = lp_quantization
	ALIGN	16
.i_1_loop_i:
	imul	eax, edx
	sar	eax, cl
	neg	eax
	add	eax, [esi]
	mov	[edi], eax
	mov	eax, [esi]
	add	edi, byte 4
	add	esi, byte 4
	dec	ebx
	jnz	.i_1_loop_i

	jmp	.end

.i_1more:
	cmp	eax, byte 32			; for order <= 32 there is a faster routine
	jbe	short .i_32

	; This version is here just for completeness, since FLAC__MAX_LPC_ORDER == 32
	ALIGN 16
.i_32more_loop_i:
	xor	ebp, ebp
	mov	ecx, [esp + 32]
	mov	edx, ecx
	shl	edx, 2
	add	edx, [esp + 28]
	neg	ecx
	ALIGN	16
.i_32more_loop_j:
	sub	edx, byte 4
	mov	eax, [edx]
	imul	eax, [esi + 4 * ecx]
	add	ebp, eax
	inc	ecx
	jnz	short .i_32more_loop_j

	mov	cl, [esp + 36]
	sar	ebp, cl
	neg	ebp
	add	ebp, [esi]
	mov	[edi], ebp
	add	esi, byte 4
	add	edi, byte 4

	dec	ebx
	jnz	.i_32more_loop_i

	jmp	.end

.i_32:
	sub	edi, esi
	neg	eax
	lea	edx, [eax + eax * 8 + .jumper_0 - .get_eip0]
	call	.get_eip0
.get_eip0:
	pop	eax
	add	edx, eax
	inc	edx
	mov	eax, [esp + 28]			; eax = qlp_coeff[]
	xor	ebp, ebp
	jmp	edx

	mov	ecx, [eax + 124]
	imul	ecx, [esi - 128]
	add	ebp, ecx
	mov	ecx, [eax + 120]
	imul	ecx, [esi - 124]
	add	ebp, ecx
	mov	ecx, [eax + 116]
	imul	ecx, [esi - 120]
	add	ebp, ecx
	mov	ecx, [eax + 112]
	imul	ecx, [esi - 116]
	add	ebp, ecx
	mov	ecx, [eax + 108]
	imul	ecx, [esi - 112]
	add	ebp, ecx
	mov	ecx, [eax + 104]
	imul	ecx, [esi - 108]
	add	ebp, ecx
	mov	ecx, [eax + 100]
	imul	ecx, [esi - 104]
	add	ebp, ecx
	mov	ecx, [eax + 96]
	imul	ecx, [esi - 100]
	add	ebp, ecx
	mov	ecx, [eax + 92]
	imul	ecx, [esi - 96]
	add	ebp, ecx
	mov	ecx, [eax + 88]
	imul	ecx, [esi - 92]
	add	ebp, ecx
	mov	ecx, [eax + 84]
	imul	ecx, [esi - 88]
	add	ebp, ecx
	mov	ecx, [eax + 80]
	imul	ecx, [esi - 84]
	add	ebp, ecx
	mov	ecx, [eax + 76]
	imul	ecx, [esi - 80]
	add	ebp, ecx
	mov	ecx, [eax + 72]
	imul	ecx, [esi - 76]
	add	ebp, ecx
	mov	ecx, [eax + 68]
	imul	ecx, [esi - 72]
	add	ebp, ecx
	mov	ecx, [eax + 64]
	imul	ecx, [esi - 68]
	add	ebp, ecx
	mov	ecx, [eax + 60]
	imul	ecx, [esi - 64]
	add	ebp, ecx
	mov	ecx, [eax + 56]
	imul	ecx, [esi - 60]
	add	ebp, ecx
	mov	ecx, [eax + 52]
	imul	ecx, [esi - 56]
	add	ebp, ecx
	mov	ecx, [eax + 48]
	imul	ecx, [esi - 52]
	add	ebp, ecx
	mov	ecx, [eax + 44]
	imul	ecx, [esi - 48]
	add	ebp, ecx
	mov	ecx, [eax + 40]
	imul	ecx, [esi - 44]
	add	ebp, ecx
	mov	ecx, [eax + 36]
	imul	ecx, [esi - 40]
	add	ebp, ecx
	mov	ecx, [eax + 32]
	imul	ecx, [esi - 36]
	add	ebp, ecx
	mov	ecx, [eax + 28]
	imul	ecx, [esi - 32]
	add	ebp, ecx
	mov	ecx, [eax + 24]
	imul	ecx, [esi - 28]
	add	ebp, ecx
	mov	ecx, [eax + 20]
	imul	ecx, [esi - 24]
	add	ebp, ecx
	mov	ecx, [eax + 16]
	imul	ecx, [esi - 20]
	add	ebp, ecx
	mov	ecx, [eax + 12]
	imul	ecx, [esi - 16]
	add	ebp, ecx
	mov	ecx, [eax + 8]
	imul	ecx, [esi - 12]
	add	ebp, ecx
	mov	ecx, [eax + 4]
	imul	ecx, [esi - 8]
	add	ebp, ecx
	mov	ecx, [eax]			; there is one byte missing
	imul	ecx, [esi - 4]
	add	ebp, ecx
.jumper_0:

	mov	cl, [esp + 36]
	sar	ebp, cl
	neg	ebp
	add	ebp, [esi]
	mov	[edi + esi], ebp
	add	esi, byte 4

	dec	ebx
	jz	short .end
	xor	ebp, ebp
	jmp	edx

.end:
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret

; WATCHOUT: this routine works on 16 bit data which means bits-per-sample for
; the channel and qlp_coeffs must be <= 16.  Especially note that this routine
; cannot be used for side-channel coded 16bps channels since the effective bps
; is 17.
	ALIGN	16
cident FLAC__lpc_compute_residual_from_qlp_coefficients_asm_ia32_mmx
	;[esp + 40]	residual[]
	;[esp + 36]	lp_quantization
	;[esp + 32]	order
	;[esp + 28]	qlp_coeff[]
	;[esp + 24]	data_len
	;[esp + 20]	data[]

	;ASSERT(order > 0)

	push	ebp
	push	ebx
	push	esi
	push	edi

	mov	esi, [esp + 20]			; esi = data[]
	mov	edi, [esp + 40]			; edi = residual[]
	mov	eax, [esp + 32]			; eax = order
	mov	ebx, [esp + 24]			; ebx = data_len

	test	ebx, ebx
	jz	near .end			; do nothing if data_len == 0
	dec	ebx
	test	ebx, ebx
	jz	near .last_one

	mov	edx, [esp + 28]			; edx = qlp_coeff[]
	movd	mm6, [esp + 36]			; mm6 = 0:lp_quantization
	mov	ebp, esp

	and	esp, 0xfffffff8

	xor	ecx, ecx
.copy_qlp_loop:
	push	word [edx + 4 * ecx]
	inc	ecx
	cmp	ecx, eax
	jnz	short .copy_qlp_loop

	and	ecx, 0x3
	test	ecx, ecx
	je	short .za_end
	sub	ecx, byte 4
.za_loop:
	push	word 0
	inc	eax
	inc	ecx
	jnz	short .za_loop
.za_end:

	movq	mm5, [esp + 2 * eax - 8]
	movd	mm4, [esi - 16]
	punpckldq	mm4, [esi - 12]
	movd	mm0, [esi - 8]
	punpckldq	mm0, [esi - 4]
	packssdw	mm4, mm0

	cmp	eax, byte 4
	jnbe	short .mmx_4more

	ALIGN	16
.mmx_4_loop_i:
	movd	mm1, [esi]
	movq	mm3, mm4
	punpckldq	mm1, [esi + 4]
	psrlq	mm4, 16
	movq	mm0, mm1
	psllq	mm0, 48
	por	mm4, mm0
	movq	mm2, mm4
	psrlq	mm4, 16
	pxor	mm0, mm0
	punpckhdq	mm0, mm1
	pmaddwd	mm3, mm5
	pmaddwd	mm2, mm5
	psllq	mm0, 16
	por	mm4, mm0
	movq	mm0, mm3
	punpckldq	mm3, mm2
	punpckhdq	mm0, mm2
	paddd	mm3, mm0
	psrad	mm3, mm6
	psubd	mm1, mm3
	movd	[edi], mm1
	punpckhdq	mm1, mm1
	movd	[edi + 4], mm1

	add	edi, byte 8
	add	esi, byte 8

	sub	ebx, 2
	jg	.mmx_4_loop_i
	jmp	.mmx_end

.mmx_4more:
	shl	eax, 2
	neg	eax
	add	eax, byte 16

	ALIGN	16
.mmx_4more_loop_i:
	movd	mm1, [esi]
	punpckldq	mm1, [esi + 4]
	movq	mm3, mm4
	psrlq	mm4, 16
	movq	mm0, mm1
	psllq	mm0, 48
	por	mm4, mm0
	movq	mm2, mm4
	psrlq	mm4, 16
	pxor	mm0, mm0
	punpckhdq	mm0, mm1
	pmaddwd	mm3, mm5
	pmaddwd	mm2, mm5
	psllq	mm0, 16
	por	mm4, mm0

	mov	ecx, esi
	add	ecx, eax
	mov	edx, esp

	ALIGN	16
.mmx_4more_loop_j:
	movd	mm0, [ecx - 16]
	movd	mm7, [ecx - 8]
	punpckldq	mm0, [ecx - 12]
	punpckldq	mm7, [ecx - 4]
	packssdw	mm0, mm7
	pmaddwd	mm0, [edx]
	punpckhdq	mm7, mm7
	paddd	mm3, mm0
	movd	mm0, [ecx - 12]
	punpckldq	mm0, [ecx - 8]
	punpckldq	mm7, [ecx]
	packssdw	mm0, mm7
	pmaddwd	mm0, [edx]
	paddd	mm2, mm0

	add	edx, byte 8
	add	ecx, byte 16
	cmp	ecx, esi
	jnz	.mmx_4more_loop_j

	movq	mm0, mm3
	punpckldq	mm3, mm2
	punpckhdq	mm0, mm2
	paddd	mm3, mm0
	psrad	mm3, mm6
	psubd	mm1, mm3
	movd	[edi], mm1
	punpckhdq	mm1, mm1
	movd	[edi + 4], mm1

	add	edi, byte 8
	add	esi, byte 8

	sub	ebx, 2
	jg	near .mmx_4more_loop_i

.mmx_end:
	emms
	mov	esp, ebp
.last_one:
	mov	eax, [esp + 32]
	inc	ebx
	jnz	near FLAC__lpc_compute_residual_from_qlp_coefficients_asm_ia32.begin

.end:
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret

; **********************************************************************
;
; void FLAC__lpc_restore_signal(const FLAC__int32 residual[], unsigned data_len, const FLAC__int32 qlp_coeff[], unsigned order, int lp_quantization, FLAC__int32 data[])
; {
; 	unsigned i, j;
; 	FLAC__int32 sum;
;
; 	FLAC__ASSERT(order > 0);
;
; 	for(i = 0; i < data_len; i++) {
; 		sum = 0;
; 		for(j = 0; j < order; j++)
; 			sum += qlp_coeff[j] * data[i-j-1];
; 		data[i] = residual[i] + (sum >> lp_quantization);
; 	}
; }
	ALIGN	16
cident FLAC__lpc_restore_signal_asm_ia32
	;[esp + 40]	data[]
	;[esp + 36]	lp_quantization
	;[esp + 32]	order
	;[esp + 28]	qlp_coeff[]
	;[esp + 24]	data_len
	;[esp + 20]	residual[]

	;ASSERT(order > 0)

	push	ebp
	push	ebx
	push	esi
	push	edi

	mov	esi, [esp + 20]			; esi = residual[]
	mov	edi, [esp + 40]			; edi = data[]
	mov	eax, [esp + 32]			; eax = order
	mov	ebx, [esp + 24]			; ebx = data_len

	test	ebx, ebx
	jz	near .end			; do nothing if data_len == 0

.begin:
	cmp	eax, byte 1
	jg	short .x87_1more

	mov	ecx, [esp + 28]
	mov	edx, [ecx]
	mov	eax, [edi - 4]
	mov	cl, [esp + 36]
	ALIGN	16
.x87_1_loop_i:
	imul	eax, edx
	sar	eax, cl
	add	eax, [esi]
	mov	[edi], eax
	add	esi, byte 4
	add	edi, byte 4
	dec	ebx
	jnz	.x87_1_loop_i

	jmp	.end

.x87_1more:
	cmp	eax, byte 32			; for order <= 32 there is a faster routine
	jbe	short .x87_32

	; This version is here just for completeness, since FLAC__MAX_LPC_ORDER == 32
	ALIGN 16
.x87_32more_loop_i:
	xor	ebp, ebp
	mov	ecx, [esp + 32]
	mov	edx, ecx
	shl	edx, 2
	add	edx, [esp + 28]
	neg	ecx
	ALIGN	16
.x87_32more_loop_j:
	sub	edx, byte 4
	mov	eax, [edx]
	imul	eax, [edi + 4 * ecx]
	add	ebp, eax
	inc	ecx
	jnz	short .x87_32more_loop_j

	mov	cl, [esp + 36]
	sar	ebp, cl
	add	ebp, [esi]
	mov	[edi], ebp
	add	edi, byte 4
	add	esi, byte 4

	dec	ebx
	jnz	.x87_32more_loop_i

	jmp	.end

.x87_32:
	sub	esi, edi
	neg	eax
	lea	edx, [eax + eax * 8 + .jumper_0 - .get_eip0]
	call	.get_eip0
.get_eip0:
	pop	eax
	add	edx, eax
	inc	edx				; compensate for the shorter opcode on the last iteration
	mov	eax, [esp + 28]			; eax = qlp_coeff[]
	xor	ebp, ebp
	jmp	edx

	mov	ecx, [eax + 124]		; ecx =  qlp_coeff[31]
	imul	ecx, [edi - 128]		; ecx =  qlp_coeff[31] * data[i-32]
	add	ebp, ecx			; sum += qlp_coeff[31] * data[i-32]
	mov	ecx, [eax + 120]		; ecx =  qlp_coeff[30]
	imul	ecx, [edi - 124]		; ecx =  qlp_coeff[30] * data[i-31]
	add	ebp, ecx			; sum += qlp_coeff[30] * data[i-31]
	mov	ecx, [eax + 116]		; ecx =  qlp_coeff[29]
	imul	ecx, [edi - 120]		; ecx =  qlp_coeff[29] * data[i-30]
	add	ebp, ecx			; sum += qlp_coeff[29] * data[i-30]
	mov	ecx, [eax + 112]		; ecx =  qlp_coeff[28]
	imul	ecx, [edi - 116]		; ecx =  qlp_coeff[28] * data[i-29]
	add	ebp, ecx			; sum += qlp_coeff[28] * data[i-29]
	mov	ecx, [eax + 108]		; ecx =  qlp_coeff[27]
	imul	ecx, [edi - 112]		; ecx =  qlp_coeff[27] * data[i-28]
	add	ebp, ecx			; sum += qlp_coeff[27] * data[i-28]
	mov	ecx, [eax + 104]		; ecx =  qlp_coeff[26]
	imul	ecx, [edi - 108]		; ecx =  qlp_coeff[26] * data[i-27]
	add	ebp, ecx			; sum += qlp_coeff[26] * data[i-27]
	mov	ecx, [eax + 100]		; ecx =  qlp_coeff[25]
	imul	ecx, [edi - 104]		; ecx =  qlp_coeff[25] * data[i-26]
	add	ebp, ecx			; sum += qlp_coeff[25] * data[i-26]
	mov	ecx, [eax + 96]			; ecx =  qlp_coeff[24]
	imul	ecx, [edi - 100]		; ecx =  qlp_coeff[24] * data[i-25]
	add	ebp, ecx			; sum += qlp_coeff[24] * data[i-25]
	mov	ecx, [eax + 92]			; ecx =  qlp_coeff[23]
	imul	ecx, [edi - 96]			; ecx =  qlp_coeff[23] * data[i-24]
	add	ebp, ecx			; sum += qlp_coeff[23] * data[i-24]
	mov	ecx, [eax + 88]			; ecx =  qlp_coeff[22]
	imul	ecx, [edi - 92]			; ecx =  qlp_coeff[22] * data[i-23]
	add	ebp, ecx			; sum += qlp_coeff[22] * data[i-23]
	mov	ecx, [eax + 84]			; ecx =  qlp_coeff[21]
	imul	ecx, [edi - 88]			; ecx =  qlp_coeff[21] * data[i-22]
	add	ebp, ecx			; sum += qlp_coeff[21] * data[i-22]
	mov	ecx, [eax + 80]			; ecx =  qlp_coeff[20]
	imul	ecx, [edi - 84]			; ecx =  qlp_coeff[20] * data[i-21]
	add	ebp, ecx			; sum += qlp_coeff[20] * data[i-21]
	mov	ecx, [eax + 76]			; ecx =  qlp_coeff[19]
	imul	ecx, [edi - 80]			; ecx =  qlp_coeff[19] * data[i-20]
	add	ebp, ecx			; sum += qlp_coeff[19] * data[i-20]
	mov	ecx, [eax + 72]			; ecx =  qlp_coeff[18]
	imul	ecx, [edi - 76]			; ecx =  qlp_coeff[18] * data[i-19]
	add	ebp, ecx			; sum += qlp_coeff[18] * data[i-19]
	mov	ecx, [eax + 68]			; ecx =  qlp_coeff[17]
	imul	ecx, [edi - 72]			; ecx =  qlp_coeff[17] * data[i-18]
	add	ebp, ecx			; sum += qlp_coeff[17] * data[i-18]
	mov	ecx, [eax + 64]			; ecx =  qlp_coeff[16]
	imul	ecx, [edi - 68]			; ecx =  qlp_coeff[16] * data[i-17]
	add	ebp, ecx			; sum += qlp_coeff[16] * data[i-17]
	mov	ecx, [eax + 60]			; ecx =  qlp_coeff[15]
	imul	ecx, [edi - 64]			; ecx =  qlp_coeff[15] * data[i-16]
	add	ebp, ecx			; sum += qlp_coeff[15] * data[i-16]
	mov	ecx, [eax + 56]			; ecx =  qlp_coeff[14]
	imul	ecx, [edi - 60]			; ecx =  qlp_coeff[14] * data[i-15]
	add	ebp, ecx			; sum += qlp_coeff[14] * data[i-15]
	mov	ecx, [eax + 52]			; ecx =  qlp_coeff[13]
	imul	ecx, [edi - 56]			; ecx =  qlp_coeff[13] * data[i-14]
	add	ebp, ecx			; sum += qlp_coeff[13] * data[i-14]
	mov	ecx, [eax + 48]			; ecx =  qlp_coeff[12]
	imul	ecx, [edi - 52]			; ecx =  qlp_coeff[12] * data[i-13]
	add	ebp, ecx			; sum += qlp_coeff[12] * data[i-13]
	mov	ecx, [eax + 44]			; ecx =  qlp_coeff[11]
	imul	ecx, [edi - 48]			; ecx =  qlp_coeff[11] * data[i-12]
	add	ebp, ecx			; sum += qlp_coeff[11] * data[i-12]
	mov	ecx, [eax + 40]			; ecx =  qlp_coeff[10]
	imul	ecx, [edi - 44]			; ecx =  qlp_coeff[10] * data[i-11]
	add	ebp, ecx			; sum += qlp_coeff[10] * data[i-11]
	mov	ecx, [eax + 36]			; ecx =  qlp_coeff[ 9]
	imul	ecx, [edi - 40]			; ecx =  qlp_coeff[ 9] * data[i-10]
	add	ebp, ecx			; sum += qlp_coeff[ 9] * data[i-10]
	mov	ecx, [eax + 32]			; ecx =  qlp_coeff[ 8]
	imul	ecx, [edi - 36]			; ecx =  qlp_coeff[ 8] * data[i- 9]
	add	ebp, ecx			; sum += qlp_coeff[ 8] * data[i- 9]
	mov	ecx, [eax + 28]			; ecx =  qlp_coeff[ 7]
	imul	ecx, [edi - 32]			; ecx =  qlp_coeff[ 7] * data[i- 8]
	add	ebp, ecx			; sum += qlp_coeff[ 7] * data[i- 8]
	mov	ecx, [eax + 24]			; ecx =  qlp_coeff[ 6]
	imul	ecx, [edi - 28]			; ecx =  qlp_coeff[ 6] * data[i- 7]
	add	ebp, ecx			; sum += qlp_coeff[ 6] * data[i- 7]
	mov	ecx, [eax + 20]			; ecx =  qlp_coeff[ 5]
	imul	ecx, [edi - 24]			; ecx =  qlp_coeff[ 5] * data[i- 6]
	add	ebp, ecx			; sum += qlp_coeff[ 5] * data[i- 6]
	mov	ecx, [eax + 16]			; ecx =  qlp_coeff[ 4]
	imul	ecx, [edi - 20]			; ecx =  qlp_coeff[ 4] * data[i- 5]
	add	ebp, ecx			; sum += qlp_coeff[ 4] * data[i- 5]
	mov	ecx, [eax + 12]			; ecx =  qlp_coeff[ 3]
	imul	ecx, [edi - 16]			; ecx =  qlp_coeff[ 3] * data[i- 4]
	add	ebp, ecx			; sum += qlp_coeff[ 3] * data[i- 4]
	mov	ecx, [eax + 8]			; ecx =  qlp_coeff[ 2]
	imul	ecx, [edi - 12]			; ecx =  qlp_coeff[ 2] * data[i- 3]
	add	ebp, ecx			; sum += qlp_coeff[ 2] * data[i- 3]
	mov	ecx, [eax + 4]			; ecx =  qlp_coeff[ 1]
	imul	ecx, [edi - 8]			; ecx =  qlp_coeff[ 1] * data[i- 2]
	add	ebp, ecx			; sum += qlp_coeff[ 1] * data[i- 2]
	mov	ecx, [eax]			; ecx =  qlp_coeff[ 0] (NOTE: one byte missing from instruction)
	imul	ecx, [edi - 4]			; ecx =  qlp_coeff[ 0] * data[i- 1]
	add	ebp, ecx			; sum += qlp_coeff[ 0] * data[i- 1]
.jumper_0:

	mov	cl, [esp + 36]
	sar	ebp, cl				; ebp = (sum >> lp_quantization)
	add	ebp, [esi + edi]		; ebp = residual[i] + (sum >> lp_quantization)
	mov	[edi], ebp			; data[i] = residual[i] + (sum >> lp_quantization)
	add	edi, byte 4

	dec	ebx
	jz	short .end
	xor	ebp, ebp
	jmp	edx

.end:
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret

; WATCHOUT: this routine works on 16 bit data which means bits-per-sample for
; the channel and qlp_coeffs must be <= 16.  Especially note that this routine
; cannot be used for side-channel coded 16bps channels since the effective bps
; is 17.
; WATCHOUT: this routine requires that each data array have a buffer of up to
; 3 zeroes in front (at negative indices) for alignment purposes, i.e. for each
; channel n, data[n][-1] through data[n][-3] should be accessible and zero.
	ALIGN	16
cident FLAC__lpc_restore_signal_asm_ia32_mmx
	;[esp + 40]	data[]
	;[esp + 36]	lp_quantization
	;[esp + 32]	order
	;[esp + 28]	qlp_coeff[]
	;[esp + 24]	data_len
	;[esp + 20]	residual[]

	;ASSERT(order > 0)

	push	ebp
	push	ebx
	push	esi
	push	edi

	mov	esi, [esp + 20]
	mov	edi, [esp + 40]
	mov	eax, [esp + 32]
	mov	ebx, [esp + 24]

	test	ebx, ebx
	jz	near .end			; do nothing if data_len == 0
	cmp	eax, byte 4
	jb	near FLAC__lpc_restore_signal_asm_ia32.begin

	mov	edx, [esp + 28]
	movd	mm6, [esp + 36]
	mov	ebp, esp

	and	esp, 0xfffffff8

	xor	ecx, ecx
.copy_qlp_loop:
	push	word [edx + 4 * ecx]
	inc	ecx
	cmp	ecx, eax
	jnz	short .copy_qlp_loop

	and	ecx, 0x3
	test	ecx, ecx
	je	short .za_end
	sub	ecx, byte 4
.za_loop:
	push	word 0
	inc	eax
	inc	ecx
	jnz	short .za_loop
.za_end:

	movq	mm5, [esp + 2 * eax - 8]
	movd	mm4, [edi - 16]
	punpckldq	mm4, [edi - 12]
	movd	mm0, [edi - 8]
	punpckldq	mm0, [edi - 4]
	packssdw	mm4, mm0

	cmp	eax, byte 4
	jnbe	short .mmx_4more

	ALIGN	16
.mmx_4_loop_i:
	movq	mm7, mm4
	pmaddwd	mm7, mm5
	movq	mm0, mm7
	punpckhdq	mm7, mm7
	paddd	mm7, mm0
	psrad	mm7, mm6
	movd	mm1, [esi]
	paddd	mm7, mm1
	movd	[edi], mm7
	psllq	mm7, 48
	psrlq	mm4, 16
	por	mm4, mm7

	add	esi, byte 4
	add	edi, byte 4

	dec	ebx
	jnz	.mmx_4_loop_i
	jmp	.mmx_end
.mmx_4more:
	shl	eax, 2
	neg	eax
	add	eax, byte 16
	ALIGN	16
.mmx_4more_loop_i:
	mov	ecx, edi
	add	ecx, eax
	mov	edx, esp

	movq	mm7, mm4
	pmaddwd	mm7, mm5

	ALIGN	16
.mmx_4more_loop_j:
	movd	mm0, [ecx - 16]
	punpckldq	mm0, [ecx - 12]
	movd	mm1, [ecx - 8]
	punpckldq	mm1, [ecx - 4]
	packssdw	mm0, mm1
	pmaddwd	mm0, [edx]
	paddd	mm7, mm0

	add	edx, byte 8
	add	ecx, byte 16
	cmp	ecx, edi
	jnz	.mmx_4more_loop_j

	movq	mm0, mm7
	punpckhdq	mm7, mm7
	paddd	mm7, mm0
	psrad	mm7, mm6
	movd	mm1, [esi]
	paddd	mm7, mm1
	movd	[edi], mm7
	psllq	mm7, 48
	psrlq	mm4, 16
	por	mm4, mm7

	add	esi, byte 4
	add	edi, byte 4

	dec	ebx
	jnz	short .mmx_4more_loop_i
.mmx_end:
	emms
	mov	esp, ebp

.end:
	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret

end

%ifdef OBJ_FORMAT_elf
       section .note.GNU-stack noalloc
%endif
