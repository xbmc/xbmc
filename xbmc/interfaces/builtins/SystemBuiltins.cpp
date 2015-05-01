/*
 *      Copyright (C) 2005-2015 Team XBMC
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

#include "SystemBuiltins.h"

#include "ApplicationMessenger.h"
#include "utils/StringUtils.h"

/*! \brief Execute a system executable.
 *  \param params The parameters.
 *  \details params[0] = The path to the executable.
 *
 *  Set the template parameter Wait to true to wait for execution exit.
 */
  template<bool Wait=false>
static int Exec(const std::vector<std::string>& params)
{
  CApplicationMessenger::Get().Minimize();
  CApplicationMessenger::Get().ExecOS(params[0], Wait);

  return 0;
}

/*! \brief Hibernate system.
 *  \param params (ignored)
 */
static int Hibernate(const std::vector<std::string>& params)
{
  CApplicationMessenger::Get().Hibernate();

  return 0;
}

/*! \brief Inhibit idle shutdown timer.
 *  \param params The parameters.
 *  \details params[0] = "true" to inhibit shutdown timer (optional).
 */
static int InhibitIdle(const std::vector<std::string>& params)
{
  bool inhibit = (params.size() == 1 && StringUtils::EqualsNoCase(params[0], "true"));
  CApplicationMessenger::Get().InhibitIdleShutdown(inhibit);

  return 0;
}

/*! \brief Minimize application.
 *  \param params (ignored)
 */
static int Minimize(const std::vector<std::string>& params)
{
  CApplicationMessenger::Get().Minimize();

  return 0;
}

/*! \brief Powerdown system.
 *  \param params (ignored)
 */
static int Powerdown(const std::vector<std::string>& params)
{
  CApplicationMessenger::Get().Powerdown();

  return 0;
}

/*! \brief Quit application.
 *  \param params (ignored)
 */
static int Quit(const std::vector<std::string>& params)
{
  CApplicationMessenger::Get().Quit();

  return 0;
}

/*! \brief Reboot system.
 *  \param params (ignored)
 */
static int Reboot(const std::vector<std::string>& params)
{
  CApplicationMessenger::Get().Restart();

  return 0;
}

/*! \brief Restart application.
 *  \param params (ignored)
 */
static int RestartApp(const std::vector<std::string>& params)
{
  CApplicationMessenger::Get().RestartApp();

  return 0;
}

/*! \brief Activate screensaver.
 *  \param params (ignored)
 */
static int Screensaver(const std::vector<std::string>& params)
{
  CApplicationMessenger::Get().ActivateScreensaver();

  return 0;
}

/*! \brief Shutdown system.
 *  \param params (ignored)
 */
static int Shutdown(const std::vector<std::string>& params)
{
  CApplicationMessenger::Get().Shutdown();

  return 0;
}

/*! \brief Suspend system.
 *  \param params (ignored)
 */
static int Suspend(const std::vector<std::string>& params)
{
  CApplicationMessenger::Get().Suspend();

  return 0;
}

CBuiltins::CommandMap CSystemBuiltins::GetOperations()
{
  return {
           {"activatescreensaver", {"Activate Screensaver", 0, Screensaver}},
           {"hibernate",           {"Hibernates the system", 0, Hibernate}},
           {"inhibitidleshutdown", {"Inhibit idle shutdown", 0, InhibitIdle}},
           {"minimize",            {"Minimize Kodi", 0, Minimize}},
           {"powerdown",           {"Powerdown system", 0, Powerdown}},
           {"quit",                {"Quit Kodi", 0, Quit}},
           {"reboot",              {"Reboot the system", 0, Reboot}},
           {"reset",               {"Reset the system (same as reboot)", 0, Reboot}},
           {"restart",             {"Restart the system (same as reboot)", 0, Reboot}},
           {"restartapp",          {"Restart Kodi", 0, RestartApp}},
           {"shutdown",            {"Shutdown the system", 0, Shutdown}},
           {"suspend",             {"Suspends the system", 0, Suspend}},
           {"system.exec",         {"Execute shell commands", 1, Exec<false>}},
           {"system.execwait",     {"Execute shell commands and freezes Kodi until shell is closed", 1, Exec<true>}}
         };
}
