; from a new GOGO-no-coda (1999/09)
;	Copyright (C) 1999 shigeo
;	special thanks to Keiichi SAKAI, URURI
; hacked and back-ported to LAME
;	 by Takehiro TOMINAGA Nov 2000

%include "nasm.h"

	globaldef fht_3DN

	segment_data
	align	16
costab	dd	0x80000000, 0
	dd	1.414213562,1.414213562
	dd	9.238795283293805e-01, 9.238795283293805e-01
	dd	3.826834424611044e-01, 3.826834424611044e-01
	dd	9.951847264044178e-01, 9.951847264044178e-01
	dd	9.801714304836734e-02, 9.801714304836734e-02
	dd	9.996988186794428e-01, 9.996988186794428e-01
	dd	2.454122920569705e-02, 2.454122920569705e-02
	dd	9.999811752815535e-01, 9.999811752815535e-01
	dd	6.135884819898878e-03, 6.135884819898878e-03
D_1_0_0_0	dd	0.0		, 1.0

	segment_code

PIC_OFFSETTABLE


;void fht_3DN(float *fz, int nn);

proc	fht_3DN

	pushd	ebp, ebx, esi, edi

	sub	esp, 20

	call	get_pc.bp
	add	ebp, PIC_BASE()

	mov	r0, [esp+40]		;fi
	mov	r1, [esp+44]		;r1 = nn
	lea	r3, [PIC_EBP_REL(costab)]		;tri = costab
	lea	r4, [r0+r1*8]		;r4 = fn = &fz[n]
	mov	[esp+16], r4
	mov	r4, 8			;kx = k1/2

	pmov	mm7, [r3]

	loopalign 16
.do1
	lea	r3, [r3+16]	;tri += 2;
	pmov	mm6, [PIC_EBP_REL(costab+8)]
	lea	r2, [r4+r4*2]		;k3*fsize/2
	mov	r5, 4		;i = 1*fsize

	loopalign 16
.do2:
	lea	r1, [r0+r4]		;gi = fi + kx
	;f
	pmov	mm0, [r0]	;fi0
	pmov	mm1, [r0+r4*2]	;fi1
	pmov	mm2, [r0+r2*2]	;fi3
	pmov	mm3, [r0+r4*4]	;fi2

	pupldq	mm0, mm0	;fi0 | fi0
	pupldq	mm1, mm1	;fi1 | fi1
	pupldq	mm2, mm2	;fi2 | fi2
	pupldq	mm3, mm3	;fi3 | fi3

	pxor	mm1, mm7	;fi1 | -fi1
	pxor	mm3, mm7	;fi3 | -fi3

	pfsub	mm0, mm1	;f1 | f0
	pfsub	mm2, mm3	;f3 | f2

	pmov	mm4, mm0
	pfadd	mm0, mm2	;f1+f3|f0+f2 = fi1 | fi0
	pfsub	mm4, mm2	;f1-f3|f0-f2 = fi3 | fi2

	pmovd	[r0], mm0	;fi[0]
	puphdq	mm0, mm0
	pmovd	[r0+r4*4], mm4	;fi[k2]
	puphdq	mm4, mm4

	pmovd	[r0+r4*2], mm4	;fi[k1]
	pmovd	[r0+r2*2], mm0	;fi[k3]
	lea	r0, [r0+r4*8]

	;g
	pmov	mm0, [r1]	;gi0
	pmov	mm1, [r1+r4*2]	;gi1
	pmov	mm2, [r1+r4*4]	;gi2
	pmov	mm3, [r1+r2*2]	;gi3

	pupldq	mm1, mm1
	pupldq	mm0, mm0	;gi0 | gi0
	pupldq	mm2, mm3	;gi3 | gi2

	pxor	mm1, mm7	;gi1 | -gi1

	pfsub	mm0, mm1	;gi0-gi1|gi0+gi1 = g1 | g0
	pfmul	mm2, mm6	;gi3*SQRT2|gi2*SQRT2 = g3 | g2

	pmov	mm4, mm0
	pfadd	mm0, mm2	;g1+g3|g0+g2 = gi1 | gi0
	pfsub	mm4, mm2	;g1-g3|g0-g2 = gi3 | gi2

	pmovd	[r1], mm0	;gi[0]
	puphdq	mm0, mm0
	pmovd	[r1+r4*4], mm4	;gi[k2]
	puphdq	mm4, mm4

	cmp	r0, [esp + 16]
	pmovd	[r1+r4*2], mm0	;gi[k1]
	pmovd	[r1+r2*2], mm4	;gi[k3]

	jb near .do2

	pmov	mm6, [r3+r5]	; this is not aligned address!!

	loopalign 16
