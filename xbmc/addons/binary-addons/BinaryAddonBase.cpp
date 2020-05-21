/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "BinaryAddonBase.h"

#include "AddonDll.h"
#include "filesystem/SpecialProtocol.h"
#include "threads/SingleLock.h"
#include "utils/URIUtils.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

using namespace ADDON;

const std::string& CBinaryAddonBase::ID() const
{
  return m_addonInfo->ID();
}

const std::string& CBinaryAddonBase::Path() const
{
  return m_addonInfo->Path();
}

TYPE CBinaryAddonBase::MainType() const
{
  return m_addonInfo->MainType();
}

const std::string& CBinaryAddonBase::MainLibName() const
{
  return m_addonInfo->LibName();
}

bool CBinaryAddonBase::HasType(TYPE type) const
{
  return m_addonInfo->HasType(type);
}

const std::vector<CAddonType>& CBinaryAddonBase::Types() const
{
  return m_addonInfo->Types();
}

const CAddonType* CBinaryAddonBase::Type(TYPE type) const
{
  return m_addonInfo->Type(type);
}

const AddonVersion& CBinaryAddonBase::Version() const
{
  return m_addonInfo->Version();
}

const AddonVersion& CBinaryAddonBase::MinVersion() const
{
  return m_addonInfo->MinVersion();
}

const AddonVersion& CBinaryAddonBase::DependencyVersion(const std::string& dependencyID) const
{
  return m_addonInfo->DependencyVersion(dependencyID);
}

const std::string& CBinaryAddonBase::Name() const
{
  return m_addonInfo->Name();
}

const std::string& CBinaryAddonBase::Summary() const
{
  return m_addonInfo->Summary();
}

const std::string& CBinaryAddonBase::Description() const
{
  return m_addonInfo->Description();
}

const std::string& CBinaryAddonBase::Author() const
{
  return m_addonInfo->Author();
}

const std::string& CBinaryAddonBase::ChangeLog() const
{
  return m_addonInfo->ChangeLog();
}

const std::string& CBinaryAddonBase::Icon() const
{
  return m_addonInfo->Icon();
}

const ArtMap& CBinaryAddonBase::Art() const
{
  return m_addonInfo->Art();
}

const std::string& CBinaryAddonBase::Disclaimer() const
{
  return m_addonInfo->Disclaimer();
}

bool CBinaryAddonBase::MeetsVersion(const AddonVersion& versionMin,
                                    const AddonVersion& version) const
{
  return m_addonInfo->MeetsVersion(versionMin, version);
}

AddonDllPtr CBinaryAddonBase::GetAddon(const IAddonInstanceHandler* handler)
{
  if (handler == nullptr)
  {
    CLog::Log(LOGERROR, "CBinaryAddonBase::%s: for Id '%s' called with empty instance handler", __FUNCTION__, ID().c_str());
    return nullptr;
  }

  CSingleLock lock(m_critSection);

  // If no 'm_activeAddon' is defined create it new.
  if (m_activeAddon == nullptr)
    m_activeAddon = std::make_shared<CAddonDll>(m_addonInfo, shared_from_this());

  // add the instance handler to the info to know used amount on addon
  m_activeAddonHandlers.insert(handler);

  return m_activeAddon;
}

void CBinaryAddonBase::ReleaseAddon(const IAddonInstanceHandler* handler)
{
  if (handler == nullptr)
  {
    CLog::Log(LOGERROR, "CBinaryAddonBase::%s: for Id '%s' called with empty instance handler", __FUNCTION__, ID().c_str());
    return;
  }

  CSingleLock lock(m_critSection);

  auto presentHandler = m_activeAddonHandlers.find(handler);
  if (presentHandler == m_activeAddonHandlers.end())
    return;

  m_activeAddonHandlers.erase(presentHandler);

  // if no handler is present anymore reset and delete the add-on class on informations
  if (m_activeAddonHandlers.empty())
  {
    m_activeAddon.reset();
  }
}

AddonDllPtr CBinaryAddonBase::GetActiveAddon()
{
  CSingleLock lock(m_critSection);
  return m_activeAddon;
}

