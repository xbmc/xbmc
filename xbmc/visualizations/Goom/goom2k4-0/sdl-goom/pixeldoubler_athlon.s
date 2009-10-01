bits 32
global pixel_doubler

%macro twopixproc 0
%rep 2
	movq mm0,[esi]			; -> P1P2
	movq mm2,[esi+8]		; -> P3P4

	movq mm1,mm0
	movq mm3,mm2

	punpckldq mm1,mm1			; P1P2 | P1P2 -> P1P1
	punpckhdq mm0,mm0			; P1P2 | P1P2 -> P2P2

	movntq [edi],mm1
	punpckldq mm3,mm3			; P3P4 | P3P4 -> P3P3

	movntq [edi+8],mm0
	punpckhdq mm2,mm2			; P3P4 | P3P4 -> P4P4

	movntq [edi+16],mm3
	movq mm4,[esi+16]

	movntq [edi+24],mm2
	movq mm6,[esi+24]

	movq mm5,mm4
	movq mm7,mm6

	punpckldq mm5,mm5			; P1P2 | P1P2 -> P1P1
	punpckhdq mm4,mm4			; P1P2 | P1P2 -> P2P2

	movntq [edi+32],mm5
	punpckldq mm7,mm7			; P3P4 | P3P4 -> P3P3

	movntq [edi+40],mm4
	punpckhdq mm6,mm6			; P3P4 | P3P4 -> P4P4

	movntq [edi+48],mm7
	add esi,32

	movntq [edi+56],mm6
	add edi,64
%endrep
%endmacro

%macro prefetcher 0
	prefetch [esi+128]
%endmacro
		

align 16
pixel_doubler:
	push ebp
	mov ebp,esp
	push edi
	push esi
	push ebx
	push ecx
	push edx
		
	mov ecx,[ebp+8]			; ecx <- src
	mov edx,[ebp+12]		; edx <- dest

	mov esi,[ecx]			; esi <- src->buf
	mov edi,[edx]			; edi <- dest->buf

	mov ebx,[ecx+4]			; ebx <- src->width
	mov eax,[ecx+8]			; eax <- src->height

	shl ebx,2				; width *= 4 (in byte)
	mov edx,ebx				; edx <- width in byte
	shr ebx,6				; width in cache page

align 16
while_1:
	prefetch [esi]			; prefetch the first cache line
	prefetch [esi+64]

	mov ecx,ebx

while_2:
	prefetcher
	twopixproc

	dec ecx
	jnz while_2
; end_while_2

	sub esi,edx
	mov ecx,ebx

	prefetch [esi]			; prefetch the first cache line
	prefetch [esi+64]

while_3:
	prefetcher
	twopixproc

	dec ecx
	jnz while_3
; end_while_3
		
	dec eax					; decremente le nombre de ligne testee
	jnz while_1				; on continue si c'etait pas la derniere

; end_while_1:

	sfence
	femms

	pop edx
	pop ecx
	pop ebx
	pop esi
	pop edi
	leave
	ret
