/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 100

attribute vec2 m_attrpos;
attribute vec4 m_attrcol;
attribute vec2 m_attrcord0;
attribute vec2 m_attrcord1;
varying vec2 m_cord0;
varying vec2 m_cord1;
varying vec4 m_colour;
uniform mat4 m_matrix;
uniform vec4 m_shaderClip;
uniform vec4 m_cordStep;
uniform float m_depth;

// this shader can be used in cases where clipping via glScissor() is not
// possible (e.g. when rotating). it can't discard triangles, but it may 
// degenerate them.

void main()
{
  // limit the vertices to the clipping area
  vec4 position = vec4(0., 0., 0., 1.);
  position.xy = clamp(m_attrpos, m_shaderClip.xy, m_shaderClip.zw);
  gl_Position = m_matrix * position;

  // set rendering depth
  gl_Position.z = m_depth * gl_Position.w;

  // correct texture coordinates for clipped vertices
  vec2 clipDist = m_attrpos - position.xy;
  m_cord0 = m_attrcord0 - clipDist * m_cordStep.xy;
  m_cord1 = m_attrcord1 - clipDist * m_cordStep.zw;

  m_colour = m_attrcol;
}
