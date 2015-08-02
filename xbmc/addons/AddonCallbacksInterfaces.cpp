/*
 *      Copyright (C) 2014 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AddonCallbacksInterfaces.h"
#include "addons/AddonManager.h"
#include "utils/log.h"
#include "include/kodi_interfaces_types.h"
#include "xbmc/interfaces/generic/ScriptInvocationManager.h"
#include "xbmc/interfaces/python/XBPython.h"
#include <vector>
#include <string>

using namespace std;

namespace ADDON
{

CAddonCallbacksInterfaces::CAddonCallbacksInterfaces(CAddon* addon)
{
  m_addon     = addon;
  m_callbacks = new CB_InterfacesLib;

  // write KODI audio DSP specific add-on function addresses to callback table
  m_callbacks->ExecuteScriptSync            = ExecuteScriptSync;
  m_callbacks->ExecuteScriptAsync           = ExecuteScriptAsync;
  m_callbacks->GetPythonInterpreter         = GetPythonInterpreter;
  m_callbacks->ActivatePythonInterpreter    = ActivatePythonInterpreter;
  m_callbacks->DeactivatePythonInterpreter  = DeactivatePythonInterpreter;
}

CAddonCallbacksInterfaces::~CAddonCallbacksInterfaces()
{
  /* delete the callback table */
  delete m_callbacks;
}

int CAddonCallbacksInterfaces::ExecuteScriptSync(void *AddonData, const char *AddonName, const char *Script, const char **Arguments, uint32_t TimeoutMs, bool WaitShutdown)
{
  if(!AddonName || !Script)
  {
    CLog::Log(LOGERROR, "%s: Invalid input! AddonData, AddonName or Script is a NULL pointer!", __FUNCTION__);
    return -1;
  }

  ADDON::AddonPtr addon;
  if(!CAddonMgr::Get().GetAddon(AddonName, addon, ADDON_UNKNOWN))
  {
    CLog::Log(LOGERROR, "%s: Invalid input! Addon: %s not found!", __FUNCTION__, AddonName);
    return -1;
  }

  vector<string> arguments;
  if(Arguments)
  {
    for(unsigned int arg = 0; Arguments[arg]; arg++)
    {
      arguments.push_back(Arguments[arg]);
    }
  }

  //return CScriptInvocationManager::Get().ExecuteSync(Script, ADDON::AddonPtr((ADDON::IAddon*)((AddonCB*)AddonData)->addonData), arguments, TimeoutMs, WaitShutdown);
  return CScriptInvocationManager::Get().ExecuteSync(Script, addon, arguments, TimeoutMs, WaitShutdown);
}

int CAddonCallbacksInterfaces::ExecuteScriptAsync(void *AddonData, const char *AddonName, const char *Script, const char **Arguments)
{
  if(!AddonData || !AddonName || !Script)
  {
    CLog::Log(LOGERROR, "%s: Invalid input! AddonData, AddonName or Script is a NULL pointer!", __FUNCTION__);
    return -1;
  }

  ADDON::AddonPtr addon;
  if(!CAddonMgr::Get().GetAddon(AddonName, addon, ADDON_UNKNOWN))
  {
    CLog::Log(LOGERROR, "%s: Invalid input! Addon: %s not found!", __FUNCTION__, AddonName);
    return -1;
  }

  vector<string> arguments;
  if(Arguments)
  {
    for(unsigned int arg = 0; Arguments[arg]; arg++)
    {
      arguments.push_back(Arguments[arg]);
    }
  }

  //return CScriptInvocationManager::Get().ExecuteAsync(Script, ADDON::AddonPtr((ADDON::IAddon*)((AddonCB*)AddonData)->addonData), arguments);
  return CScriptInvocationManager::Get().ExecuteAsync(Script, addon, arguments);
}

int CAddonCallbacksInterfaces::GetPythonInterpreter(void *AddonData)
{
  return g_pythonParser.GetAddonInterpreter();
}

bool CAddonCallbacksInterfaces::ActivatePythonInterpreter(void *AddonData, int InterpreterId)
{
  return g_pythonParser.ActiveAddonInterpreter(InterpreterId);
}

bool CAddonCallbacksInterfaces::DeactivatePythonInterpreter(void *AddonData, int InterpreterId)
{
  return g_pythonParser.DeactiveAddonInterpreter(InterpreterId);
}

}; /* namespace ADDON */
