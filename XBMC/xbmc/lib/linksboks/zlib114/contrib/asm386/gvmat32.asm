;
; gvmat32.asm -- Asm portion of the optimized longest_match for 32 bits x86
; Copyright (C) 1995-1996 Jean-loup Gailly and Gilles Vollant.
; File written by Gilles Vollant, by modifiying the longest_match
;  from Jean-loup Gailly in deflate.c
; It need wmask == 0x7fff
;     (assembly code is faster with a fixed wmask)
;
; For Visual C++ 4.2 and ML 6.11c (version in directory \MASM611C of Win95 DDK)
;   I compile with : "ml /coff /Zi /c gvmat32.asm"
;

;uInt longest_match_7fff(s, cur_match)
;    deflate_state *s;
;    IPos cur_match;                             /* current match */

        NbStack         equ     76
        cur_match       equ     dword ptr[esp+NbStack-0]
        str_s           equ     dword ptr[esp+NbStack-4]
; 5 dword on top (ret,ebp,esi,edi,ebx)
        adrret          equ     dword ptr[esp+NbStack-8]
        pushebp         equ     dword ptr[esp+NbStack-12]
        pushedi         equ     dword ptr[esp+NbStack-16]
        pushesi         equ     dword ptr[esp+NbStack-20]
        pushebx         equ     dword ptr[esp+NbStack-24]

        chain_length    equ     dword ptr [esp+NbStack-28]
        limit           equ     dword ptr [esp+NbStack-32]
        best_len        equ     dword ptr [esp+NbStack-36]
        window          equ     dword ptr [esp+NbStack-40]
        prev            equ     dword ptr [esp+NbStack-44]
        scan_start      equ      word ptr [esp+NbStack-48]
        wmask           equ     dword ptr [esp+NbStack-52]
        match_start_ptr equ     dword ptr [esp+NbStack-56]
        nice_match      equ     dword ptr [esp+NbStack-60]
        scan            equ     dword ptr [esp+NbStack-64]

        windowlen       equ     dword ptr [esp+NbStack-68]
        match_start     equ     dword ptr [esp+NbStack-72]
        strend          equ     dword ptr [esp+NbStack-76]
        NbStackAdd      equ     (NbStack-24)

    .386p

    name    gvmatch
    .MODEL  FLAT



