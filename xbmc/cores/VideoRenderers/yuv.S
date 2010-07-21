/*
    Copyright (C) 2008 Mans Rullgard

    Permission is hereby granted, free of charge, to any person
    obtaining a copy of this software and associated documentation
    files (the "Software"), to deal in the Software without
    restriction, including without limitation the rights to use, copy,
    modify, merge, publish, distribute, sublicense, and/or sell copies
    of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be
    included in all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
    EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
    MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
    NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
    HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
    WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
 */

        .macro mov32    rd, val
        movw            \rd, #:lower16:\val
        movt            \rd, #:upper16:\val
        .endm

        .fpu neon
        .text

@ yuv420_to_yuv422(uint8_t *yuv, uint8_t *y, uint8_t *u, uint8_t *v,
@                  int w, int h, int yw, int cw, int dw)

#define yuv  r0
#define y    r1
#define u    r2
#define v    r3
#define w    r4
#define h    r5
#define yw   r6
#define cw   r7
#define dw   r8

#define tyuv r9
#define ty   r10
#define tu   r11
#define tv   r12
#define i    lr

        .global yuv420_to_yuv422
        .func   yuv420_to_yuv422
yuv420_to_yuv422:
        push            {r4-r11,lr}
        add             r4,  sp,  #36
.Ldo_conv:
        ldm             r4, {r4-r8}
        push            {r4}
1:
        mov             tu,   u
        mov             tv,   v
        vld1.64         {d2}, [u,:64], cw               @ u0
        vld1.64         {d3}, [v,:64], cw               @ v0
        mov             tyuv, yuv
        mov             ty,   y
        vzip.8          d2,   d3                        @ u0v0
        mov             i,    #16
2:                      
        pld             [y, #64]
        vld1.64         {d0, d1},   [y,:128], yw        @ y0
        pld             [u, #64]
        subs            i,    i,    #4
        vld1.64         {d6},       [u,:64],  cw        @ u2
        pld             [y, #64]
        vld1.64         {d4, d5},   [y,:128], yw        @ y1
        pld             [v, #64]
        vld1.64         {d7},       [v,:64],  cw        @ v2
        pld             [y, #64]
        vld1.64         {d16,d17},  [y,:128], yw        @ y2
        vzip.8          d6,   d7                        @ u2v2
        pld             [u, #64]
        vld1.64         {d22},      [u,:64],  cw        @ u4
        pld             [v, #64]
        vld1.64         {d23},      [v,:64],  cw        @ v4
        pld             [y, #64]
        vld1.64         {d20,d21},  [y,:128], yw        @ y3
        vmov            q9,   q3                        @ u2v2
        vzip.8          d22,  d23                       @ u4v4
        vrhadd.u8       q3,   q1,   q3                  @ u1v1
        vzip.8          q0,   q1                        @ y0u0y0v0
        vmov            q12,  q11                       @ u4v4
        vzip.8          q2,   q3                        @ y1u1y1v1
        vrhadd.u8       q11,  q9,   q11                 @ u3v3
        vst1.64         {d0-d3},    [yuv,:128], dw      @ y0u0y0v0
        vzip.8          q8,   q9                        @ y2u2y2v2
        vst1.64         {d4-d7},    [yuv,:128], dw      @ y1u1y1v1
        vzip.8          q10,  q11                       @ y3u3y3v3
        vst1.64         {d16-d19},  [yuv,:128], dw      @ y2u2y2v2
        vmov            q1,   q12
        vst1.64         {d20-d23},  [yuv,:128], dw      @ y3u3y3v3
        bgt             2b

        subs            w,    w,    #16
        add             yuv,  tyuv, #32
        add             y,    ty,   #16
        add             u,    tu,   #8
        add             v,    tv,   #8
        bgt             1b

        ldr             w,    [sp]
        subs            h,    h,    #16
        add             yuv,  yuv,  dw, lsl #4
        sub             yuv,  yuv,  w,  lsl #1
        add             y,    y,    yw, lsl #4
        sub             y,    y,    w
        add             u,    u,    cw, lsl #3
        sub             u,    u,    w,  asr #1
        add             v,    v,    cw, lsl #3
        sub             v,    v,    w,  asr #1
        bgt             1b

        pop             {r3-r11,pc}
        .endfunc

        .func   neon_open
neon_open:
        push            {r4-r8,lr}
        ldrd            r4,  r5,  [r0, #16]
        ldrd            r6,  r7,  [r0, #24]
        lsl             r8,  r4,  #1
        mov32           r0,  conv_params
        stm             r0,  {r4-r8}
        mov             r0,  #0
        pop             {r4-r8,pc}
        .endfunc

        .func   neon_convert
neon_convert:
        push            {r4-r11,lr}
        ldr             r0,  [r0]
        mov32           r4,  conv_params
        ldm             r1,  {r1-r3}
        b               .Ldo_conv
        .endfunc

        .func   neon_nop
neon_nop:
        bx              lr
        .endfunc

        .section .bss
conv_params:
        .skip           5*4
        .size           conv_params, . - conv_params

        .section        .rodata
.Lname: .asciz          "neon"
ofb_pixconv_neon:
        .word           .Lname
        .word           0               @ flags
        .word           neon_open
        .word           neon_convert
        .word           neon_nop        @ finish
        .word           neon_nop        @ close
        .size           ofb_pixconv_neon, . - ofb_pixconv_neon

        .section        .ofb_pixconv, "a"
ofb_pixconv_neon_p:
        .word           ofb_pixconv_neon
