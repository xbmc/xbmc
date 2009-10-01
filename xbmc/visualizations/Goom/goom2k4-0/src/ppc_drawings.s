; PowerPC optimized drawing methods for Goom
; © 2003 Guillaume Borios
; This Source Code is released under the terms of the General Public License

; Change log :
; 30 May 2003 : File creation

; Section definition : We use a read only code section for the whole file
.section __TEXT,__text,regular,pure_instructions


; --------------------------------------------------------------------------------------
; Single 32b pixel drawing macros
; Usage :
; 	DRAWMETHOD_XXXX_MACRO *pixelIN, *pixelOUT, COLOR, WR1, WR2, WR3, WR4
;	Only the work registers (WR) can be touched by the macros
;
; Available methods :
;	DRAWMETHOD_DFLT_MACRO : Default drawing method (Actually OVRW)
;	DRAWMETHOD_PLUS_MACRO : RVB Saturated per channel addition (SLOWEST)
;	DRAWMETHOD_HALF_MACRO : 50% Transparency color drawing
;	DRAWMETHOD_OVRW_MACRO : Direct COLOR drawing (FASTEST)
;	DRAWMETHOD_B_OR_MACRO : Bitwise OR
;	DRAWMETHOD_BAND_MACRO : Bitwise AND
;	DRAWMETHOD_BXOR_MACRO : Bitwise XOR
;	DRAWMETHOD_BNOT_MACRO : Bitwise NOT
; --------------------------------------------------------------------------------------

.macro DRAWMETHOD_OVRW_MACRO
    stw		$2,0($1)	;; *$1 <- $2
.endmacro

.macro DRAWMETHOD_B_OR_MACRO
    lwz		$3,0($0)	;; $3 <- *$0
    or		$3,$3,$2	;; $3 <- $3 | $2
    stw		$3,0($1)	;; *$1 <- $3
.endmacro

.macro DRAWMETHOD_BAND_MACRO
    lwz		$3,0($0)	;; $3 <- *$0
    and		$3,$3,$2	;; $3 <- $3 & $2
    stw		$3,0($1)	;; *$1 <- $3
.endmacro

.macro DRAWMETHOD_BXOR_MACRO
    lwz		$3,0($0)	;; $3 <- *$0
    xor		$3,$3,$2	;; $3 <- $3 ^ $2
    stw		$3,0($1)	;; *$1 <- $3
.endmacro

.macro DRAWMETHOD_BNOT_MACRO
    lwz		$3,0($0)	;; $3 <- *$0
    nand	$3,$3,$3	;; $3 <- ~$3
    stw		$3,0($1)	;; *$1 <- $3
.endmacro

.macro DRAWMETHOD_PLUS_MACRO
    lwz		$4,0($0)	;; $4 <- *$0
    andi.	$3,$4,0xFF00	;; $3 <- $4 & 0x0000FF00
    andi.	$5,$2,0xFF00	;; $5 <- $2 & 0x0000FF00
    add		$3,$3,$5	;; $3 <- $3 + $5
    rlwinm	$5,$3,15,0,0	;; $5 <- 0 | ($3[15] << 15)
    srawi	$5,$5,23	;; $5 <- $5 >> 23 (algebraic for sign extension)
    or		$3,$3,$5	;; $3 <- $3 | $5
    lis		$5,0xFF		;; $5 <- 0x00FF00FF
    addi	$5,$5,0xFF
    and		$4,$4,$5	;; $4 <- $4 & $5
    and		$6,$2,$5	;; $6 <- $2 & $5
    add		$4,$4,$6	;; $4 <- $4 + $6
    rlwinm	$6,$4,7,0,0	;; $6 <- 0 | ($4[7] << 7)
    srawi	$6,$6,15	;; $6 <- $6 >> 15 (algebraic for sign extension)
    rlwinm	$5,$4,23,0,0	;; $5 <- 0 | ($4[23] << 23)
    srawi	$5,$5,31	;; $5 <- $5 >> 31 (algebraic for sign extension)
    rlwimi	$6,$5,0,24,31	;; $6[24..31] <- $5[24..31]
    or		$4,$4,$6	;; $4 <- $4 | $6
    rlwimi	$4,$3,0,16,23	;; $4[16..23] <- $3[16..23]
    stw		$4,0($1)	;; *$1 <- $4
.endmacro

