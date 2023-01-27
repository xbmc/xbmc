/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIControl.h"
#include "guilib/guiinfo/GUIInfoLabel.h"

#include <memory>

namespace KODI
{
namespace RETRO
{
class CGUIRenderSettings;
class CGUIRenderHandle;
class IGUIRenderSettings;

// Label to use when disabling rendering via the <pixels> property. A path
// to pixel data is expected, but instead this constant can be provided to
// skip a file existence check in the renderer.
constexpr const char* NO_PIXEL_DATA = "-";

class CGUIGameControl : public CGUIControl
{
public:
  CGUIGameControl(int parentID, int controlID, float posX, float posY, float width, float height);
  CGUIGameControl(const CGUIGameControl& other);
  ~CGUIGameControl() override;

  // GUI functions
  void SetVideoFilter(const KODI::GUILIB::GUIINFO::CGUIInfoLabel& videoFilter);
  void SetStretchMode(const KODI::GUILIB::GUIINFO::CGUIInfoLabel& stretchMode);
  void SetRotation(const KODI::GUILIB::GUIINFO::CGUIInfoLabel& rotation);
  void SetPixels(const KODI::GUILIB::GUIINFO::CGUIInfoLabel& pixels);

  // Rendering functions
  bool HasVideoFilter() const { return m_bHasVideoFilter; }
  bool HasStretchMode() const { return m_bHasStretchMode; }
  bool HasRotation() const { return m_bHasRotation; }
  bool HasPixels() const { return m_bHasPixels; }
  IGUIRenderSettings* GetRenderSettings() const;

  // implementation of CGUIControl
  CGUIGameControl* Clone() const override { return new CGUIGameControl(*this); }
  void Process(unsigned int currentTime, CDirtyRegionList& dirtyregions) override;
  void Render() override;
  void RenderEx() override;
  bool CanFocus() const override;
  void SetPosition(float posX, float posY) override;
  void SetWidth(float width) override;
  void SetHeight(float height) override;
  void UpdateInfo(const CGUIListItem* item = nullptr) override;

private:
  void Reset();

  void RegisterControl();
  void UnregisterControl();

  // GUI properties
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_videoFilterInfo;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_stretchModeInfo;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_rotationInfo;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_pixelInfo;

  // Rendering properties
  bool m_bHasVideoFilter = false;
  bool m_bHasStretchMode = false;
  bool m_bHasRotation = false;
  bool m_bHasPixels = false;
  std::unique_ptr<CGUIRenderSettings> m_renderSettings;
  std::shared_ptr<CGUIRenderHandle> m_renderHandle;
};

} // namespace RETRO
} // namespace KODI
