/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GUIRenderSettings.h"
#include "GUIGameControl.h"
#include "threads/SingleLock.h"

using namespace KODI;
using namespace RETRO;

CGUIRenderSettings::CGUIRenderSettings(CGUIGameControl &guiControl) :
  m_guiControl(guiControl)
{
}

bool CGUIRenderSettings::HasVideoFilter() const
{
  return m_guiControl.HasVideoFilter();
}

bool CGUIRenderSettings::HasViewMode() const
{
  return m_guiControl.HasViewMode();
}

bool CGUIRenderSettings::HasRotation() const
{
  return m_guiControl.HasRotation();
}

CRenderSettings CGUIRenderSettings::GetSettings() const
{
  CSingleLock lock(m_mutex);

  return m_renderSettings;
}

CRect CGUIRenderSettings::GetDimensions() const
{
  CSingleLock lock(m_mutex);

  return m_renderDimensions;
}

void CGUIRenderSettings::Reset()
{
  CSingleLock lock(m_mutex);

  return m_renderSettings.Reset();
}

void CGUIRenderSettings::SetSettings(CRenderSettings settings)
{
  CSingleLock lock(m_mutex);

  m_renderSettings = std::move(settings);
}

void CGUIRenderSettings::SetDimensions(const CRect &dimensions)
{
  CSingleLock lock(m_mutex);

  m_renderDimensions = dimensions;
}

void CGUIRenderSettings::SetVideoFilter(const std::string &videoFilter)
{
  CSingleLock lock(m_mutex);

  m_renderSettings.VideoSettings().SetVideoFilter(videoFilter);
}

void CGUIRenderSettings::SetViewMode(VIEWMODE viewMode)
{
  CSingleLock lock(m_mutex);

  m_renderSettings.VideoSettings().SetRenderViewMode(viewMode);
}

void CGUIRenderSettings::SetRotationDegCCW(unsigned int rotationDegCCW)
{
  CSingleLock lock(m_mutex);

  m_renderSettings.VideoSettings().SetRenderRotation(rotationDegCCW);
}
