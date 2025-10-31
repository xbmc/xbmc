/*
 *  Copyright (C) 2021-2022 Team Kodi
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

class ILabHUD;
class IStationHUD;
class ITrainHUD;

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
  virtual void RegisterImageTopic(const std::string& topic) = 0;
  virtual void UnregisterImageTopic(const std::string& topic) = 0;
  virtual ILabHUD* GetLabHUD() const = 0;
  virtual IStationHUD* GetStationHUD() const = 0;
  virtual ITrainHUD* GetTrainHUD() const = 0;

  //! @todo Remove GUI dependency
  virtual void FrameMove() = 0;
};
} // namespace SMART_HOME
} // namespace KODI