.macro	DRAWMETHOD_HALF_MACRO
    lwz		$4,0($0)	;; $4 <- *$0
    andi.	$3,$4,0xFF00	;; $3 <- $4 & 0x0000FF00
    andi.	$5,$2,0xFF00	;; $5 <- $2 & 0x0000FF00
    add		$3,$3,$5	;; $3 <- $3 + $5
    lis		$5,0xFF		;; $5 <- 0x00FF00FF
    addi	$5,$5,0xFF
    and		$4,$4,$5	;; $4 <- $4 & $5
    and		$5,$2,$5	;; $5 <- $2 & $5
    add		$4,$4,$5	;; $4 <- $4 + $5
    srwi	$4,$4,1		;; $4 <- $4 >> 1
    rlwimi	$4,$3,31,16,23	;; $4[16..23] <- $3[15..22]
    stw		$4,0($1)	;; *$1 <- $4
.endmacro

.macro DRAWMETHOD_DFLT_MACRO
    DRAWMETHOD_PLUS_MACRO
.endmacro

; --------------------------------------------------------------------------------------



; **************************************************************************************
; void DRAWMETHOD_PLUS_PPC(unsigned int * buf, unsigned int _col);
; void DRAWMETHOD_PLUS_2_PPC(unsigned * in, unsigned int * out, unsigned int _col);
; **************************************************************************************
.globl _DRAWMETHOD_PLUS_2_PPC
.align 3
_DRAWMETHOD_PLUS_2_PPC:
    DRAWMETHOD_PLUS_MACRO	r3,r4,r5,r6,r7,r8,r9
    blr				;; return

.globl _DRAWMETHOD_PLUS_PPC
.align 3
_DRAWMETHOD_PLUS_PPC:
    DRAWMETHOD_PLUS_MACRO	r3,r3,r4,r5,r6,r7,r9
    blr				;; return


; **************************************************************************************
; void DRAWMETHOD_HALF_PPC(unsigned int * buf, unsigned int _col);
; void DRAWMETHOD_HALF_2_PPC(unsigned * in, unsigned int * out, unsigned int _col);
; **************************************************************************************
.globl _DRAWMETHOD_HALF_2_PPC
.align 3
_DRAWMETHOD_HALF_2_PPC:
    DRAWMETHOD_HALF_MACRO	r3,r4,r5,r6,r7,r8
    blr				;; return

.globl _DRAWMETHOD_HALF_PPC
.align 3
_DRAWMETHOD_HALF_PPC:
    DRAWMETHOD_HALF_MACRO	r3,r3,r4,r5,r6,r7
    blr				;; return


; **************************************************************************************
; void DRAW_LINE_PPC(unsigned int *data, int x1, int y1, int x2, int y2, unsigned int col,
; 			unsigned int screenx, unsigned int screeny)
; **************************************************************************************
.globl _DRAW_LINE_PPC
.align 3
_DRAW_LINE_PPC:
    ;; NOT IMPLEMENTED YET
    blr				;; return


; **************************************************************************************
; void _ppc_brightness(Pixel * src, Pixel * dest, unsigned int size, unsigned int coeff)
; **************************************************************************************


.const
.align 4
vectorZERO:
    .long 0,0,0,0
    .long 0x10101000, 0x10101001, 0x10101002, 0x10101003
    .long 0x10101004, 0x10101005, 0x10101006, 0x10101007
    .long 0x10101008, 0x10101009, 0x1010100A, 0x1010100B
    .long 0x1010100C, 0x1010100D, 0x1010100E, 0x1010100F


.section __TEXT,__text,regular,pure_instructions

.globl _ppc_brightness_G4
.align 3
_ppc_brightness_G4:


;; PowerPC Altivec code
    srwi    r5,r5,2
    mtctr   r5

;;vrsave
    mfspr   r11,256
    lis     r12,0xCFFC
    mtspr   256,r12

        mflr r0
        bcl 20,31,"L00000000001$pb"
"L00000000001$pb":
        mflr r10
        mtlr r0

    addis   r9,r10,ha16(vectorZERO-"L00000000001$pb")
    addi    r9,r9,lo16(vectorZERO-"L00000000001$pb")
    
    vxor    v0,v0,v0 ;; V0 = NULL vector

    addi    r9,r9,16
    lvx     v10,0,r9
    addi    r9,r9,16
    lvx     v11,0,r9
    addi    r9,r9,16
    lvx     v12,0,r9
    addi    r9,r9,16
    lvx     v13,0,r9

    addis   r9,r10,ha16(vectortmpwork-"L00000000001$pb")
    addi    r9,r9,lo16(vectortmpwork-"L00000000001$pb")
    stw     r6,0(r9)
    li      r6,8
    stw     r6,4(r9)
    lvx     v9,0,r9
    li      r9,128
    vspltw  v8,v9,0
    vspltw  v9,v9,1

