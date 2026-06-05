/*
 *  Copyright (C) 2010-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#version 150

uniform sampler2D m_samp0;
uniform vec4 m_unicol;
in vec2 m_cord0;
out vec4 fragColor;

// SM_TEXTURE shader
//! @todo This shader assumes the texture is limited-range RGB, but the only
//! consumer (COverlayTextureGL with paletted overlays: PGS/VOBSUB/DVD SPU/
//! paletted BD-PG) uploads full-range RGB (FFmpeg's PGS decoder converts via
//! YUV_TO_RGB2_CCIR with the 255/219 expansion factor, producing full-range
//! output). The math here therefore renders with incorrect brightness in both
//! output modes. The GLES counterpart (gles_shader_texture_noblend.frag) uses
//! full-range-in/limited-range-out math which matches the texture data; it
//! also accepts an m_pma uniform that scales the limited-range offset by
//! alpha for PMA-stored overlays. The fix here is the equivalent: replace the
//! math below with the GLES-style "rgb *= 219/255; rgb += mix(1.0, a, m_pma)
//! * 16/255" inside an "#if defined(KODI_LIMITED_RANGE)" block, and add the
//! m_pma uniform plumbing on the GL side. See Kodi issue #22383.
void main()
{
  fragColor = texture(m_samp0, m_cord0) * m_unicol;
#if !defined(KODI_LIMITED_RANGE)
  fragColor.rgb = clamp((fragColor.rgb-(16.0/255.0)) * 255.0/219.0, 0, 1);
#endif
}
