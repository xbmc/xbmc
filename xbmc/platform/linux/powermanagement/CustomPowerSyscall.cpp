/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "CustomPowerSyscall.h"

#include "ServiceBroker.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/log.h"

#include <cstdlib>

#include <sys/wait.h>

CCustomPowerSyscall::CCustomPowerSyscall()
{
  CLog::Log(LOGINFO, "Selected Custom as PowerSyscall");

  const auto settings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  if (settings)
  {
    m_powerdownCommand = settings->m_powerdownCommand;
    m_rebootCommand = settings->m_rebootCommand;
    m_suspendCommand = settings->m_suspendCommand;
    m_hibernateCommand = settings->m_hibernateCommand;
  }
  else
  {
    CLog::Log(LOGWARNING, "CCustomPowerSyscall: Could not get settings component");
  }

  if (!m_powerdownCommand.empty())
  {
    CLog::Log(LOGINFO, "CCustomPowerSyscall: Powerdown command: {}", m_powerdownCommand);
  }

  if (!m_rebootCommand.empty())
  {
    CLog::Log(LOGINFO, "CCustomPowerSyscall: Reboot command: {}", m_rebootCommand);
  }

  if (!m_suspendCommand.empty())
  {
    CLog::Log(LOGINFO, "CCustomPowerSyscall: Suspend command: {}", m_suspendCommand);
  }

  if (!m_hibernateCommand.empty())
  {
    CLog::Log(LOGINFO, "CCustomPowerSyscall: Hibernate command: {}", m_hibernateCommand);
  }
}

bool CCustomPowerSyscall::Powerdown()
{
  if (m_powerdownCommand.empty())
    return false;

  CLog::Log(LOGINFO, "CCustomPowerSyscall: Executing powerdown: {}", m_powerdownCommand);
  return ExecuteCommand(m_powerdownCommand);
}

bool CCustomPowerSyscall::Reboot()
{
  if (m_rebootCommand.empty())
    return false;

  CLog::Log(LOGINFO, "CCustomPowerSyscall: Executing reboot: {}", m_rebootCommand);
  return ExecuteCommand(m_rebootCommand);
}

bool CCustomPowerSyscall::Suspend()
{
  if (m_suspendCommand.empty())
    return false;

  CLog::Log(LOGINFO, "CCustomPowerSyscall: Executing suspend: {}", m_suspendCommand);
  return ExecuteCommand(m_suspendCommand);
}

bool CCustomPowerSyscall::Hibernate()
{
  if (m_hibernateCommand.empty())
    return false;

  CLog::Log(LOGINFO, "CCustomPowerSyscall: Executing hibernate: {}", m_hibernateCommand);
  return ExecuteCommand(m_hibernateCommand);
}

bool CCustomPowerSyscall::CanPowerdown()
{
  return !m_powerdownCommand.empty();
}

bool CCustomPowerSyscall::CanReboot()
{
  return !m_rebootCommand.empty();
}

bool CCustomPowerSyscall::CanSuspend()
{
  return !m_suspendCommand.empty();
}

bool CCustomPowerSyscall::CanHibernate()
{
  return !m_hibernateCommand.empty();
}

bool CCustomPowerSyscall::HasCustomCommands()
{
  const auto settings = CServiceBroker::GetSettingsComponent()->GetAdvancedSettings();
  if (settings)
  {
    return !settings->m_powerdownCommand.empty() || !settings->m_rebootCommand.empty() ||
           !settings->m_suspendCommand.empty() || !settings->m_hibernateCommand.empty();
  }

  return false;
}

bool CCustomPowerSyscall::ExecuteCommand(const std::string& command)
{
  if (command.empty())
    return false;

  int status = system(command.c_str());
  if (status == -1)
    return false;

  if (WEXITSTATUS(status) != 0)
  {
    CLog::Log(LOGERROR, "CCustomPowerSyscall: Failed to execute command: {}", command);
    return false;
  }

  return true;
}
