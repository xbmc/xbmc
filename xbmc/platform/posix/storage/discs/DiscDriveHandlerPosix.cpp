/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DiscDriveHandlerPosix.h"

#include "storage/cdioSupport.h"
#include "utils/log.h"

#include <memory>

namespace
{
constexpr int MAX_OPEN_RETRIES = 3;
}
using namespace MEDIA_DETECT;

std::shared_ptr<IDiscDriveHandler> IDiscDriveHandler::CreateInstance()
{
  return std::make_shared<CDiscDriveHandlerPosix>();
}

DriveState CDiscDriveHandlerPosix::GetDriveState(const std::string& devicePath)
{
  DriveState driveStatus = DriveState::NOT_READY;
  const std::shared_ptr<CLibcdio> libCdio = CLibcdio::GetInstance();
  if (!libCdio)
  {
    CLog::LogF(LOGERROR, "Failed to obtain libcdio handler");
    return driveStatus;
  }

  CdIo_t* cdio = libCdio->cdio_open(devicePath.c_str(), DRIVER_UNKNOWN);
  if (!cdio)
  {
    return driveStatus;
  }

  CdioTrayStatus status = libCdio->mmc_get_tray_status(cdio);

  switch (status)
  {
    case CdioTrayStatus::CLOSED:
    case CdioTrayStatus::UNKNOWN:
      driveStatus = DriveState::CLOSED_MEDIA_UNDEFINED;
      break;

    case CdioTrayStatus::OPEN:
      driveStatus = DriveState::OPEN;
      break;

    case CdioTrayStatus::DRIVER_ERROR:
      driveStatus = DriveState::NOT_READY;
      break;

    default:
      CLog::LogF(LOGWARNING, "Unknown/unhandled drive state interpreted as DRIVE_NOT_READY");
      break;
  }
  libCdio->cdio_destroy(cdio);

  return driveStatus;
}

TrayState CDiscDriveHandlerPosix::GetTrayState(const std::string& devicePath)
{
  TrayState trayStatus = TrayState::UNDEFINED;
  const std::shared_ptr<CLibcdio> libCdio = CLibcdio::GetInstance();
  if (!libCdio)
  {
    CLog::LogF(LOGERROR, "Failed to obtain libcdio handler");
    return trayStatus;
  }

  discmode_t discmode = CDIO_DISC_MODE_NO_INFO;
  CdIo_t* cdio = libCdio->cdio_open(devicePath.c_str(), DRIVER_UNKNOWN);
  if (!cdio)
  {
    return trayStatus;
  }

  discmode = libCdio->cdio_get_discmode(cdio);

  if (discmode == CDIO_DISC_MODE_NO_INFO)
  {
    trayStatus = TrayState::CLOSED_NO_MEDIA;
  }
  else if (discmode == CDIO_DISC_MODE_ERROR)
  {
    trayStatus = TrayState::UNDEFINED;
  }
  else
  {
    trayStatus = TrayState::CLOSED_MEDIA_PRESENT;
  }
  libCdio->cdio_destroy(cdio);

  return trayStatus;
}

void CDiscDriveHandlerPosix::EjectDriveTray(const std::string& devicePath)
{
  const std::shared_ptr<CLibcdio> libCdio = CLibcdio::GetInstance();
  if (!libCdio)
  {
    CLog::LogF(LOGERROR, "Failed to obtain libcdio handler");
    return;
  }

  int retries = MAX_OPEN_RETRIES;
  CdIo_t* cdio = libCdio->cdio_open(devicePath.c_str(), DRIVER_UNKNOWN);
  while (cdio && retries-- > 0)
  {
    const driver_return_code_t ret = libCdio->cdio_eject_media(&cdio);
    if (ret == DRIVER_OP_SUCCESS)
      break;
  }
  libCdio->cdio_destroy(cdio);
}

void CDiscDriveHandlerPosix::CloseDriveTray(const std::string& devicePath)
{
  const std::shared_ptr<CLibcdio> libCdio = CLibcdio::GetInstance();
  if (!libCdio)
  {
    CLog::LogF(LOGERROR, "Failed to obtain libcdio handler");
    return;
  }

  const driver_return_code_t ret = libCdio->cdio_close_tray(devicePath.c_str(), nullptr);
  if (ret != DRIVER_OP_SUCCESS)
  {
    CLog::LogF(LOGERROR, "Closing tray failed for device {}: {}", devicePath,
               libCdio->cdio_driver_errmsg(ret));
  }
}

void CDiscDriveHandlerPosix::ToggleDriveTray(const std::string& devicePath)
{
  if (GetDriveState(devicePath) == DriveState::OPEN)
  {
    CloseDriveTray(devicePath);
  }
  else
  {
    EjectDriveTray(devicePath);
  }
}
