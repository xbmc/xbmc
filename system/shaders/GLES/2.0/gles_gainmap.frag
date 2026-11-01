/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 100

#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif

varying vec2 v_tex;
uniform sampler2D u_picture; // SDR picture, sRGB curve
uniform sampler2D u_gainMap;
uniform vec2 u_gainMapScale; // gain map texture coordinates from the picture's
uniform vec3 u_min;         // log2
uniform vec3 u_max;         // log2
uniform vec3 u_gamma;
uniform vec3 u_offsetSdr;
uniform vec3 u_offsetHdr;
uniform float u_weight;      // how much of the gain map the display can show, 0 to 1
uniform float u_alpha;

// the picture's linear RGB -> BT.2020, BT.709's unless it kept wider colours
uniform mat3 u_toBT2020;

// SDR white at the BT.2408 reference level, the same as the GUI
const float SDR_WHITE = 203.0 / 10000.0;

vec3 ToLinear(vec3 c)
{
  return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(vec3(0.04045), c));
}

vec3 ToPQ(vec3 L)
{
  const float m1 = 0.1593017578125;
  const float m2 = 78.84375;
  const float c1 = 0.8359375;
  const float c2 = 18.8515625;
  const float c3 = 18.6875;
  vec3 Lm = pow(max(L, vec3(0.0)), vec3(m1));
  return pow((c1 + c2 * Lm) / (1.0 + c3 * Lm), vec3(m2));
}

void main()
{
  vec4 picture = texture2D(u_picture, v_tex);
  vec3 sdr = ToLinear(picture.rgb);

  // Ultra HDR: recover the HDR picture from the SDR one and the gain map
  // a gain map with one channel decodes to grey, the same in all three
  vec3 gain = texture2D(u_gainMap, v_tex * u_gainMapScale).rgb;
  vec3 logBoost = mix(u_min, u_max, pow(gain, 1.0 / u_gamma));
  vec3 hdr = (sdr + u_offsetSdr) * exp2(logBoost * u_weight) - u_offsetHdr;

  vec3 result = ToPQ(u_toBT2020 * max(hdr, vec3(0.0)) * SDR_WHITE);

#if defined(KODI_LIMITED_RANGE)
  result = result * (235.0 - 16.0) / 255.0 + 16.0 / 255.0;
#endif

  gl_FragColor = vec4(result, picture.a * u_alpha);
}
