/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
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
#include "Service.h"
#include "AddonManager.h"
#include "interfaces/generic/ScriptInvocationManager.h"
#include "utils/log.h"

using namespace std;

namespace ADDON
{

CService::CService(const cp_extension_t *ext)
  : CAddon(ext), m_type(UNKNOWN), m_startOption(LOGIN)
{
  BuildServiceType();

  CStdString start = CAddonMgr::Get().GetExtValue(ext->configuration, "@start");
  if (start.Equals("startup"))
    m_startOption = STARTUP;
}


CService::CService(const AddonProps &props)
  : CAddon(props), m_type(UNKNOWN), m_startOption(LOGIN)
{
  BuildServiceType();
}

AddonPtr CService::Clone() const
{
  return AddonPtr(new CService(*this));
}

bool CService::Start()
{
  bool ret = true;
  switch (m_type)
  {
#ifdef HAS_PYTHON
  case PYTHON:
    ret = (CScriptInvocationManager::Get().Execute(LibPath(), this->shared_from_this()) != -1);
    break;
#endif

  case UNKNOWN:
  default:
    ret = false;
    break;
  }

  return ret;
}

bool CService::Stop()
{
  bool ret = true;

  switch (m_type)
  {
#ifdef HAS_PYTHON
  case PYTHON:
    ret = CScriptInvocationManager::Get().Stop(LibPath());
    break;
#endif

  case UNKNOWN:
  default:
    ret = false;
    break;
  }

  return ret;
}

void CService::BuildServiceType()
{
  CStdString str = LibPath();
  CStdString ext;

  size_t p = str.find_last_of('.');
  if (p != string::npos)
    ext = str.substr(p + 1);

#ifdef HAS_PYTHON
  CStdString pythonExt = ADDON_PYTHON_EXT;
  pythonExt.erase(0, 2);
  if ( ext.Equals(pythonExt) )
    m_type = PYTHON;
  else
#endif
  {
    m_type = UNKNOWN;
    CLog::Log(LOGERROR, "ADDON: extension '%s' is not currently supported for service addon", ext.c_str());
  }
}

CServiceManager::CServiceManager()
{
  CAddonDatabase::RegisterAddonDatabaseCallback(ADDON_SERVICE, this);
}

CServiceManager::~CServiceManager()
{
  CAddonDatabase::UnregisterAddonDatabaseCallback(ADDON_SERVICE);
}

CServiceManager &CServiceManager::Get()
{
  static CServiceManager _singleton;
  return _singleton;
}

void CServiceManager::AddonEnabled(AddonPtr addon, bool bDisabled)
{
  if (!addon)
    return;

  // If the addon is a service, start it
  if (bDisabled)
  {
    boost::shared_ptr<CService> service = boost::dynamic_pointer_cast<CService>(addon);
    if (service)
      service->Start();
  }
}

void CServiceManager::AddonDisabled(AddonPtr addon)
{
  // If the addon is a service, stop it
  if (!addon)
    return;

  boost::shared_ptr<CService> service = boost::dynamic_pointer_cast<CService>(addon);
  if (service)
    service->Stop();
}

} // namespace ADDON
