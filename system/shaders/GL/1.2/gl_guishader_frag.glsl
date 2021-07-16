/*
 *  Copyright (C) 2010-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 120

uniform sampler2D m_samp0;
uniform sampler2D m_samp1;
varying vec4 m_cord0;
varying vec4 m_cord1;
varying vec4 m_colour;
uniform vec4 m_unicol;

void main ()
{
#if defined(SM_DEFAULT)
  gl_FragColor = m_unicol;
#elif defined(SM_TEXTURE_NOBLEND)
  gl_FragColor = texture2D(m_samp0, m_cord0.xy);
#elif defined(SM_TEXTURE)
  gl_FragColor = texture2D(m_samp0, m_cord0.xy) * m_unicol;
#elif defined(SM_MULTI)
  gl_FragColor = texture2D(m_samp0, m_cord0.xy) * texture2D(m_samp1, m_cord1.xy);
#elif defined(SM_MULTI_BLENDCOLOR)
  gl_FragColor = texture2D(m_samp0, m_cord0.xy) * texture2D(m_samp1, m_cord1.xy) * m_unicol;
#elif defined(SM_FONTS)
  gl_FragColor.rgb = m_colour.rgb;
  gl_FragColor.a   = texture2D(m_samp0, m_cord0.xy).x * m_colour.a;
#else //fallback
  gl_FragColor = vec4(1.0);
#endif
}
