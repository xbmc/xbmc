/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/GUIControl.h"
#include "guilib/guiinfo/GUIInfoLabel.h"

#include <memory>
#include <string>

namespace KODI
{
namespace GUILIB
{
namespace GUIINFO
{
class CGUIInfoLabel;
}
} // namespace GUILIB

namespace SMART_HOME
{
class CGUICameraConfig;
class CGUIRenderHandle;
class CGUIRenderSettings;

class CGUICameraControl : public CGUIControl
{
public:
  CGUICameraControl(int parentID, int controlID, float posX, float posY, float width, float height);
  CGUICameraControl(const CGUICameraControl& other);
  ~CGUICameraControl() override;

  // GUI functions
  void SetPubSubTopic(const GUILIB::GUIINFO::CGUIInfoLabel& topic);
  void SetStretchMode(const GUILIB::GUIINFO::CGUIInfoLabel& stretchMode);
  void SetRotation(const GUILIB::GUIINFO::CGUIInfoLabel& rotation);

  // Rendering functions
  bool HasStretchMode() const { return m_bHasStretchMode; }
  bool HasRotation() const { return m_bHasRotation; }
  const CGUIRenderSettings& GetRenderSettings() const { return *m_renderSettings; }

  // Implementation of CGUIControl
  CGUICameraControl* Clone() const override { return new CGUICameraControl(*this); };
  void Process(unsigned int currentTime, CDirtyRegionList& dirtyregions) override;
  void Render() override;
  bool CanFocus() const override;
  void SetPosition(float posX, float posY) override;
  void SetWidth(float width) override;
  void SetHeight(float height) override;
  void UpdateInfo(const CGUIListItem* item = nullptr) override;

private:
  // Utility class
  struct RenderSettingsDeleter
  {
    // Utility function
    void operator()(CGUIRenderSettings* obj);
  };

  void UpdateTopic(const std::string& topic);
  void UpdateStretchMode(const std::string& strStretchMode);
  void UpdateRotation(const std::string& strRotation);
  void ResetInfo();

  void RegisterControl(const std::string& topic);
  void UnregisterControl();

  // Camera properties
  std::unique_ptr<CGUICameraConfig> m_cameraConfig;

  // Rendering properties
  std::unique_ptr<CGUIRenderSettings, RenderSettingsDeleter> m_renderSettings;
  std::shared_ptr<CGUIRenderHandle> m_renderHandle;
  bool m_bHasStretchMode{false};
  bool m_bHasRotation{false};

  // GUI properties
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_topicInfo;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_stretchModeInfo;
  KODI::GUILIB::GUIINFO::CGUIInfoLabel m_rotationInfo;
};

} // namespace SMART_HOME
} // namespace KODI
