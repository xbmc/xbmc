; back port from GOGO-no coda 2.24b by Takehiro TOMINAGA

; GOGO-no-coda
;	Copyright (C) 1999 shigeo
;	special thanks to URURI

%include "nasm.h"

	externdef costab_fft
	externdef sintab_fft

	segment_data
	align 32
D_1_41421	dd	1.41421356
D_1_0	dd	1.0
D_0_5	dd	0.5
D_0_25	dd	0.25
D_0_0005	dd	0.0005
D_0_0	dd	0.0

	segment_code

;void fht(float *fz, int n);
proc	fht_FPU

%$fz	arg	4
%$n	arg	4

%$k	local	4

%$f0	local	4
%$f1	local	4
%$f2	local	4
%$f3	local	4

%$g0	local	4
%$g1	local	4
%$g2	local	4
%$g3	local	4

%$s1	local	4
%$c1	local	4
%$s2	local	4
%$c2	local	4

%$t_s	local	4
%$t_c	local	4
	alloc

	pushd	ebp, ebx, esi, edi

fht_FPU_1st_part:

fht_FPU_2nd_part:

fht_FPU_3rd_part:

.do_init:
	mov	r3, 16		;k1*fsize = 4*fsize = k4
	mov	r4, 8		;kx = k1/2
	mov	r2, 48		;k3*fsize
	mov	dword [sp(%$k)], 2	;k = 2
	mov	r0, [sp(%$fz)]	;fi
	lea	r1, [r0+8]		;gi = fi + kx

.do:
.do2:
	;f
	fld	dword [r0]
	fsub	dword [r0+r3]

	fld	dword [r0]
	fadd	dword [r0+r3]

	fld	dword [r0+r3*2]
	fsub	dword [r0+r2]

	fld	dword [r0+r3*2]
	fadd	dword [r0+r2]		;f2 f3 f0 f1

	fld	st2			;f0 f2 f3 f0 f1
	fadd	st0, st1
	fstp	dword [r0]		;fi[0]

	fld	st3			;f1 f2 f3 f0 f1
	fadd	st0, st2
	fstp	dword [r0+r3]		;fi[k1]

	fsubr	st0, st2		;f0-f2 f3 f0 f1
	fstp	dword [r0+r3*2]		;fi[k2]

	fsubr	st0, st2		;f1-f3 f0 f1
	fstp	dword [r0+r2]		;fi[k3]
	fcompp

	;g
	fld	dword [r1]
	fsub	dword [r1+r3]

	fld	dword [r1]
	fadd	dword [r1+r3]

	fld	dword [D_1_41421]
	fmul	dword [r1+r2]

	fld	dword [D_1_41421]
	fmul	dword [r1+r3*2]		;g2 g3 g0 g1

	fld	st2			;g0 g2 g3 g0 g1
	fadd	st0, st1
	fstp	dword [r1]		;gi[0]

	fld	st3			;g1 g2 g3 g0 g1
	fadd	st0, st2
	fstp	dword [r1+r3]		;gi[k1]

	fsubr	st0, st2		;g0-g2 g3 g0 g1
	fstp	dword [r1+r3*2]		;gi[k2]

	fsubr	st0, st2		;g1-g3 g0 g1
	fstp	dword [r1+r2]		;gi[k3]
	fcompp

	lea	r0, [r0+r3*4]
	lea	r1, [r1+r3*4]
	cmp	r0, r6
	jb	.do2


	mov	r0, [sp(%$k)]
	fld	dword [costab_fft +r0*4]
	fstp	dword [sp(%$t_c)]
	fld	dword [sintab_fft +r0*4]
	fstp	dword [sp(%$t_s)]
	fld	dword [D_1_0]
	fstp	dword [sp(%$c1)]
	fld	dword [D_0_0]
	fstp	dword [sp(%$s1)]

.for_init:
	mov	r5, 4		;i = 1*fsize

