/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIFontTTF.h"

#include <string>
#include <vector>

#include "system_gl.h"

class CGUIFontTTFGLBase : public CGUIFontTTF
{
public:
  explicit CGUIFontTTFGLBase(const std::string& strFileName);
  ~CGUIFontTTFGLBase() override;

  virtual bool FirstBegin() override = 0;
  virtual void LastEnd() override = 0;

  CVertexBuffer CreateVertexBuffer(const std::vector<SVertex>& vertices) const override;
  void DestroyVertexBuffer(CVertexBuffer& bufferHandle) const override;
  void CreateStaticVertexBuffers();
  void DestroyStaticVertexBuffers();

protected:
  CTexture* ReallocTexture(unsigned int& newHeight) override;
  bool CopyCharToTexture(FT_BitmapGlyph bitGlyph,
                         unsigned int x1,
                         unsigned int y1,
                         unsigned int x2,
                         unsigned int y2) override;
  void DeleteHardwareTexture() override;

  void RenderCharacter(CRect* texture, SVertex* v, float* x, float* y, float* z) override;

  unsigned int m_updateY1;
  unsigned int m_updateY2;

  GLuint m_elementArrayHandle;

  enum TextureStatus
  {
    TEXTURE_VOID = 0,
    TEXTURE_READY,
    TEXTURE_REALLOCATED,
    TEXTURE_UPDATED,
  };

  TextureStatus m_textureStatus;

  bool m_staticVertexBufferCreated{false};
};

