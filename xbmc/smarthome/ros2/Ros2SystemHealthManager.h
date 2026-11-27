/*
 *  Copyright (C) 2024-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "smarthome/guiinfo/ISystemHealthHUD.h"

#include <chrono>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace rclcpp
{
class Node;
}

namespace KODI
{
namespace SMART_HOME
{
class CRos2SystemHealthSubscriber;
class CRos2PropertyManager;

/*!
 * \brief ROS 2 subscription manager for system health
 */
class CRos2SystemHealthManager : public ISystemHealthHUD
{
public:
  CRos2SystemHealthManager(std::string rosNamespace);
  ~CRos2SystemHealthManager();

  // ROS functions
  void Initialize(std::shared_ptr<rclcpp::Node> node);
  void Deinitialize();

  // Implementation of ISystemHealthHUD
  bool IsActive(const std::string& systemName, std::chrono::milliseconds timeout) override;
  CTemperature CPUTemperature(const std::string& systemName) override;
  float CPUUtilization(const std::string& systemName) override;
  double CPUFrequencyHz(const std::string& systemName) override;
  float MemoryUtilization(const std::string& systemName) override;
  unsigned int BatteryCharge(const std::string& systemName) override;
  float BatteryLoad(const std::string& systemName) override;
  std::optional<SmartHomePropertyValue> Property(const std::string& systemName,
                                                 const std::string& propertyName,
                                                 SmartHomePropertyType type) override;
  void MarkActive(const std::string& systemName);

private:
  // Utility functions
  void AddSystem(const std::string& systemName);

  // ROS parameters
  const std::string m_rosNamespace;
  std::shared_ptr<rclcpp::Node> m_node;

  // System health parameters
  std::map<std::string, CRos2SystemHealthSubscriber> m_systemHealths;
  std::unique_ptr<CRos2PropertyManager> m_propertyManager;
  mutable std::mutex m_mutex;
};
} // namespace SMART_HOME
} // namespace KODI
