/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 100

precision mediump float;
varying vec2 v_tex;
uniform sampler2D u_samp;
uniform float u_sdrPeak; // SDR peak in normalized PQ (e.g. 100/10000 = 0.01)

// ST2084 (PQ) constants
const float ST2084_m1 = 0.1593017578125;   // 2610/16384
const float ST2084_m2 = 78.84375;           // 2523/4096 * 128
const float ST2084_c1 = 0.8359375;          // 3424/4096
const float ST2084_c2 = 18.8515625;         // 2413/4096 * 32
const float ST2084_c3 = 18.6875;            // 2392/4096 * 32

// BT.709 -> BT.2020 color space conversion matrix (applied in linear light)
const mat3 bt709_to_bt2020 = mat3(
  0.6274,  0.0691,  0.0164,
  0.3293,  0.9195,  0.0880,
  0.0433,  0.0114,  0.8956
);

vec3 forwardPQ(vec3 L)
{
  vec3 Lm1 = pow(L, vec3(ST2084_m1));
  return pow((ST2084_c1 + ST2084_c2 * Lm1) / (1.0 + ST2084_c3 * Lm1), vec3(ST2084_m2));
}

void main()
{
  vec4 gui = texture2D(u_samp, v_tex);

  // sRGB -> linear (approximate)
  vec3 linear = pow(gui.rgb, vec3(2.2));

  // BT.709 -> BT.2020 gamut mapping
  linear = bt709_to_bt2020 * linear;

  // scale to PQ luminance range: SDR content at u_sdrPeak of 10000 nits
  linear *= u_sdrPeak;

  // linear -> PQ
  vec3 pq = forwardPQ(linear);

  gl_FragColor = vec4(pq, gui.a);
}
