/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file
\brief
*/

#include "GUIControl.h"
#include "GUITexture.h"

/*!
 \ingroup controls
 \brief
 */
class GUIScrollBarControl :
      public CGUIControl
{
public:
  GUIScrollBarControl(int parentID, int controlID, float posX, float posY,
                       float width, float height,
                       const CTextureInfo& backGroundTexture,
                       const CTextureInfo& barTexture, const CTextureInfo& barTextureFocus,
                       const CTextureInfo& nibTexture, const CTextureInfo& nibTextureFocus,
                       ORIENTATION orientation, bool showOnePage);
  ~GUIScrollBarControl() override = default;
  GUIScrollBarControl* Clone() const override { return new GUIScrollBarControl(*this); }

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  bool OnAction(const CAction &action) override;
  void AllocResources() override;
  void FreeResources(bool immediately = false) override;
  void DynamicResourceAlloc(bool bOnOff) override;
  void SetInvalid() override;
  virtual void SetRange(int pageSize, int numItems);
  bool OnMessage(CGUIMessage& message) override;
  void SetValue(int value);
  int GetValue() const;
  std::string GetDescription() const override;
  bool IsVisible() const override;
protected:
  EVENT_RESULT OnMouseEvent(const CPoint& point, const KODI::MOUSE::CMouseEvent& event) override;
  bool UpdateColors(const CGUIListItem* item) override;
  bool UpdateBarSize();
  bool Move(int iNumSteps);
  virtual void SetFromPosition(const CPoint &point);

  std::unique_ptr<CGUITexture> m_guiBackground;
  std::unique_ptr<CGUITexture> m_guiBarNoFocus;
  std::unique_ptr<CGUITexture> m_guiBarFocus;
  std::unique_ptr<CGUITexture> m_guiNibNoFocus;
  std::unique_ptr<CGUITexture> m_guiNibFocus;

  int m_numItems;
  int m_pageSize;
  int m_offset;

  bool m_showOnePage;
  ORIENTATION m_orientation;

private:
  GUIScrollBarControl(const GUIScrollBarControl& control);
};

