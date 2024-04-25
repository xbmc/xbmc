/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIFadeLabelControl.h
\brief
*/

#include "GUIControl.h"
#include "GUILabel.h"
#include "guilib/guiinfo/GUIInfoLabel.h"

#include <cstdint>
#include <vector>

/*!
 \ingroup controls
 \brief
 */
class CGUIFadeLabelControl : public CGUIControl
{
public:
  CGUIFadeLabelControl(int parentID, int controlID, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, bool scrollOut, unsigned int timeToDelayAtEnd, bool resetOnLabelChange, bool randomized);
  CGUIFadeLabelControl(const CGUIFadeLabelControl &from);
  ~CGUIFadeLabelControl(void) override;
  CGUIFadeLabelControl* Clone() const override { return new CGUIFadeLabelControl(*this); }

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  bool CanFocus() const override;
  bool OnMessage(CGUIMessage& message) override;
  void AssignDepth() override;

  void SetInfo(const std::vector<KODI::GUILIB::GUIINFO::CGUIInfoLabel> &vecInfo);
  void SetScrolling(bool scroll) { m_scroll = scroll; }

  bool AllLabelsShown() const { return m_allLabelsShown; }

protected:
  bool UpdateColors(const CGUIListItem* item) override;
  std::string GetDescription() const override;
  void AddLabel(const std::string &label);

  /*! \brief retrieve the current label for display

   The fadelabel has multiple labels which it cycles through. This routine retrieves the current label.
   It first checks the current label and if non-empty returns it. Otherwise it will iterate through all labels
   until it has a non-empty label to return.

   \return the label that should be displayed.  If empty, there is no label available.
   */
  std::string GetLabel();

  std::vector<KODI::GUILIB::GUIINFO::CGUIInfoLabel> m_infoLabels;
  unsigned int m_currentLabel;
  unsigned int m_lastLabel;

  CLabelInfo m_label;

  bool m_scroll;      // true if we scroll the text
  bool m_scrollOut;   // true if we scroll the text all the way to the left before fading in the next label
  bool m_shortText;   // true if the text we have is shorter than the width of the control

  CScrollInfo m_scrollInfo;
  CGUITextLayout m_textLayout;
  CAnimation m_fadeAnim;
  TransformMatrix m_fadeMatrix;
  unsigned int m_scrollSpeed;
  bool m_resetOnLabelChange;
  bool m_randomized;
  bool m_allLabelsShown = true;

  uint32_t m_fadeDepth{0};
};

