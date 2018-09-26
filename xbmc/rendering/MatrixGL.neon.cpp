/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */


void Matrix4Mul(float* src_mat_1, const float* src_mat_2)
{
  asm volatile (
    // Store A & B leaving room at top of registers for result (q0-q3)
    "vldmia %0, { q4-q7 }  \n\t"
    "vldmia %1, { q8-q11 } \n\t"

    // result = first column of B x first row of A
    "vmul.f32 q0, q8, d8[0]\n\t"
    "vmul.f32 q1, q8, d10[0]\n\t"
    "vmul.f32 q2, q8, d12[0]\n\t"
    "vmul.f32 q3, q8, d14[0]\n\t"

    // result += second column of B x second row of A
    "vmla.f32 q0, q9, d8[1]\n\t"
    "vmla.f32 q1, q9, d10[1]\n\t"
    "vmla.f32 q2, q9, d12[1]\n\t"
    "vmla.f32 q3, q9, d14[1]\n\t"

    // result += third column of B x third row of A
    "vmla.f32 q0, q10, d9[0]\n\t"
    "vmla.f32 q1, q10, d11[0]\n\t"
    "vmla.f32 q2, q10, d13[0]\n\t"
    "vmla.f32 q3, q10, d15[0]\n\t"

    // result += last column of B x last row of A
    "vmla.f32 q0, q11, d9[1]\n\t"
    "vmla.f32 q1, q11, d11[1]\n\t"
    "vmla.f32 q2, q11, d13[1]\n\t"
    "vmla.f32 q3, q11, d15[1]\n\t"

    // output = result registers
    "vstmia %1, { q0-q3 }"
    : //no output
    : "r" (src_mat_2), "r" (src_mat_1)       // input - note *value* of pointer doesn't change
    : "memory", "q0", "q1", "q2", "q3", "q4", "q5", "q6", "q7", "q8", "q9", "q10", "q11" //clobber
    );
}
