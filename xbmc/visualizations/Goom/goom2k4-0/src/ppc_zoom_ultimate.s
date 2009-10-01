; PowerPC optimized zoom for Goom
; © 2001-2003 Guillaume Borios
; This Source Code is released under the terms of the General Public License

; Change log :
; 21 Dec 2003 : Use of altivec is now determined with a parameter

; Section definition : We use a read only section
.text

; name of the function to call by C program : ppc_zoom
; We declare this label as a global to extend its scope outside this file
.globl _ppc_zoom_generic
.globl _ppc_zoom_G4

; Description :
; This routine dynamically computes and applies a zoom filter

; parameters :
; r3  <=> unsigned int sizeX (in pixels)
; r4  <=> unsigned int sizeY (in pixels)
; r5  <=> unsigned int * frompixmap
; r6  <=> unsigned int * topixmap
; r7  <=> unsigned int * brutS
; r8  <=> unsigned int * brutD
; r9  <=> unsigned int buffratio
; r10 <=> int [16][16] precalccoeffs

; globals after init
; r5  <=> frompixmap - 1 byte needed for preincremental fetch (replaces r5)
; r6  <=> topixmap - 1 byte needed for preincremental fetch (replaces r6)
; r3 <=> ax = x max in 16th of pixels (replaces old r3)
; r4 <=> ay = y max in 16th of pixels (replaces old r4)
; r20 <=> row size in bytes
; r12 <=> 0xFF00FF (mask for parallel 32 bits pixs computing)
; r30 <=> brutS - 1 byte needed for preincremental fetch (replaces r7)
; r31 <=> brutD - 1 byte needed for preincremental fetch (replaces r8)

; ABI notes :
; r1 is the Stack Pointer (SP) => Do not use
; r13..r31 are non-volatiles => Do not use

_ppc_zoom_generic:

; Saves the used non volatile registers in the Mach-O stack s Red-Zone
stmw 	r18,-56(r1)

; init
li      r18,0		; Default value if out of range : 0 (Black)
mr      r11,r10
lis     r12,0xFF
mullw   r2,r3,r4	; Number of pixels to compute
subi    r30,r8,0
slwi	r20,r3,2
srawi   r19,r20,2
ori     r12,r12,0xFF
subi    r3,r3,1
subi    r4,r4,1
mtspr	ctr,r2		; Init the loop count (one loop per pixel computed)
subi    r31,r7,0
subi    r6,r6,4
slwi	r3,r3,4
slwi	r4,r4,4

;pre init for loop
lwz	r2,0(r31)    ; px
lwz	r29,4(r31)   ; py
lwz	r8,0(r30)    ; px2
lwz	r10,4(r30)   ; py2

b       L1
.align  5
L1:

; computes dynamically the position to fetch
sub     r8,r8,r2
sub     r10,r10,r29
mullw   r8,r8,r9
addi    r31,r31,8
mullw   r10,r10,r9
addi    r30,r30,8

srawi   r8,r8,16
srawi   r10,r10,16
add     r2,r2,r8
add     r29,r29,r10

; if px>ax or py>ay goto outofrange
; computes the attenuation coeffs and the original point address
rlwinm  r10,r2,6,28-6,31-6 ; r10 <- (r2 << 2) & 0x000002D0   (r10=(r2%16)*4*16)
cmpl    cr4,0,r2,r3
rlwimi  r10, r29, 2, 28-2, 31-2 ; r10 <- ((r29 << 2) & 0x0000002D) | (r10 & !0x0000002D)      (r10=(r10%16)*4 | r10)
cmpl    cr7,0,r29,r4
srawi   r29,r29,4     ; pos computing
bge-	cr4,L4
srawi   r2,r2,4       ; pos computing
mullw   r29, r29,r19  ; pos computing
bge-	cr7,L4

; Channels notation : 00112233 (AARRVVBB)

add     r2,r2,r29    		; pos computing
lwzx    r10,r11,r10		; Loads coefs
slwi    r2,r2,2      		; pos computing
add	r2,r2,r5     		; pos computing
rlwinm  r21,r10,0,24,31	        ; Isolates coef1 (??????11 -> 00000011)
lwz	r25,0(r2)		; Loads col1 -> r25
lwz	r26,4(r2)		; Loads col2 -> r26
rlwinm  r22,r10,24,24,31	; Isolates coef2 (????22?? -> 00000022)
rlwinm  r23,r10,16,24,31	; Isolates coef3 (??33???? -> 00000033)
add	r2,r2,r20		; Adds one line for future load of col3 and col4
and	r8, r25,r12		; Masks col1 channels 1 & 3 : 0x00XX00XX
rlwinm  r24,r10,8,24,31		; Isolates coef4 (44?????? -> 00000044)
andi.	r25,r25,0xFF00		; Masks col1 channel 2 : 0x0000XX00
mullw	r8, r8, r21		; Applies coef1 on col1 channels 1 & 3


