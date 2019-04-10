/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUITextureD3D.h
\brief
*/

#include "GUITexture.h"
#include "utils/Color.h"

class CGUITextureD3D : public CGUITexture
{
public:
  CGUITextureD3D(float posX, float posY, float width, float height, const CTextureInfo& texture);
  CGUITextureD3D(const CGUITexture& left);

  ~CGUITextureD3D();

protected:
  void Begin(UTILS::Color color) override;
  void Draw(float* x,
            float* y,
            float* z,
            const CRect& texture,
            const CRect& diffuse,
            int orientation) override;
  void End() override;

private:
  UTILS::Color       m_col;
};
