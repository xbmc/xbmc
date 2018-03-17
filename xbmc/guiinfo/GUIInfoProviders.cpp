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

#include "guiinfo/GUIInfoProviders.h"
#include "guiinfo/IGUIInfoProvider.h"

#include <algorithm>

namespace GUIINFO
{

void CGUIInfoProviders::RegisterProvider(IGUIInfoProvider *provider)
{
  auto it = std::find(m_providers.begin(), m_providers.end(), provider);
  if (it == m_providers.end())
    m_providers.emplace_back(provider);
}

void CGUIInfoProviders::UnregisterProvider(IGUIInfoProvider *provider)
{
  auto it = std::find(m_providers.begin(), m_providers.end(), provider);
  if (it != m_providers.end())
    m_providers.erase(it);
}

bool CGUIInfoProviders::GetLabel(std::string& value, const CFileItem *item, const GUIInfo &info, std::string *fallback) const
{
  for (const auto& provider : m_providers)
  {
    if (provider->GetLabel(value, item, info, fallback))
      return true;
  }
  return false;
}

bool CGUIInfoProviders::GetInt(int& value, const CGUIListItem *item, const GUIInfo &info) const
{
  for (const auto& provider : m_providers)
  {
    if (provider->GetInt(value, item, info))
      return true;
  }
  return false;
}

bool CGUIInfoProviders::GetBool(bool& value, const CGUIListItem *item, const GUIINFO::GUIInfo &info) const
{
  for (const auto& provider : m_providers)
  {
    if (provider->GetBool(value, item, info))
      return true;
  }
  return false;
}

} // namespace GUIINFO
