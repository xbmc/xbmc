/*
 *  Copyright (C) 2010-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 150

uniform sampler2D m_samp0;
uniform sampler2D m_samp1;
uniform vec4 m_unicol;
in vec4 m_colour;
in vec4 m_cord0;
in vec4 m_cord1;
out vec4 fragColor;


void main ()
{
#if defined(SM_DEFAULT)
  fragColor = m_unicol;
#elif defined(SM_TEXTURE_NOBLEND)
  fragColor = texture(m_samp0, m_cord0.xy);
#elif defined(SM_TEXTURE) || defined(SM_TEXTURE_LIM)
  fragColor = texture(m_samp0, m_cord0.xy) * m_unicol;
#elif defined(SM_MULTI)
  fragColor = texture(m_samp0, m_cord0.xy) * texture(m_samp1, m_cord1.xy);
#elif defined(SM_MULTI_BLENDCOLOR)
  fragColor = texture(m_samp0, m_cord0.xy) * texture(m_samp1, m_cord1.xy) * m_unicol;
#elif defined(SM_FONTS)
  fragColor.rgb = m_colour.rgb;
  fragColor.a   = texture(m_samp0, m_cord0.xy).x * m_colour.a;
#else //fallback
  fragColor = vec4(1.0);
#endif

#if defined(KODI_LIMITED_RANGE) && !defined(SM_TEXTURE_LIM)
  fragColor.rgb *= 219.0 / 255.0;
  fragColor.rgb += 16.0 / 255.0;
#elif !defined(KODI_LIMITED_RANGE) && defined(SM_TEXTURE_LIM)
  fragColor.rgb = clamp((fragColor.rgb-(16.0/255.0)) * 255.0/219.0, 0, 1);
#endif
}
