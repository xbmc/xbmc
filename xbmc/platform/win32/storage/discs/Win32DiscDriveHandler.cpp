/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Win32DiscDriveHandler.h"

#include "utils/log.h"

#include "platform/win32/WIN32Util.h"

#include <memory>

namespace
{
/*! Abstraction for win32 return codes - state drive error */
constexpr int WIN32_DISCDRIVESTATE_DRIVE_ERROR = -1;
/*! Abstraction for win32 return codes - state no media in tray */
constexpr int WIN32_DISCDRIVESTATE_NO_MEDIA = 0;
/*! Abstraction for win32 return codes - state tray open */
constexpr int WIN32_DISCDRIVESTATE_TRAY_OPEN = 1;
/*! Abstraction for win32 return codes - state media accessible */
constexpr int WIN32_DISCDRIVESTATE_MEDIA_ACCESSIBLE = 2;
} // namespace

std::shared_ptr<IDiscDriveHandler> IDiscDriveHandler::CreateInstance()
{
  return std::make_shared<CWin32DiscDriveHandler>();
}

DriveState CWin32DiscDriveHandler::GetDriveState(const std::string& devicePath)
{
  DriveState driveState = DriveState::NOT_READY;
  int status = CWIN32Util::GetDriveStatus(devicePath);
  switch (status)
  {
    case WIN32_DISCDRIVESTATE_DRIVE_ERROR:
      driveState = DriveState::NOT_READY;
      break;
    case WIN32_DISCDRIVESTATE_NO_MEDIA:
      driveState = DriveState::CLOSED_NO_MEDIA;
      break;
    case WIN32_DISCDRIVESTATE_TRAY_OPEN:
      driveState = DriveState::OPEN;
      break;
    case WIN32_DISCDRIVESTATE_MEDIA_ACCESSIBLE:
      driveState = DriveState::CLOSED_MEDIA_PRESENT;
      break;
    default:
      CLog::LogF(LOGWARNING, "Unknown/unhandled drive state interpreted as NOT_READY");
      break;
  }
  return driveState;
}

TrayState CWin32DiscDriveHandler::GetTrayState(const std::string& devicePath)
{
  TrayState trayState = TrayState::UNDEFINED;
  DriveState driveState = GetDriveState(devicePath);
  switch (driveState)
  {
    case DriveState::OPEN:
      trayState = TrayState::OPEN;
      break;
    case DriveState::CLOSED_NO_MEDIA:
      trayState = TrayState::CLOSED_NO_MEDIA;
      break;
    case DriveState::CLOSED_MEDIA_PRESENT:
      trayState = TrayState::CLOSED_MEDIA_PRESENT;
      break;
    default:
      CLog::LogF(LOGWARNING, "Unknown/unhandled tray state interpreted as TrayState::UNDEFINED");
      break;
  }
  return trayState;
}

void CWin32DiscDriveHandler::EjectDriveTray(const std::string& devicePath)
{
  if (devicePath.empty())
  {
    CLog::LogF(LOGERROR, "Invalid/Empty devicePath provided");
    return;
  }
  CWIN32Util::EjectTray(devicePath[0]);
}

void CWin32DiscDriveHandler::CloseDriveTray(const std::string& devicePath)
{
  if (devicePath.empty())
  {
    CLog::LogF(LOGERROR, "Invalid/Empty devicePath provided");
    return;
  }
  CWIN32Util::CloseTray(devicePath[0]);
}

void CWin32DiscDriveHandler::ToggleDriveTray(const std::string& devicePath)
{
  if (devicePath.empty())
  {
    CLog::LogF(LOGERROR, "Invalid/Empty devicePath provided");
    return;
  }
  CWIN32Util::ToggleTray(devicePath[0]);
}
