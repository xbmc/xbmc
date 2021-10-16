/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
  CGUIListLabel* Clone() const override { return new CGUIListLabel(*this); }

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  bool CanFocus() const override { return false; }
  void UpdateInfo(const CGUIListItem *item = NULL) override;
  void SetFocus(bool focus) override;
  void SetInvalid() override;
  void SetWidth(float width) override;

  void SetLabel(const std::string &label);
  void SetSelected(bool selected);

  static void CheckAndCorrectOverlap(CGUIListLabel &label1, CGUIListLabel &label2)
  {
    CGUILabel::CheckAndCorrectOverlap(label1.m_label, label2.m_label);
  }

  CRect CalcRenderRegion() const override;

protected:
  bool UpdateColors(const CGUIListItem* item) override;

  CGUILabel     m_label;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_info;
  CGUIControl::GUISCROLLVALUE m_scroll;
};
