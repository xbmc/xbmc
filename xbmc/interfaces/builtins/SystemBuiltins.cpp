/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SystemBuiltins.h"

#include "messaging/ApplicationMessenger.h"
#include "utils/StringUtils.h"

using namespace KODI::MESSAGING;

/*! \brief Execute a system executable.
 *  \param params The parameters.
 *  \details params[0] = The path to the executable.
 *
 *  Set the template parameter Wait to true to wait for execution exit.
 */
  template<int Wait=0>
static int Exec(const std::vector<std::string>& params)
{
  CApplicationMessenger::GetInstance().PostMsg(TMSG_MINIMIZE);
  CApplicationMessenger::GetInstance().PostMsg(TMSG_EXECUTE_OS, Wait, -1, nullptr, params[0]);

  return 0;
}

/*! \brief Hibernate system.
 *  \param params (ignored)
 */
static int Hibernate(const std::vector<std::string>& params)
{
  CApplicationMessenger::GetInstance().PostMsg(TMSG_HIBERNATE);

  return 0;
}

/*! \brief Inhibit idle shutdown timer.
 *  \param params The parameters.
 *  \details params[0] = "true" to inhibit shutdown timer (optional).
 */
static int InhibitIdle(const std::vector<std::string>& params)
{
  bool inhibit = (params.size() == 1 && StringUtils::EqualsNoCase(params[0], "true"));
  CApplicationMessenger::GetInstance().PostMsg(TMSG_INHIBITIDLESHUTDOWN, inhibit);

  return 0;
}

/*! \brief Minimize application.
 *  \param params (ignored)
 */
static int Minimize(const std::vector<std::string>& params)
{
  CApplicationMessenger::GetInstance().PostMsg(TMSG_MINIMIZE);

  return 0;
}

/*! \brief Powerdown system.
 *  \param params (ignored)
 */
static int Powerdown(const std::vector<std::string>& params)
{
  CApplicationMessenger::GetInstance().PostMsg(TMSG_POWERDOWN);

  return 0;
}

/*! \brief Quit application.
 *  \param params (ignored)
 */
static int Quit(const std::vector<std::string>& params)
{
  CApplicationMessenger::GetInstance().PostMsg(TMSG_QUIT);

  return 0;
}

/*! \brief Reboot system.
 *  \param params (ignored)
 */
static int Reboot(const std::vector<std::string>& params)
{
  CApplicationMessenger::GetInstance().PostMsg(TMSG_RESTART);

  return 0;
}

/*! \brief Restart application.
 *  \param params (ignored)
 */
static int RestartApp(const std::vector<std::string>& params)
{
  CApplicationMessenger::GetInstance().PostMsg(TMSG_RESTARTAPP);

  return 0;
}

/*! \brief Activate screensaver.
 *  \param params (ignored)
 */
static int Screensaver(const std::vector<std::string>& params)
{
  CApplicationMessenger::GetInstance().PostMsg(TMSG_ACTIVATESCREENSAVER);

  return 0;
}

/*! \brief Inhibit screensaver.
 *  \param params The parameters.
 *  \details params[0] = "true" to inhibit screensaver (optional).
 */
static int InhibitScreenSaver(const std::vector<std::string>& params)
{
  bool inhibit = (params.size() == 1 && StringUtils::EqualsNoCase(params[0], "true"));
  CApplicationMessenger::GetInstance().PostMsg(TMSG_INHIBITSCREENSAVER, inhibit);

  return 0;
}

/*! \brief Shutdown system.
 *  \param params (ignored)
 */
static int Shutdown(const std::vector<std::string>& params)
{
  CApplicationMessenger::GetInstance().PostMsg(TMSG_SHUTDOWN);

  return 0;
}

/*! \brief Suspend system.
 *  \param params (ignored)
 */
static int Suspend(const std::vector<std::string>& params)
{
  CApplicationMessenger::GetInstance().PostMsg(TMSG_SUSPEND);

  return 0;
}


