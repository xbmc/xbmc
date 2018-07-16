/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
