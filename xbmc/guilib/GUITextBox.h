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

/*!
\file GUITextBox.h
\brief
*/

#include "GUITextLayout.h"
#include "GUIControl.h"
#include "GUILabel.h"
#include "guilib/guiinfo/GUIInfoLabel.h"

/*!
 \ingroup controls
 \brief
 */

class TiXmlNode;

class CGUITextBox : public CGUIControl, public CGUITextLayout
{
public:
  CGUITextBox(int parentID, int controlID, float posX, float posY, float width, float height,
              const CLabelInfo &labelInfo, int scrollTime = 200,
              const CLabelInfo* labelInfoMono = nullptr);
  CGUITextBox(const CGUITextBox &from);
  ~CGUITextBox(void) override;
  CGUITextBox *Clone() const override { return new CGUITextBox(*this); };

  void DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  bool OnMessage(CGUIMessage& message) override;
  float GetHeight() const override;
  void SetMinHeight(float minHeight);

  void SetPageControl(int pageControl);

  bool CanFocus() const override;
  void SetInfo(const KODI::GUILIB::GUIINFO::CGUIInfoLabel &info);
  void SetAutoScrolling(const TiXmlNode *node);
  void SetAutoScrolling(int delay, int time, int repeatTime, const std::string &condition = "");
  void ResetAutoScrolling();

  bool GetCondition(int condition, int data) const override;
  virtual std::string GetLabel(int info) const;
  std::string GetDescription() const override;

  void Scroll(unsigned int offset);

protected:
  void UpdateVisibility(const CGUIListItem *item = NULL) override;
  bool UpdateColors() override;
  void UpdateInfo(const CGUIListItem *item = NULL) override;
  void UpdatePageControl();
  void ScrollToOffset(int offset, bool autoScroll = false);
  unsigned int GetRows() const;
  int GetCurrentPage() const;
  int GetNumPages() const;

  // auto-height
  float m_minHeight;
  float m_renderHeight;

  // offset of text in the control for scrolling
  unsigned int m_offset;
  float m_scrollOffset;
  float m_scrollSpeed;
  int   m_scrollTime;
  unsigned int m_itemsPerPage;
  float m_itemHeight;
  unsigned int m_lastRenderTime;

  CLabelInfo m_label;

  TransformMatrix m_cachedTextMatrix;

  // autoscrolling
  INFO::InfoPtr m_autoScrollCondition;
  int          m_autoScrollTime;      // time to scroll 1 line (ms)
  int          m_autoScrollDelay;     // delay before scroll (ms)
  unsigned int m_autoScrollDelayTime; // current offset into the delay
  CAnimation *m_autoScrollRepeatAnim;

  int m_pageControl;

  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_info;
};

