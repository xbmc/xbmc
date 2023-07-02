/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIRenderSettings.h"

#include "GUIGameControl.h"

#include <mutex>

using namespace KODI;
using namespace RETRO;

CGUIRenderSettings::CGUIRenderSettings(CGUIGameControl& guiControl) : m_guiControl(guiControl)
{
}

bool CGUIRenderSettings::HasVideoFilter() const
{
  return m_guiControl.HasVideoFilter();
}

bool CGUIRenderSettings::HasStretchMode() const
{
  return m_guiControl.HasStretchMode();
}

bool CGUIRenderSettings::HasRotation() const
{
  return m_guiControl.HasRotation();
}

bool CGUIRenderSettings::HasPixels() const
{
  return m_guiControl.HasPixels();
}

CRenderSettings CGUIRenderSettings::GetSettings() const
{
  std::unique_lock<CCriticalSection> lock(m_mutex);

  return m_renderSettings;
}

CRect CGUIRenderSettings::GetDimensions() const
{
  std::unique_lock<CCriticalSection> lock(m_mutex);

  return m_renderDimensions;
}

void CGUIRenderSettings::Reset()
{
  std::unique_lock<CCriticalSection> lock(m_mutex);

  return m_renderSettings.Reset();
}

void CGUIRenderSettings::SetSettings(const CRenderSettings& settings)
{
  std::unique_lock<CCriticalSection> lock(m_mutex);

  m_renderSettings = settings;
}

void CGUIRenderSettings::SetDimensions(const CRect& dimensions)
{
  std::unique_lock<CCriticalSection> lock(m_mutex);

  m_renderDimensions = dimensions;
}

void CGUIRenderSettings::SetVideoFilter(const std::string& videoFilter)
{
  std::unique_lock<CCriticalSection> lock(m_mutex);

  m_renderSettings.VideoSettings().SetVideoFilter(videoFilter);
}

void CGUIRenderSettings::SetStretchMode(STRETCHMODE stretchMode)
{
  std::unique_lock<CCriticalSection> lock(m_mutex);

  m_renderSettings.VideoSettings().SetRenderStretchMode(stretchMode);
}

void CGUIRenderSettings::SetRotationDegCCW(unsigned int rotationDegCCW)
{
  std::unique_lock<CCriticalSection> lock(m_mutex);

  m_renderSettings.VideoSettings().SetRenderRotation(rotationDegCCW);
}

void CGUIRenderSettings::SetPixels(const std::string& pixelPath)
{
  std::unique_lock<CCriticalSection> lock(m_mutex);

  m_renderSettings.VideoSettings().SetPixels(pixelPath);
}
