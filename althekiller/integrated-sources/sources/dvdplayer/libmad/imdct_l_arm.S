/*****************************************************************************
* Copyright (C) 2000-2001 Andre McCurdy  <armccurdy@yahoo.co.uk>
*
* This program is free software. you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation@ either version 2 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY, without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program@ if not, write to the Free Software
* Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*****************************************************************************
*
* Notes:
*
*
*****************************************************************************
*
* $Id: imdct_l_arm.S,v 1.7 2001/03/25 20:03:34 rob Rel $
*
* 2001/03/24:  Andre McCurdy <armccurdy@yahoo.co.uk>
*   - Corrected PIC unsafe loading of address of 'imdct36_long_karray'
*
* 2000/09/20:  Robert Leslie <rob@mars.org>
*   - Added a global symbol with leading underscore per suggestion of
*     Simon Burge to support linking with the a.out format.
*
* 2000/09/15:  Robert Leslie <rob@mars.org>
*   - Fixed a small bug where flags were changed before a conditional branch.
*
* 2000/09/15:  Andre McCurdy <armccurdy@yahoo.co.uk>
*   - Applied Nicolas Pitre's rounding optimisation in all remaining places.
*
* 2000/09/09:  Nicolas Pitre <nico@cam.org>
*   - Optimized rounding + scaling operations.
*
* 2000/08/09:  Andre McCurdy <armccurdy@yahoo.co.uk>
*   - Original created.
*
****************************************************************************/


/*
   On entry:

      r0 = pointer to 18 element input  array
      r1 = pointer to 36 element output array
      r2 = windowing block type


   Stack frame created during execution of the function:

   Initial   Holds:
   Stack
   pointer
   minus:

       0
       4     lr
       8     r11
      12     r10
      16     r9
      20     r8
      24     r7
      28     r6
      32     r5
      36     r4

      40     r2 : windowing block type

      44     ct00 high
      48     ct00 low
      52     ct01 high
      56     ct01 low
      60     ct04 high
      64     ct04 low
      68     ct06 high
      72     ct06 low
      76     ct05 high
      80     ct05 low
      84     ct03 high
      88     ct03 low
      92    -ct05 high
      96    -ct05 low
     100    -ct07 high
     104    -ct07 low
     108     ct07 high
     112     ct07 low
     116     ct02 high
     120     ct02 low
*/

#define BLOCK_MODE_NORMAL   0
#define BLOCK_MODE_START    1
#define BLOCK_MODE_STOP     3


#define X0   0x00
#define X1   0x04
#define X2   0x08
#define X3   0x0C
#define X4   0x10
#define X5   0x14
#define X6   0x18
#define X7   0x1c
#define X8   0x20
#define X9   0x24
#define X10  0x28
#define X11  0x2c
#define X12  0x30
#define X13  0x34
#define X14  0x38
#define X15  0x3c
#define X16  0x40
#define X17  0x44

#define x0   0x00
#define x1   0x04
#define x2   0x08
#define x3   0x0C
#define x4   0x10
#define x5   0x14
#define x6   0x18
#define x7   0x1c
#define x8   0x20
#define x9   0x24
#define x10  0x28
#define x11  0x2c
#define x12  0x30
#define x13  0x34
#define x14  0x38
#define x15  0x3c
#define x16  0x40
#define x17  0x44
#define x18  0x48
#define x19  0x4c
#define x20  0x50
#define x21  0x54
#define x22  0x58
#define x23  0x5c
#define x24  0x60
#define x25  0x64
#define x26  0x68
#define x27  0x6c
#define x28  0x70
#define x29  0x74
#define x30  0x78
#define x31  0x7c
#define x32  0x80
#define x33  0x84
#define x34  0x88
#define x35  0x8c

#define K00  0x0ffc19fd
#define K01  0x00b2aa3e
#define K02  0x0fdcf549
#define K03  0x0216a2a2
#define K04  0x0f9ee890
#define K05  0x03768962
#define K06  0x0f426cb5
#define K07  0x04cfb0e2
#define K08  0x0ec835e8
#define K09  0x061f78aa
#define K10  0x0e313245
#define K11  0x07635284
#define K12  0x0d7e8807
#define K13  0x0898c779
#define K14  0x0cb19346
#define K15  0x09bd7ca0
#define K16  0x0bcbe352
#define K17  0x0acf37ad

#define minus_K02 0xf0230ab7

#define WL0  0x00b2aa3e
#define WL1  0x0216a2a2
#define WL2  0x03768962
#define WL3  0x04cfb0e2
#define WL4  0x061f78aa
#define WL5  0x07635284
#define WL6  0x0898c779
#define WL7  0x09bd7ca0
#define WL8  0x0acf37ad
#define WL9  0x0bcbe352
#define WL10 0x0cb19346
#define WL11 0x0d7e8807
#define WL12 0x0e313245
#define WL13 0x0ec835e8
#define WL14 0x0f426cb5
#define WL15 0x0f9ee890
#define WL16 0x0fdcf549
#define WL17 0x0ffc19fd


@*****************************************************************************


    .text
    .align

    .global III_imdct_l
    .global _III_imdct_l

