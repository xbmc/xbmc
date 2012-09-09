#ifndef CGUIBorderedImage_H
#define CGUIBorderedImage_H

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "GUIControl.h"
#include "TextureManager.h"
#include "GUIImage.h"

class CGUIBorderedImage : public CGUIImage
{
public:
  CGUIBorderedImage(int parentID, int controlID, float posX, float posY, float width, float height, const CTextureInfo& texture, const CTextureInfo& borderTexture, const CRect &borderSize);
  CGUIBorderedImage(const CGUIBorderedImage &right);
  virtual ~CGUIBorderedImage(void);
  virtual CGUIBorderedImage *Clone() const { return new CGUIBorderedImage(*this); };

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  
  virtual CRect CalcRenderRegion() const;

protected:
  CGUITexture m_borderImage;
  CRect m_borderSize;
};

#endif
