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
#include "system.h"

namespace ADDON
{

std::unique_ptr<CService> CService::FromExtension(CAddonInfo addonInfo, const cp_extension_t* ext)
{
  START_OPTION startOption(LOGIN);
  std::string start = CAddonMgr::GetInstance().GetExtValue(ext->configuration, "@start");
  if (start == "startup")
    startOption = STARTUP;
  return std::unique_ptr<CService>(new CService(std::move(addonInfo), TYPE(UNKNOWN), startOption));
}


CService::CService(CAddonInfo addonInfo, TYPE type, START_OPTION startOption)
  : CAddon(std::move(addonInfo)), m_type(type), m_startOption(startOption)
{
  BuildServiceType();
}

bool CService::Start()
{
  bool ret = true;
  switch (m_type)
  {
#ifdef HAS_PYTHON
  case PYTHON:
    ret = (CScriptInvocationManager::GetInstance().ExecuteAsync(LibPath(), this->shared_from_this()) != -1);
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
    ret = CScriptInvocationManager::GetInstance().Stop(LibPath());
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
  std::string str = LibPath();
  std::string ext;

  size_t p = str.find_last_of('.');
  if (p != std::string::npos)
    ext = str.substr(p + 1);

#ifdef HAS_PYTHON
  std::string pythonExt = ADDON_PYTHON_EXT;
  pythonExt.erase(0, 2);
  if ( ext == pythonExt )
    m_type = PYTHON;
  else
#endif
  {
    m_type = UNKNOWN;
    CLog::Log(LOGERROR, "ADDON: extension '%s' is not currently supported for service addon", ext.c_str());
  }
}

}
