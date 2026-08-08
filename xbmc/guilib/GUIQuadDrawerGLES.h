/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/ColorUtils.h"
#include "utils/GLBufferObject.h"
#include "utils/Geometry.h"

class CTexture;

// Used for control hit-rect/dirty-region debug overlays via CGUITexture::DrawQuad(). Owned by
// CRenderSystemGLES, which binds it as the DrawQuadFunc callback in CGUITextureGLES::Register().
class CGUIQuadDrawerGLES
{
public:
  void DrawQuad(const CRect& coords,
                KODI::UTILS::COLOR::Color color,
                CTexture* texture,
                const CRect* texCoords,
                float depth,
                bool blending);

  // Called explicitly by CRenderSystemGLES::DestroyRenderSystem() before the context is destroyed
  // -- this object isn't destructed until CRenderSystemGLES itself is, long after that point.
  void Destroy();

private:
  KODI::UTILS::GL::CGLBufferObject m_posVBO{GL_ARRAY_BUFFER};
  KODI::UTILS::GL::CGLBufferObject m_texVBO{GL_ARRAY_BUFFER};
  KODI::UTILS::GL::CGLBufferObject m_IBO{GL_ELEMENT_ARRAY_BUFFER};
};
