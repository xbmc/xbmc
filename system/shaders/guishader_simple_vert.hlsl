/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/*
 * Vertex shader for font rendering (SHADER_METHOD_RENDER_FONT).
 * Direct3D equivalent of gles_shader_simple.vert
 *
 * Constants:
 *
 * - m_matrix — combined transformation matrix
 *                proj * model * GUI * translate * scale * pixel-correction
 */

#include "guishader_common.hlsl"

PS_INPUT VS(VS_INPUT input)
{
  PS_INPUT output = (PS_INPUT)0;
  output.pos   = mul(input.pos, m_matrix);
  // set rendering depth
  output.pos.z = depth * output.pos.w;
  output.color = input.color;
  output.tex   = input.tex;
  output.tex2  = input.tex2;

  return output;
}
