/*
 *      Copyright (C) 2017-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "GameSettings.h"

CGameSettings &CGameSettings::operator=(const CGameSettings &rhs)
{
  if (this != &rhs)
  {
    m_scalingMethod = rhs.m_scalingMethod;
    m_viewMode = rhs.m_viewMode;
  }
  return *this;
}

void CGameSettings::Reset()
{
  m_scalingMethod = VS_SCALINGMETHOD_AUTO;
  m_viewMode = ViewModeNormal;
}

bool CGameSettings::operator==(const CGameSettings &rhs) const
{
  return m_scalingMethod == rhs.m_scalingMethod &&
         m_viewMode == rhs.m_viewMode;
}

void CGameSettings::SetScalingMethod(ESCALINGMETHOD scalingMethod)
{
  if (scalingMethod != m_scalingMethod)
  {
    m_scalingMethod = scalingMethod;
    SetChanged();
  }
}

void CGameSettings::SetViewMode(enum ViewMode viewMode)
{
  if (viewMode != m_viewMode)
  {
    m_viewMode = viewMode;
    SetChanged();
  }
}
