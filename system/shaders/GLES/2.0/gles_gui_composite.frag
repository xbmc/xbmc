/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 100

precision mediump float;
varying vec2 v_tex;
uniform sampler2D u_samp;       // GUI FBO texture (sRGB, rendered by GUI shaders)
uniform sampler2D u_lutDegamma; // sRGB -> linear LUT (1024 entries, pow 2.2)
uniform sampler2D u_lutTF;      // linear -> PQ LUT (1024 entries, sdrPeak baked in)
uniform float u_ootfGamma;      // HLG: OOTF gamma (1.2 for BT.2100 1000-nit ref)
                                // PQ: 0.0 (use LUT path instead)

// BT.709 -> BT.2020 color space conversion matrix (applied in linear light)
const mat3 bt709_to_bt2020 = mat3(
  0.6274,  0.0691,  0.0164,
  0.3293,  0.9195,  0.0880,
  0.0433,  0.0114,  0.8956
);

void main()
{
  vec4 gui = texture2D(u_samp, v_tex);

  // sRGB -> linear via LUT (replaces pow(gui.rgb, 2.2))
  vec3 linear = vec3(
    texture2D(u_lutDegamma, vec2(gui.r, 0.5)).r,
    texture2D(u_lutDegamma, vec2(gui.g, 0.5)).r,
    texture2D(u_lutDegamma, vec2(gui.b, 0.5)).r
  );

  // BT.709 -> BT.2020 gamut mapping
  linear = bt709_to_bt2020 * linear;

  vec3 result;

  if (u_ootfGamma > 0.0)
  {
    // HLG path: direct computation following libplacebo pl_color_delinearize.
    // BT.2100 reference display: 1000 nits. GUI white (linear 1.0) = 203 nits
    // (BT.2408 reference white) should map to 75% HLG signal.
    //
    // Step 1: normalize to display-peak-relative units
    // Step 2: inverse OOTF with 12x prescale for OETF input domain
    // Step 3: HLG OETF (ARIB STD-B67) piecewise: sqrt for <=1, log for >1
    const float csp_max = 1000.0 / 203.0; // display peak in ref-white units
    const float HLG_A = 0.17883277;
    const float HLG_B = 0.28466892;
    const float HLG_C = 0.55991073;

    vec3 scene = linear / csp_max;
    float Y = dot(scene, vec3(0.2627, 0.6780, 0.0593));
    scene *= 12.0 * pow(max(1e-6, Y), (1.0 - u_ootfGamma) / u_ootfGamma);

    // HLG OETF piecewise (threshold at scene-light 1.0)
    vec3 lo = vec3(0.5) * sqrt(max(scene, vec3(0.0)));
    vec3 hi = vec3(HLG_A) * log(max(scene - vec3(HLG_B), vec3(1e-6))) + vec3(HLG_C);
    result = mix(lo, hi, step(vec3(1.0), scene));
  }
  else
  {
    // PQ path: LUT lookup (sdrPeak baked into LUT range)
    result = vec3(
      texture2D(u_lutTF, vec2(linear.r, 0.5)).r,
      texture2D(u_lutTF, vec2(linear.g, 0.5)).r,
      texture2D(u_lutTF, vec2(linear.b, 0.5)).r
    );
  }

  gl_FragColor = vec4(result, gui.a);
}