III_imdct_l:
_III_imdct_l:

    stmdb   sp!, { r2, r4 - r11, lr }   @ all callee saved regs, plus arg3

    ldr     r4, =K08                    @ r4 =  K08
    ldr     r5, =K09                    @ r5 =  K09
    ldr     r8, [r0, #X4]               @ r8 =  X4
    ldr     r9, [r0, #X13]              @ r9 =  X13
    rsb     r6, r4, #0                  @ r6 = -K08
    rsb     r7, r5, #0                  @ r7 = -K09

    smull   r2, r3, r4, r8              @ r2..r3  = (X4 * K08)
    smlal   r2, r3, r5, r9              @ r2..r3  = (X4 * K08) + (X13 *  K09) = ct01

    smull   r10, lr, r8, r5             @ r10..lr = (X4 * K09)
    smlal   r10, lr, r9, r6             @ r10..lr = (X4 * K09) + (X13 * -K08) = ct00

    ldr     r8, [r0, #X7]               @ r8 = X7
    ldr     r9, [r0, #X16]              @ r9 = X16

    stmdb   sp!, { r2, r3, r10, lr }    @ stack ct00_h, ct00_l, ct01_h, ct01_l

    add     r8, r8, r9                  @ r8 = (X7 + X16)
    ldr     r9, [r0, #X1]               @ r9 = X1

    smlal   r2, r3, r6, r8              @ r2..r3  = ct01 + ((X7 + X16) * -K08)
    smlal   r2, r3, r7, r9              @ r2..r3 += (X1  * -K09)

    ldr     r7, [r0, #X10]              @ r7 = X10

    rsbs    r10, r10, #0
    rsc     lr, lr, #0                  @ r10..lr  = -ct00

    smlal   r2, r3, r5, r7              @ r2..r3  += (X10 *  K09) = ct06

    smlal   r10, lr, r9, r6             @ r10..lr  = -ct00 + ( X1        * -K08)
    smlal   r10, lr, r8, r5             @ r10..lr +=         ((X7 + X16) *  K09)
    smlal   r10, lr, r7, r4             @ r10..lr +=         ( X10       *  K08) = ct04

    stmdb   sp!, { r2, r3, r10, lr }    @ stack ct04_h, ct04_l, ct06_h, ct06_l

    @----

    ldr     r7, [r0, #X0]
    ldr     r8, [r0, #X11]
    ldr     r9, [r0, #X12]
    sub     r7, r7, r8
    sub     r7, r7, r9                  @ r7 = (X0 - X11 -X12) = ct14

    ldr     r9,  [r0, #X3]
    ldr     r8,  [r0, #X8]
    ldr     r11, [r0, #X15]
    sub     r8, r8, r9
    add     r8, r8, r11                 @ r8 = (X8 - X3 + X15) = ct16

    add     r11, r7, r8                 @ r11 = ct14 + ct16 = ct18

    smlal   r2, r3, r6, r11             @ r2..r3 = ct06 + ((X0 - X11 - X3 + X15 + X8 - X12) * -K08)

    ldr     r6,  [r0, #X2]
    ldr     r9,  [r0, #X9]
    ldr     r12, [r0, #X14]
    sub     r6, r6, r9
    sub     r6, r6, r12                 @ r6 = (X2 - X9 - X14) = ct15

    ldr     r9,  [r0, #X5]
    ldr     r12, [r0, #X6]
    sub     r9, r9, r12
    ldr     r12, [r0, #X17]
    sub     r9, r9, r12                 @ r9 = (X5 - X6 - X17) = ct17

    add     r12, r9, r6                 @ r12 = ct15 + ct17 = ct19

    smlal   r2, r3, r5, r12             @ r2..r3 += ((X2 - X9 + X5 - X6 - X17 - X14) * K09)

    smlal   r10, lr, r11, r5            @ r10..lr = ct04 + (ct18 * K09)
    smlal   r10, lr, r12, r4            @ r10..lr = ct04 + (ct18 * K09) + (ct19 * K08)

    movs    r2, r2, lsr #28
    adc     r2, r2, r3, lsl #4          @ r2 = bits[59..28] of r2..r3
    str     r2, [r1, #x22]              @ store result x22

    movs    r10, r10, lsr #28
    adc     r10, r10, lr, lsl #4        @ r10 = bits[59..28] of r10..lr
    str     r10, [r1, #x4]              @ store result x4

    @----

    ldmia   sp, { r2, r3, r4, r5 }      @ r2..r3 = ct06, r4..r5 = ct04 (dont update sp)

    @ r2..r3 = ct06
    @ r4..r5 = ct04
    @ r6     = ct15
    @ r7     = ct14
    @ r8     = ct16
    @ r9     = ct17
    @ r10    = .
    @ r11    = .
    @ r12    = .
    @ lr     = .

    ldr     r10, =K03                   @ r10 = K03
    ldr     lr,  =K15                   @ lr  = K15

    smlal   r2, r3, r10, r7             @ r2..r3 = ct06 + (ct14 * K03)
    smlal   r4, r5,  lr, r7             @ r4..r5 = ct04 + (ct14 * K15)

    ldr     r12, =K14                   @ r12 =  K14
    rsb     r10, r10, #0                @ r10 = -K03

    smlal   r2, r3,  lr, r6             @ r2..r3 += (ct15 *  K15)
    smlal   r4, r5, r10, r6             @ r4..r5 += (ct15 * -K03)
    smlal   r2, r3, r12, r8             @ r2..r3 += (ct16 *  K14)

    ldr     r11, =minus_K02             @ r11 = -K02
    rsb     r12, r12, #0                @ r12 = -K14

    smlal   r4, r5, r12, r9             @ r4..r5 += (ct17 * -K14)
    smlal   r2, r3, r11, r9             @ r2..r3 += (ct17 * -K02)
    smlal   r4, r5, r11, r8             @ r4..r5 += (ct16 * -K02)

    movs    r2, r2, lsr #28
    adc     r2, r2, r3, lsl #4          @ r2 = bits[59..28] of r2..r3
    str     r2, [r1, #x7]               @ store result x7

    movs    r4, r4, lsr #28
    adc     r4, r4, r5, lsl #4          @ r4 = bits[59..28] of r4..r5
    str     r4, [r1, #x1]               @ store result x1

    @----

    ldmia   sp, { r2, r3, r4, r5 }      @ r2..r3 = ct06, r4..r5 = ct04 (dont update sp)

    @ r2..r3 = ct06
    @ r4..r5 = ct04
    @ r6     = ct15
    @ r7     = ct14
    @ r8     = ct16
    @ r9     = ct17
    @ r10    = -K03
    @ r11    = -K02
    @ r12    = -K14
    @ lr     =  K15

    rsbs    r2, r2, #0
    rsc     r3, r3, #0                  @ r2..r3 = -ct06

    smlal   r2, r3, r12, r7             @ r2..r3  = -ct06 + (ct14 * -K14)
    smlal   r2, r3, r10, r8             @ r2..r3 += (ct16 * -K03)

    smlal   r4, r5, r12, r6             @ r4..r5  =  ct04 + (ct15 * -K14)
    smlal   r4, r5, r10, r9             @ r4..r5 += (ct17 * -K03)
    smlal   r4, r5,  lr, r8             @ r4..r5 += (ct16 *  K15)
    smlal   r4, r5, r11, r7             @ r4..r5 += (ct14 * -K02)

    rsb     lr, lr, #0                  @ lr  = -K15
    rsb     r11, r11, #0                @ r11 =  K02

    smlal   r2, r3,  lr, r9             @ r2..r3 += (ct17 * -K15)
    smlal   r2, r3, r11, r6             @ r2..r3 += (ct15 *  K02)

    movs    r4, r4, lsr #28
    adc     r4, r4, r5, lsl #4          @ r4 = bits[59..28] of r4..r5
    str     r4, [r1, #x25]              @ store result x25

    movs    r2, r2, lsr #28
    adc     r2, r2, r3, lsl #4          @ r2 = bits[59..28] of r2..r3
    str     r2, [r1, #x19]              @ store result x19

    @----

    ldr     r2, [sp, #16]               @ r2 = ct01_l
    ldr     r3, [sp, #20]               @ r3 = ct01_h

    ldr     r6, [r0, #X1]
    ldr     r8, [r0, #X7]
    ldr     r9, [r0, #X10]
    ldr     r7, [r0, #X16]

    rsbs    r2, r2, #0
    rsc     r3, r3, #0                  @ r2..r3 = -ct01

    mov     r4, r2
    mov     r5, r3                      @ r4..r5 = -ct01

    @ r2..r3 = -ct01
    @ r4..r5 = -ct01
    @ r6     =  X1
    @ r7     =  X16
    @ r8     =  X7
    @ r9     =  X10
    @ r10    = -K03
    @ r11    =  K02
    @ r12    = -K14
    @ lr     = -K15

    smlal   r4, r5, r12, r7             @ r4..r5 = -ct01 + (X16 * -K14)
    smlal   r2, r3,  lr, r9             @ r2..r3 = -ct01 + (X10 * -K15)

    smlal   r4, r5, r10, r8             @ r4..r5 += (X7  * -K03)
    smlal   r2, r3, r10, r7             @ r2..r3 += (X16 * -K03)

    smlal   r4, r5, r11, r9             @ r4..r5 += (X10 *  K02)
    smlal   r2, r3, r12, r8             @ r2..r3 += (X7  * -K14)

    rsb     lr, lr, #0                  @ lr  =  K15
    rsb     r11, r11, #0                @ r11 = -K02

    smlal   r4, r5,  lr, r6             @ r4..r5 += (X1  *  K15) = ct05
    smlal   r2, r3, r11, r6             @ r2..r3 += (X1  * -K02) = ct03

    stmdb   sp!, { r2, r3, r4, r5 }     @ stack ct05_h, ct05_l, ct03_h, ct03_l

    rsbs    r4, r4, #0
    rsc     r5, r5, #0                  @ r4..r5 = -ct05

    stmdb   sp!, { r4, r5 }             @ stack -ct05_h, -ct05_l

    ldr     r2, [sp, #48]               @ r2 = ct00_l
    ldr     r3, [sp, #52]               @ r3 = ct00_h

    rsb     r10, r10, #0                @ r10 = K03

    rsbs    r4, r2, #0
    rsc     r5, r3, #0                  @ r4..r5 = -ct00

    @ r2..r3 =  ct00
    @ r4..r5 = -ct00
    @ r6     =  X1
    @ r7     =  X16
    @ r8     =  X7
    @ r9     =  X10
    @ r10    =  K03
    @ r11    = -K02
    @ r12    = -K14
    @ lr     =  K15

    smlal   r4, r5, r10, r6             @ r4..r5 = -ct00 + (X1  * K03)
    smlal   r2, r3, r10, r9             @ r2..r3 =  ct00 + (X10 * K03)

    smlal   r4, r5, r12, r9             @ r4..r5 += (X10 * -K14)
    smlal   r2, r3, r12, r6             @ r2..r3 += (X1  * -K14)

    smlal   r4, r5, r11, r7             @ r4..r5 += (X16 * -K02)
    smlal   r4, r5,  lr, r8             @ r4..r5 += (X7  *  K15) = ct07

    rsb     lr, lr, #0                  @ lr  = -K15
    rsb     r11, r11, #0                @ r11 =  K02

    smlal   r2, r3, r11, r8             @ r2..r3 += (X7  *  K02)
    smlal   r2, r3,  lr, r7             @ r2..r3 += (X16 * -K15) = ct02

    rsbs    r6, r4, #0
    rsc     r7, r5, #0                  @ r6..r7 = -ct07

    stmdb   sp!, { r2 - r7 }            @ stack -ct07_h, -ct07_l, ct07_h, ct07_l, ct02_h, ct02_l


    @----

    add     r2, pc, #(imdct36_long_karray-.-8)  @ r2 = base address of Knn array (PIC safe ?)


loop:
    ldr     r12, [r0, #X0]

    ldmia   r2!, { r5 - r11 }           @ first 7 words from Karray element

    smull   r3, r4, r5, r12             @ sum =  (Kxx * X0)
    ldr     r12, [r0, #X2]
    ldr     r5,  [r0, #X3]
    smlal   r3, r4, r6, r12             @ sum += (Kxx * X2)
    ldr     r12, [r0, #X5]
    ldr     r6,  [r0, #X6]
    smlal   r3, r4, r7, r5              @ sum += (Kxx * X3)
    smlal   r3, r4, r8, r12             @ sum += (Kxx * X5)
    ldr     r12, [r0, #X8]
    ldr     r5,  [r0, #X9]
    smlal   r3, r4, r9, r6              @ sum += (Kxx * X6)
    smlal   r3, r4, r10, r12            @ sum += (Kxx * X8)
    smlal   r3, r4, r11, r5             @ sum += (Kxx * X9)

    ldmia   r2!, { r5 - r10 }           @ final 6 words from Karray element

    ldr     r11, [r0, #X11]
    ldr     r12, [r0, #X12]
    smlal   r3, r4, r5, r11             @ sum += (Kxx * X11)
    ldr     r11, [r0, #X14]
    ldr     r5,  [r0, #X15]
    smlal   r3, r4, r6, r12             @ sum += (Kxx * X12)
    smlal   r3, r4, r7, r11             @ sum += (Kxx * X14)
    ldr     r11, [r0, #X17]
    smlal   r3, r4, r8, r5              @ sum += (Kxx * X15)
    smlal   r3, r4, r9, r11             @ sum += (Kxx * X17)

    add     r5, sp, r10, lsr #16        @ create index back into stack for required ctxx

    ldmia   r5, { r6, r7 }              @ r6..r7 = ctxx

    mov     r8, r10, lsl #16            @ push ctxx index off the top end

    adds    r3, r3, r6                  @ add low words
    adc     r4, r4, r7                  @ add high words, with carry
    movs    r3, r3, lsr #28
    adc     r3, r3, r4, lsl #4          @ r3 = bits[59..28] of r3..r4

    str     r3, [r1, r8, lsr #24]       @ push completion flag off the bottom end

    movs    r8, r8, lsl #8              @ push result location index off the top end
    beq     loop                        @ loop back if completion flag not set
    b       imdct_l_windowing           @ branch to windowing stage if looping finished

imdct36_long_karray:

    .word   K17, -K13,  K10, -K06, -K05,  K01, -K00,  K04, -K07,  K11,  K12, -K16, 0x00000000
    .word   K13,  K07,  K16,  K01,  K10, -K05,  K04, -K11,  K00, -K17,  K06, -K12, 0x00200800
    .word   K11,  K17,  K05,  K12, -K01,  K06, -K07,  K00, -K13,  K04, -K16,  K10, 0x00200c00
    .word   K07,  K00, -K12,  K05, -K16, -K10,  K11, -K17,  K04,  K13,  K01,  K06, 0x00001400
    .word   K05,  K10, -K00, -K17,  K07, -K13,  K12,  K06, -K16,  K01, -K11, -K04, 0x00181800
    .word   K01,  K05, -K07, -K11,  K13,  K17, -K16, -K12,  K10,  K06, -K04, -K00, 0x00102000
    .word  -K16,  K12, -K11,  K07,  K04, -K00, -K01,  K05, -K06,  K10,  K13, -K17, 0x00284800
    .word  -K12,  K06,  K17, -K00, -K11,  K04,  K05, -K10,  K01,  K16, -K07, -K13, 0x00085000
    .word  -K10,  K16,  K04, -K13, -K00,  K07,  K06, -K01, -K12, -K05,  K17,  K11, 0x00105400
    .word  -K06, -K01,  K13,  K04,  K17, -K11, -K10, -K16, -K05,  K12,  K00,  K07, 0x00185c00
    .word  -K04, -K11, -K01,  K16,  K06,  K12,  K13, -K07, -K17, -K00, -K10, -K05, 0x00006000
    .word  -K00, -K04, -K06, -K10, -K12, -K16, -K17, -K13, -K11, -K07, -K05, -K01, 0x00206801


    @----
    @-------------------------------------------------------------------------
    @----

imdct_l_windowing:

    ldr     r11, [sp, #80]              @ fetch function parameter 3 from out of the stack
    ldmia   r1!, { r0, r2 - r9 }        @ load 9 words from x0, update pointer

    @ r0     = x0
    @ r1     = &x[9]
    @ r2     = x1
    @ r3     = x2
    @ r4     = x3
    @ r5     = x4
    @ r6     = x5
    @ r7     = x6
    @ r8     = x7
    @ r9     = x8
    @ r10    = .
    @ r11    = window mode: (0 == normal), (1 == start block), (3 == stop block)
    @ r12    = .
    @ lr     = .

    cmp     r11, #BLOCK_MODE_STOP       @ setup flags
    rsb     r10, r0, #0                 @ r10 = -x0 (DONT change flags !!)
    beq     stop_block_x0_to_x17


    @ start and normal blocks are treated the same for x[0]..x[17]

normal_block_x0_to_x17:

    ldr     r12, =WL9                   @ r12 = window_l[9]

    rsb     r0,  r9, #0                 @ r0  = -x8
    rsb     r9,  r2, #0                 @ r9  = -x1
    rsb     r2,  r8, #0                 @ r2  = -x7
    rsb     r8,  r3, #0                 @ r8  = -x2
    rsb     r3,  r7, #0                 @ r3  = -x6
    rsb     r7,  r4, #0                 @ r7  = -x3
    rsb     r4,  r6, #0                 @ r4  = -x5
    rsb     r6,  r5, #0                 @ r6  = -x4

    @ r0     = -x8
    @ r1     = &x[9]
    @ r2     = -x7
    @ r3     = -x6
    @ r4     = -x5
    @ r5     = .
    @ r6     = -x4
    @ r7     = -x3
    @ r8     = -x2
    @ r9     = -x1
    @ r10    = -x0
    @ r11    = window mode: (0 == normal), (1 == start block), (3 == stop block)
    @ r12    = window_l[9]
    @ lr     = .

    smull   r5, lr, r12, r0             @ r5..lr = (window_l[9]  * (x[9]  == -x[8]))
    ldr     r12, =WL10                  @ r12 = window_l[10]
    movs    r5, r5, lsr #28
    adc     r0, r5, lr, lsl #4          @ r0 = bits[59..28] of windowed x9

    smull   r5, lr, r12, r2             @ r5..lr = (window_l[10] * (x[10] == -x[7]))
    ldr     r12, =WL11                  @ r12 = window_l[11]
    movs    r5, r5, lsr #28
    adc     r2, r5, lr, lsl #4          @ r2 = bits[59..28] of windowed x10

    smull   r5, lr, r12, r3             @ r5..lr = (window_l[11] * (x[11] == -x[6]))
    ldr     r12, =WL12                  @ r12 = window_l[12]
    movs    r5, r5, lsr #28
    adc     r3, r5, lr, lsl #4          @ r3 = bits[59..28] of windowed x11

    smull   r5, lr, r12, r4             @ r5..lr = (window_l[12] * (x[12] == -x[5]))
    ldr     r12, =WL13                  @ r12 = window_l[13]
    movs    r5, r5, lsr #28
    adc     r4, r5, lr, lsl #4          @ r4 = bits[59..28] of windowed x12

    smull   r5, lr, r12, r6             @ r5..lr = (window_l[13] * (x[13] == -x[4]))
    ldr     r12, =WL14                  @ r12 = window_l[14]
    movs    r5, r5, lsr #28
    adc     r6, r5, lr, lsl #4          @ r6 = bits[59..28] of windowed x13

    smull   r5, lr, r12, r7             @ r5..lr = (window_l[14] * (x[14] == -x[3]))
    ldr     r12, =WL15                  @ r12 = window_l[15]
    movs    r5, r5, lsr #28
    adc     r7, r5, lr, lsl #4          @ r7 = bits[59..28] of windowed x14

    smull   r5, lr, r12, r8             @ r5..lr = (window_l[15] * (x[15] == -x[2]))
    ldr     r12, =WL16                  @ r12 = window_l[16]
    movs    r5, r5, lsr #28
    adc     r8, r5, lr, lsl #4          @ r8 = bits[59..28] of windowed x15

    smull   r5, lr, r12, r9             @ r5..lr = (window_l[16] * (x[16] == -x[1]))
    ldr     r12, =WL17                  @ r12 = window_l[17]
    movs    r5, r5, lsr #28
    adc     r9, r5, lr, lsl #4          @ r9 = bits[59..28] of windowed x16

    smull   r5, lr, r12, r10            @ r5..lr = (window_l[17] * (x[17] == -x[0]))
    ldr     r12, =WL0                   @ r12 = window_l[0]
    movs    r5, r5, lsr #28
    adc     r10, r5, lr, lsl #4         @ r10 = bits[59..28] of windowed x17


    stmia   r1,  { r0, r2 - r4, r6 - r10 } @ store windowed x[9] .. x[17]
    ldmdb   r1!, { r0, r2 - r9 }           @ load 9 words downto (and including) x0


    smull   r10, lr, r12, r0            @ r10..lr = (window_l[0] * x[0])
    ldr     r12, =WL1                   @ r12 = window_l[1]
    movs    r10, r10, lsr #28
    adc     r0, r10, lr, lsl #4         @ r0 = bits[59..28] of windowed x0

    smull   r10, lr, r12, r2            @ r10..lr = (window_l[1] * x[1])
    ldr     r12, =WL2                   @ r12 = window_l[2]
    movs    r10, r10, lsr #28
    adc     r2, r10, lr, lsl #4         @ r2 = bits[59..28] of windowed x1

    smull   r10, lr, r12, r3            @ r10..lr = (window_l[2] * x[2])
    ldr     r12, =WL3                   @ r12 = window_l[3]
    movs    r10, r10, lsr #28
    adc     r3, r10, lr, lsl #4         @ r3 = bits[59..28] of windowed x2

    smull   r10, lr, r12, r4            @ r10..lr = (window_l[3] * x[3])
    ldr     r12, =WL4                   @ r12 = window_l[4]
    movs    r10, r10, lsr #28
    adc     r4, r10, lr, lsl #4         @ r4 = bits[59..28] of windowed x3

    smull   r10, lr, r12, r5            @ r10..lr = (window_l[4] * x[4])
    ldr     r12, =WL5                   @ r12 = window_l[5]
    movs    r10, r10, lsr #28
    adc     r5, r10, lr, lsl #4         @ r5 = bits[59..28] of windowed x4

    smull   r10, lr, r12, r6            @ r10..lr = (window_l[5] * x[5])
    ldr     r12, =WL6                   @ r12 = window_l[6]
    movs    r10, r10, lsr #28
    adc     r6, r10, lr, lsl #4         @ r6 = bits[59..28] of windowed x5

    smull   r10, lr, r12, r7            @ r10..lr = (window_l[6] * x[6])
    ldr     r12, =WL7                   @ r12 = window_l[7]
    movs    r10, r10, lsr #28
    adc     r7, r10, lr, lsl #4         @ r7 = bits[59..28] of windowed x6

    smull   r10, lr, r12, r8            @ r10..lr = (window_l[7] * x[7])
    ldr     r12, =WL8                   @ r12 = window_l[8]
    movs    r10, r10, lsr #28
    adc     r8, r10, lr, lsl #4         @ r8 = bits[59..28] of windowed x7

    smull   r10, lr, r12, r9            @ r10..lr = (window_l[8] * x[8])
    movs    r10, r10, lsr #28
    adc     r9, r10, lr, lsl #4         @ r9 = bits[59..28] of windowed x8

    stmia   r1, { r0, r2 - r9 }         @ store windowed x[0] .. x[8]

    cmp     r11, #BLOCK_MODE_START
    beq     start_block_x18_to_x35


    @----


normal_block_x18_to_x35:

    ldr     r11, =WL3                   @ r11 = window_l[3]
    ldr     r12, =WL4                   @ r12 = window_l[4]

    add     r1, r1, #(18*4)             @ r1 = &x[18]

    ldmia   r1!, { r0, r2 - r4, r6 - r10 }  @ load 9 words from x18, update pointer

    @ r0     = x18
    @ r1     = &x[27]
    @ r2     = x19
    @ r3     = x20
    @ r4     = x21
    @ r5     = .
    @ r6     = x22
    @ r7     = x23
    @ r8     = x24
    @ r9     = x25
    @ r10    = x26
    @ r11    = window_l[3]
    @ r12    = window_l[4]
    @ lr     = .

    smull   r5, lr, r12, r6             @ r5..lr = (window_l[4] * (x[22] == x[31]))
    movs    r5, r5, lsr #28
    adc     r5, r5, lr, lsl #4          @ r5 = bits[59..28] of windowed x31

    smull   r6, lr, r11, r4             @ r5..lr = (window_l[3] * (x[21] == x[32]))
    ldr     r12, =WL5                   @ r12    =  window_l[5]
    movs    r6, r6, lsr #28
    adc     r6, r6, lr, lsl #4          @ r6 = bits[59..28] of windowed x32

    smull   r4, lr, r12, r7             @ r4..lr = (window_l[5] * (x[23] == x[30]))
    ldr     r11, =WL1                   @ r11    =  window_l[1]
    ldr     r12, =WL2                   @ r12    =  window_l[2]
    movs    r4, r4, lsr #28
    adc     r4, r4, lr, lsl #4          @ r4 = bits[59..28] of windowed x30

    smull   r7, lr, r12, r3             @ r7..lr = (window_l[2] * (x[20] == x[33]))
    ldr     r12, =WL6                   @ r12 = window_l[6]
    movs    r7, r7, lsr #28
    adc     r7, r7, lr, lsl #4          @ r7 = bits[59..28] of windowed x33

    smull   r3, lr, r12, r8             @ r3..lr = (window_l[6] * (x[24] == x[29]))
    movs    r3, r3, lsr #28
    adc     r3, r3, lr, lsl #4          @ r3 = bits[59..28] of windowed x29

    smull   r8, lr, r11, r2             @ r7..lr = (window_l[1] * (x[19] == x[34]))
    ldr     r12, =WL7                   @ r12    =  window_l[7]
    ldr     r11, =WL8                   @ r11    =  window_l[8]
    movs    r8, r8, lsr #28
    adc     r8, r8, lr, lsl #4          @ r8 = bits[59..28] of windowed x34

    smull   r2, lr, r12, r9             @ r7..lr = (window_l[7] * (x[25] == x[28]))
    ldr     r12, =WL0                   @ r12 = window_l[0]
    movs    r2, r2, lsr #28
    adc     r2, r2, lr, lsl #4          @ r2 = bits[59..28] of windowed x28

    smull   r9, lr, r12, r0             @ r3..lr = (window_l[0] * (x[18] == x[35]))
    movs    r9, r9, lsr #28
    adc     r9, r9, lr, lsl #4          @ r9 = bits[59..28] of windowed x35

    smull   r0, lr, r11, r10            @ r7..lr = (window_l[8] * (x[26] == x[27]))
    ldr     r11, =WL16                  @ r11    =  window_l[16]
    ldr     r12, =WL17                  @ r12    =  window_l[17]
    movs    r0, r0, lsr #28
    adc     r0, r0, lr, lsl #4          @ r0 = bits[59..28] of windowed x27


    stmia   r1,  { r0, r2 - r9 }        @ store windowed x[27] .. x[35]
    ldmdb   r1!, { r0, r2 - r9 }        @ load 9 words downto (and including) x18


    smull   r10, lr, r12, r0            @ r10..lr = (window_l[17] * x[18])
    movs    r10, r10, lsr #28
    adc     r0,  r10, lr, lsl #4        @ r0 = bits[59..28] of windowed x0

    smull   r10, lr, r11, r2            @ r10..lr = (window_l[16] * x[19])
    ldr     r11, =WL14                  @ r11     =  window_l[14]
    ldr     r12, =WL15                  @ r12     =  window_l[15]
    movs    r10, r10, lsr #28
    adc     r2,  r10, lr, lsl #4        @ r2 = bits[59..28] of windowed x1

    smull   r10, lr, r12, r3            @ r10..lr = (window_l[15] * x[20])
    movs    r10, r10, lsr #28
    adc     r3,  r10, lr, lsl #4        @ r3 = bits[59..28] of windowed x2

    smull   r10, lr, r11, r4            @ r10..lr = (window_l[14] * x[21])
    ldr     r11, =WL12                  @ r11     =  window_l[12]
    ldr     r12, =WL13                  @ r12     =  window_l[13]
    movs    r10, r10, lsr #28
    adc     r4,  r10, lr, lsl #4        @ r4 = bits[59..28] of windowed x3

    smull   r10, lr, r12, r5            @ r10..lr = (window_l[13] * x[22])
    movs    r10, r10, lsr #28
    adc     r5,  r10, lr, lsl #4        @ r5 = bits[59..28] of windowed x4

    smull   r10, lr, r11, r6            @ r10..lr = (window_l[12] * x[23])
    ldr     r11, =WL10                  @ r12 = window_l[10]
    ldr     r12, =WL11                  @ r12 = window_l[11]
    movs    r10, r10, lsr #28
    adc     r6,  r10, lr, lsl #4        @ r6 = bits[59..28] of windowed x5

    smull   r10, lr, r12, r7            @ r10..lr = (window_l[11] * x[24])
    movs    r10, r10, lsr #28
    adc     r7,  r10, lr, lsl #4        @ r7 = bits[59..28] of windowed x6

    smull   r10, lr, r11, r8            @ r10..lr = (window_l[10] * x[25])
    ldr     r12, =WL9                   @ r12 = window_l[9]
    movs    r10, r10, lsr #28
    adc     r8,  r10, lr, lsl #4        @ r8 = bits[59..28] of windowed x7

    smull   r10, lr, r12, r9            @ r10..lr = (window_l[9] * x[26])

    movs    r10, r10, lsr #28
    adc     r9,  r10, lr, lsl #4        @ r9 = bits[59..28] of windowed x8

    stmia   r1, { r0, r2 - r9 }         @ store windowed x[18] .. x[26]

    @----
    @ NB there are 2 possible exits from this function - this is only one of them
    @----

    add     sp, sp, #(21*4)             @ return stack frame
    ldmia   sp!, { r4 - r11, pc }       @ restore callee saved regs, and return

    @----


stop_block_x0_to_x17:

    @ r0     =  x0
    @ r1     = &x[9]
    @ r2     =  x1
    @ r3     =  x2
    @ r4     =  x3
    @ r5     =  x4
    @ r6     =  x5
    @ r7     =  x6
    @ r8     =  x7
    @ r9     =  x8
    @ r10    = -x0
    @ r11    =  window mode: (0 == normal), (1 == start block), (3 == stop block)
    @ r12    =  .
    @ lr     =  .

    rsb     r0, r6, #0                  @ r0 = -x5
    rsb     r6, r2, #0                  @ r6 = -x1
    rsb     r2, r5, #0                  @ r2 = -x4
    rsb     r5, r3, #0                  @ r5 = -x2
    rsb     r3, r4, #0                  @ r3 = -x3

    add     r1, r1, #(3*4)                      @ r1 = &x[12]
    stmia   r1, { r0, r2, r3, r5, r6, r10 }     @ store unchanged x[12] .. x[17]

    ldr     r0, =WL1                    @ r0 = window_l[1]  == window_s[0]

    rsb     r10, r9, #0                 @ r10 = -x8
    rsb     r12, r8, #0                 @ r12 = -x7
    rsb     lr,  r7, #0                 @ lr  = -x6

    @ r0     =  WL1
    @ r1     = &x[12]
    @ r2     =  .
    @ r3     =  .
    @ r4     =  .
    @ r5     =  .
    @ r6     =  .
    @ r7     =  x6
    @ r8     =  x7
    @ r9     =  x8
    @ r10    = -x8
    @ r11    =  window mode: (0 == normal), (1 == start block), (3 == stop block)
    @ r12    = -x7
    @ lr     = -x6

    smull   r5, r6, r0, r7              @ r5..r6 = (window_l[1] * x[6])
    ldr     r2, =WL4                    @ r2     =  window_l[4] == window_s[1]
    movs    r5, r5, lsr #28
    adc     r7, r5, r6, lsl #4          @ r7 = bits[59..28] of windowed x6

    smull   r5, r6, r2, r8              @ r5..r6 = (window_l[4] * x[7])
    ldr     r3, =WL7                    @ r3     =  window_l[7] == window_s[2]
    movs    r5, r5, lsr #28
    adc     r8, r5, r6, lsl #4          @ r8 = bits[59..28] of windowed x7

    smull   r5, r6, r3, r9              @ r5..r6 = (window_l[7] * x[8])
    ldr     r4, =WL10                   @ r4     =  window_l[10] == window_s[3]
    movs    r5, r5, lsr #28
    adc     r9, r5, r6, lsl #4          @ r9 = bits[59..28] of windowed x8

    smull   r5, r6, r4, r10             @ r5..r6 = (window_l[10] * (x[9] == -x[8]))
    ldr     r0, =WL13                   @ r0     =  window_l[13] == window_s[4]
    movs    r5, r5, lsr #28
    adc     r10, r5, r6, lsl #4         @ r10 = bits[59..28] of windowed x9

    smull   r5, r6, r0, r12             @ r5..r6 = (window_l[13] * (x[10] == -x[7]))
    ldr     r2, =WL16                   @ r2     =  window_l[16] == window_s[5]
    movs    r5, r5, lsr #28
    adc     r12, r5, r6, lsl #4         @ r10 = bits[59..28] of windowed x9

    smull   r5, r6, r2, lr              @ r5..r6 = (window_l[16] * (x[11] == -x[6]))

    ldr     r0, =0x00

    movs    r5, r5, lsr #28
    adc     lr, r5, r6, lsl #4          @ r10 = bits[59..28] of windowed x9

    stmdb   r1!, { r7 - r10, r12, lr }  @ store windowed x[6] .. x[11]

    ldr     r5, =0x00
    ldr     r6, =0x00
    ldr     r2, =0x00
    ldr     r3, =0x00
    ldr     r4, =0x00

    stmdb   r1!, { r0, r2 - r6 }        @ store windowed x[0] .. x[5]

    b       normal_block_x18_to_x35


    @----


start_block_x18_to_x35:

    ldr     r4, =WL1                    @ r0 = window_l[1]  == window_s[0]

    add     r1, r1, #(24*4)             @ r1 = &x[24]

    ldmia   r1, { r0, r2, r3 }          @ load 3 words from x24, dont update pointer

    @ r0     = x24
    @ r1     = &x[24]
    @ r2     = x25
    @ r3     = x26
    @ r4     = WL1
    @ r5     = WL4
    @ r6     = WL7
    @ r7     = WL10
    @ r8     = WL13
    @ r9     = WL16
    @ r10    = .
    @ r11    = .
    @ r12    = .
    @ lr     = .

    ldr     r5, =WL4                    @ r5 = window_l[4] == window_s[1]

    smull   r10, r11, r4, r0            @ r10..r11 = (window_l[1] * (x[24] == x[29]))
    ldr     r6, =WL7                    @ r6       =  window_l[7]  == window_s[2]
    movs    r10, r10, lsr #28
    adc     lr, r10, r11, lsl #4        @ lr = bits[59..28] of windowed x29

    smull   r10, r11, r5, r2            @ r10..r11 = (window_l[4] * (x[25] == x[28]))
    ldr     r7, =WL10                   @ r7       =  window_l[10] == window_s[3]
    movs    r10, r10, lsr #28
    adc     r12, r10, r11, lsl #4       @ r12 = bits[59..28] of windowed x28

    smull   r10, r11, r6, r3            @ r10..r11 = (window_l[7] * (x[26] == x[27]))
    ldr     r8, =WL13                   @ r8       =  window_l[13] == window_s[4]
    movs    r10, r10, lsr #28
    adc     r4, r10, r11, lsl #4        @ r4 = bits[59..28] of windowed x27

    smull   r10, r11, r7, r3            @ r10..r11 = (window_l[10] * x[26])
    ldr     r9, =WL16                   @ r9       =  window_l[16] == window_s[5]
    movs    r10, r10, lsr #28
    adc     r3, r10, r11, lsl #4        @ r3 = bits[59..28] of windowed x26

    smull   r10, r11, r8, r2            @ r10..r11 = (window_l[13] * x[25])
    ldr     r5, =0x00
    movs    r10, r10, lsr #28
    adc     r2, r10, r11, lsl #4        @ r2 = bits[59..28] of windowed x25

    smull   r10, r11, r9, r0            @ r10..r11 = (window_l[16] * x[24])
    ldr     r6, =0x00
    movs    r10, r10, lsr #28
    adc     r0, r10, r11, lsl #4        @ r0 = bits[59..28] of windowed x24

    stmia   r1!, { r0, r2, r3, r4, r12, lr }    @ store windowed x[24] .. x[29]

    ldr     r7, =0x00
    ldr     r8, =0x00
    ldr     r9, =0x00
    ldr     r10, =0x00

    stmia   r1!, { r5 - r10 }           @ store windowed x[30] .. x[35]

    @----
    @ NB there are 2 possible exits from this function - this is only one of them
    @----

    add     sp, sp, #(21*4)             @ return stack frame
    ldmia   sp!, { r4 - r11, pc }       @ restore callee saved regs, and return

    @----
    @END
    @----

