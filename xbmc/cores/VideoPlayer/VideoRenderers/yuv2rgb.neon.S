// Copyright (c) 2011 ARM Limited. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
//
//
// http://code.google.com/p/chromium/issues/detail?id=71403
//
//
//
#if defined(HAS_NEON) && !defined(__LP64__)

 /* Initial ARM Neon implementation of core YUV2RGB functions. */

    .text
    .align 4
#ifndef __APPLE__
    .global yuv420_2_rgb8888_neon
    .type   yuv420_2_rgb8888_neon, %function
#else
    .globl  _yuv420_2_rgb8888_neon
#endif
#ifndef __APPLE__
    .global yuv422_2_rgb8888_neon
    .type   yuv422_2_rgb8888_neon, %function
#else
    .globl  _yuv422_2_rgb8888_neon
#endif

/* Constants */
#define coef_y         d0
#define coef_v_r       d1
#define coef_u_g       d2
#define coef_v_g       d3
#define coef_u_b       d4
/* D5 is spare */
#define bias_r         q3
#define bias_r_lo      d6
#define bias_r_hi      d7
#define bias_g         q4
#define bias_g_lo      d8
#define bias_g_hi      d9
#define bias_b         q5
#define bias_b_lo      d10
#define bias_b_hi      d11

/* Input data */
#define y_even         d24
#define y_odd          d26
#define u              d16 /*overlaps with q8 - b_delta, but safe */
#define v              d17 /*overlaps with q8 - b_delta, but safe */

/* Chrominance signal for whole 16x2 block */
#define r_delta        q6
#define g_delta        q7
#define b_delta        q8

/* Current group of 8 pixels */
#define red            q9
#define grn            q10
#define blu            q11
#define y_scale        q15

/* output area, in the right order for interleaved output with VST4 */
#define blu8_e         d24 /* overlaps with y_even, but safe */
#define red8_e         d25
#define blu8_o         d26 /* overlaps with y_odd, but safe */
#define red8_o         d27
#define grn8_e         d28
#define alp8_e         d29
#define grn8_o         d30 /* overlaps with q15 - y_scale, but safe */
#define alp8_o         d31 /* overlaps with q15 - y_scale, but safe */

/* ARM registers */
#define rgb_t_ptr      r0
#define y_t_ptr        r1
#define u_ptr          r2
#define v_ptr          r3
#define width          r4
#define height         r5
#define y_pitch        r6
#define uv_pitch       r7
#define rgb_pitch      r8
#define count          r9
#define aligned_count  sl
#define rgb_b_ptr      fp
#define y_b_ptr        ip

/* Constants */
/* 8-bit constants can be loaded into vectors using VMOV */
#define C_Y_SCALE      74   /* Y scale , 74 */
#define C_V_RED        102  /* v -> red coefficient, 102 */
#define C_U_GREEN      25   /* u -> green , -25 */
#define C_V_GREEN      52   /* v -> green , -52 */
#define C_U_BLUE       129  /* u -> blue, +129 */

/* Coefficients */
    .align 4
coefficients:
#coeff_bias_r:
    .short  -14240  /* bias_r = 74 * (-16)                + (102 * -128) */
                    /*          -1,184                    + -13,056      */
#coeff_bias_g:
    .short    8672  /* bias_g = 74 * (-16) -  25 * (-128) - ( 52 * -128) */
                    /*          -1,184     -  -3200       - -6,656       */
#coeff_bias_b:
    .short  -17696  /* bias_b = 74 * (-16) + 129 * (-128)                */
                    /*          -1,184     + -16,512                     */
#coeff_pad:
    .short       0

#ifndef __APPLE__
yuv420_2_rgb8888_neon:
#else
_yuv420_2_rgb8888_neon:
#endif
    /*  r0 = dst_ptr */
    /*  r1 = y_ptr */
    /*  r2 = u_ptr */
    /*  r3 = v_ptr */
    /*  <> = width */
    /*  <> = height */
    /*  <> = y_pitch */
    /*  <> = uv_pitch */
    /*  <> = rgb_pitch */
#ifndef __APPLE__
    .fnstart