;  all the +4 offsets are due to the addition of pending_buf_size (in zlib
;  in the deflate_state structure since the asm code was first written
;  (if you compile with zlib 1.0.4 or older, remove the +4).
;  Note : these value are good with a 8 bytes boundary pack structure
    dep_chain_length    equ     70h+4
    dep_window          equ     2ch+4
    dep_strstart        equ     60h+4
    dep_prev_length     equ     6ch+4
    dep_nice_match      equ     84h+4
    dep_w_size          equ     20h+4
    dep_prev            equ     34h+4
    dep_w_mask          equ     28h+4
    dep_good_match      equ     80h+4
    dep_match_start     equ     64h+4
    dep_lookahead       equ     68h+4


_TEXT                   segment

IFDEF NOUNDERLINE
                        public  longest_match_7fff
;                        public  match_init
ELSE
                        public  _longest_match_7fff
;                        public  _match_init
ENDIF

    MAX_MATCH           equ     258
    MIN_MATCH           equ     3
    MIN_LOOKAHEAD       equ     (MAX_MATCH+MIN_MATCH+1)



IFDEF NOUNDERLINE
;match_init      proc near
;                ret
;match_init      endp
ELSE
;_match_init     proc near
;                ret
;_match_init     endp
ENDIF


IFDEF NOUNDERLINE
longest_match_7fff   proc near
ELSE
_longest_match_7fff  proc near
ENDIF

        mov     edx,[esp+4]



        push    ebp
        push    edi
        push    esi
        push    ebx

        sub     esp,NbStackAdd

; initialize or check the variables used in match.asm.
        mov     ebp,edx

; chain_length = s->max_chain_length
; if (prev_length>=good_match) chain_length >>= 2
        mov     edx,[ebp+dep_chain_length]
        mov     ebx,[ebp+dep_prev_length]
        cmp     [ebp+dep_good_match],ebx
        ja      noshr
        shr     edx,2
noshr:
; we increment chain_length because in the asm, the --chain_lenght is in the beginning of the loop
        inc     edx
        mov     edi,[ebp+dep_nice_match]
        mov     chain_length,edx
        mov     eax,[ebp+dep_lookahead]
        cmp     eax,edi
; if ((uInt)nice_match > s->lookahead) nice_match = s->lookahead;
        jae     nolookaheadnicematch
        mov     edi,eax
nolookaheadnicematch:
; best_len = s->prev_length
        mov     best_len,ebx

; window = s->window
        mov     esi,[ebp+dep_window]
        mov     ecx,[ebp+dep_strstart]
        mov     window,esi

        mov     nice_match,edi
; scan = window + strstart
        add     esi,ecx
        mov     scan,esi
; dx = *window
        mov     dx,word ptr [esi]
; bx = *(window+best_len-1)
        mov     bx,word ptr [esi+ebx-1]
        add     esi,MAX_MATCH-1
; scan_start = *scan
        mov     scan_start,dx
; strend = scan + MAX_MATCH-1
        mov     strend,esi
; bx = scan_end = *(window+best_len-1)

;    IPos limit = s->strstart > (IPos)MAX_DIST(s) ?
;        s->strstart - (IPos)MAX_DIST(s) : NIL;

        mov     esi,[ebp+dep_w_size]
        sub     esi,MIN_LOOKAHEAD
; here esi = MAX_DIST(s)
        sub     ecx,esi
        ja      nodist
        xor     ecx,ecx
nodist:
        mov     limit,ecx

; prev = s->prev
        mov     edx,[ebp+dep_prev]
        mov     prev,edx

;
        mov     edx,dword ptr [ebp+dep_match_start]
        mov     bp,scan_start
        mov     eax,cur_match
        mov     match_start,edx

        mov     edx,window
        mov     edi,edx
        add     edi,best_len
        mov     esi,prev
        dec     edi
; windowlen = window + best_len -1
        mov     windowlen,edi

        jmp     beginloop2
        align   4

; here, in the loop
;       eax = ax = cur_match
;       ecx = limit
;        bx = scan_end
;        bp = scan_start
;       edi = windowlen (window + best_len -1)
;       esi = prev


;// here; chain_length <=16
normalbeg0add16:
        add     chain_length,16
        jz      exitloop
normalbeg0:
        cmp     word ptr[edi+eax],bx
        je      normalbeg2noroll
rcontlabnoroll:
; cur_match = prev[cur_match & wmask]
        and     eax,7fffh
        mov     ax,word ptr[esi+eax*2]
; if cur_match > limit, go to exitloop
        cmp     ecx,eax
        jnb     exitloop
; if --chain_length != 0, go to exitloop
        dec     chain_length
        jnz     normalbeg0
        jmp     exitloop

normalbeg2noroll:
; if (scan_start==*(cur_match+window)) goto normalbeg2
        cmp     bp,word ptr[edx+eax]
        jne     rcontlabnoroll
        jmp     normalbeg2

contloop3:
        mov     edi,windowlen

; cur_match = prev[cur_match & wmask]
        and     eax,7fffh
        mov     ax,word ptr[esi+eax*2]
; if cur_match > limit, go to exitloop
        cmp     ecx,eax
jnbexitloopshort1:
        jnb     exitloop
; if --chain_length != 0, go to exitloop


; begin the main loop
beginloop2:
        sub     chain_length,16+1
; if chain_length <=16, don't use the unrolled loop
        jna     normalbeg0add16

do16:
        cmp     word ptr[edi+eax],bx
        je      normalbeg2dc0

maccn   MACRO   lab
        and     eax,7fffh
        mov     ax,word ptr[esi+eax*2]
        cmp     ecx,eax
        jnb     exitloop
        cmp     word ptr[edi+eax],bx
        je      lab
        ENDM

rcontloop0:
        maccn   normalbeg2dc1

rcontloop1:
        maccn   normalbeg2dc2

rcontloop2:
        maccn   normalbeg2dc3

rcontloop3:
        maccn   normalbeg2dc4

rcontloop4:
        maccn   normalbeg2dc5

rcontloop5:
        maccn   normalbeg2dc6

rcontloop6:
        maccn   normalbeg2dc7

rcontloop7:
        maccn   normalbeg2dc8

rcontloop8:
        maccn   normalbeg2dc9

rcontloop9:
        maccn   normalbeg2dc10

rcontloop10:
        maccn   short normalbeg2dc11

rcontloop11:
        maccn   short normalbeg2dc12

rcontloop12:
        maccn   short normalbeg2dc13

rcontloop13:
        maccn   short normalbeg2dc14

rcontloop14:
        maccn   short normalbeg2dc15

rcontloop15:
        and     eax,7fffh
        mov     ax,word ptr[esi+eax*2]
        cmp     ecx,eax
        jnb     exitloop

        sub     chain_length,16
        ja      do16
        jmp     normalbeg0add16

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

normbeg MACRO   rcontlab,valsub
; if we are here, we know that *(match+best_len-1) == scan_end
        cmp     bp,word ptr[edx+eax]
; if (match != scan_start) goto rcontlab
        jne     rcontlab
; calculate the good chain_length, and we'll compare scan and match string
        add     chain_length,16-valsub
        jmp     iseq
        ENDM


normalbeg2dc11:
        normbeg rcontloop11,11

normalbeg2dc12:
        normbeg short rcontloop12,12

normalbeg2dc13:
        normbeg short rcontloop13,13

normalbeg2dc14:
        normbeg short rcontloop14,14

normalbeg2dc15:
        normbeg short rcontloop15,15

normalbeg2dc10:
        normbeg rcontloop10,10

normalbeg2dc9:
        normbeg rcontloop9,9

normalbeg2dc8:
        normbeg rcontloop8,8

normalbeg2dc7:
        normbeg rcontloop7,7

normalbeg2dc6:
        normbeg rcontloop6,6

normalbeg2dc5:
        normbeg rcontloop5,5

normalbeg2dc4:
        normbeg rcontloop4,4

normalbeg2dc3:
        normbeg rcontloop3,3

normalbeg2dc2:
        normbeg rcontloop2,2

normalbeg2dc1:
        normbeg rcontloop1,1

normalbeg2dc0:
        normbeg rcontloop0,0


; we go in normalbeg2 because *(ushf*)(match+best_len-1) == scan_end

normalbeg2:
        mov     edi,window

        cmp     bp,word ptr[edi+eax]
        jne     contloop3                   ; if *(ushf*)match != scan_start, continue

iseq:
; if we are here, we know that *(match+best_len-1) == scan_end
; and (match == scan_start)

        mov     edi,edx
        mov     esi,scan                    ; esi = scan
        add     edi,eax                     ; edi = window + cur_match = match

        mov     edx,[esi+3]                 ; compare manually dword at match+3
        xor     edx,[edi+3]                 ; and scan +3

        jz      begincompare                ; if equal, go to long compare

; we will determine the unmatch byte and calculate len (in esi)
        or      dl,dl
        je      eq1rr
        mov     esi,3
        jmp     trfinval
eq1rr:
        or      dx,dx
        je      eq1

        mov     esi,4
        jmp     trfinval
eq1:
        and     edx,0ffffffh
        jz      eq11
        mov     esi,5
        jmp     trfinval
eq11:
        mov     esi,6
        jmp     trfinval

begincompare:
        ; here we now scan and match begin same
        add     edi,6
        add     esi,6
        mov     ecx,(MAX_MATCH-(2+4))/4     ; scan for at most MAX_MATCH bytes
        repe    cmpsd                       ; loop until mismatch

        je      trfin                       ; go to trfin if not unmatch
; we determine the unmatch byte
        sub     esi,4
        mov     edx,[edi-4]
        xor     edx,[esi]

        or      dl,dl
        jnz     trfin
        inc     esi

        or      dx,dx
        jnz     trfin
        inc     esi

        and     edx,0ffffffh
        jnz     trfin
        inc     esi

trfin:
        sub     esi,scan          ; esi = len
trfinval:
; here we have finised compare, and esi contain len of equal string
        cmp     esi,best_len        ; if len > best_len, go newbestlen
        ja      short newbestlen
; now we restore edx, ecx and esi, for the big loop
        mov     esi,prev
        mov     ecx,limit
        mov     edx,window
        jmp     contloop3

newbestlen:
        mov     best_len,esi        ; len become best_len

        mov     match_start,eax     ; save new position as match_start
        cmp     esi,nice_match      ; if best_len >= nice_match, exit
        jae     exitloop
        mov     ecx,scan
        mov     edx,window          ; restore edx=window
        add     ecx,esi
        add     esi,edx

        dec     esi
        mov     windowlen,esi       ; windowlen = window + best_len-1
        mov     bx,[ecx-1]          ; bx = *(scan+best_len-1) = scan_end

; now we restore ecx and esi, for the big loop :
        mov     esi,prev
        mov     ecx,limit
        jmp     contloop3

exitloop:
; exit : s->match_start=match_start
        mov     ebx,match_start
        mov     ebp,str_s
        mov     ecx,best_len
        mov     dword ptr [ebp+dep_match_start],ebx        
        mov     eax,dword ptr [ebp+dep_lookahead]
        cmp     ecx,eax
        ja      minexlo
        mov     eax,ecx
minexlo:
; return min(best_len,s->lookahead)
        
; restore stack and register ebx,esi,edi,ebp
        add     esp,NbStackAdd

        pop     ebx
        pop     esi
        pop     edi
        pop     ebp
        ret
InfoAuthor:
; please don't remove this string !
; Your are free use gvmat32 in any fre or commercial apps if you don't remove the string in the binary!
        db     0dh,0ah,"GVMat32 optimised assembly code written 1996-98 by Gilles Vollant",0dh,0ah



IFDEF NOUNDERLINE
longest_match_7fff   endp
ELSE
_longest_match_7fff  endp
ENDIF


IFDEF NOUNDERLINE
cpudetect32     proc near
ELSE
_cpudetect32    proc near
ENDIF


	pushfd                  ; push original EFLAGS
	pop     eax             ; get original EFLAGS
	mov     ecx, eax        ; save original EFLAGS
	xor     eax, 40000h     ; flip AC bit in EFLAGS
	push    eax             ; save new EFLAGS value on stack
	popfd                   ; replace current EFLAGS value
	pushfd                  ; get new EFLAGS
	pop     eax             ; store new EFLAGS in EAX
	xor     eax, ecx        ; can’t toggle AC bit, processor=80386
	jz      end_cpu_is_386  ; jump if 80386 processor
	push    ecx
	popfd                   ; restore AC bit in EFLAGS first

	pushfd
	pushfd
	pop     ecx
			
	mov     eax, ecx        ; get original EFLAGS
	xor     eax, 200000h    ; flip ID bit in EFLAGS
	push    eax             ; save new EFLAGS value on stack
	popfd                   ; replace current EFLAGS value
	pushfd                  ; get new EFLAGS
	pop		eax	            ; store new EFLAGS in EAX
	popfd                   ; restore original EFLAGS
	xor		eax, ecx        ; can’t toggle ID bit,
	je		is_old_486		; processor=old

	mov     eax,1
	db      0fh,0a2h        ;CPUID   

exitcpudetect:
	ret

end_cpu_is_386:
	mov     eax,0300h
	jmp     exitcpudetect

is_old_486:
	mov     eax,0400h
	jmp     exitcpudetect

IFDEF NOUNDERLINE
cpudetect32     endp
ELSE
_cpudetect32    endp
ENDIF

_TEXT   ends
end
