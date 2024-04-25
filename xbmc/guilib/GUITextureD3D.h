/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUITexture.h"
#include "utils/ColorUtils.h"

class CGUITextureD3D : public CGUITexture
{
public:
  static void Register();
  static CGUITexture* CreateTexture(
      float posX, float posY, float width, float height, const CTextureInfo& texture);

  static void DrawQuad(const CRect& coords,
                       UTILS::COLOR::Color color,
                       CTexture* texture = nullptr,
                       const CRect* texCoords = nullptr,
                       const float depth = 1.0,
                       const bool blending = true);

  CGUITextureD3D(float posX, float posY, float width, float height, const CTextureInfo& texture);
  ~CGUITextureD3D() override = default;

  CGUITextureD3D* Clone() const override;

protected:
  void Begin(UTILS::COLOR::Color color);
  void Draw(float *x, float *y, float *z, const CRect &texture, const CRect &diffuse, int orientation);
  void End();

private:
  CGUITextureD3D(const CGUITextureD3D& texture) = default;

  UTILS::COLOR::Color m_col;
};