.for:
;
; mm6 = c1 | s1
; mm7 = 0x800000000 | 0
;
	pmov	mm1, mm6
	mov	r0, [esp+40]	; fz
	puphdq	mm1, mm1	; c1 | c1
	lea	r1, [r0+r4*2]
	pfadd	mm1, mm1	; c1+c1 | c1+c1
	pfmul	mm1, mm6	; 2*c1*c1 | 2*c1*s1
	pfsub	mm1, [PIC_EBP_REL(D_1_0_0_0)] ; 2*c1*c1-1.0 | 2*c1*s1 = -c2 | s2

	pmov	mm0, mm1
	pxor	mm7, mm6	; c1 | -s1

	pupldq	mm2, mm0
	pupldq	mm3, mm6	; ** | c1
	puphdq	mm0, mm2	; s2 | c2
	puphdq	mm6, mm3	;-s1 | c1

	pxor	mm0, [PIC_EBP_REL(costab)]	; c2 | -s2

; mm0 =  s2| c2
; mm1 = -c2| s2
; mm6 =  c1| s1
; mm7 =  s1|-c1 (we use the opposite sign. from GOGO here)

	pmov	[esp], mm0
	pmov	[esp+8], mm1

	sub	r1, r5		;r1 = gi
	add	r0, r5		;r0 = fi

	loopalign 16
.do3:
	pmov	mm2, [r0+r4*2] ; fi[k1]
	pmov	mm4, [r1+r4*2] ; gi[k1]
	pmov	mm3, [r0+r2*2] ; fi[k3]
	pmov	mm5, [r1+r2*2] ; gi[k3]

	pupldq	mm2, mm2	; fi1 | fi1
	pupldq	mm4, mm4	; gi1 | gi1
	pupldq	mm3, mm3	; fi3 | fi3
	pupldq	mm5, mm5	; gi3 | gi3

	pfmul	mm2, mm0	; s2 * fi1 | c2 * fi1
	pfmul	mm4, mm1	;-c2 * gi1 | s2 * gi1
	pfmul	mm3, mm0	; s2 * fi3 | c2 * fi3
	pfmul	mm5, mm1	;-c2 * gi3 | s2 * gi3

	pfadd	mm2, mm4		;b | a
	pfadd	mm3, mm5		;d | c

	pmov	mm0, [r0]
	pmov	mm4, [r1]
	pmov	mm1, [r0+r4*4]
	pmov	mm5, [r1+r4*4]

	pupldq	mm0, mm4		;gi0 | fi0
	pupldq	mm1, mm5		;gi2 | fi2

	pmov	mm4, mm2
	pmov	mm5, mm3

	pfadd	mm2, mm0		;g0 | f0
	pfadd	mm3, mm1		;g2 | f2

	pfsub	mm0, mm4		;g1 | f1
	pfsub	mm1, mm5		;g3 | f3

	pmov	mm4, mm3
	pmov	mm5, mm1

	pupldq	mm4, mm4		;f2 | f2
	puphdq	mm5, mm5		;g3 | g3
	puphdq	mm3, mm3		;g2 | g2
	pupldq	mm1, mm1		;f3 | f3

	pfmul	mm4, mm6		;f2 * c1 | f2 * s1
	pfmul	mm5, mm7		;g3 * s1 | g3 *-c1
	pfmul	mm3, mm6		;g2 * c1 | g2 * s1
	pfmul	mm1, mm7		;f3 * s1 | f3 *-c1

	pfadd	mm4, mm5		;a | b
	pfsub	mm3, mm1		;d | c

	pmov	mm5, mm2
	pmov	mm1, mm0

	pupldq	mm2, mm2		;f0 | f0
	pupldq	mm0, mm0		;f1 | f1

	puphdq	mm1, mm2		;f0 | g1
	puphdq	mm5, mm0		;f1 | g0

	pmov	mm2, mm4
	pmov	mm0, mm3

	pfadd	mm4, mm1		;fi0 | gi1
	pfadd	mm3, mm5		;fi1 | gi0
	pfsub	mm1, mm2		;fi2 | gi3
	pfsub	mm5, mm0		;fi3 | gi2

	pmovd	[r1+r4*2], mm4	;gi[k1]
	puphdq	mm4, mm4
	pmovd	[r1], mm3		;gi[0]
	puphdq	mm3, mm3
	pmovd	[r1+r2*2], mm1	;gi[k3]
	puphdq	mm1, mm1
	pmovd	[r1+r4*4], mm5	;gi[k2]
	puphdq	mm5, mm5

	pmovd	[r0], mm4	;fi[0]
	pmovd	[r0+r4*2], mm3	;fi[k1]
	pmovd	[r0+r4*4], mm1	;fi[k2]
	pmovd	[r0+r2*2], mm5	;fi[k3]

	lea	r0, [r0+r4*8]
	lea	r1, [r1+r4*8]
	cmp	r0, [esp + 16]
	pmov	mm0, [esp]
	pmov	mm1, [esp+8]

	jb near	.do3

	add	r5, 4
