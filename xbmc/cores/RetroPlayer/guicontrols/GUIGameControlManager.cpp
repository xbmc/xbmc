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

#include "GUIGameControlManager.h"
#include "GUIGameControl.h"
#include "cores/RetroPlayer/rendering/GUIRenderSettings.h"

using namespace KODI;
using namespace RETRO;

void CGUIGameControlManager::SetActiveControl(CGUIGameControl *control)
{
  m_activeControl = control;
}

bool CGUIGameControlManager::IsControlActive() const
{
  return m_activeControl != nullptr;
}

void CGUIGameControlManager::ResetActiveControl()
{
  m_activeControl = nullptr;
}

const CGUIRenderSettings &CGUIGameControlManager::GetRenderSettings() const
{
  if (m_activeControl != nullptr)
    return m_activeControl->GetRenderSettings();

  static const CGUIRenderSettings defaultSettings;
  return defaultSettings;
}
