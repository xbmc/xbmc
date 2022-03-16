/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <string>

/*! \brief Represents the state of a disc (optical) drive */
enum class DriveState
{
  /*! The drive is open */
  OPEN,
  /*! The drive is not ready (happens when openning or closing) */
  NOT_READY,
  /*! The drive is ready */
  READY,
  /*! The drive is closed but no media could be detected in the drive */
  CLOSED_NO_MEDIA,
  /*! The drive is closed and there is media in the drive */
  CLOSED_MEDIA_PRESENT,
  /*! The system does not have an optical drive */
  NONE,
  /*! The drive is closed but we don't know yet if there's media there */
  CLOSED_MEDIA_UNDEFINED
};

/*! \brief Represents the state of the drive tray */
enum class TrayState
{
  /*!  The tray is in an undefined state, we don't know yet */
  UNDEFINED,
  /*!  The tray is open */
  OPEN,
  /*! The tray is closed and doesn't have any optical media */
  CLOSED_NO_MEDIA,
  /*! The tray is closed and contains optical media */
  CLOSED_MEDIA_PRESENT
};

/*! \brief Generic interface for platform disc drive handling
*/
class IDiscDriveHandler
{
public:
  /*! \brief Get the optical drive state provided its device path
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  * \return The drive state
  */
  virtual DriveState GetDriveState(const std::string& devicePath) = 0;

  /*! \brief Get the optical drive tray state provided the drive device path
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  * \return The drive state
  */
  virtual TrayState GetTrayState(const std::string& devicePath) = 0;

  /*! \brief Eject the provided drive device
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  */
  virtual void EjectDriveTray(const std::string& devicePath) = 0;

  /*! \brief Close the provided drive device
  * \note Some drives support closing appart from opening/eject
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  */
  virtual void CloseDriveTray(const std::string& devicePath) = 0;

  /*! \brief Toggle the state of a given drive device
  *
  * Will internally call EjectDriveTray or CloseDriveTray depending on
  * the internal state of the drive (i.e. if open -> CloseDriveTray /
  * if closed -> EjectDriveTray)
  *
  * \param devicePath the path for the device drive (e.g. /dev/sr0)
  */
  virtual void ToggleDriveTray(const std::string& devicePath) = 0;

  /*! \brief Called to create platform-specific disc drive handler
  *
  * This method is used to create platform-specific disc drive handler
  */
  static std::shared_ptr<IDiscDriveHandler> CreateInstance();

protected:
  virtual ~IDiscDriveHandler() = default;
  IDiscDriveHandler() = default;
};
