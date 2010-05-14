; back port from GOGO-no coda 2.24b by Takehiro TOMINAGA

; GOGO-no-coda
;	Copyright (C) 1999 shigeo
;	special thanks to Keiichi SAKAI
 
%include "nasm.h"

	globaldef fht_SSE

	segment_data
	align 16
Q_MMPP	dd	0x0,0x0,0x80000000,0x80000000
Q_MPMP	dd	0x0,0x80000000,0x0,0x80000000
D_1100	dd 0.0, 0.0, 1.0, 1.0
costab_fft:
	dd 9.238795325112867e-01
	dd 3.826834323650898e-01
	dd 9.951847266721969e-01
	dd 9.801714032956060e-02
	dd 9.996988186962042e-01
	dd 2.454122852291229e-02
	dd 9.999811752836011e-01
	dd 6.135884649154475e-03
S_SQRT2	dd	1.414213562

	segment_code

PIC_OFFSETTABLE

;------------------------------------------------------------------------
;	by K. SAKAI
;	99/08/18	PIII 23k[clk]
;	99/08/19	命令順序入れ換え PIII 22k[clk]
;	99/08/20	bit reversal を旧午後から移植した PIII 17k[clk]
;	99/08/23	一部 unroll PIII 14k[clk]
;	99/11/12	clean up
;
;void fht_SSE(float *fz, int n);
	align 16
fht_SSE:
	push	ebx
	push	esi
	push	edi
	push	ebp

%assign _P 4*5

	;2つ目のループ
	mov	eax,[esp+_P+0]	;eax=fz
	mov	ebp,[esp+_P+4]	;=n
	shl	ebp,3
	add	ebp,eax		; fn  = fz + n, この関数終了まで不変
	push	ebp

	call	get_pc.bp
	add	ebp, PIC_BASE()

	lea	ecx,[PIC_EBP_REL(costab_fft)]
	xor	eax,eax
	mov	al,8		; =k1=1*(sizeof float)	// 4, 16, 64, 256,...