; mm6 =  c1| s1
; mm7 =  s1|-c1 (we use the opposite sign. from GOGO here)
	pfmul	mm6, [r3]	; c1*a | s1*a
	pfmul	mm7, [r3+8]	; s1*b |-c1*b
	cmp	r5, r4

	pfsub	mm6, mm7	; c1*a-s1*b | s1*a+c1*b
	pupldq	mm7,mm6
	puphdq	mm6,mm7
	pmov	mm7, [PIC_EBP_REL(costab)]
	jb near	.for

	mov	r0, [esp+40]	;fi
	cmp	r4, [esp+40+4]
	lea	r4, [r4*4]	;kx *= 4

	jb near	.do1
.exitttt
	femms
	add	esp,20
	popd	ebp, ebx, esi, edi
endproc


;void fht_E3DN(float *fz, int nn);

proc	fht_E3DN

	pushd	ebp, ebx, esi, edi

	sub	esp, 20

	call	get_pc.bp
	add	ebp, PIC_BASE()

	mov	r0, [esp+40]		;fi
	mov	r1, [esp+44]		;r1 = nn
	lea	r3, [PIC_EBP_REL(costab)]		;tri = costab
	lea	r4, [r0+r1*8]		;r4 = fn = &fz[n]
	mov	[esp+16], r4
	mov	r4, 8			;kx = k1/2

	pmov	mm7, [r3]

	loopalign 16
.do1
	lea	r3, [r3+16]	;tri += 2;
	pmov	mm6, [PIC_EBP_REL(costab+8)]
	lea	r2, [r4+r4*2]		;k3*fsize/2
	mov	r5, 4		;i = 1*fsize

	loopalign 16
.do2:
	lea	r1, [r0+r4]		;gi = fi + kx
;f
	pmov	mm0, [r0]	; X  | fi0
	pmov	mm1, [r0+r4*4]	; X  | fi2
	pupldq	mm0, [r0+r4*2]	;fi1 | fi0
	pupldq	mm1, [r0+r2*2]	;fi3 | fi2
	pfpnacc	mm0, mm0	;fi0+fi1 | fi0-fi1 = f0|f1
	pfpnacc	mm1, mm1	;fi2+fi3 | fi2-fi3 = f2|f3

	pmov	mm2, mm0
	pfadd	mm0, mm1	;f0+f2|f1+f3 = fi0 | fi1
	pfsub	mm2, mm1	;f0-f2|f1-f3 = fi2 | fi3

	pmovd	[r0+r4*2], mm0	;fi[k1]
	pmovd	[r0+r2*2], mm2	;fi[k3]

	puphdq	mm0, mm0
	puphdq	mm2, mm2
	pmovd	[r0], mm0	;fi[0]
	pmovd	[r0+r4*4], mm2	;fi[k2]

	lea	r0, [r0+r4*8]
;g
	pmov	mm3, [r1]	;    gi0
	pmov	mm4, [r1+r2*2]	;    gi3
	pupldq	mm3, [r1+r4*2]	;gi1|gi0
	pupldq	mm4, [r1+r4*4]	;gi2|gi3

	pfpnacc	mm3, mm3	;gi0+gi1  |gi0-gi1   = f0|f1
	pfmul	mm4, mm6	;gi2*SQRT2|gi3*SQRT2 = f2|f3

	pmov	mm5, mm3
	pfadd	mm3, mm4	;f0+f2|f1+f3
	pfsub	mm5, mm4	;f0-f2|f1-f3

	cmp	r0, [esp + 16]
	pmovd	[r1+r4*2], mm3	;gi[k1]
	pmovd	[r1+r2*2], mm5	;gi[k3]
	puphdq	mm3, mm3
	puphdq	mm5, mm5
	pmovd	[r1], mm3	;gi[0]
	pmovd	[r1+r4*4], mm5	;gi[k2]

	jb near .do2

	pmov	mm6, [r3+r5]	; this is not aligned address!!

	loopalign 16
.for:
;
; mm6 = c1 | s1
; mm7 = 0x800000000 | 0
;
	pmov	mm5, mm6
	mov	r0, [esp+40]	; fz
	puphdq	mm5, mm5	; c1 | c1
	lea	r1, [r0+r4*2]
	pfadd	mm5, mm5	; c1+c1 | c1+c1
	pfmul	mm5, mm6	; 2*c1*c1 | 2*c1*s1
	pfsub	mm5, [PIC_EBP_REL(D_1_0_0_0)] ; 2*c1*c1-1.0 | 2*c1*s1 = -c2 | s2

	pswapd	mm4, mm5	; s2 |-c2
	pxor	mm4, mm7	; s2 | c2
	pxor	mm7, mm6	; c1 |-s1
	pswapd	mm6, mm6	; s1 | c1

