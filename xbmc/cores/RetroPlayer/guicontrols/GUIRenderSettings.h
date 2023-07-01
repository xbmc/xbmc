/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/GameSettings.h"
#include "cores/RetroPlayer/guibridge/IGUIRenderSettings.h"
#include "cores/RetroPlayer/rendering/RenderSettings.h"
#include "threads/CriticalSection.h"
#include "utils/Geometry.h"

namespace KODI
{
namespace RETRO
{
class CGUIGameControl;

class CGUIRenderSettings : public IGUIRenderSettings
{
public:
  CGUIRenderSettings(CGUIGameControl& guiControl);
  ~CGUIRenderSettings() override = default;

  // implementation of IGUIRenderSettings
  bool HasVideoFilter() const override;
  bool HasStretchMode() const override;
  bool HasRotation() const override;
  bool HasPixels() const override;
  CRenderSettings GetSettings() const override;
  CRect GetDimensions() const override;

  // Render functions
  void Reset();
  void SetSettings(const CRenderSettings& settings);
  void SetDimensions(const CRect& dimensions);
  void SetVideoFilter(const std::string& videoFilter);
  void SetStretchMode(STRETCHMODE stretchMode);
  void SetRotationDegCCW(unsigned int rotationDegCCW);
  void SetPixels(const std::string& pixelPath);

private:
  // Construction parameters
  CGUIGameControl& m_guiControl;

  // Render parameters
  CRenderSettings m_renderSettings;
  CRect m_renderDimensions;

  // Synchronization parameters
  mutable CCriticalSection m_mutex;
};
} // namespace RETRO
} // namespace KODI
