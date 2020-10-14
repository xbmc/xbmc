/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIProgressControl.h
\brief
*/

#include "GUIControl.h"
#include "GUITexture.h"

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
  ~CGUIProgressControl() override = default;
  CGUIProgressControl *Clone() const override { return new CGUIProgressControl(*this); };

  void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;
  void Render() override;
  bool CanFocus() const override;
  void AllocResources() override;
  void FreeResources(bool immediately = false) override;
  void DynamicResourceAlloc(bool bOnOff) override;
  void SetInvalid() override;
  bool OnMessage(CGUIMessage& message) override;
  void SetPosition(float posX, float posY) override;
  void SetPercentage(float fPercent);
  void SetInfo(int iInfo, int iInfo2 = 0);
  int GetInfo() const {return m_iInfoCode;};

  float GetPercentage() const;
  std::string GetDescription() const override;
  void UpdateInfo(const CGUIListItem *item = NULL) override;
  bool UpdateLayout(void);
protected:
  bool UpdateColors() override;
  std::unique_ptr<CGUITexture> m_guiBackground;
  std::unique_ptr<CGUITexture> m_guiLeft;
  std::unique_ptr<CGUITexture> m_guiMid;
  std::unique_ptr<CGUITexture> m_guiRight;
  std::unique_ptr<CGUITexture> m_guiOverlay;
  CRect m_guiMidClipRect;

  int m_iInfoCode;
  int m_iInfoCode2 = 0;
  float m_fPercent;
  float m_fPercent2 = 0.0f;
  bool m_bReveal;
  bool m_bChanged;

private:
  CGUIProgressControl(const CGUIProgressControl& control);
};

