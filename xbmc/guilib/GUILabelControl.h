/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUILabelControl.h
\brief
*/

#include "GUIControl.h"
#include "GUILabel.h"
#include "guilib/guiinfo/GUIInfoLabel.h"

/*!
 \ingroup controls
 \brief
 */
class CGUILabelControl :
      public CGUIControl
{
public:
  CGUILabelControl(int parentID, int controlID, float posX, float posY, float width, float height, const CLabelInfo& labelInfo, bool wrapMultiLine, bool bHasPath);
  ~CGUILabelControl(void) override;
  CGUILabelControl* Clone() const override { return new CGUILabelControl(*this); }

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  void UpdateInfo(const CGUIListItem *item = NULL) override;
  bool CanFocus() const override;
  bool OnMessage(CGUIMessage& message) override;
  std::string GetDescription() const override;
  float GetWidth() const override;
  void SetWidth(float width) override;
  CRect CalcRenderRegion() const override;

  const CLabelInfo& GetLabelInfo() const { return m_label.GetLabelInfo(); }
  void SetLabel(const std::string &strLabel);
  void ShowCursor(bool bShow = true);
  void SetCursorPos(int iPos);
  int GetCursorPos() const { return m_iCursorPos; }
  void SetInfo(const KODI::GUILIB::GUIINFO::CGUIInfoLabel&labelInfo);
  void SetWidthControl(float minWidth, bool bScroll);
  void SetAlignment(uint32_t align);
  void SetHighlight(unsigned int start, unsigned int end);
  void SetSelection(unsigned int start, unsigned int end);

protected:
  bool UpdateColors(const CGUIListItem* item) override;
  std::string ShortenPath(const std::string &path);

  /*! \brief Return the maximum width of this label control.
   \return Return the width of the control if available, else the width of the current text.
   */
  float GetMaxWidth() const { return m_width ? m_width : m_label.GetTextWidth(); }

  CGUILabel m_label;

  bool m_bHasPath;
  bool m_bShowCursor;
  int m_iCursorPos;
  unsigned int m_dwCounter;

  // stuff for autowidth
  float m_minWidth;

  // multi-info stuff
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_infoLabel;

  unsigned int m_startHighlight;
  unsigned int m_endHighlight;
  unsigned int m_startSelection;
  unsigned int m_endSelection;
};

