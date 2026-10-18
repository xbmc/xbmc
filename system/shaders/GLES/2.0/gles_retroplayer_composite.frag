/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 100

precision highp float;

varying vec2 v_tex;
uniform sampler2D u_samp;
uniform float u_paperWhite; // Nits / 10000 of the frame's ordinary colours
uniform float u_peakRatio; // Peak over paper white
uniform float u_alpha;

// BT.709 -> BT.2020, in linear light
const mat3 bt709_to_bt2020 = mat3(
  0.6274, 0.0691, 0.0164,
  0.3293, 0.9195, 0.0880,
  0.0433, 0.0114, 0.8956
);

// SMPTE ST 2084 (PQ), for luminance in units of 10000 nits
vec3 PQ(vec3 luminance)
{
  const float m1 = 0.1593017578125;
  const float m2 = 78.84375;
  const float c1 = 0.8359375;
  const float c2 = 18.8515625;
  const float c3 = 18.6875;

  vec3 power = pow(max(luminance, vec3(0.0)), vec3(m1));
  return pow((c1 + c2 * power) / (1.0 + c3 * power), vec3(m2));
}

void main()
{
  vec4 colour = texture2D(u_samp, v_tex);

  // Filters write for an SDR display with a gamma of 2.2
  vec3 linear = bt709_to_bt2020 * pow(max(colour.rgb, vec3(0.0)), vec3(2.2));

  // Dim and middle values keep their paper white brightness, while the
  // brightest, such as a phosphor lit at full drive, rise to the peak. All
  // three channels are raised by what their brightest one gets, which keeps
  // hues.
  float brightest = max(max(linear.r, linear.g), linear.b);
  float raise = 1.0 / max(1.0 - brightest * (1.0 - 1.0 / u_peakRatio), 1.0 / u_peakRatio);

  vec3 result = PQ(linear * raise * u_paperWhite);

#if defined(KODI_LIMITED_RANGE)
  result = result * ((235.0 - 16.0) / 255.0) + (16.0 / 255.0);
#endif

  gl_FragColor = vec4(result, u_alpha);
}
