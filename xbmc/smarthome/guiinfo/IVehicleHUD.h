/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <chrono>
#include <string>

namespace KODI
{
namespace SMART_HOME
{
/*!
 * \brief Interface exposing information to be used in a HUD (heads up display)
 * for a movable vehicle
 */
class IVehicleHUD
{
public:
  virtual ~IVehicleHUD() = default;

  /*!
   * \brief Get the forward speed of the vehicle, in meters per second
   *
   * The vehicle will be subscribed to if not already subscribed.
   *
   * \param vehicleName The name of the vehicle
   *
   * \return The vehicle's forward speed, in meters per second
   */
  virtual float ForwardSpeed(const std::string& vehicleName) = 0;

  /*!
   * \brief Get the standard deviation (1-sigma uncertainty) of the forward
   * speed, in meters per second
   *
   * The vehicle will be subscribed to if not already subscribed.
   *
   * \param vehicleName The name of the vehicle
   *
   * \return The forward speed standard deviation, in meters per second
   */
  virtual float ForwardSpeedStdDev(const std::string& vehicleName) = 0;

  /*!
   * \brief Get the roll of the vehicle, in degrees
   *
   * The vehicle will be subscribed to if not already subscribed.
   *
   * \param vehicleName The name of the vehicle
   *
   * \return The roll, in degrees
   */
  virtual float Roll(const std::string& vehicleName) = 0;

  /*!
   * \brief Get the standard deviation (1-sigma uncertainty) of the roll, in
   * degrees
   *
   * The vehicle will be subscribed to if not already subscribed.
   *
   * \param vehicleName The name of the vehicle
   *
   * \return The roll standard deviation, in degrees
   */
  virtual float RollStdDev(const std::string& vehicleName) = 0;

  /*!
   * \brief Get the pitch of the vehicle, in degrees
   *
   * The vehicle will be subscribed to if not already subscribed.
   *
   * \param vehicleName The name of the vehicle
   *
   * \return The pitch, in degrees
   */
  virtual float Pitch(const std::string& vehicleName) = 0;

  /*!
   * \brief Get the standard deviation (1-sigma uncertainty) of the pitch, in
   * degrees
   *
   * The vehicle will be subscribed to if not already subscribed.
   *
   * \param vehicleName The name of the vehicle
   *
   * \return The pitch standard deviation, in degrees
   */
  virtual float PitchStdDev(const std::string& vehicleName) = 0;

  /*!
   * \brief Get the yaw of the vehicle, in degrees
   *
   * The vehicle will be subscribed to if not already subscribed.
   *
   * \param vehicleName The name of the vehicle
   *
   * \return The yaw, in degrees
   */
  virtual float Yaw(const std::string& vehicleName) = 0;

  /*!
   * \brief Get the standard deviation (1-sigma uncertainty) of the yaw, in
   * degrees
   *
   * The vehicle will be subscribed to if not already subscribed.
   *
   * \param vehicleName The name of the vehicle
   *
   * \return The yaw standard deviation, in degrees
   */
  virtual float YawStdDev(const std::string& vehicleName) = 0;

  /*!
   * \brief Get the tilt magnitude of the vehicle, in degrees
   *
   * The vehicle will be subscribed to if not already subscribed.
   *
   * \param vehicleName The name of the vehicle
   *
   * \return The tilt magnitude, in degrees
   */
  virtual float Tilt(const std::string& vehicleName) = 0;

  /*!
   * \brief Get the standard deviation (1-sigma uncertainty) of the tilt
   * magnitude, in degrees
   *
   * The vehicle will be subscribed to if not already subscribed.
   *
   * \param vehicleName The name of the vehicle
   *
   * \return The tilt magnitude standard deviation, in degrees
   */
  virtual float TiltStdDev(const std::string& vehicleName) = 0;
};
} // namespace SMART_HOME
} // namespace KODI
