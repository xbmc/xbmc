/*
 *  Copyright (C) 2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIRangesControl.h
\brief
*/

#include "GUIControl.h"
#include "GUITexture.h"

#include <utility>
#include <vector>

/*!
 \ingroup controls
 \brief
 */
class CGUIRangesControl : public CGUIControl
{
  class CGUIRange
  {
  public:
    CGUIRange(float fPosX, float fPosY, float fWidth, float fHeight,
              const CTextureInfo& lowerTextureInfo, const CTextureInfo& fillTextureInfo,
              const CTextureInfo& upperTextureInfo, const std::pair<float, float>& percentages);

    CGUIRange(const CGUIRange& range);

    void AllocResources();
    void FreeResources(bool bImmediately);
    void DynamicResourceAlloc(bool bOnOff);
    void SetInvalid();
    bool SetDiffuseColor(const KODI::GUILIB::GUIINFO::CGUIInfoColor& color);

    bool Process(unsigned int iCurrentTime);
    void Render();
    bool UpdateLayout(float fBackgroundTextureHeight, float fPosX, float fPosY, float fWidth, float fScaleX, float fScaleY);

  private:
    std::unique_ptr<CGUITexture> m_guiLowerTexture;
    std::unique_ptr<CGUITexture> m_guiFillTexture;
    std::unique_ptr<CGUITexture> m_guiUpperTexture;
    std::pair<float,float> m_percentValues;
  };

public:
  CGUIRangesControl(int iParentID, int iControlID, float fPosX, float fPosY,
                    float fWidth, float fHeight, const CTextureInfo& backGroundTexture,
                    const CTextureInfo& leftTexture, const CTextureInfo& midTexture,
                    const CTextureInfo& rightTexture, const CTextureInfo& overlayTexture,
                    int iInfo);
  ~CGUIRangesControl() override = default;
  CGUIRangesControl* Clone() const override { return new CGUIRangesControl(*this); };

  void Process(unsigned int iCurrentTime, CDirtyRegionList& dirtyregions) override;
  void Render() override;
  bool CanFocus() const override;
  void AllocResources() override;
  void FreeResources(bool bImmediately = false) override;
  void DynamicResourceAlloc(bool bOnOff) override;
  void SetInvalid() override;
  void SetPosition(float fPosX, float fPosY) override;
  void UpdateInfo(const CGUIListItem* item = nullptr) override;

protected:
  void SetRanges(const std::vector<std::pair<float, float>>& ranges);
  void ClearRanges();

  bool UpdateColors() override;
  bool UpdateLayout();

  std::unique_ptr<CGUITexture> m_guiBackground;
  std::unique_ptr<CGUITexture> m_guiOverlay;
  const CTextureInfo m_guiLowerTextureInfo;
  const CTextureInfo m_guiFillTextureInfo;
  const CTextureInfo m_guiUpperTextureInfo;
  std::vector<CGUIRange> m_ranges;
  int m_iInfoCode = 0;
  std::string m_prevRanges;

private:
  CGUIRangesControl(const CGUIRangesControl& control);
};
