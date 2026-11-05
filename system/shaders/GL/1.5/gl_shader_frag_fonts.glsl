/*
 *  Copyright (C) 2010-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 150

uniform sampler2D m_samp0;
in vec2 m_cord0;
in vec4 m_colour;
out vec4 fragColor;

// SM_FONTS and SM_FONTS_RGBA shader
void main()
{
#if defined(KODI_FONTS_RGBA)
  fragColor = m_colour * texture(m_samp0, m_cord0);
#else
  fragColor = m_colour;
  fragColor.a *= texture(m_samp0, m_cord0).r;
#endif
#if defined(KODI_LIMITED_RANGE)
  fragColor.rgb *= (235.0-16.0) / 255.0;
  fragColor.rgb += 16.0 / 255.0;
#endif
}
