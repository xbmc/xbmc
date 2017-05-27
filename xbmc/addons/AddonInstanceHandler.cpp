/*
 *      Copyright (C) 2005-2017 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AddonInstanceHandler.h"

#include "utils/log.h"
#include "utils/StringUtils.h"

namespace ADDON
{

IAddonInstanceHandler::IAddonInstanceHandler(ADDON_TYPE type, const AddonDllPtr& addonInfo, KODI_HANDLE parentInstance/* = nullptr*/, const std::string& instanceID/* = ""*/)
  : m_type(type),
    m_parentInstance(parentInstance),
    m_addon(addonInfo)
{
  // if no special instance ID is given generate one from class pointer (is
  // faster as unique id and also safe enough for them).
  m_instanceId = !instanceID.empty() ? instanceID : StringUtils::Format("%p", static_cast<void*>(this));
}

IAddonInstanceHandler::~IAddonInstanceHandler()
{
}

bool IAddonInstanceHandler::CreateInstance(KODI_HANDLE instance)
{
  if (!m_addon)
  {
    CLog::Log(LOGFATAL, "Addon: Tried to create instance with not present addon class");
    return false;
  }

  ADDON_STATUS status = m_addon->CreateInstance(m_type, m_instanceId, instance, m_parentInstance);
  if (status != ADDON_STATUS_OK)
  {
    CLog::Log(LOGERROR, "Addon: %s returned bad status \"%s\" during instance creation", m_addon->ID().c_str(), kodi::TranslateAddonStatus(status).c_str());
    return false;
  }
  return true;
}

void IAddonInstanceHandler::DestroyInstance()
{
  if (m_addon)
    m_addon->DestroyInstance(m_instanceId);
}

} /* namespace ADDON */

