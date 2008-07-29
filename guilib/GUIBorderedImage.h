#ifndef CGUIBorderedImage_H
#define CGUIBorderedImage_H

/*
 *      Copyright (C) 2005-2008 Team XBMC
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIControl.h"
#include "TextureManager.h"
#include "guiImage.h"

class CGUIBorderedImage : public CGUIImage
{
public:
  CGUIBorderedImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CImage& texture, const CImage& borderTexture, const FRECT &borderSize, DWORD dwColorKey = 0);
  CGUIBorderedImage(const CGUIBorderedImage &right);
  virtual ~CGUIBorderedImage(void);
  virtual CGUIBorderedImage *Clone() const { return new CGUIBorderedImage(*this); };

  virtual void Render();
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);
  virtual bool OnMessage(CGUIMessage& message);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool IsAllocated() const;
  
protected:
  CGUIImage m_borderImage;
  FRECT m_borderSize;
};

#endif
