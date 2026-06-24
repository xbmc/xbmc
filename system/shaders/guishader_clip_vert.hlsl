/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/*
 * Vertex shader for font rendering with shader-based clipping (SHADER_METHOD_RENDER_FONT_SHADER_CLIP).
 * Direct3D equivalent of gles_shader_clip.vert
 *
 * Constants:
 *
 * - m_matrix — combined transformation matrix
 *                proj * model * GUI * translate * scale * pixel-correction
 * - m_shaderClip — clip rect in font-local space: float4(x1, y1, x2, y2)
 * - m_texStep, m_texStep2 — bound texture coordinates unit step
 *
 * This shader can be used when clipping with scissors is not possible (e.g. when rotating).
 * It can't discard triangles, but it may degenerate them.
 */

#include "guishader_common.hlsl"

PS_INPUT VS(VS_INPUT input)
{
  PS_INPUT output = (PS_INPUT)0;
  
  // limit the vertices to the clipping area
  float4 position  = input.pos;
  position.xy = clamp(position.xy, m_shaderClip.xy, m_shaderClip.zw);
  output.pos = mul(position, worldViewProj);
  // set rendering depth
  output.pos.z = depth * output.pos.w;

  // correct texture coordinates for clipped vertices  
  float2 clipDist = input.pos.xy - position.xy;
  output.tex = input.tex - clipDist * m_texStep;
  output.tex2 = input.tex2 - clipDist * m_texStep2;
  
  output.color = input.color;
  
  return output;
}
