.section regular,__TEXT
.globl _ppc_doubling	; name of the function to call by C program

; width (src width)->r3
; myx (src) ->r4
; myX (dest) ->r5
; myX2 (dest + 1 complete line)->r6
; heigth (src height)->r7
; inc (increment for next line in dest) ->r8

_ppc_doubling:

mtspr	ctr,r3

addi r4,r4,-4
addi r5,r5,-4
addi r6,r6,-4

1:;boucle:

lwzu	r10,4(r4)
stwu	r10,4(r5)
stwu	r10,4(r5)
stwu	r10,4(r6)
stwu	r10,4(r6)

bdnz	1boucle

subi	r7,r7,1
add	r5,r5,r8
cmpwi 	cr1,r7,0
add	r6,r6,r8
mtspr	ctr,r3
bgt	cr1,1boucle

blr

;backup

lwzu	r10,4(r4)
stwu	r10,4(r5)
stwu	r10,4(r6)
stwu	r10,4(r5)
stwu	r10,4(r6)

lwzu	r10,4(r4)
stwu	r10,4(r5)
stwu	r10,4(r6)
stwu	r10,4(r5)
stwu	r10,4(r6)
