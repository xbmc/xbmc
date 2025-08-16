/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI
{
namespace SMART_HOME
{
/*!
 * \brief Interface exposing information to be used in a HUD (heads up display)
 * for a LEGO train power station.
 */
class IStationHUD
{
public:
  virtual ~IStationHUD() = default;

  /*!
   * \brief Returns true if the motor has been controlled recently, false
   * otherwise
   */
  virtual bool IsActive() const = 0;

  /*!
   * \brief Get the supply voltage of the power source, in Volts
   */
  virtual float SupplyVoltage() const = 0;

  /*!
   * \brief Get the motor voltage, in Volts
   */
  virtual float MotorVoltage() const = 0;

  /*!
   * \brief Get the motor current, in Amps
   */
  virtual float MotorCurrent() const = 0;

  /*!
   * \brief Get the CPU usage, in percent
   */
  virtual unsigned int CPUPercent() const = 0;

  /*!
   * \brief Get the last string reported by the microcontroller
   */
  virtual const std::string& Message() const = 0;
};
} // namespace SMART_HOME
} // namespace KODI