; mm4 =  s2| c2
; mm5 = -c2| s2
; mm6 =  c1| s1
; mm7 =  s1|-c1 (we use the opposite sign. from GOGO here)

	pmov	[esp], mm4
	pmov	[esp+8], mm5

	sub	r1, r5		;r1 = gi
	add	r0, r5		;r0 = fi

	loopalign 16
.do3:
	pmov	mm0, [r0+r2*2] ; fi[k1]
	pmov	mm2, [r1+r2*2] ; gi[k1]
	pmov	mm1, [r0+r4*2] ; fi[k3]
	pmov	mm3, [r1+r4*2] ; gi[k3]

	pupldq	mm0, mm0
	pupldq	mm2, mm2
	pupldq	mm1, mm1
	pupldq	mm3, mm3

	pfmul	mm0, mm4
	pfmul	mm2, mm5
	pfmul	mm1, mm4
	pfmul	mm3, mm5

	pfadd	mm0, mm2		;d | c
	pfadd	mm1, mm3		;b | a

	pmov	mm2, [r0+r4*4]		;fi2
	pupldq	mm3, [r1+r4*4]		;gi2 | -
	pmov	mm4, [r0]		;fi0
	pupldq	mm5, [r1]		;gi0 | -

	pupldq	mm2, mm0		;c | fi2
	puphdq	mm3, mm0		;d | gi2
	pupldq	mm4, mm1		;a | fi0
	puphdq	mm5, mm1		;b | gi0

	pfpnacc	mm2, mm2		;f2 | f3
	pfpnacc	mm3, mm3		;g2 | g3
	pfpnacc	mm4, mm4		;f0 | f1
	pfpnacc	mm5, mm5		;g0 | g1

	pmov	mm0, mm2
	pmov	mm1, mm3
	pupldq	mm2, mm2		;f3 | f3
	pupldq	mm3, mm3		;g3 | g3
	puphdq	mm0, mm0		;f2 | f2
	puphdq	mm1, mm1		;g2 | g2

	pswapd	mm4, mm4		;f1 | f0
	pswapd	mm5, mm5		;g1 | g0

	pfmul	mm0, mm7		;f2 * s1 | f2 *-c1
	pfmul	mm3, mm6		;g3 * c1 | g3 * s1
	pfmul	mm1, mm6		;g2 * c1 | g2 * s1
	pfmul	mm2, mm7		;f3 * s1 | f3 *-c1

	pfsub	mm0, mm3		; b |-a
	pfsub	mm1, mm2		; d | c

	pmov	mm2, mm5
	pmov	mm3, mm4
	pupldq	mm4, mm0		;-a | f0
	pupldq	mm5, mm1		; c | g0
	puphdq	mm2, mm0		; b | g1
	puphdq	mm3, mm1		; d | f1

	pfpnacc	mm4, mm4		;fi2 | fi0
	pfpnacc	mm5, mm5		;gi0 | gi2
	pfpnacc	mm2, mm2		;gi1 | gi3
	pfpnacc	mm3, mm3		;fi1 | fi3

	pmovd	[r0], mm4		;fi[0]
	pmovd	[r1+r4*4], mm5		;gi[k2]
	pmovd	[r1+r2*2], mm2		;gi[k3]
	pmovd	[r0+r2*2], mm3		;fi[k3]

	puphdq	mm4, mm4
	puphdq	mm5, mm5
	puphdq	mm2, mm2
	puphdq	mm3, mm3
	pmovd	[r0+r4*4], mm4		;fi[k2]
	pmovd	[r1], mm5		;gi[0]
	pmovd	[r1+r4*2], mm2		;gi[k1]
	pmovd	[r0+r4*2], mm3		;fi[k1]

	lea	r0, [r0+r4*8]
	lea	r1, [r1+r4*8]
	cmp	r0, [esp + 16]
	pmov	mm4, [esp]
	pmov	mm5, [esp+8]

	jb near	.do3

	add	r5, 4
; mm6 =  c1| s1
; mm7 =  s1|-c1 (we use the opposite sign. from GOGO here)
	pfmul	mm6, [r3]	; c1*a | s1*a
	pfmul	mm7, [r3+8]	; s1*b |-c1*b
	cmp	r5, r4

	pfsub	mm6, mm7	; c1*a-s1*b | s1*a+c1*b
	pswapd	mm6, mm6 ; ???	; s1*a+c1*b | c1*a-s1*b
	pmov	mm7, [PIC_EBP_REL(costab)]
	jb near	.for

	mov	r0, [esp+40]	;fi
	cmp	r4, [esp+40+4]
	lea	r4, [r4*4]	;kx *= 4

	jb near	.do1
.exitttt
	femms
	add	esp,20
	popd	ebp, ebx, esi, edi
endproc
