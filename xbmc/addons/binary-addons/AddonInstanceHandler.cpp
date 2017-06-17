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
#include "BinaryAddonBase.h"

#include "utils/log.h"
#include "utils/StringUtils.h"

namespace ADDON
{

IAddonInstanceHandler::IAddonInstanceHandler(ADDON_TYPE type, const BinaryAddonBasePtr& addonBase, KODI_HANDLE parentInstance/* = nullptr*/, const std::string& instanceID/* = ""*/)
  : m_type(type),
    m_parentInstance(parentInstance),
    m_addonBase(addonBase)
{
  // if no special instance ID is given generate one from class pointer (is
  // faster as unique id and also safe enough for them).
  m_instanceId = !instanceID.empty() ? instanceID : StringUtils::Format("%p", static_cast<void*>(this));

  m_addon = m_addonBase->GetAddon(this);
  if (!m_addon)
    CLog::Log(LOGFATAL, "IAddonInstanceHandler::%s: Tried to get add-on '%s' who not available!",
                __FUNCTION__,
                m_addonBase->ID().c_str());
}

IAddonInstanceHandler::~IAddonInstanceHandler()
{
  m_addonBase->ReleaseAddon(this);
}

std::string IAddonInstanceHandler::ID() const
{
  return m_addon ? m_addon->ID() : "";
}

std::string IAddonInstanceHandler::Name() const
{
  return m_addon ? m_addon->Name() : "";
}

std::string IAddonInstanceHandler::Author() const
{
  return m_addon ? m_addon->Author() : "";
}

std::string IAddonInstanceHandler::Icon() const
{
  return m_addon ? m_addon->Icon() : "";
}

std::string IAddonInstanceHandler::Path() const
{
  return m_addon ? m_addon->Path() : "";
}

std::string IAddonInstanceHandler::Profile() const
{
  return m_addon ? m_addon->Profile() : "";
}

AddonVersion IAddonInstanceHandler::Version() const
{
  return m_addon ? m_addon->Version() : AddonVersion("0.0.0");
}

bool IAddonInstanceHandler::CreateInstance(KODI_HANDLE instance)
{
  if (!m_addon)
    return false;

  ADDON_STATUS status = m_addon->CreateInstance(m_type, m_instanceId, instance, m_parentInstance);
  if (status != ADDON_STATUS_OK)
  {
    CLog::Log(LOGERROR, "IAddonInstanceHandler::%s: %s returned bad status \"%s\" during instance creation",
                __FUNCTION__,
                m_addon->ID().c_str(),
                kodi::TranslateAddonStatus(status).c_str());
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