.for:
	fld	dword [sp(%$c1)]
	fmul	dword [sp(%$t_c)]
	fld	dword [sp(%$s1)]
	fmul	dword [sp(%$t_s)]
	fsubp	st1, st0		;c1

	fld	dword [sp(%$c1)]
	fmul	dword [sp(%$t_s)]
	fld	dword [sp(%$s1)]
	fmul	dword [sp(%$t_c)]
	faddp	st1, st0		;s1 c1
	
	fld	st1
	fmul	st0, st0		;c1c1 s1 c1
	fld	st1
	fmul	st0, st0		;s1s1 c1c1 s1 c1
	fsubp	st1, st0		;c2 s1 c1
	fstp	dword [sp(%$c2)]	;s1 c1

	fld	st1			;c1 s1 c1
	fmul	st0, st1		;c1s1 s1 c1
	fadd	st0, st0		;s2 s1 c1
	fstp	dword [sp(%$s2)]	;s1 c1

	fstp	dword [sp(%$s1)]	;c1
	fstp	dword [sp(%$c1)]	;
	
	mov	r0, [sp(%$fz)]
	add	r0, r5		;r0 = fi
	mov	r1, [sp(%$fz)]
	add	r1, r3
	sub	r1, r5		;r1 = gi

.do3:
	fld	dword [sp(%$s2)]
	fmul	dword [r0+r3]
	fld	dword [sp(%$c2)]
	fmul	dword [r1+r3]
	fsubp	st1, st0		;b = s2*fi[k1] - c2*gi[k1]

	fld	dword [sp(%$c2)]
	fmul	dword [r0+r3]
	fld	dword [sp(%$s2)]
	fmul	dword [r1+r3]
	faddp	st1, st0		;a = c2*fi[k1] + s2*gi[k1]  b

	fld	dword [r0]
	fsub	st0, st1		;f1 a b
	fstp	dword [sp(%$f1)]	;a b

	fadd	dword [r0]		;f0 b
	fstp	dword [sp(%$f0)]	;b

	fld	dword [r1]
	fsub	st0, st1		;g1 b
	fstp	dword [sp(%$g1)]	;b

	fadd	dword [r1]		;g0
	fstp	dword [sp(%$g0)]	;


	fld	dword [sp(%$s2)]
	fmul	dword [r0+r2]
	fld	dword [sp(%$c2)]
	fmul	dword [r1+r2]
	fsubp	st1, st0		;b = s2*fi[k3] - c2*gi[k3]

	fld	dword [sp(%$c2)]
	fmul	dword [r0+r2]
	fld	dword [sp(%$s2)]
	fmul	dword [r1+r2]
	faddp	st1, st0		;a = c2*fi[k3] + s2*gi[k3]  b

	fld	dword [r0+r3*2]
	fsub	st0, st1		;f3 a b
	fstp	dword [sp(%$f3)]	;a b

	fadd	dword [r0+r3*2]	;f2 b
	fstp	dword [sp(%$f2)]	;b

	fld	dword [r1+r3*2]
	fsub	st0, st1		;g3 b
	fstp	dword [sp(%$g3)]	;b

	fadd	dword [r1+r3*2]	;g2
	fstp	dword [sp(%$g2)]	;


	fld	dword [sp(%$s1)]
	fmul	dword [sp(%$f2)]
	fld	dword [sp(%$c1)]
	fmul	dword [sp(%$g3)]
	fsubp	st1, st0		;b = s1*f2 - c1*g3
	
	fld	dword [sp(%$c1)]
	fmul	dword [sp(%$f2)]
	fld	dword [sp(%$s1)]
	fmul	dword [sp(%$g3)]
	faddp	st1, st0		;a = c1*f2 + s1*g3  b

	fld	dword [sp(%$f0)]
	fsub	st0, st1		;fi[k2] a b
	fstp	dword [r0+r3*2]

	fadd	dword [sp(%$f0)]	;fi[0] b
	fstp	dword [r0]

	fld	dword [sp(%$g1)]
	fsub	st0, st1		;gi[k3] b
	fstp	dword [r1+r2]

	fadd	dword [sp(%$g1)]	;gi[k1]
	fstp	dword [r1+r3]


	fld	dword [sp(%$c1)]
	fmul	dword [sp(%$g2)]
	fld	dword [sp(%$s1)]
	fmul	dword [sp(%$f3)]
	fsubp	st1, st0		;b = c1*g2 - s1*f3
	
	fld	dword [sp(%$s1)]
	fmul	dword [sp(%$g2)]
	fld	dword [sp(%$c1)]
	fmul	dword [sp(%$f3)]
	faddp	st1, st0		;a = s1*g2 + c1*f3  b

	fld	dword [sp(%$g0)]
	fsub	st0, st1		;gi[k2] a b
	fstp	dword [r1+r3*2]

	fadd	dword [sp(%$g0)]	;gi[0] b
	fstp	dword [r1]

	fld	dword [sp(%$f1)]
	fsub	st0, st1		;fi[k3] b
	fstp	dword [r0+r2]

	fadd	dword [sp(%$f1)]	;fi[k1]
	fstp	dword [r0+r3]


	lea	r0, [r0+r3*4]
	lea	r1, [r1+r3*4]
	cmp	r0, r6
	jb near	.do3

	add	r5, 4
	cmp	r5, r4
	jb near	.for

	cmp	r3, [sp(%$n)]
	jae	.exit

	add	dword [sp(%$k)], 2	;k  += 2;
	lea	r3, [r3*4]		;k1 *= 4
	lea	r2, [r2*4]		;k3 *= 4
	lea	r4, [r4*4]		;kx *= 4
	mov	r0, [sp(%$fz)]	;fi
	lea	r1, [r0+r4]		;gi = fi + kx
	jmp	.do

