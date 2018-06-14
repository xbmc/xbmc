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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GameSettings.h"

using namespace KODI;

CGameSettings &CGameSettings::operator=(const CGameSettings &rhs)
{
  if (this != &rhs)
  {
    m_videoFilter = rhs.m_videoFilter;
    m_viewMode = rhs.m_viewMode;
    m_rotationDegCCW = rhs.m_rotationDegCCW;
  }
  return *this;
}

void CGameSettings::Reset()
{
  m_videoFilter.clear();
  m_viewMode = RETRO::VIEWMODE::Normal;
  m_rotationDegCCW = 0;
}

bool CGameSettings::operator==(const CGameSettings &rhs) const
{
  return m_videoFilter == rhs.m_videoFilter &&
         m_viewMode == rhs.m_viewMode &&
         m_rotationDegCCW == rhs.m_rotationDegCCW;
}

void CGameSettings::SetVideoFilter(const std::string &videoFilter)
{
  if (videoFilter != m_videoFilter)
  {
    m_videoFilter = videoFilter;
    SetChanged();
  }
}

void CGameSettings::SetViewMode(RETRO::VIEWMODE viewMode)
{
  if (viewMode != m_viewMode)
  {
    m_viewMode = viewMode;
    SetChanged();
  }
}

void CGameSettings::SetRotationDegCCW(unsigned int rotation)
{
  if (rotation != m_rotationDegCCW)
  {
    m_rotationDegCCW = rotation;
    SetChanged();
  }
}
