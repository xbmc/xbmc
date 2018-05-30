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
\file GUIListLabel.h
\brief
*/

#include "GUIControl.h"
#include "GUILabel.h"
#include "guilib/guiinfo/GUIInfoLabel.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIListLabel :
      public CGUIControl
{
public:
  CGUIListLabel(int parentID, int controlID, float posX, float posY, float width, float height,
                const CLabelInfo& labelInfo, const KODI::GUILIB::GUIINFO::CGUIInfoLabel &label, CGUIControl::GUISCROLLVALUE scroll);
  ~CGUIListLabel(void) override;
  CGUIListLabel *Clone() const override { return new CGUIListLabel(*this); };

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  bool CanFocus() const override { return false; };
  void UpdateInfo(const CGUIListItem *item = NULL) override;
  void SetFocus(bool focus) override;
  void SetInvalid() override;
  void SetWidth(float width) override;

  void SetLabel(const std::string &label);
  void SetSelected(bool selected);
  void SetScrolling(bool scrolling);

  static void CheckAndCorrectOverlap(CGUIListLabel &label1, CGUIListLabel &label2)
  {
    CGUILabel::CheckAndCorrectOverlap(label1.m_label, label2.m_label);
  }

  CRect CalcRenderRegion() const override;

protected:
  bool UpdateColors() override;

  CGUILabel     m_label;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_info;
  CGUIControl::GUISCROLLVALUE m_scroll;
};