.lp2:				; do{
	mov	esi,[esp+_P+4]	; esi=fi=fz
	lea	edx,[eax+eax*2]
	mov	ebx, esi

; たかだか2並列しか期待できない部分はFPUのほうが速い。
	loopalign	16
.lp20:				; do{
;                       f0     = fi[0 ] + fi[k1];
;                       f2     = fi[k2] + fi[k3];
;                       f1     = fi[0 ] - fi[k1];
;                       f3     = fi[k2] - fi[k3];
;                       fi[0 ] = f0     + f2;
;                       fi[k1] = f1     + f3;
;                       fi[k2] = f0     - f2;
;                       fi[k3] = f1     - f3;
	lea	edi,[ebx+eax]	; edi=gi=fi+ki/2
	fld	dword [ebx]
	fadd	dword [ebx+eax*2]
	fld	dword [ebx+eax*4]
	fadd	dword [ebx+edx*2]

	fld	dword [ebx]
	fsub	dword [ebx+eax*2]
	fld	dword [ebx+eax*4]
	fsub	dword [ebx+edx*2]

	fld	st1
	fadd	st0,st1
	fstp	dword [ebx+eax*2]
	fsubp	st1,st0
	fstp	dword [ebx+edx*2]

	fld	st1
	fadd	st0,st1
	fstp	dword [ebx]
	fsubp	st1,st0
	fstp	dword [ebx+eax*4]

	lea	ebx,[ebx + eax*8]	; = fi += (k1 * 4);
;                       g0     = gi[0 ] + gi[k1];
;                       g2     = SQRT2  * gi[k2];
;                       g1     = gi[0 ] - gi[k1];
;                       g3     = SQRT2  * gi[k3];
;                       gi[0 ] = g0     + g2;
;                       gi[k2] = g0     - g2;
;                       gi[k1] = g1     + g3;
;                       gi[k3] = g1     - g3;
	fld	dword [edi]
	fadd	dword [edi+eax*2]
	fld	dword [PIC_EBP_REL(S_SQRT2)]
	fmul	dword [edi+eax*4]

	fld	dword [edi]
	fsub	dword [edi+eax*2]
	fld	dword [PIC_EBP_REL(S_SQRT2)]
	fmul	dword [edi+edx*2]

	fld	st1
	fadd	st0,st1
	fstp	dword [edi+eax*2]
	fsubp	st1,st0
	fstp	dword [edi+edx*2]

	fld	st1
	fadd	st0,st1
	fstp	dword [edi]
	fsubp	st1,st0
	fstp	dword [edi+eax*4]

	cmp	ebx,[esp]
	jl	near .lp20		; while (fi<fn);


;               i = 1; //for (i=1;i<kx;i++){
;                       c1 = 1.0*t_c - 0.0*t_s;
;                       s1 = 0.0*t_c + 1.0*t_s;
	movlps	xmm6,[ecx] ; = { --,  --,  s1, c1}
	movaps	xmm7,xmm6

	shufps	xmm6,xmm6,R4(0,1,1,0)	; = {+c1, +s1, +s1, +c1} -> 必要
;                       c2 = c1*c1 - s1*s1 = 1 - (2*s1)*s1;
;                       s2 = c1*s1 + s1*c1 = 2*s1*c1;
	shufps	xmm7,xmm7,R4(1,0,0,1)
	movss	xmm5,xmm7		; = { --,  --,  --, s1}
	xorps	xmm7,[PIC_EBP_REL(Q_MMPP)]	; = {-s1, -c1, +c1, +s1} -> 必要

	addss	xmm5,xmm5		; = (--, --,  --, 2*s1)
	add	esi,4		; esi = fi = fz + i
	shufps	xmm5,xmm5,R4(0,0,0,0)	; = (2*s1, 2*s1, 2*s1, 2*s1)
	mulps	xmm5,xmm6		; = (2*s1*c1, 2*s1*s1, 2*s1*s1, 2*s1*c1)
	subps	xmm5,[PIC_EBP_REL(D_1100)]		; = (--, 2*s1*s1-1, --, 2*s1*c1) = {-- -c2 -- s2}
	movaps	xmm4,xmm5
	shufps	xmm5,xmm5,R4(2,0,2,0)	; = {-c2, s2, -c2, s2} -> 必要

	xorps	xmm4,[PIC_EBP_REL(Q_MMPP)]		; = {--, c2, --, s2}
	shufps	xmm4,xmm4,R4(0,2,0,2)	; = {s2, c2, s2, c2} -> 必要

	loopalign	16
.lp21:				; do{
;                               a       = c2*fi[k1] + s2*gi[k1];
;                               b       = s2*fi[k1] - c2*gi[k1];
;                               c       = c2*fi[k3] + s2*gi[k3];
;                               d       = s2*fi[k3] - c2*gi[k3];
;                               f0      = fi[0 ]        + a;
;                               g0      = gi[0 ]        + b;
;                               f2      = fi[k1 * 2]    + c;
;                               g2      = gi[k1 * 2]    + d;
;                               f1      = fi[0 ]        - a;
;                               g1      = gi[0 ]        - b;
;                               f3      = fi[k1 * 2]    - c;
;                               g3      = gi[k1 * 2]    - d;
	lea	edi,[esi + eax*2 - 8]	; edi = gi = fz +k1-i

	movss	xmm0,[esi + eax*2]	; = fi[k1]
	movss	xmm2,[esi + edx*2]	; = fi[k3]
	shufps	xmm0,xmm2,0x00	; = {fi[k3], fi[k3], fi[k1], fi[k1]}
	movss	xmm1,[edi + eax*2]	; = fi[k1]
	movss	xmm3,[edi + edx*2]	; = fi[k3]
	shufps	xmm1,xmm3,0x00	; = {gi[k3], gi[k3], gi[k1], gi[k1]}
	movss	xmm2,[esi]		; = fi[0]
	mulps	xmm0,xmm4		; *= {+s2, +c2, +s2, +c2}
	movss	xmm3,[esi + eax*4]	; = fi[k2]
	unpcklps	xmm2,xmm3	; = {--, --, fi[k2], fi[0]}
	mulps	xmm1,xmm5		; *= {-c2, +s2, -c2, +s2}
	movss	xmm3,[edi + eax*4]	; = gi[k2]
	addps	xmm0,xmm1		; = {d, c, b, a}
	movss	xmm1,[edi]		; = gi[0]
	unpcklps	xmm1,xmm3	; = {--,  --, gi[k2], gi[0]}
	unpcklps	xmm2,xmm1	; = {gi[k2], fi[k2], gi[0], fi[0]}
	movaps	xmm1,xmm2
	addps	xmm1,xmm0	; = {g2, f2, g0, f0}
	subps	xmm2,xmm0	; = {g3, f3, g1, f1}

;                               a       = c1*f2     + s1*g3;
;                               c       = s1*g2     + c1*f3;
;                               b       = s1*f2     - c1*g3;
;                               d       = c1*g2     - s1*f3;
;                               fi[0 ]  = f0        + a;
;                               gi[0 ]  = g0        + c;
;                               gi[k1]  = g1        + b;
;                               fi[k1]  = f1        + d;
;                               fi[k1 * 2]  = f0    - a;
;                               gi[k1 * 2]  = g0    - c;
;                               gi[k3]      = g1    - b;
;                               fi[k3]      = f1    - d;
	movaps	xmm3,xmm1
	movhlps	xmm1,xmm1	; = {g2, f2, g2, f2}
	shufps	xmm3,xmm2,0x14	; = {f1, g1, g0, f0}
	mulps	xmm1,xmm6	; *= {+c1, +s1, +s1, +c1}
	shufps	xmm2,xmm2,0xBB	; = {f3, g3, f3, g3}
	mulps	xmm2,xmm7	; *= {-s1, -c1, +c1, +s1}
	addps	xmm1,xmm2	; = {d, b, c, a}
	movaps	xmm2,xmm3
	addps	xmm3,xmm1	; = {fi[k1], gi[k1], gi[0], fi[0]}
	subps	xmm2,xmm1	; = {fi[k3], gi[k3], gi[k1*2], fi[k1*2]}
	movhlps	xmm0,xmm3
	movss	[esi],xmm3
	shufps	xmm3,xmm3,0x55
	movss	[edi+eax*2],xmm0
	shufps	xmm0,xmm0,0x55
	movss	[edi],xmm3
	movss	[esi+eax*2],xmm0
	movhlps	xmm0,xmm2
	movss	[esi+eax*4],xmm2
	shufps	xmm2,xmm2,0x55
	movss	[edi+edx*2],xmm0
	shufps	xmm0,xmm0,0x55
	movss	[edi+eax*4],xmm2
	movss	[esi+edx*2],xmm0
	lea	esi,[esi + eax*8] ; fi += (k1 * 4);
	cmp	esi,[esp]
	jl	near .lp21		; while (fi<fn);


; unroll前のdo loopは43+4命令

; 最内周ではないforループのi=2から先をunrollingした
; kx=   2,   8,  32,  128
; k4=  16,  64, 256, 1024
;       0, 6/2,30/2,126/2

	xor	ebx,ebx
	mov	bl, 4*2		; = i = 4
	cmp	ebx,eax		; i < k1
	jnl	near .F22
;               for (i=2;i<kx;i+=2){
	loopalign	16
.lp22:
; at here, xmm6 is {c3, s3, s3, c3}
;                       c1 = c3*t_c - s3*t_s;
;                       s1 = c3*t_s + s3*t_c;
	movlps	xmm0,[ecx]
	shufps	xmm0,xmm0,R4(1,1,0,0)	; = {t_s, t_s, t_c, t_c}
	mulps	xmm6,xmm0	; = {c3*ts, s3*ts, s3*tc, c3*tc}
	movhlps	xmm4,xmm6	; = {--,    --,    c3*ts, s3*ts}
	xorps	xmm4,[PIC_EBP_REL(Q_MPMP)]	; = {--,    --,   -c3*ts, s3*ts}
	subps	xmm6,xmm4	; = {-,-, c3*ts+s3*tc, c3*tc-s3*ts}={-,-,s1,c1}

;                       c3 = c1*t_c - s1*t_s;
;                       s3 = s1*t_c + c1*t_s;
	shufps	xmm6,xmm6,0x14	; = {c1, s1, s1, c1}
	mulps	xmm0,xmm6	; = {ts*c1 ts*s1 tc*s1 tc*c1}
	movhlps	xmm3,xmm0
	xorps	xmm3,[PIC_EBP_REL(Q_MPMP)]
	subps	xmm0,xmm3	; = {--, --, s3, c3}

; {s2 s4 c4 c2} = {2*s1*c1 2*s3*c3 1-2*s3*s3 1-2*s1*s1}
	unpcklps	xmm6,xmm0	; xmm6 = {s3, s1, c3, c1}
	movaps	xmm7, xmm6
	shufps	xmm6,xmm6,R4(2,3,1,0)	; xmm6 = {s1, s3, c3, c1}
	addps	xmm7, xmm7		; {s3*2, s1*2,   --,   --}
	mov	edi,[esp+_P+4]		; = fz
	shufps	xmm7, xmm7, R4(2,3,3,2)	; {s1*2, s3*2, s3*2, s1*2}
	sub	edi,ebx			; edi = fz - i/2
	mulps	xmm7, xmm6		; {s1*s1*2, s3*s3*2, s3*c3*2, s1*c1*2}
	lea	esi,[edi + ebx*2]	; esi = fi = fz +i/2
	subps	xmm7, [PIC_EBP_REL(D_1100)]		; {-c2, -c4, s4, s2}
	lea	edi,[edi + eax*2-4]	; edi = gi = fz +k1-i/2

;                       fi = fz +i;
;                       gi = fz +k1-i;
;                       do{
.lp220:
; unroll後のdo loopは51+4命令
;                               a       = c2*fi[k1  ] + s2*gi[k1  ];
;                               e       = c4*fi[k1+1] + s4*gi[k1-1];
;                               f       = s4*fi[k1+1] - c4*gi[k1-1];
;                               b       = s2*fi[k1  ] - c2*gi[k1  ];
;                               c       = c2*fi[k3  ] + s2*gi[k3  ];
;                               g       = c4*fi[k3+1] + s4*gi[k3-1];
;                               h       = s4*fi[k3+1] - c4*gi[k3-1];
;                               d       = s2*fi[k3  ] - c2*gi[k3  ];

	movaps	xmm4,xmm7	; = {-c2 -c4  s4  s2}
	xorps	xmm4,[PIC_EBP_REL(Q_MMPP)]	; = { c2  c4  s4  s2}
	shufps	xmm4,xmm4,0x1B	; = { s2  s4  c4  c2}
	movlps	xmm0,[esi+eax*2]
	movlps	xmm1,[edi+eax*2]
	movlps	xmm2,[esi+edx*2]
	movlps	xmm3,[edi+edx*2]
	shufps	xmm0,xmm0,0x14
	shufps	xmm1,xmm1,0x41
	shufps	xmm2,xmm2,0x14
	shufps	xmm3,xmm3,0x41
	mulps	xmm0,xmm4
	mulps	xmm1,xmm7
	mulps	xmm2,xmm4
	mulps	xmm3,xmm7
	addps	xmm0,xmm1	; xmm0 = {b, f, e, a}
	addps	xmm2,xmm3	; xmm2 = {d, h, g, c}
;17

;                               f0      = fi[0   ]    + a;
;                               f4      = fi[0 +1]    + e;
;                               g4      = gi[0 -1]    + f;
;                               g0      = gi[0   ]    + b;
;                               f1      = fi[0   ]    - a;
;                               f5      = fi[0 +1]    - e;
;                               g5      = gi[0 -1]    - f;
;                               g1      = gi[0   ]    - b;
;                               f2      = fi[k2  ]    + c;
;                               f6      = fi[k2+1]    + g;
;                               g6      = gi[k2-1]    + h;
;                               g2      = gi[k2  ]    + d;
;                               f3      = fi[k2  ]    - c;
;                               f7      = fi[k2+1]    - g;
;                               g7      = gi[k2-1]    - h;
;                               g3      = gi[k2  ]    - d;
	movlps	xmm1,[esi      ]
	movhps	xmm1,[edi      ]
	movaps	xmm4,xmm1
	subps	xmm1,xmm0	; xmm1 = {g1, g5, f5, f1}
	movlps	xmm3,[esi+eax*4]
	movhps	xmm3,[edi+eax*4]
	movaps	xmm5,xmm3
	subps	xmm3,xmm2	; xmm3 = {g3, g7, f7, f3}
	addps	xmm0,xmm4	; xmm0 = {g0, g4, f4, f0}
	addps	xmm2,xmm5	; xmm2 = {g2, g6, f6, f2}
;10

;                               a       = c1*f2     + s1*g3;	順*順 + 逆*逆
;                               e       = c3*f6     + s3*g7;
;                               g       = s3*g6     + c3*f7;
;                               c       = s1*g2     + c1*f3;
;                               d       = c1*g2     - s1*f3;	順*逆 - 逆*順
;                               h       = c3*g6     - s3*f7;
;                               f       = s3*f6     - c3*g7;
;                               b       = s1*f2     - c1*g3;

	movaps	xmm5,xmm6	; xmm6 = {s1, s3, c3, c1}
	shufps	xmm5,xmm5,0x1B	; = {c1, c3, s3, s1}
	movaps	xmm4,xmm2
	mulps	xmm4,xmm6
	shufps	xmm2,xmm2,0x1B	; xmm2 = {f2, f6, g6, g2}
	mulps	xmm2,xmm6
	mulps	xmm5,xmm3
	mulps	xmm3,xmm6
	shufps	xmm3,xmm3,0x1B
	addps	xmm4,xmm3	; = {c, g, e, a}
	subps	xmm2,xmm5	; = {b, f, h, d}
;10

;                               fi[0   ]  = f0        + a;
;                               fi[0 +1]  = f4        + e;
;                               gi[0 -1]  = g4        + g;
;                               gi[0   ]  = g0        + c;
;                               fi[k2  ]  = f0        - a;
;                               fi[k2+1]  = f4        - e;
;                               gi[k2-1]  = g4        - g;
;                               gi[k2  ]  = g0        - c;
;                               fi[k1  ]  = f1        + d;
;                               fi[k1+1]  = f5        + h;
;                               gi[k1-1]  = g5        + f;
;                               gi[k1  ]  = g1        + b;
;                               fi[k3  ]  = f1        - d;
;                               fi[k3+1]  = f5        - h;
;                               gi[k3-1]  = g5        - f;
;                               gi[k3  ]  = g1        - b;
	movaps	xmm3,xmm0
	subps	xmm0,xmm4
	movlps	[esi+eax*4],xmm0
	movhps	[edi+eax*4],xmm0
	addps	xmm4,xmm3
	movlps	[esi      ],xmm4
	movhps	[edi      ],xmm4

	movaps	xmm5,xmm1
	subps	xmm1,xmm2
	movlps	[esi+edx*2],xmm1
	movhps	[edi+edx*2],xmm1
	addps	xmm2,xmm5
	movlps	[esi+eax*2],xmm2
	movhps	[edi+eax*2],xmm2
; 14
;                               gi     += k4;
;                               fi     += k4;
	lea	edi,[edi + eax*8] ; gi += (k1 * 4);
	lea	esi,[esi + eax*8] ; fi += (k1 * 4);
	cmp	esi,[esp]
	jl	near .lp220		; while (fi<fn);
;                       } while (fi<fn);

	add	ebx,byte 2*4	; i+= 4
	cmp	ebx,eax		; i < k1
	shufps	xmm6,xmm6,R4(1,2,2,1)	; (--,s3,c3,--) => {c3, s3, s3, c3}
	jl	near .lp22
;               }
.F22:
	shl	eax,2
	add	ecx, byte 8
	cmp	eax,[esp+_P+8]	; while ((k1 * 4)<n);
	jle	near .lp2
	pop	ebp
	pop	ebp
	pop	edi
	pop	esi
	pop	ebx
	ret

	end
