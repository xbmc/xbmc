
%include "MACLib/Assembly/Tools.inc"

segment_code

;
; void  Adapt ( short* pM, const short* pAdapt, int nDirection, int nOrder )
;
;   [esp+16]    nOrder
;   [esp+12]    nDirection
;   [esp+ 8]    pAdapt
;   [esp+ 4]    pM
;   [esp+ 0]    Return Address

            align 16
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
proc        Adapt

            mov  eax, [esp +  4]                ; pM
            mov  ecx, [esp +  8]                ; pAdapt
            mov  edx, [esp + 16]                ; nOrder
            shr  edx, 4

            cmp  dword [esp + 12], byte 0       ; nDirection
            jle  short AdaptSub

AdaptAddLoop:
            movq  mm0, [eax]
            paddw mm0, [ecx]
            movq  [eax], mm0
            movq  mm1, [eax + 8]
            paddw mm1, [ecx + 8]
            movq  [eax + 8], mm1
            movq  mm2, [eax + 16]
            paddw mm2, [ecx + 16]
            movq  [eax + 16], mm2
            movq  mm3, [eax + 24]
            paddw mm3, [ecx + 24]
            movq  [eax + 24], mm3
            add   eax, byte 32
            add   ecx, byte 32
            dec   edx
            jnz   AdaptAddLoop

            emms
            ret

            align 16
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop

AdaptSub:   je    short AdaptDone

AdaptSubLoop:
            movq  mm0, [eax]
            psubw mm0, [ecx]
            movq  [eax], mm0
            movq  mm1, [eax + 8]
            psubw mm1, [ecx + 8]
            movq  [eax + 8], mm1
            movq  mm2, [eax + 16]
            psubw mm2, [ecx + 16]
            movq  [eax + 16], mm2
            movq  mm3, [eax + 24]
            psubw mm3, [ecx + 24]
            movq  [eax + 24], mm3
            add   eax, byte 32
            add   ecx, byte 32
            dec   edx
            jnz   AdaptSubLoop

            emms
AdaptDone:

endproc

;
; int  CalculateDotProduct ( const short* pA, const short* pB, int nOrder )
;
;   [esp+12]    nOrder
;   [esp+ 8]    pB
;   [esp+ 4]    pA
;   [esp+ 0]    Return Address

            align   16
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop
            nop

proc        CalculateDotProduct

            mov     eax, [esp +  4]             ; pA
            mov     ecx, [esp +  8]             ; pB
            mov     edx, [esp + 12]             ; nOrder
            shr     edx, 4
            pxor    mm7, mm7

loopDot:    movq    mm0, [eax]
            pmaddwd mm0, [ecx]
            paddd   mm7, mm0
            movq    mm1, [eax +  8]
            pmaddwd mm1, [ecx +  8]
            paddd   mm7, mm1
            movq    mm2, [eax + 16]
            pmaddwd mm2, [ecx + 16]
            paddd   mm7, mm2
            movq    mm3, [eax + 24]
            pmaddwd mm3, [ecx + 24]
            add     eax, byte 32
            add     ecx, byte 32
            paddd   mm7, mm3
            dec     edx
            jnz     loopDot

            movq    mm6, mm7
            psrlq   mm7, 32
            paddd   mm6, mm7
            movd    [esp + 4], mm6
            emms
            mov     eax, [esp + 4]
endproc


;
; BOOL GetMMXAvailable ( void );
;

proc        GetMMXAvailable
            pushad
            pushfd
            pop     eax
            mov     ecx, eax
            xor     eax, 0x200000
            push    eax
            popfd
            pushfd
            pop     eax
            cmp     eax, ecx
            jz      short return        ; no CPUID command, so no MMX

            mov     eax,1
            CPUID
            test    edx,0x800000
return:     popad
            setnz   al
            and     eax, byte 1
endproc

            end