; computes final pixel color
and	r10,r26,r12		; Masks col2 channels 1 & 3 : 0x00XX00XX
lwz	r27,0(r2)		; Loads col3 -> r27
mullw	r10,r10,r22		; Applies coef2 on col2 channels 1 & 3
mullw	r25,r25,r21		; Applies coef1 on col1 channel 2
andi.	r29,r26,0xFF00		; Masks col2 channel 2 : 0x0000XX00
mullw	r29,r29,r22		; Applies coef2 on col2 channel 2
lwz	r28,4(r2)		; Loads col4 -> r28
add	r8 ,r8 ,r10		; Adds col1 & col2 channels 1 & 3
and	r10,r27,r12		; Masks col3 channels 1 & 3 : 0x00XX00XX
add	r25,r25,r29		; Adds col1 & col2 channel 2
mullw	r10,r10,r23		; Applies coef3 on col3 channels 1 & 3
andi.	r29,r27,0xFF00		; Masks col3 channel 2 : 0x0000XX00
mullw	r29,r29,r23		; Applies coef3 on col3 channel 2
lwz	r2,0(r31)		; px
add	r7 ,r8 ,r10		; Adds col3 to (col1 + col2) channels 1 & 3
and	r10,r28,r12		; Masks col4 channels 1 & 3 : 0x00XX00XX
mullw	r10,r10,r24		; Applies coef4 on col4 channels 1 & 3
add	r25,r25,r29		; Adds col 3 to (col1 + col2) channel 2
lwz 	r8,0(r30)    		; px2
andi.	r28,r28,0xFF00		; Masks col4 channel 2 : 0x0000XX00
add	r7 ,r7 ,r10		; Adds col4 to (col1 + col2 + col3) channels 1 & 3
lwz	r10,4(r30)   		; py2
mullw	r28,r28,r24		; Applies coef4 on col4 channel 2
srawi	r7, r7, 8		; (sum of channels 1 & 3) >> 8
lwz	r29,4(r31)              ; py
add	r25,r25,r28		; Adds col 4 to (col1 + col2 + col3) channel 2
rlwimi  r7, r25, 24, 16, 23	; (((sum of channels 2) >> 8 ) & 0x0000FF00) | ((sum of channels 1 and 3) & 0xFFFF00FF)
stwu	r7,4(r6)		; Stores the computed pixel
bdnz	L1			; Iterate again if needed
b       L3	;goto end	; If not, returns from the function


; if out of range
L4:
stwu	r18,4(r6)
lwz	r8,0(r30)    ; px2
lwz	r10,4(r30)   ; py2
lwz	r2,0(r31)    ; px
lwz	r29,4(r31)   ; py
bdnz	L1


L3:

; Restore saved registers and return
lmw	r18,-56(r1)
blr








_ppc_zoom_G4:

; Saves the used non volatile registers in the Mach-O stack s Red-Zone
stmw 	r17,-60(r1)

; init
li      r18,0		; Default value if out of range : 0 (Black)
mr      r11,r10
lis     r12,0xFF
mullw   r2,r3,r4	; Number of pixels to compute
subi    r30,r8,0
slwi	r20,r3,2
srawi   r19,r20,2
ori     r12,r12,0xFF
subi    r3,r3,1
subi    r4,r4,1
mtspr	ctr,r2		; Init the loop count (one loop per pixel computed)
subi    r31,r7,0
subi    r6,r6,4
slwi	r3,r3,4
slwi	r4,r4,4

;pre init for loop
lwz	r2,0(r31)    ; px
lwz	r29,4(r31)   ; py
lwz	r8,0(r30)    ; px2
lwz	r10,4(r30)   ; py2

;*********************
lis     r17,0x0F01

b       L100
.align  5
L100:

addi    r6,r6,4

; Optimization to ensure the destination buffer
; won't be loaded into the data cache
rlwinm. r0,r6,0,27,31
bne+    L500
dcbz    0,r6
;dcba    0,r6
L500:

; computes dynamically the position to fetch
;mullw   r8,r8,r29
;mullw   r2,r2,r29
;add     r2,r8,r2
;srawi   r2,r2,17

sub     r8,r8,r2
sub     r10,r10,r29
mullw   r8,r8,r9
addi    r31,r31,8
mullw   r10,r10,r9
addi    r30,r30,8

dst     r30,r17,0

srawi    r8,r8,16
srawi    r10,r10,16
add     r2,r2,r8
add     r29,r29,r10

dst     r31,r17,1

