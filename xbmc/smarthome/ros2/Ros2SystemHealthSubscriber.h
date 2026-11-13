/*
 *  Copyright (C) 2024-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/Temperature.h"

#include <chrono>
#include <mutex>

#include <oasis_msgs/msg/system_telemetry.hpp>
#include <oasis_msgs/msg/ups_status.hpp>
#include <rclcpp/subscription.hpp>

namespace rclcpp
{
class Node;
}

namespace KODI
{
namespace SMART_HOME
{
/*!
 * \brief ROS 2 system health subscriber
 */
class CRos2SystemHealthSubscriber
{
public:
  CRos2SystemHealthSubscriber(std::string rosNamespace);
  CRos2SystemHealthSubscriber(const CRos2SystemHealthSubscriber&) = delete;
  CRos2SystemHealthSubscriber(CRos2SystemHealthSubscriber&&) = delete;

  // ROS functions
  void Initialize(std::shared_ptr<rclcpp::Node> node, const std::string& systemName);
  void Deinitialize();

  // GUI functions
  bool IsActive(std::chrono::milliseconds timeout) const;
  CTemperature CPUTemperature() const;
  float CPUUtilization() const;
  double CPUFrequencyHz() const;
  float MemoryUtilization() const;
  unsigned int BatteryCharge() const;
  float BatteryLoad() const;
  void MarkActive();

private:
  // ROS messages
  using SystemTelemetry = oasis_msgs::msg::SystemTelemetry;
  using UPSStatus = oasis_msgs::msg::UPSStatus;

  // ROS 2 subscriber callbacks
  void OnSystemTelemetry(const SystemTelemetry::SharedPtr msg);
  void OnUPSStatus(const UPSStatus::SharedPtr msg);

  // ROS parameters
  const std::string m_rosNamespace;
  rclcpp::Subscription<SystemTelemetry>::SharedPtr m_telemetrySubscriber;
  rclcpp::Subscription<UPSStatus>::SharedPtr m_upsStatusSubscriber;

  // GUI parameters
  std::chrono::time_point<std::chrono::steady_clock> m_lastActive;
  CTemperature m_cpuTemperature;
  float m_cpuUtilization{0.0f};
  double m_cpuFrequencyHz{0.0};
  float m_memoryUtilization{0.0f};
  unsigned int m_batteryCharge{0};
  float m_batteryLoadWatts{0.0f};

  // Synchronization parameters
  mutable std::mutex m_mutex;
};
} // namespace SMART_HOME
} // namespace KODI
