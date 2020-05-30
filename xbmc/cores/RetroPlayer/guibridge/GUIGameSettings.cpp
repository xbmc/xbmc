/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIGameSettings.h"

#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/RenderContext.h"
#include "settings/GameSettings.h"
#include "threads/SingleLock.h"

using namespace KODI;
using namespace RETRO;

CGUIGameSettings::CGUIGameSettings(CRPProcessInfo& processInfo)
  : m_processInfo(processInfo), m_guiSettings(processInfo.GetRenderContext().GetGameSettings())
{
  // Reset game settings
  m_guiSettings = m_processInfo.GetRenderContext().GetDefaultGameSettings();

  UpdateSettings();

  m_guiSettings.RegisterObserver(this);
}

CGUIGameSettings::~CGUIGameSettings()
{
  m_guiSettings.UnregisterObserver(this);
}

CRenderSettings CGUIGameSettings::GetSettings() const
{
  CSingleLock lock(m_mutex);

  return m_renderSettings;
}

void CGUIGameSettings::Notify(const Observable& obs, const ObservableMessage msg)
{
  switch (msg)
  {
    case ObservableMessageSettingsChanged:
    {
      UpdateSettings();
      break;
    }
    default:
      break;
  }
}

void CGUIGameSettings::UpdateSettings()
{
  CSingleLock lock(m_mutex);

  // Get settings from GUI
  std::string videoFilter = m_guiSettings.VideoFilter();
  STRETCHMODE stretchMode = m_guiSettings.StretchMode();
  unsigned int rotationDegCCW = m_guiSettings.RotationDegCCW();

  // Save settings for renderer
  m_renderSettings.VideoSettings().SetVideoFilter(videoFilter);
  m_renderSettings.VideoSettings().SetRenderStretchMode(stretchMode);
  m_renderSettings.VideoSettings().SetRenderRotation(rotationDegCCW);
}