.exit:
	popd	ebp, ebx, esi, edi
endproc

;*************************************************************

;void fht_FPU_FXCH(float *fz, int n);
proc	fht_FPU_FXCH

%$fz	arg	4
%$n	arg	4

%$k	local	4

%$f0	local	4
%$f1	local	4
%$f2	local	4
%$f3	local	4

%$g0	local	4
%$g1	local	4
%$g2	local	4
%$g3	local	4

%$s1	local	4
%$c1	local	4
%$s2	local	4
%$c2	local	4

%$t_s	local	4
%$t_c	local	4
	alloc

	pushd	ebp, ebx, esi, edi

fht_FPU_FXCH_1st_part:

fht_FPU_FXCH_2nd_part:

fht_FPU_FXCH_3rd_part:

.do_init:
	mov	r3, 16		;k1*fsize = 4*fsize = k4
	mov	r4, 8		;kx = k1/2
	mov	r2, 48		;k3*fsize
	mov	dword [sp(%$k)], 2	;k = 2
	mov	r0, [sp(%$fz)]	;fi
	lea	r1, [r0+8]		;gi = fi + kx

.do:
.do2:
	;f
	fld	dword [r0]
	fsub	dword [r0+r3]
	fld	dword [r0]
	fadd	dword [r0+r3]

	fld	dword [r0+r3*2]
	fsub	dword [r0+r2]
	fld	dword [r0+r3*2]
	fadd	dword [r0+r2]		;f2 f3 f0 f1

	fld	st3
	fld	st3
	fxch	st5
	fadd	st0, st3
	fxch	st4
	fadd	st0, st2
	fxch	st3
	fsubp	st1, st0
	fxch	st1
	fsubp	st4, st0
	fxch	st2

	fstp	dword [r0+r3]		;fi[k1]
	fstp	dword [r0]		;fi[0]
	fstp	dword [r0+r2]		;fi[k3]
	fstp	dword [r0+r3*2]		;fi[k2]

	;g
	fld	dword [r1]
	fsub	dword [r1+r3]
	fld	dword [r1]
	fadd	dword [r1+r3]

	fld	dword [D_1_41421]
	fmul	dword [r1+r2]
	fld	dword [D_1_41421]
	fmul	dword [r1+r3*2]		;g2 g3 g0 g1

	fld	st3
	fld	st3
	fxch	st5
	fadd	st0, st3
	fxch	st4
	fadd	st0, st2
	fxch	st3
	fsubp	st1, st0
	fxch	st1
	fsubp	st4, st0
	fxch	st2

	fstp	dword [r1+r3]		;gi[k1]
	fstp	dword [r1]		;gi[0]
	fstp	dword [r1+r2]		;gi[k3]
	fstp	dword [r1+r3*2]		;gi[k2]

	lea	r0, [r0+r3*4]
	lea	r1, [r1+r3*4]
	cmp	r0, r6
	jb	.do2


	mov	r0, [sp(%$k)]
	fld	dword [costab_fft +r0*4]
	fld	dword [sintab_fft +r0*4]
	fld	dword [D_1_0]
	fld	dword [D_0_0]
	fxch	st3
	fstp	dword [sp(%$t_c)]
	fxch	st1
	fstp	dword [sp(%$t_s)]
	fstp	dword [sp(%$c1)]
	fstp	dword [sp(%$s1)]

