/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/GameSettings.h"
#include "cores/RetroPlayer/guibridge/IGUIRenderSettings.h"
#include "cores/RetroPlayer/rendering/RenderSettings.h"
#include "utils/Geometry.h"

#include <mutex>

namespace KODI
{
namespace SMART_HOME
{
class CGUICameraControl;

class CGUIRenderSettings : public RETRO::IGUIRenderSettings
{
public:
  CGUIRenderSettings(CGUICameraControl& guiControl);
  CGUIRenderSettings(const CGUIRenderSettings& other);
  ~CGUIRenderSettings() override;

  // implementation of RETRO::IGUIRenderSettings
  bool HasStretchMode() const override;
  bool HasRotation() const override;
  RETRO::CRenderSettings GetSettings() const override { return m_renderSettings; }
  CRect GetDimensions() const override;

  // GUI functions
  void SetStretchMode(RETRO::STRETCHMODE stretchMode);
  void SetRotationDegCCW(unsigned int rotationDegCCW);
  void SetDimensions(const CRect& dimensions);

private:
  void InitializeRenderSettings();

  // Construction parameters
  CGUICameraControl& m_guiControl;

  // Render parameters
  RETRO::CRenderSettings m_renderSettings;
  CRect m_renderDimensions;

  // Synchronization parameters
  mutable std::mutex m_mutex;
};
} // namespace SMART_HOME
} // namespace KODI
