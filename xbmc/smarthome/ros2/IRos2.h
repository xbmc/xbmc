/*
 *  Copyright (C) 2021-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "smarthome/ImageSubscriptionKey.h"

#include <string>

namespace KODI
{
namespace SMART_HOME
{
class ISystemHealthHUD;
class IPowerMeterHUD;
class IVehicleHUD;

class IRos2
{
public:
  virtual ~IRos2() = default;

  /*!
   * \brief Initialize ROS 2 lifecycle
   */
  virtual void Initialize() = 0;

  /*!
   * \brief Deinitialize ROS 2 lifecycle
   */
  virtual void Deinitialize() = 0;

  // GUI interface
  virtual void RegisterImageTopic(const ImageSubscriptionKey& subscription) = 0;
  virtual void UnregisterImageTopic(const ImageSubscriptionKey& subscription) = 0;
  virtual ISystemHealthHUD* GetSystemHealthHUD() const = 0;
  virtual IPowerMeterHUD* GetPowerMeterHUD() const = 0;
  virtual IVehicleHUD* GetVehicleHUD() const = 0;

  //! @todo Remove GUI dependency
  virtual void FrameMove() = 0;
};
} // namespace SMART_HOME
} // namespace KODI