.for_init:
	mov	r5, 4		;i = 1*fsize

.for:
	fld	dword [sp(%$c1)]
	fmul	dword [sp(%$t_c)]
	fld	dword [sp(%$s1)]
	fmul	dword [sp(%$t_s)]

	fld	dword [sp(%$c1)]
	fmul	dword [sp(%$t_s)]
	fld	dword [sp(%$s1)]
	fmul	dword [sp(%$t_c)]
	fxch	st2
	fsubp	st3, st0		;c1
	faddp	st1, st0		;s1 c1
	
	fld	st1
	fxch	st2
	fmul	st0, st0		;c1c1 s1 c1
	fld	st1
	fxch	st2
	fmul	st0, st0		;s1s1 c1c1 s1 c1

	fxch	st3
	fst	dword [sp(%$c1)]	;c1
	fxch	st2
	fst	dword [sp(%$s1)]	;s1 c1c1 c1 s1s1

	fmulp	st2, st0
	fsubrp	st2, st0
	fadd	st0, st0		;s2 c2
	fxch	st1
	fstp	dword [sp(%$c2)]
	fstp	dword [sp(%$s2)]

	mov	r0, [sp(%$fz)]
	mov	r1, [sp(%$fz)]
	add	r0, r5		;r0 = fi
	add	r1, r3
	sub	r1, r5		;r1 = gi

