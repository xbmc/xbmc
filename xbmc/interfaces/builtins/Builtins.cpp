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
#include "Builtins.h"

#include "AddonBuiltins.h"
#include "ApplicationBuiltins.h"
#include "CECBuiltins.h"
#include "GUIBuiltins.h"
#include "GUIControlBuiltins.h"
#include "GUIContainerBuiltins.h"
#include "LibraryBuiltins.h"
#include "OpticalBuiltins.h"
#include "PictureBuiltins.h"
#include "PlayerBuiltins.h"
#include "ProfileBuiltins.h"
#include "PVRBuiltins.h"
#include "SkinBuiltins.h"
#include "SystemBuiltins.h"
#include "WeatherBuiltins.h"

#include "input/InputManager.h"
#include "powermanagement/PowerManager.h"
#include "settings/Settings.h"
#include "Util.h"
#include "utils/log.h"
#include "utils/StringUtils.h"

#if defined(TARGET_ANDROID)
#include "AndroidBuiltins.h"
#endif

#if defined(TARGET_POSIX)
#include "linux/PlatformDefs.h"
#endif

CBuiltins::CBuiltins()
{
  RegisterCommands<CAddonBuiltins>();
  RegisterCommands<CApplicationBuiltins>();
  RegisterCommands<CGUIBuiltins>();
  RegisterCommands<CGUIContainerBuiltins>();
  RegisterCommands<CGUIControlBuiltins>();
  RegisterCommands<CLibraryBuiltins>();
  RegisterCommands<COpticalBuiltins>();
  RegisterCommands<CPictureBuiltins>();
  RegisterCommands<CPlayerBuiltins>();
  RegisterCommands<CProfileBuiltins>();
  RegisterCommands<CPVRBuiltins>();
  RegisterCommands<CSkinBuiltins>();
  RegisterCommands<CSystemBuiltins>();
  RegisterCommands<CWeatherBuiltins>();

#if defined(HAVE_LIBCEC)
  RegisterCommands<CCECBuiltins>();
#endif

#if defined(TARGET_ANDROID)
  RegisterCommands<CAndroidBuiltins>();
#endif
}

CBuiltins::~CBuiltins()
{
}

CBuiltins& CBuiltins::GetInstance()
{
  static CBuiltins sBuiltins;
  return sBuiltins;
}

bool CBuiltins::HasCommand(const std::string& execString)
{
  std::string function;
  std::vector<std::string> parameters;
  CUtil::SplitExecFunction(execString, function, parameters);
  StringUtils::ToLower(function);

  if (CInputManager::GetInstance().HasBuiltin(function))
    return true;

  const auto& it = m_command.find(function);
  if (it != m_command.end())
  {
    if (it->second.parameters == 0 || it->second.parameters <= parameters.size())
      return true;
  }

  return false;
}

bool CBuiltins::IsSystemPowerdownCommand(const std::string& execString)
{
  std::string execute;
  std::vector<std::string> params;
  CUtil::SplitExecFunction(execString, execute, params);
  StringUtils::ToLower(execute);

  // Check if action is resulting in system powerdown.
  if (execute == "reboot"    ||
      execute == "restart"   ||
      execute == "reset"     ||
      execute == "powerdown" ||
      execute == "hibernate" ||
      execute == "suspend" )
  {
    return true;
  }
  else if (execute == "shutdown")
  {
    switch (CSettings::GetInstance().GetInt(CSettings::SETTING_POWERMANAGEMENT_SHUTDOWNSTATE))
    {
      case POWERSTATE_SHUTDOWN:
      case POWERSTATE_SUSPEND:
      case POWERSTATE_HIBERNATE:
        return true;

      default:
        return false;
    }
  }
  return false;
}

void CBuiltins::GetHelp(std::string &help)
{
  help.clear();

  for (const auto& it : m_command)
  {
    help += it.first;
    help += "\t";
    help += it.second.description;
    help += "\n";
  }
}

int CBuiltins::Execute(const std::string& execString)
{
  // Deprecated. Get the text after the "XBMC."
  std::string execute;
  std::vector<std::string> params;
  CUtil::SplitExecFunction(execString, execute, params);
  StringUtils::ToLower(execute);

  const auto& it = m_command.find(execute);
  if (it != m_command.end())
  {
    if (it->second.parameters == 0 || params.size() >= it->second.parameters)
      return it->second.Execute(params);
    else
    {
      CLog::Log(LOGERROR, "%s called with invalid number of parameters (should be: %" PRIdS ", is %" PRIdS")",
                          execute.c_str(), it->second.parameters, params.size());
      return -1;
    }
  } 
  else
    return CInputManager::GetInstance().ExecuteBuiltin(execute, params);
}
