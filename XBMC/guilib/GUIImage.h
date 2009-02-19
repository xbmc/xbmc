/*!
\file guiImage.h
\brief 
*/

#ifndef GUILIB_GUIIMAGECONTROL_H
#define GUILIB_GUIIMAGECONTROL_H

#pragma once

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
#include "GUITexture.h"
#include "GUILabelControl.h"  // for CGUIInfoLabel

/*!
 \ingroup controls
 \brief 
 */

class CGUIImage : public CGUIControl
{
public:
  CGUIImage(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height, const CTextureInfo& texture);
  CGUIImage(const CGUIImage &left);
  virtual ~CGUIImage(void);
  virtual CGUIImage *Clone() const { return new CGUIImage(*this); };

  virtual void Render();
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);
  virtual bool OnAction(const CAction &action) ;
  virtual bool OnMessage(CGUIMessage& message);
  virtual void PreAllocResources();
  virtual void AllocResources();
  virtual void FreeResources();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool IsDynamicallyAllocated() { return m_bDynamicResourceAlloc; };
  virtual bool CanFocus() const;
  virtual bool IsAllocated() const;
  virtual void UpdateInfo(const CGUIListItem *item = NULL);

  virtual void SetInfo(const CGUIInfoLabel &info);
  virtual void SetFileName(const CStdString& strFileName, bool setConstant = false);
  virtual void SetAspectRatio(const CAspectRatio &aspect);
  void SetAlpha(unsigned char alpha);
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);
  virtual void SetPosition(float posX, float posY);

  const CStdString& GetFileName() const;
  float GetTextureWidth() const;
  float GetTextureHeight() const;

#ifdef _DEBUG
  virtual void DumpTextureUse();
#endif
protected:
  virtual void AllocateOnDemand();
  virtual void FreeTextures(bool immediately = false);
  void FreeResourcesButNotAnims();
  void Process();
  void Render(float left, float top, float bottom, float right, float u1, float v1, float u2, float v2);

  bool m_bDynamicResourceAlloc;

  // for when we are changing textures
  bool m_texturesAllocated;

  // border + conditional info
  CTextureInfo m_image;
  CGUIInfoLabel m_info;

  CGUITexture m_texture;
};
#endif