.do3:
	fld	dword [sp(%$s2)]
	fmul	dword [r0+r3]
	fld	dword [sp(%$c2)]
	fmul	dword [r1+r3]

	fld	dword [sp(%$c2)]
	fmul	dword [r0+r3]
	fld	dword [sp(%$s2)]
	fmul	dword [r1+r3]
	fxch	st2
	fsubp	st3, st0		;b = s2*fi[k1] - c2*gi[k1]
	faddp	st1, st0		;a = c2*fi[k1] + s2*gi[k1]  b

	fld	dword [r1]
	fsub	st0, st2		;g1 a b
	fxch	st2
	fadd	dword [r1]		;g0 a g1

	fld	dword [r0]
	fsub	st0, st2		;f1 g0 a g1
	fxch	st2
	fadd	dword [r0]		;f0 g0 f1 g1

	fxch	st3
	fstp	dword [sp(%$g1)]
	fstp	dword [sp(%$g0)]
	fstp	dword [sp(%$f1)]
	fstp	dword [sp(%$f0)]


	fld	dword [sp(%$s2)]
	fmul	dword [r0+r2]
	fld	dword [sp(%$c2)]
	fmul	dword [r1+r2]

	fld	dword [sp(%$c2)]
	fmul	dword [r0+r2]
	fld	dword [sp(%$s2)]
	fmul	dword [r1+r2]
	fxch	st2
	fsubp	st3, st0		;b = s2*fi[k3] - c2*gi[k3]
	faddp	st1, st0		;a = c2*fi[k3] + s2*gi[k3]  b


	fld	dword [r1+r3*2]
	fsub	st0, st2		;g3 a b
	fxch	st2
	fadd	dword [r1+r3*2]	;g2 a g3

	fld	dword [r0+r3*2]
	fsub	st0, st2		;f3 g2 a g3
	fxch	st2
	fadd	dword [r0+r3*2]	;f2 g2 f3 g3

	fxch	st3
	fstp	dword [sp(%$g3)]
	fstp	dword [sp(%$g2)]
	fstp	dword [sp(%$f3)]
	fstp	dword [sp(%$f2)]


	fld	dword [sp(%$s1)]
	fmul	dword [sp(%$f2)]
	fld	dword [sp(%$c1)]
	fmul	dword [sp(%$g3)]
	
	fld	dword [sp(%$c1)]
	fmul	dword [sp(%$f2)]
	fld	dword [sp(%$s1)]
	fmul	dword [sp(%$g3)]
	fxch	st2
	fsubp	st3, st0		;b = s1*f2 - c1*g3
	faddp	st1, st0		;a = c1*f2 + s1*g3  b

	fld	dword [sp(%$g1)]
	fsub	st0, st2		;gi[k3] a b
	fxch	st2
	fadd	dword [sp(%$g1)]	;gi[k1] a gi[k3]

	fld	dword [sp(%$f0)]
	fsub	st0, st2		;fi[k2] gi[k1] a gi[k3]
	fxch	st2
	fadd	dword [sp(%$f0)]	;fi[0] gi[k1] fi[k2] gi[k3]

	fxch	st3
	fstp	dword [r1+r2]
	fstp	dword [r1+r3]
	fstp	dword [r0+r3*2]
	fstp	dword [r0]


	fld	dword [sp(%$c1)]
	fmul	dword [sp(%$g2)]
	fld	dword [sp(%$s1)]
	fmul	dword [sp(%$f3)]
	
	fld	dword [sp(%$s1)]
	fmul	dword [sp(%$g2)]
	fld	dword [sp(%$c1)]
	fmul	dword [sp(%$f3)]
	fxch	st2
	fsubp	st3, st0		;b = c1*g2 - s1*f3
	faddp	st1, st0		;a = s1*g2 + c1*f3  b

	fld	dword [sp(%$f1)]
	fsub	st0, st2		;fi[k3] a b
	fxch	st2
	fadd	dword [sp(%$f1)]	;fi[k1] a fi[k3]

	fld	dword [sp(%$g0)]
	fsub	st0, st2		;gi[k2] fi[k1] a fi[k3]
	fxch	st2
	fadd	dword [sp(%$g0)]	;gi[0] fi[k1] gi[k2] fi[k3]

	fxch	st3
	fstp	dword [r0+r2]
	fstp	dword [r0+r3]
	fstp	dword [r1+r3*2]
	fstp	dword [r1]


	lea	r0, [r0+r3*4]
	lea	r1, [r1+r3*4]
	cmp	r0, r6
	jb near	.do3

	add	r5, 4
	cmp	r5, r4
	jb near	.for

	cmp	r3, [sp(%$n)]
	jae	.exit

	add	dword [sp(%$k)], 2	;k  += 2;
	lea	r3, [r3*4]		;k1 *= 4
	lea	r2, [r2*4]		;k3 *= 4
	lea	r4, [r4*4]		;kx *= 4
	mov	r0, [sp(%$fz)]	;fi
	lea	r1, [r0+r4]		;gi = fi + kx
	jmp	.do

.exit:
	popd	ebp, ebx, esi, edi
endproc

	end
