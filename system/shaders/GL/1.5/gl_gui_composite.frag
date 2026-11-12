/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 150

in vec2 v_tex;
out vec4 fragColor;
uniform sampler2D u_samp;
uniform float u_sdrPeak; // SDR peak in normalized PQ (e.g. 100/10000 = 0.01)
uniform float u_ootfGamma; // HLG: OOTF gamma (1.2 for BT.2100 1000-nit ref)
                           // PQ: 0.0

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
  vec4 gui = texture(u_samp, v_tex);

  // Skip pixels the GUI never wrote to. Blend unit would still preserve video
  // bit-exactly via DST*(1-0)+garbage*0=DST, but discard makes it structural
  // and avoids the tone-map math, BO read and BO write for those pixels.
  if (gui.a == 0.0)
    discard;

  // sRGB -> linear (IEC 61966-2-1 piecewise EOTF)
  vec3 linear = mix(gui.rgb / 12.92,
                    pow((gui.rgb + 0.055) / 1.055, vec3(2.4)),
                    step(0.04045, gui.rgb));

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
    // scale to PQ luminance range: SDR content at u_sdrPeak of 10000 nits
    linear *= u_sdrPeak;

    // linear -> PQ
    result = forwardPQ(linear);
  }

  // Limited-range encoding at the BO write boundary. Canonical normalized
  // ratios (bit-depth-agnostic in float space; BO write quantizes to the
  // active surface bit depth).
#ifdef KODI_LIMITED_RANGE
  result = result * ((235.0 - 16.0) / 255.0) + (16.0 / 255.0);
#endif

  // The FBO holds coverage; squaring compensates for PQ/HLG blending and keeps opaque GUI opaque.
  fragColor = vec4(result, gui.a * gui.a);
}