#endif
        push            {r4-r12, lr}         /* 10 words */
        vpush           {q4-q7}              /* 4Q -> 16 words */

        ldr             width,  [sp, #26*4]
        ldr             height, [sp, #27*4]
        ldr             y_pitch, [sp, #28*4]
        ldr             uv_pitch, [sp, #29*4]
        ldr             rgb_pitch, [sp, #30*4]
        adr             lr, coefficients

        /* We can't cope with a width less than 16. Check for that. */
        cmp             width, #16
        vpoplt          {q4-q7}
        poplt           {r4-r12, pc}

        /* Load up vectors containing the bias values. */
        vld1.s16        {bias_r_lo[], bias_r_hi[]}, [lr]!
        vld1.s16        {bias_g_lo[], bias_g_hi[]}, [lr]!
        vld1.s16        {bias_b_lo[], bias_b_hi[]}, [lr]!

        /* Build coefficient vectors containing the same value in each element. */
        vmov.u8         coef_y, #C_Y_SCALE
        vmov.u8         coef_v_r, #C_V_RED
        vmov.u8         coef_u_g, #C_U_GREEN
        vmov.u8         coef_v_g, #C_V_GREEN
        vmov.u8         coef_u_b, #C_U_BLUE

loop_v_420:
        add             y_b_ptr, y_t_ptr, y_pitch
        add             rgb_b_ptr, rgb_t_ptr, rgb_pitch
        mov             aligned_count, width

        /* If width is not an integer multiple of 16, run the
           first pass through the loop with the correct number
           of pixels to correct the size for the remaining loops. */
        ands            count, width, #15
        /* If we're already aligned (i.e. count is now 0), set count
           to 16 to run the first loop as normal. */
        moveq           count, #16

loop_h_420:
        /*****************************/
        /* COMMON CODE FOR BOTH ROWS */
        /*****************************/
        /* Load u and v. */
        vld1.u8         v, [v_ptr]
        add             v_ptr, count, ASR #1
        vld1.u8         u, [u_ptr]
        add             u_ptr, count, ASR #1

        /* Calculate contribution from chrominance signals. */
        vmull.u8        r_delta, v, coef_v_r
        vmull.u8        g_delta, u, coef_u_g
        vmlal.u8        g_delta, v, coef_v_g
        vmull.u8        b_delta, u, coef_u_b

        /* add bias. */
        vadd.s16        r_delta, r_delta, bias_r
        vsub.s16        g_delta, bias_g, g_delta
        vadd.s16        b_delta, b_delta, bias_b

        /* Attempt to preload the next set of u and v input data, for
           better performance. */
        pld             [v_ptr]
        pld             [u_ptr]

        /***********/
        /* TOP ROW */
        /***********/
        /* Top row: Load 16 pixels of y, even and odd. */
        vld2.u8         {y_even, y_odd}, [y_t_ptr], count

        /* Top row, even: combine luminance and chrominance. */
        vmull.u8        y_scale, y_even, coef_y
        vqadd.s16       red, y_scale, r_delta
        vqadd.s16       grn, y_scale, g_delta
        vqadd.s16       blu, y_scale, b_delta

        /* Top row, even: set up alpha data. */
        vmov.u8         alp8_e, #0xFF

        /* Top row, even: clamp, rescale and clip colour components to 8 bits. */
        vqrshrun.s16    red8_e, red, #6
        vqrshrun.s16    grn8_e, grn, #6
        vqrshrun.s16    blu8_e, blu, #6

        /* Top row: attempt to preload the next set of Y data, for
           better performance. */
        pld             [y_t_ptr]

        /* Top row, even: interleave the colour and alpha components
           ready for storage. */
        vzip.u8         red8_e, alp8_e
        vzip.u8         blu8_e, grn8_e

        /* Top row, odd: combine luminance and chrominance. */
        vmull.u8        y_scale, y_odd, coef_y
        vqadd.s16       red, y_scale, r_delta
        vqadd.s16       grn, y_scale, g_delta
        vqadd.s16       blu, y_scale, b_delta

        /* Top row, odd: set up alpha data. */
        vmov.u8         alp8_o, #0xFF

        /* Top row, odd: clamp, rescale and clip colour components to 8 bits. */
        vqrshrun.s16    red8_o, red, #6
        vqrshrun.s16    blu8_o, blu, #6
        vqrshrun.s16    grn8_o, grn, #6

        /* Top row, odd: interleave the colour and alpha components
           ready for storage. */
        vzip.u8         red8_o, alp8_o
        vzip.u8         blu8_o, grn8_o

        /* Top row: Store 16 pixels of ARGB32, interleaving even and
           odd. */
        vst4.u16        {blu8_e, red8_e, blu8_o, red8_o}, [rgb_t_ptr]
        add             rgb_t_ptr, count, LSL #1
        vst4.u16        {grn8_e, alp8_e, grn8_o, alp8_o}, [rgb_t_ptr]
        add             rgb_t_ptr, count, LSL #1

        /**************/
        /* BOTTOM ROW */
        /**************/
        /* Bottom row: Load 16 pixels of y, even and odd. */
        vld2.u8         {y_even, y_odd}, [y_b_ptr], count

        /* Bottom row, even: combine luminance and chrominance. */
        vmull.u8        y_scale, y_even, coef_y
        vqadd.s16       red, y_scale, r_delta
        vqadd.s16       grn, y_scale, g_delta
        vqadd.s16       blu, y_scale, b_delta

        /* Bottom row, even: set up alpha data. */
        vmov.u8         alp8_e, #0xFF

        /* Bottom row, even: clamp, rescale and clip colour components to 8 bits. */
        vqrshrun.s16    red8_e, red, #6
        vqrshrun.s16    blu8_e, blu, #6
        vqrshrun.s16    grn8_e, grn, #6

        /* Bottom row: attempt to preload the next set of Y data, for
           better performance. */
        pld             [y_b_ptr]

        /* Bottom row, even: interleave the colour and alpha components
           ready for storage. */
        vzip.u8         red8_e, alp8_e
        vzip.u8         blu8_e, grn8_e

        /* Bottom row, odd: combine luminance and chrominance. */
        vmull.u8        y_scale, y_odd, coef_y
        vqadd.s16       red, y_scale, r_delta
        vqadd.s16       grn, y_scale, g_delta
        vqadd.s16       blu, y_scale, b_delta

        /* Bottom row, odd: set up alpha data. */
        vmov.u8         alp8_o, #0xFF

        /* Bottom row, odd: clamp, rescale and clip colour components to 8 bits. */
        vqrshrun.s16    red8_o, red, #6
        vqrshrun.s16    blu8_o, blu, #6
        vqrshrun.s16    grn8_o, grn, #6

        /* Bottom row, odd: Interleave the colour and alpha components
           ready for storage. */
        vzip.u8         red8_o, alp8_o
        vzip.u8         blu8_o, grn8_o

        /* Have we reached the end of the row yet? */
        subs            aligned_count, aligned_count, count

        /* Bottom row: Store 16 pixels of ARGB32, interleaving even and
           odd. */
        vst4.u16        {blu8_e, red8_e, blu8_o, red8_o}, [rgb_b_ptr]
        add             rgb_b_ptr, count, LSL #1
        vst4.u16        {grn8_e, alp8_e, grn8_o, alp8_o}, [rgb_b_ptr]
        add             rgb_b_ptr, count, LSL #1

        /* On the second (and subsequent) passes through this code,
           we'll always be working on 16 pixels at once. */
        mov             count, #16
        bgt             loop_h_420

        /* Update pointers for new row of data. */
        sub             rgb_t_ptr, width, LSL #2
        sub             y_t_ptr, width
        sub             u_ptr, width, ASR #1
        sub             v_ptr, width, ASR #1
        add             rgb_t_ptr, rgb_pitch, LSL #1
        add             y_t_ptr, y_pitch, LSL #1
        add             u_ptr, uv_pitch
        add             v_ptr, uv_pitch

        /* Have we reached the bottom row yet? */
        subs            height, height, #2
        bgt             loop_v_420

        vpop            {q4-q7}
        pop             {r4-r12, pc}
#ifndef __APPLE__
        .fnend
#endif

/* Much the same as the above code, but simplified to work on a single
   row at a time. Each U and V value only covers 2 adjacent pixels on
   one row, not a 2x2 matrix */
#define rgb_ptr     rgb_t_ptr
#define y_ptr       y_t_ptr

#ifndef __APPLE__
yuv422_2_rgb8888_neon:
#else
_yuv422_2_rgb8888_neon:
#endif
    /*  r0 = dst_ptr */
    /*  r1 = y_ptr */
    /*  r2 = u_ptr */
    /*  r3 = v_ptr */
    /*  <> = width */
    /*  <> = height */
    /*  <> = y_pitch */
    /*  <> = uv_pitch */
    /*  <> = rgb_pitch */
#ifndef __APPLE__
    .fnstart
#endif
        push            {r4-r12, lr}         /* 10 words */
        vpush           {q4-q7}              /* 4Q -> 16 words */

        ldr             width,  [sp, #26*4]
        ldr             height, [sp, #27*4]
        ldr             y_pitch, [sp, #28*4]
        ldr             uv_pitch, [sp, #29*4]
        ldr             rgb_pitch, [sp, #30*4]
        adr             lr, coefficients

        /* We can't cope with a width less than 16. Check for that. */
        cmp             width, #16
        vpoplt          {q4-q7}
        poplt           {r4-r12, pc}

        /* Load up vectors containing the bias values. */
        vld1.s16        {bias_r_lo[], bias_r_hi[]}, [lr]!
        vld1.s16        {bias_g_lo[], bias_g_hi[]}, [lr]!
        vld1.s16        {bias_b_lo[], bias_b_hi[]}, [lr]!

        /* Build coefficient vectors containing the same value in each element. */
        vmov.u8         coef_y, #C_Y_SCALE
        vmov.u8         coef_v_r, #C_V_RED
        vmov.u8         coef_u_g, #C_U_GREEN
        vmov.u8         coef_v_g, #C_V_GREEN
        vmov.u8         coef_u_b, #C_U_BLUE

loop_v_422:
        mov             aligned_count, width
        /* If width is not an integer multiple of 16, run the
           first pass through the loop with the correct number
           of pixels to correct the size for the remaining loops. */
        ands            count, width, #15
        /* If we're already aligned (i.e. count is now 0), set count
           to 16 to run the first loop as normal. */
        moveq           count, #16

loop_h_422:
        /* Load u and v. */
        vld1.u8         v, [v_ptr]
        add             v_ptr, count, ASR #1
        vld1.u8         u, [u_ptr]
        add             u_ptr, count, ASR #1

        /* Calculate contribution from chrominance signals. */
        vmull.u8        r_delta, v, coef_v_r
        vmull.u8        g_delta, u, coef_u_g
        vmlal.u8        g_delta, v, coef_v_g
        vmull.u8        b_delta, u, coef_u_b

        /* Attempt to preload the next set of u and v input data, for
           better performance. */
        pld             [v_ptr]
        pld             [u_ptr]

        /* Load 16 pixels of y, even and odd. */
        vld2.u8         {y_even, y_odd}, [y_ptr], count

        /* Add bias. */
        vadd.s16        r_delta, r_delta, bias_r
        vsub.s16        g_delta, bias_g, g_delta
        vadd.s16        b_delta, b_delta, bias_b

        /* Even: combine luminance and chrominance. */
        vmull.u8        y_scale, y_even, coef_y
        vqadd.s16       red, y_scale, r_delta
        vqadd.s16       grn, y_scale, g_delta
        vqadd.s16       blu, y_scale, b_delta

        /* Even: set up alpha data. */
        vmov.u8         alp8_e, #0xFF

        /* Attempt to preload the next set of Y data, for better
           performance. */
        pld             [y_ptr]

        /* Even: clamp, rescale and clip colour components to 8 bits. */
        vqrshrun.s16    red8_e, red, #6
        vqrshrun.s16    grn8_e, grn, #6
        vqrshrun.s16    blu8_e, blu, #6

        /* Even: Interleave the colour and alpha components
           ready for storage. */
        vzip.u8         red8_e, alp8_e
        vzip.u8         blu8_e, grn8_e

        /* Odd: combine luminance and chrominance. */
        vmull.u8        y_scale, y_odd, coef_y
        vqadd.s16       red, y_scale, r_delta
        vqadd.s16       grn, y_scale, g_delta
        vqadd.s16       blu, y_scale, b_delta

        /* Odd: set up alpha data. */
        vmov.u8         alp8_o, #0xFF

        /* Odd: clamp, rescale and clip colour components to 8 bits. */
        vqrshrun.s16    red8_o, red, #6
        vqrshrun.s16    blu8_o, blu, #6
        vqrshrun.s16    grn8_o, grn, #6

        /* Odd: Interleave the colour and alpha components
           ready for storage. */
        vzip.u8         red8_o, alp8_o
        vzip.u8         blu8_o, grn8_o

        /* Have we reached the end of the row yet? */
        subs            aligned_count, aligned_count, count

        /* Store 16 pixels of ARGB32, interleaving even and odd. */
        vst4.u16        {blu8_e, red8_e, blu8_o, red8_o}, [rgb_ptr]
        add             rgb_ptr, count, LSL #1
        vst4.u16        {grn8_e, alp8_e, grn8_o, alp8_o}, [rgb_ptr]
        add             rgb_ptr, count, LSL #1

        /* On the second (and subsequent) passes through this code,
           we'll always be working on 16 pixels at once. */
        mov             count, #16
        bgt             loop_h_422

        /* Update pointers for new row of data. */
        sub             rgb_ptr, width, LSL #2
        sub             y_ptr, width
        sub             u_ptr, width, ASR #1
        sub             v_ptr, width, ASR #1
        add             rgb_ptr, rgb_pitch
        add             y_ptr, y_pitch
        add             u_ptr, uv_pitch
        add             v_ptr, uv_pitch

        /* Have we reached the bottom yet? */
        subs            height, height, #1
        bgt             loop_v_422

        vpop            {q4-q7}
        pop             {r4-r12, pc}
#ifndef __APPLE__
        .fnend
#endif

#endif /* __ARM_NEON__ */
