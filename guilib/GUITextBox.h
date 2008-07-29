/*!
\file GUITextBox.h
\brief 
*/

#ifndef GUILIB_GUITEXTBOX_H
#define GUILIB_GUITEXTBOX_H

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

#include "GUISpinControl.h"
#include "GUITextLayout.h"

/*!
 \ingroup controls
 \brief 
 */

class TiXmlNode;

class CGUITextBox : public CGUIControl, public CGUITextLayout
{
public:
  CGUITextBox(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height,
              float spinWidth, float spinHeight,
              const CImage& textureUp, const CImage& textureDown,
              const CImage& textureUpFocus, const CImage& textureDownFocus,
              const CLabelInfo& spinInfo, float spinX, float spinY,
              const CLabelInfo &labelInfo, int scrollTime = 200);
  CGUITextBox(const CGUITextBox &from);
  virtual ~CGUITextBox(void);
  virtual CGUITextBox *Clone() const { return new CGUITextBox(*this); };

  virtual void DoRender(DWORD currentTime);
  virtual void Render();
  virtual bool OnAction(const CAction &action) ;
  virtual void OnRight();
  virtual void OnLeft();
  virtual bool OnMessage(CGUIMessage& message);

  virtual void PreAllocResources();
  virtual void AllocResources() ;
  virtual void FreeResources() ;
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual void SetPosition(float posX, float posY);
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);
  virtual void SetColorDiffuse(const CGUIInfoColor &color);
  virtual void SetPulseOnSelect(bool pulse);
  virtual void SetNavigation(DWORD up, DWORD down, DWORD left, DWORD right);
  void SetPageControl(DWORD pageControl);

  virtual bool HitTest(const CPoint &point) const;
  virtual bool CanFocus() const;
  virtual bool OnMouseOver(const CPoint &point);
  virtual bool OnMouseClick(DWORD dwButton, const CPoint &point);
  virtual bool OnMouseWheel(char wheel, const CPoint &point);
  void SetInfo(const CGUIInfoLabel &info);
  void SetAutoScrolling(const TiXmlNode *node);
  void ResetAutoScrolling();

protected:
  void OnPageUp();
  void OnPageDown();
  void UpdatePageControl();
  void ScrollToOffset(int offset, bool autoScroll = false);

  float m_spinPosX;
  float m_spinPosY;

  // offset of text in the control for scrolling
  unsigned int m_offset;
  float m_scrollOffset;
  float m_scrollSpeed;
  int   m_scrollTime;
  unsigned int m_itemsPerPage;
  float m_itemHeight;
  DWORD m_renderTime;
  DWORD m_lastRenderTime;

  CLabelInfo m_label;

  // autoscrolling
  int   m_autoScrollCondition;
  int   m_autoScrollTime;      // time to scroll 1 line (ms)
  int   m_autoScrollDelay;     // delay before scroll (ms)
  DWORD m_autoScrollDelayTime; // current offset into the delay
  CAnimation *m_autoScrollRepeatAnim;

  CGUISpinControl m_upDown;

  DWORD m_pageControl;

  CGUIInfoLabel m_info;
};
#endif
