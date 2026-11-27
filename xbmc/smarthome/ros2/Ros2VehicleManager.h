/*
 *  Copyright (C) 2024-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "smarthome/guiinfo/IVehicleHUD.h"

#include <chrono>
#include <map>
#include <memory>
#include <string>

namespace rclcpp
{
class Node;
}

namespace KODI
{
namespace SMART_HOME
{
class CRos2VehicleSubscriber;

/*!
 * \brief ROS 2 subscription manager for movable vehicles
 */
class CRos2VehicleManager : public IVehicleHUD
{
public:
  CRos2VehicleManager(std::string rosNamespace);
  ~CRos2VehicleManager();

  // ROS functions
  void Initialize(std::shared_ptr<rclcpp::Node> node);
  void Deinitialize();

  // Implementation of IVehicleHUD
  float ForwardSpeed(const std::string& vehicleName) override;
  float ForwardSpeedStdDev(const std::string& vehicleName) override;
  float Roll(const std::string& vehicleName) override;
  float RollStdDev(const std::string& vehicleName) override;
  float Pitch(const std::string& vehicleName) override;
  float PitchStdDev(const std::string& vehicleName) override;
  float Yaw(const std::string& vehicleName) override;
  float YawStdDev(const std::string& vehicleName) override;
  float Tilt(const std::string& vehicleName) override;
  float TiltStdDev(const std::string& vehicleName) override;

private:
  // Utility functions
  void AddVehicle(const std::string& vehicleName);

  // ROS parameters
  const std::string m_rosNamespace;
  std::shared_ptr<rclcpp::Node> m_node;

  // System health parameters
  std::map<std::string, CRos2VehicleSubscriber> m_vehicles;
};
} // namespace SMART_HOME
} // namespace KODI
