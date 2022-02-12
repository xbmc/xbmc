/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file CGUIColorButtonControl.h
\brief
*/
#include "GUIButtonControl.h"
#include "guilib/GUILabel.h"
#include "guilib/guiinfo/GUIInfoColor.h"
#include "utils/ColorUtils.h"

/*!
 \ingroup controls
 \brief
 */
class CGUIColorButtonControl : public CGUIButtonControl
{
public:
  CGUIColorButtonControl(int parentID,
                         int controlID,
                         float posX,
                         float posY,
                         float width,
                         float height,
                         const CTextureInfo& textureFocus,
                         const CTextureInfo& textureNoFocus,
                         const CLabelInfo& labelInfo,
                         const CTextureInfo& colorMask,
                         const CTextureInfo& colorDisabledMask);

  ~CGUIColorButtonControl() override = default;
  CGUIColorButtonControl* Clone() const override { return new CGUIColorButtonControl(*this); }
  CGUIColorButtonControl(const CGUIColorButtonControl& control);

  void Process(unsigned int currentTime, CDirtyRegionList& dirtyregions) override;
  void Render() override;
  bool OnAction(const CAction& action) override;
  bool OnMessage(CGUIMessage& message) override;
  void AllocResources() override;
  void FreeResources(bool immediately = false) override;
  void DynamicResourceAlloc(bool bOnOff) override;
  void SetInvalid() override;
  void SetPosition(float posX, float posY) override;
  void SetWidth(float width) override;
  void SetHeight(float height) override;
  std::string GetDescription() const override;
  void SetColorDimensions(float posX, float posY, float width, float height);
  bool IsSelected() const { return m_bSelected; }
  void SetImageBoxColor(const std::string& hexColor);
  void SetImageBoxColor(KODI::GUILIB::GUIINFO::CGUIInfoColor color);

protected:
  bool UpdateColors(const CGUIListItem* item) override;
  void ProcessInfoText(unsigned int currentTime);
  void RenderInfoText();
  CGUILabel::COLOR GetTextColor() const override;
  std::unique_ptr<CGUITexture> m_imgColorMask;
  std::unique_ptr<CGUITexture> m_imgColorDisabledMask;
  float m_colorPosX;
  float m_colorPosY;
  KODI::GUILIB::GUIINFO::CGUIInfoColor m_imgBoxColor;
  CGUILabel m_labelInfo;
};
