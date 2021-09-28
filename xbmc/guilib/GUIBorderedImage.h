/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIControl.h"
#include "GUIImage.h"
#include "TextureManager.h"

class CGUIBorderedImage : public CGUIImage
{
public:
  CGUIBorderedImage(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& texture, const CTextureInfo& borderTexture, const CRect &borderSize);
  ~CGUIBorderedImage(void) override = default;
  CGUIBorderedImage* Clone() const override { return new CGUIBorderedImage(*this); }

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  void AllocResources() override;
  void FreeResources(bool immediately = false) override;
  void DynamicResourceAlloc(bool bOnOff) override;

  CRect CalcRenderRegion() const override;

protected:
  std::unique_ptr<CGUITexture> m_borderImage;
  CRect m_borderSize;

private:
  CGUIBorderedImage(const CGUIBorderedImage& right);
};