// Note: For new Texts with comma add a "\" before!!! Is used for table text.
//
/// \page page_List_of_built_in_functions
/// \section built_in_functions_15 System built-in's
///
/// -----------------------------------------------------------------------------
///
/// \table_start
///   \table_h2_l{
///     Function,
///     Description }
///   \table_row2_l{
///     <b>`ActivateScreensaver`</b>
///     ,
///     Starts the screensaver
///   }
///   \table_row2_l{
///     <b>`InhibitScreensaver(yesNo)`</b>
///     ,
///     Inhibit the screensaver
///     @param[in] yesNo   value with "true" or "false" to inhibit or allow screensaver (leaving empty defaults to false)
///   }
///   \table_row2_l{
///     <b>`Hibernate`</b>
///     ,
///     Hibernate (S4) the System
///   }
///   \table_row2_l{
///     <b>`InhibitIdleShutdown(true/false)`</b>
///     ,
///     Prevent the system to shutdown on idle.
///     @param[in] value                 "true" to inhibit shutdown timer (optional).
///   }
///   \table_row2_l{
///     <b>`Minimize`</b>
///     ,
///     Minimizes Kodi
///   }
///   \table_row2_l{
///     <b>`Powerdown`</b>
///     ,
///     Powerdown system
///   }
///   \table_row2_l{
///     <b>`Quit`</b>
///     ,
///     Quits Kodi
///   }
///   \table_row2_l{
///     <b>`Reboot`</b>
///     ,
///     Cold reboots the system (power cycle)
///   }
///   \table_row2_l{
///     <b>`Reset`</b>
///     ,
///     Reset the system (same as reboot)
///   }
///   \table_row2_l{
///     <b>`Restart`</b>
///     ,
///     Restart the system (same as reboot)
///   }
///   \table_row2_l{
///     <b>`RestartApp`</b>
///     ,
///     Restarts Kodi (only implemented under Windows and Linux)
///   }
///   \table_row2_l{
///     <b>`ShutDown`</b>
///     ,
///     Trigger default Shutdown action defined in System Settings
///   }
///   \table_row2_l{
///     <b>`Suspend`</b>
///     ,
///     Suspends (S3 / S1 depending on bios setting) the System
///   }
///   \table_row2_l{
///     <b>`System.Exec(exec)`</b>
///     ,
///     Execute shell commands
///     @param[in] exec                  The path to the executable
///   }
///   \table_row2_l{
///     <b>`System.ExecWait(exec)`</b>
///     ,
///     Execute shell commands and freezes Kodi until shell is closed
///     @param[in] exec                  The path to the executable
///   }
/// \table_end
///

CBuiltins::CommandMap CSystemBuiltins::GetOperations() const
{
  return {
           {"activatescreensaver", {"Activate Screensaver", 0, Screensaver}},
           {"hibernate",           {"Hibernates the system", 0, Hibernate}},
           {"inhibitidleshutdown", {"Inhibit idle shutdown", 0, InhibitIdle}},
           {"inhibitscreensaver",  {"Inhibit Screensaver", 0, InhibitScreenSaver}},
           {"minimize",            {"Minimize Kodi", 0, Minimize}},
           {"powerdown",           {"Powerdown system", 0, Powerdown}},
           {"quit",                {"Quit Kodi", 0, Quit}},
           {"reboot",              {"Reboot the system", 0, Reboot}},
           {"reset",               {"Reset the system (same as reboot)", 0, Reboot}},
           {"restart",             {"Restart the system (same as reboot)", 0, Reboot}},
           {"restartapp",          {"Restart Kodi", 0, RestartApp}},
           {"shutdown",            {"Shutdown the system", 0, Shutdown}},
           {"suspend",             {"Suspends the system", 0, Suspend}},
           {"system.exec",         {"Execute shell commands", 1, Exec<0>}},
           {"system.execwait",     {"Execute shell commands and freezes Kodi until shell is closed", 1, Exec<1>}}
         };
}
