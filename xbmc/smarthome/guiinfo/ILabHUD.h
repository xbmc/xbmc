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
 * for a LEGO train development lab.
 */
class ILabHUD
{
public:
  virtual ~ILabHUD() = default;

  /*!
   * \brief Returns true if the train has been used recently, false otherwise
   */
  virtual bool IsActive() const = 0;

  /*!
   * \brief Get the CPU usage of the host, in percent
   */
  virtual unsigned int CPUPercent() const = 0;

  /*!
   * \brief Get the RAM usage of the microcontroller, in percent
   */
  virtual unsigned int MemoryPercent() const = 0;

  /*!
   * \brief Get the current going through the shunt current sensor, in Amps
   */
  virtual float ShuntCurrent() const = 0;

  /*!
   * \brief Get the output voltage of the IR reflectance sensor, in Volts
   */
  virtual float IRVoltage() const = 0;
};
} // namespace SMART_HOME
} // namespace KODI