;; elt counter
    li      r9,0
    lis     r7,0x0F01
    b L7
.align 4
L7:
    lvx     v1,r9,r3

    vperm   v4,v1,v0,v10
    ;*********************
     add r10,r9,r3
    ;*********************
    vperm   v5,v1,v0,v11
    vperm   v6,v1,v0,v12
    vperm   v7,v1,v0,v13

    vmulouh  v4,v4,v8
    ;*********************
     dst     r10,r7,3
    ;*********************
    vmulouh  v5,v5,v8
    vmulouh  v6,v6,v8
    vmulouh  v7,v7,v8
    vsrw     v4,v4,v9
    vsrw     v5,v5,v9
    vsrw     v6,v6,v9
    vsrw     v7,v7,v9 
    
    vpkuwus v4,v4,v5
    vpkuwus v6,v6,v7
    vpkuhus v1,v4,v6

    stvx    v1,r9,r4
    addi    r9,r9,16

    bdnz L7

    mtspr   256,r11
    blr


.globl _ppc_brightness_G5
.align 3
_ppc_brightness_G5:

;; PowerPC Altivec G5 code
    srwi    r5,r5,2
    mtctr   r5

;;vrsave
    mfspr   r11,256
    lis     r12,0xCFFC
    mtspr   256,r12

        mflr r0
        bcl 20,31,"L00000000002$pb"
"L00000000002$pb":
        mflr r10
        mtlr r0

    addis   r9,r10,ha16(vectorZERO-"L00000000002$pb")
    addi    r9,r9,lo16(vectorZERO-"L00000000002$pb")
    
    vxor    v0,v0,v0 ;; V0 = NULL vector

    addi    r9,r9,16
    lvx     v10,0,r9
    addi    r9,r9,16
    lvx     v11,0,r9
    addi    r9,r9,16
    lvx     v12,0,r9
    addi    r9,r9,16
    lvx     v13,0,r9

    addis   r9,r10,ha16(vectortmpwork-"L00000000002$pb")
    addi    r9,r9,lo16(vectortmpwork-"L00000000002$pb")
    stw     r6,0(r9)
    li      r6,8
    stw     r6,4(r9)
    lvx     v9,0,r9
    li      r9,128
    vspltw  v8,v9,0
    vspltw  v9,v9,1

;; elt counter
    li      r9,0
    lis     r7,0x0F01
    b L6
.align 4
L6:
    lvx     v1,r9,r3

    vperm   v4,v1,v0,v10
    ;*********************
    add r10,r9,r3
    ;*********************
    vperm   v5,v1,v0,v11
    vperm   v6,v1,v0,v12
    vperm   v7,v1,v0,v13

    vmulouh  v4,v4,v8
    vmulouh  v5,v5,v8
    vmulouh  v6,v6,v8
    vmulouh  v7,v7,v8
    vsrw     v4,v4,v9
    vsrw     v5,v5,v9
    vsrw     v6,v6,v9
    vsrw     v7,v7,v9 
    
    vpkuwus v4,v4,v5
    vpkuwus v6,v6,v7
    vpkuhus v1,v4,v6

    stvx    v1,r9,r4
    addi    r9,r9,16

    bdnz L6

    mtspr   256,r11
    blr


.globl _ppc_brightness_generic
.align 3
_ppc_brightness_generic:
    lis   r12,0x00FF
    ori   r12,r12,0x00FF
    subi  r3,r3,4
    subi  r4,r4,4
    mtctr r5
    b L1
.align 4
L1:
    lwzu  r7,4(r3)

    rlwinm  r8,r7,16,24,31
    rlwinm  r9,r7,24,24,31
    mullw   r8,r8,r6
    rlwinm  r10,r7,0,24,31
    mullw   r9,r9,r6
    srwi    r8,r8,8
    mullw   r10,r10,r6
    srwi    r9,r9,8

    rlwinm. r11,r8,0,0,23
    beq     L2
    li      r8,0xFF
L2:
    srwi    r10,r10,8
    rlwinm. r11,r9,0,0,23
    beq     L3
    li      r9,0xFF
L3:
    rlwinm  r7,r8,16,8,15
    rlwinm. r11,r10,0,0,23
    beq     L4
    li      r10,0xFF
L4:
    rlwimi  r7,r9,8,16,23
    rlwimi  r7,r10,0,24,31

    stwu    r7,4(r4)
    bdnz L1

    blr



.static_data
.align 4
vectortmpwork:
    .long 0,0,0,0

