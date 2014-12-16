/*!
\file GUIProgressControl.h
\brief
*/

#ifndef GUILIB_GUIPROGRESSCONTROL_H
#define GUILIB_GUIPROGRESSCONTROL_H

#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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

#include "GUITexture.h"
#include "GUIControl.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIProgressControl :
      public CGUIControl
{
public:
  CGUIProgressControl(int parentID, int controlID, float posX, float posY,
                      float width, float height, const CTextureInfo& backGroundTexture,
                      const CTextureInfo& leftTexture, const CTextureInfo& midTexture,
                      const CTextureInfo& rightTexture, const CTextureInfo& overlayTexture,
                      bool reveal=false);
  virtual ~CGUIProgressControl(void);
  virtual CGUIProgressControl *Clone() const { return new CGUIProgressControl(*this); };

  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Render();
  virtual bool CanFocus() const;
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetInvalid();
  virtual bool OnMessage(CGUIMessage& message);
  virtual void SetPosition(float posX, float posY);
  void SetPercentage(float fPercent);
  void SetInfo(int iInfo);
  int GetInfo() const {return m_iInfoCode;};

  float GetPercentage() const;
  std::string GetDescription() const;
  virtual void UpdateInfo(const CGUIListItem *item = NULL);
  bool UpdateLayout(void);
protected:
  virtual bool UpdateColors();
  CGUITexture m_guiBackground;
  CGUITexture m_guiLeft;
  CGUITexture m_guiMid;
  CGUITexture m_guiRight;
  CGUITexture m_guiOverlay;
  CRect m_guiMidClipRect;

  int m_iInfoCode;
  float m_fPercent;
  bool m_bReveal;
  bool m_bChanged;
};
#endif
