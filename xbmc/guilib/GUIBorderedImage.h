/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include "GUIControl.h"
#include "TextureManager.h"
#include "GUIImage.h"

class CGUIBorderedImage : public CGUIImage
{
public:
  CGUIBorderedImage(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& texture, const CTextureInfo& borderTexture, const CRect &borderSize);
  CGUIBorderedImage(const CGUIBorderedImage &right);
  ~CGUIBorderedImage(void) override;
  CGUIBorderedImage *Clone() const override { return new CGUIBorderedImage(*this); };

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  void AllocResources() override;
  void FreeResources(bool immediately = false) override;
  void DynamicResourceAlloc(bool bOnOff) override;

  CRect CalcRenderRegion() const override;

protected:
  CGUITexture m_borderImage;
  CRect m_borderSize;
};

