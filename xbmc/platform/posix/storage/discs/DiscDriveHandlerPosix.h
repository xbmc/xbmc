/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "storage/discs/IDiscDriveHandler.h"

#include <string>

class CDiscDriveHandlerPosix : public IDiscDriveHandler
{
public:
  /*! \brief Posix DiscDriveHandler constructor
  */
  CDiscDriveHandlerPosix() = default;

  /*! \brief Posix DiscDriveHandler default destructor
  */
  ~CDiscDriveHandlerPosix() override = default;

  /*! \brief Get the optical drive state provided its device path
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  * \return The drive state
  */
  DriveState GetDriveState(const std::string& devicePath) override;

  /*! \brief Get the optical drive tray state provided the drive device path
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  * \return The drive state
  */
  TrayState GetTrayState(const std::string& devicePath) override;

  /*! \brief Eject the provided drive device
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  */
  void EjectDriveTray(const std::string& devicePath) override;

  /*! \brief Close the provided drive device
  * \note Some drives support closing apart from opening/eject
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  */
  void CloseDriveTray(const std::string& devicePath) override;

  /*! \brief Toggle the state of a given drive device
  *
  * Will internally call EjectDriveTray or CloseDriveTray depending on
  * the internal state of the drive (i.e. if open -> CloseDriveTray /
  * if closed -> EjectDriveTray)
  *
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  */
  void ToggleDriveTray(const std::string& devicePath) override;
};
