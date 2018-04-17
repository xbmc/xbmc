/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "guilib/guiinfo/GUIInfoProviders.h"
#include "guilib/guiinfo/IGUIInfoProvider.h"

#include <algorithm>

using namespace KODI::GUILIB::GUIINFO;

CGUIInfoProviders::CGUIInfoProviders()
{
  RegisterProvider(&m_guiControlsGUIInfo);
  RegisterProvider(&m_musicGUIInfo);
  RegisterProvider(&m_videoGUIInfo);
  RegisterProvider(&m_picturesGUIInfo);
  RegisterProvider(&m_playerGUIInfo);
  RegisterProvider(&m_libraryGUIInfo);
  RegisterProvider(&m_addonsGUIInfo);
  RegisterProvider(&m_weatherGUIInfo);
  RegisterProvider(&m_gamesGUIInfo);
  RegisterProvider(&m_systemGUIInfo);
  RegisterProvider(&m_visualisationGUIInfo);
  RegisterProvider(&m_skinGUIInfo);
}

CGUIInfoProviders::~CGUIInfoProviders()
{
  UnregisterProvider(&m_skinGUIInfo);
  UnregisterProvider(&m_visualisationGUIInfo);
  UnregisterProvider(&m_systemGUIInfo);
  UnregisterProvider(&m_gamesGUIInfo);
  UnregisterProvider(&m_weatherGUIInfo);
  UnregisterProvider(&m_addonsGUIInfo);
  UnregisterProvider(&m_libraryGUIInfo);
  UnregisterProvider(&m_playerGUIInfo);
  UnregisterProvider(&m_picturesGUIInfo);
  UnregisterProvider(&m_videoGUIInfo);
  UnregisterProvider(&m_musicGUIInfo);
  UnregisterProvider(&m_guiControlsGUIInfo);
}

void CGUIInfoProviders::RegisterProvider(IGUIInfoProvider *provider, bool bAppend /* = true */)
{
  auto it = std::find(m_providers.begin(), m_providers.end(), provider);
  if (it == m_providers.end())
  {
    if (bAppend)
      m_providers.emplace_back(provider);
    else
      m_providers.insert(m_providers.begin(), provider);
  }
}

void CGUIInfoProviders::UnregisterProvider(IGUIInfoProvider *provider)
{
  auto it = std::find(m_providers.begin(), m_providers.end(), provider);
  if (it != m_providers.end())
    m_providers.erase(it);
}

bool CGUIInfoProviders::InitCurrentItem(CFileItem *item)
{
  bool bReturn = false;

  for (const auto& provider : m_providers)
  {
    bReturn = provider->InitCurrentItem(item);
  }
  return bReturn;
}

bool CGUIInfoProviders::GetLabel(std::string& value, const CFileItem *item, int contextWindow, const CGUIInfo &info, std::string *fallback) const
{
  for (const auto& provider : m_providers)
  {
    if (provider->GetLabel(value, item, contextWindow, info, fallback))
      return true;
  }
  return false;
}

bool CGUIInfoProviders::GetInt(int& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const
{
  for (const auto& provider : m_providers)
  {
    if (provider->GetInt(value, item, contextWindow, info))
      return true;
  }
  return false;
}

bool CGUIInfoProviders::GetBool(bool& value, const CGUIListItem *item, int contextWindow, const CGUIInfo &info) const
{
  for (const auto& provider : m_providers)
  {
    if (provider->GetBool(value, item, contextWindow, info))
      return true;
  }
  return false;
}

void CGUIInfoProviders::UpdateAVInfo(const AudioStreamInfo& audioInfo, const VideoStreamInfo& videoInfo)
{
  for (const auto& provider : m_providers)
  {
    provider->UpdateAVInfo(audioInfo, videoInfo);
  }
}