; if px>ax or py>ay goto outofrange
; computes the attenuation coeffs and the original point address
rlwinm  r10,r2,6,28-6,31-6 ; r10 <- (r2 << 2) & 0x000002D0   (r10=(r2%16)*4*16)
cmpl    cr4,0,r2,r3
rlwimi  r10, r29, 2, 28-2, 31-2 ; r10 <- ((r29 << 2) & 0x0000002D) | (r10 & !0x0000002D)      (r10=(r29%16)*4 | r10)
cmpl    cr7,0,r29,r4
srawi   r29,r29,4     ; pos computing
bge-	cr4,L400
srawi   r2,r2,4       ; pos computing
mullw   r29, r29,r19  ; pos computing
bge-	cr7,L400

; Channels notation : 00112233 (AARRVVBB)

add     r2,r2,r29    		; pos computing
lwzx    r10,r11,r10		; Loads coefs
slwi    r2,r2,2      		; pos computing
add	r2,r2,r5     		; pos computing
rlwinm  r21,r10,0,24,31	        ; Isolates coef1 (??????11 -> 00000011)
lwz	r25,0(r2)		; Loads col1 -> r25
lwz	r26,4(r2)		; Loads col2 -> r26
rlwinm  r22,r10,24,24,31	; Isolates coef2 (????22?? -> 00000022)
rlwinm  r23,r10,16,24,31	; Isolates coef3 (??33???? -> 00000033)
add	r2,r2,r20		; Adds one line for future load of col3 and col4
and	r8, r25,r12		; Masks col1 channels 1 & 3 : 0x00XX00XX
rlwinm  r24,r10,8,24,31		; Isolates coef4 (44?????? -> 00000044)
dst     r2,r17,2
rlwinm  r25,r25,0,16,23		; Masks col1 channel 2 : 0x0000XX00
;andi.	r25,r25,0xFF00		; Masks col1 channel 2 : 0x0000XX00
mullw	r8, r8, r21		; Applies coef1 on col1 channels 1 & 3


; computes final pixel color
and	r10,r26,r12		; Masks col2 channels 1 & 3 : 0x00XX00XX
lwz	r27,0(r2)		; Loads col3 -> r27
mullw	r10,r10,r22		; Applies coef2 on col2 channels 1 & 3
mullw	r25,r25,r21		; Applies coef1 on col1 channel 2
rlwinm  r29,r26,0,16,23		; Masks col2 channel 2 : 0x0000XX00
;andi.	r29,r26,0xFF00		; Masks col2 channel 2 : 0x0000XX00
mullw	r29,r29,r22		; Applies coef2 on col2 channel 2
lwz	r28,4(r2)		; Loads col4 -> r28
add	r8 ,r8 ,r10		; Adds col1 & col2 channels 1 & 3
and	r10,r27,r12		; Masks col3 channels 1 & 3 : 0x00XX00XX
add	r25,r25,r29		; Adds col1 & col2 channel 2
mullw	r10,r10,r23		; Applies coef3 on col3 channels 1 & 3
rlwinm  r29,r27,0,16,23		; Masks col3 channel 2 : 0x0000XX00
;andi.	r29,r27,0xFF00		; Masks col3 channel 2 : 0x0000XX00
mullw	r29,r29,r23		; Applies coef3 on col3 channel 2
lwz	r2,0(r31)		; px
add	r7 ,r8 ,r10		; Adds col3 to (col1 + col2) channels 1 & 3
and	r10,r28,r12		; Masks col4 channels 1 & 3 : 0x00XX00XX
mullw	r10,r10,r24		; Applies coef4 on col4 channels 1 & 3
add	r25,r25,r29		; Adds col 3 to (col1 + col2) channel 2
lwz 	r8,0(r30)    		; px2
rlwinm  r28,r28,0,16,23		; Masks col4 channel 2 : 0x0000XX00
;andi.	r28,r28,0xFF00		; Masks col4 channel 2 : 0x0000XX00
add	r7 ,r7 ,r10		; Adds col4 to (col1 + col2 + col3) channels 1 & 3
lwz	r10,4(r30)   		; py2
mullw	r28,r28,r24		; Applies coef4 on col4 channel 2
srawi	r7, r7, 8		; (sum of channels 1 & 3) >> 8
lwz	r29,4(r31)              ; py
add	r25,r25,r28		; Adds col 4 to (col1 + col2 + col3) channel 2
rlwimi  r7, r25, 24, 16, 23	; (((sum of channels 2) >> 8 ) & 0x0000FF00) | ((sum of channels 1 and 3) & 0xFFFF00FF)
stw	r7,0(r6)		; Stores the computed pixel
bdnz	L100			; Iterate again if needed
b       L300	;goto end	; If not, returns from the function


; if out of range
L400:
stw	r18,0(r6)
lwz	r8,0(r30)    ; px2
lwz	r10,4(r30)   ; py2
lwz	r2,0(r31)    ; px
lwz	r29,4(r31)   ; py
bdnz	L100


L300:

; Restore saved registers and return
lmw	r17,-60(r1)
blr
